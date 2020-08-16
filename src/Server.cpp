/*---------------------------------------------------------------
 * Copyright (c) 1999,2000,2001,2002,2003
 * The Board of Trustees of the University of Illinois
 * All Rights Reserved.
 *---------------------------------------------------------------
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software (Iperf) and associated
 * documentation files (the "Software"), to deal in the Software
 * without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute,
 * sublicense, and/or sell copies of the Software, and to permit
 * persons to whom the Software is furnished to do
 * so, subject to the following conditions:
 *
 *
 * Redistributions of source code must retain the above
 * copyright notice, this list of conditions and
 * the following disclaimers.
 *
 *
 * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following
 * disclaimers in the documentation and/or other materials
 * provided with the distribution.
 *
 *
 * Neither the names of the University of Illinois, NCSA,
 * nor the names of its contributors may be used to endorse
 * or promote products derived from this Software without
 * specific prior written permission.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE CONTIBUTORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * ________________________________________________________________
 * National Laboratory for Applied Network Research
 * National Center for Supercomputing Applications
 * University of Illinois at Urbana-Champaign
 * http://www.ncsa.uiuc.edu
 * ________________________________________________________________
 *
 * Server.cpp
 * by Mark Gates <mgates@nlanr.net>
 *     Ajay Tirumala (tirumala@ncsa.uiuc.edu>.
 * -------------------------------------------------------------------
 * A server thread is initiated for each connection accept() returns.
 * Handles sending and receiving data, and then closes socket.
 * Changes to this version : The server can be run as a daemon
 * ------------------------------------------------------------------- */

#define HEADERS()

#include "headers.h"
#include "Server.hpp"
#include "List.h"
#include "Extractor.h"
#include "Reporter.h"
#include "Locale.h"
#include "delay.h"
#include "PerfSocket.hpp"
#include "Write_ack.hpp"
#include "SocketAddr.h"
#include "payloads.h"
#if defined(HAVE_LINUX_FILTER_H) && defined(HAVE_AF_PACKET)
#include "checksums.h"
#endif


/* -------------------------------------------------------------------
 * Stores connected socket and socket info.
 * ------------------------------------------------------------------- */

Server::Server( thread_Settings *inSettings ) {
#ifdef HAVE_THREAD_DEBUG
    thread_debug("Server constructor with thread=%p multihdr=%p(sock=%d)", (void *) inSettings, (void *)inSettings->multihdr, inSettings->mSock);
#endif
    mSettings = inSettings;
    mBuf = NULL;
    myJob = NULL;
    reportstruct = &scratchpad;
    mySocket = inSettings->mSock;
#if defined(HAVE_LINUX_FILTER_H) && defined(HAVE_AF_PACKET)
    myDropSocket = inSettings->mSockDrop;
    if (isL2LengthCheck(mSettings)) {
	// For L2 UDP make sure we can receive a full ethernet packet plus a bit more
	if (mSettings->mBufLen < (2 * ETHER_MAX_LEN)) {
	    mSettings->mBufLen = (2 * ETHER_MAX_LEN);
	}
    }
#endif
    // initialize buffer, length checking done by the Listener
    mBuf = new char[MBUFALLOCSIZE]; // defined in payloads.h
    FAIL_errno(mBuf == NULL, "No memory for buffer\n", mSettings);
    mSettings->mBufLen = MBUFALLOCSIZE;
    if (mSettings->mBufLen < (int) sizeof(UDP_datagram)) {
	fprintf(stderr, warn_buffer_too_small, mSettings->mBufLen);
    }
    SockAddr_Ifrname(mSettings);
    // Enable kernel level timestamping if available
    InitKernelTimeStamping();
    int sorcvtimer = 0;
    // sorcvtimer units microseconds convert to that
    // minterval double, units seconds
    // mAmount integer, units 10 milliseconds
    // divide by two so timeout is 1/2 the interval
    if (mSettings->mInterval && (mSettings->mIntervalMode == kInterval_Time)) {
	sorcvtimer = (mSettings->mInterval / 2);
    } else if (isServerModeTime(mSettings)) {
	sorcvtimer = (mSettings->mAmount * 1000) / 2;
    }
    if (sorcvtimer > 0) {
#ifdef WIN32
	// Windows SO_RCVTIMEO uses ms
	DWORD timeout = (double) sorcvtimer / 1e3;
#else
	struct timeval timeout;
	timeout.tv_sec = sorcvtimer / 1000000;
	timeout.tv_usec = sorcvtimer % 1000000;
#endif
	if (setsockopt( mSettings->mSock, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout)) < 0 ) {
	    WARN_errno( mSettings->mSock == SO_RCVTIMEO, "socket" );
	}
    }
    myJob = InitIndividualReport(mSettings);
    myReport = (struct ReporterData *)myJob->this_report;
    assert(myJob != NULL);
}

/* -------------------------------------------------------------------
 * Destructor close socket.
 * ------------------------------------------------------------------- */

Server::~Server (void) {
    int do_close = 1;
#if HAVE_THREAD_DEBUG
    thread_debug("Server destructor sock=%d bidir=%s", mySocket, (isBidir(mSettings) ? "true" : "false"));
#endif
    if (isBidir(mSettings) && (do_close = bidir_stop_barrier(&mSettings->bidir_startstop))) {
	struct Condition *tmp = &mSettings->bidir_startstop.await;
	Condition_Destroy(tmp);
    }
    if ((mySocket != INVALID_SOCKET) && do_close) {
#if HAVE_THREAD_DEBUG
	thread_debug("Socket close sock=%d (server destructor)", mySocket);
#endif
	int rc = close(mySocket);
	WARN_errno( rc == SOCKET_ERROR, "server close" );
	mySocket = INVALID_SOCKET;
    }
#if defined(HAVE_LINUX_FILTER_H) && defined(HAVE_AF_PACKET)
    if (myDropSocket != INVALID_SOCKET) {
	int rc = close(myDropSocket);
	WARN_errno( rc == SOCKET_ERROR, "server close drop" );
	myDropSocket = INVALID_SOCKET;
    }
#endif
    DELETE_ARRAY(mBuf);
}

inline bool Server::InProgress (void) {
    if (sInterupted ||
	((isServerModeTime(mSettings) || (isModeTime(mSettings) && isReverse(mSettings))) && mEndTime.before(reportstruct->packetTime)))
	return false;
    return true;
}

/* -------------------------------------------------------------------
 * Receive TCP data from the (connected) socket.
 * Sends termination flag several times at the end.
 * Does not close the socket.
 * ------------------------------------------------------------------- */
void Server::RunTCP( void ) {
    long currLen;
    intmax_t totLen = 0;
    bool err  = 0;
    struct TCP_burst_payload burst_info;

    Timestamp time1, time2;
    double tokens=0.000004;

    InitTrafficLoop();

    int burst_nleft = 0;
    burst_info.burst_id = 0;
    struct timeval prevsend = {.tv_sec = 0, .tv_usec = 0};

    burst_info.send_tt.write_tv_sec = 0;
    burst_info.send_tt.write_tv_usec = 0;
    int alignbytes = AlignPayloads();

    while (InProgress() && !err) {
	reportstruct->emptyreport=0;
	// perform read
	if (isBWSet(mSettings)) {
	    time2.setnow();
	    tokens += time2.subSec(time1) * (mSettings->mUDPRate / 8.0);
	    time1 = time2;
	}
	if (tokens >= 0.0) {
	    int n = 0;
	    if (isIsochronous(mSettings) || isTripTime(mSettings)) {
		reportstruct->transit_ready = 0;
		if (burst_nleft == 0) {
		    if ((n = recvn(mSettings->mSock, (char *)&burst_info, sizeof(struct TCP_burst_payload), 0)) == sizeof(struct TCP_burst_payload)) {
			burst_info.typelen.type = ntohl(burst_info.typelen.type);
			burst_info.typelen.length = ntohl(burst_info.typelen.length);
			burst_info.flags = ntohl(burst_info.flags);
			burst_info.burst_size = ntohl(burst_info.burst_size);
			assert(burst_info.burst_size > 0);
			reportstruct->burstsize = burst_info.burst_size;
			burst_info.burst_id = ntohl(burst_info.burst_id);
			// printf("**** burst size = %d id = %d\n", burst_info.burst_size, burst_info.burst_id);
			reportstruct->frameID = burst_info.burst_id;
		        reportstruct->prevSentTime = prevsend;
		        reportstruct->sentTime.tv_sec = ntohl(burst_info.send_tt.write_tv_sec);
		        reportstruct->sentTime.tv_usec = ntohl(burst_info.send_tt.write_tv_usec);
			prevsend = reportstruct->sentTime;
			burst_nleft = burst_info.burst_size - n;
			// thread_debug("***read burst header size %d id=%d", burst_info.burst_size, burst_info.burst_id);
		    } else {
#ifdef HAVE_THREAD_DEBUG
		        thread_debug("TCP burst partial read of %d wanted %d", n, sizeof(struct TCP_burst_payload));
#endif
		        goto end;
		    }
		}
		WARN(burst_nleft <= 0, "invalid burst read req size");
		currLen = recv(mSettings->mSock, mBuf, ((mSettings->mBufLen < burst_nleft) ? mSettings->mBufLen : burst_nleft), 0);
		if (currLen > 0) {
		    burst_nleft -= currLen;
		    if ((burst_nleft == 0) && burst_info.burst_id) {
		        if (isWriteAck(mSettings)) {
			    enqueue_ackring(mSettings->ackring, reportstruct);
			}
			reportstruct->transit_ready = 1;
		    }
		}
		// printf("currlen = %d, n=%d, burst_nleft=%d\n", currLen, n, burst_nleft);
	    } else {
	        currLen = recv(mSettings->mSock, mBuf, mSettings->mBufLen, 0);
	    }
	    if (currLen <= 0) {
		reportstruct->emptyreport=1;
		// End loop on 0 read or socket error
		// except for socket read timeout
		if (currLen == 0 ||
  #ifdef WIN32
		    (WSAGetLastError() != WSAEWOULDBLOCK)
  #else
		    (errno != EAGAIN && errno != EWOULDBLOCK)
  #endif // WIN32
		    ) {
		    err = 1;
  #ifdef HAVE_THREAD_DEBUG
		    thread_debug("Server thread detected EOF on socket %d", mSettings->mSock);
  #endif
		}
		currLen = 0;
	    }
	    if (alignbytes) {
		currLen += alignbytes;
		alignbytes = 0;
	    }
	    currLen += n;
	    now.setnow();
	    reportstruct->packetTime.tv_sec = now.getSecs();
	    reportstruct->packetTime.tv_usec = now.getUsecs();
	    totLen += currLen;
	    if (isBWSet(mSettings))
		tokens -= currLen;

	    reportstruct->packetLen = currLen;
	    ReportPacket(myJob, reportstruct);

	    // Check for reverse and amount where
	    // the server stops after receiving
	    // the expected byte count
	    if (isReverse(mSettings) && !isModeTime(mSettings) && (totLen >= (intmax_t) mSettings->mAmount)) {
	        break;
	    }
	} else {
	    // Use a 4 usec delay to fill tokens
	    delay_loop(4);
	}
    }
  end:
    Iperf_remove_host(&mSettings->peer);
    // stop timing
    now.setnow();
    reportstruct->packetTime.tv_sec = now.getSecs();
    reportstruct->packetTime.tv_usec = now.getUsecs();
    reportstruct->packetLen = 0;
    CloseReport(myJob, reportstruct);
}

void Server::InitKernelTimeStamping (void) {
#if HAVE_DECL_SO_TIMESTAMP
    iov[0].iov_base=mBuf;
    iov[0].iov_len=mSettings->mBufLen;

    message.msg_iov=iov;
    message.msg_iovlen=1;
    message.msg_name=&srcaddr;
    message.msg_namelen=sizeof(srcaddr);

    message.msg_control = (char *) ctrl;
    message.msg_controllen = sizeof(ctrl);

    int timestampOn = 1;
    if (setsockopt(mSettings->mSock, SOL_SOCKET, SO_TIMESTAMP, (int *) &timestampOn, sizeof(timestampOn)) < 0) {
	WARN_errno( mSettings->mSock == SO_TIMESTAMP, "socket" );
    }
#endif
}

int Server::AlignPayloads (void) {
    int len=0;
    int n;
    uint32_t flags;
    // align the burst header
    if (isTripTime(mSettings) && !isUDP(mSettings) && \
	((n = recvn(mSettings->mSock, mBuf, 4, MSG_PEEK)) == 4)) {
	flags = ntohl((uint32_t) * mBuf);
	if ((flags & HEADER_EXTEND) != 0) {
	    len = SIZEOF_TCPHDRMSG_EXT;
	} else if ((flags & HEADER_VERSION1) != 0) {
	    len = SIZEOF_TCPHDRMSG_V1;
	}
	if (len && ((n = recvn(mSettings->mSock, mBuf, len, 0)) != len)) {
	    len = 0;
	}
    }
    return len;
}
//
// Set the report start times and next report times, options
// are now, the accept time or the first write time
//
inline void Server::SetReportStartTime (void) {
    if (!isReverse(mSettings) && isTripTime(mSettings)) {
	// Start times come from the sender's timestamp
	if (isUDP(mSettings)) {
	    // Trip times use the first packet's sent time
	    // RJM add some defense to the below
	    int n;
	    if ((n = recvn(mSettings->mSock, mBuf, sizeof(struct UDP_datagram), MSG_PEEK)) \
		== sizeof(struct UDP_datagram)) {
		struct UDP_datagram* mBuf_UDP  = (struct UDP_datagram *) mBuf;
		myReport->info.ts.startTime.tv_sec = ntohl(mBuf_UDP->tv_sec);
		myReport->info.ts.startTime.tv_usec = ntohl(mBuf_UDP->tv_usec);
	    }
	} else {
	    // Set up trip time values that don't change
	    struct TCP_burst_payload * mBuf_burst = (struct TCP_burst_payload *) mBuf;
	    if ((n = recvn(mSettings->mSock, mBuf, sizeof(struct TCP_burst_payload *), MSG_PEEK)) \
		== sizeof(struct TCP_burst_payload)) {
		myReport->info.ts.startTime.tv_sec =  = ntohl(mBuf_burst->start_tv_sec);
		myReport->info.ts.startTime.tv_usec =  = ntohl(mBuf_burst->start_tv_usec);
	    }
	}
	if (TimeZero(myReport->info.ts.startTime)) {
	    // Servers that aren't full duplex use the accept timestamp for start
	    myReport->info.tsreport.startTime.tv_sec = mSettings->accept_time.tv_sec;
	    myReport->info.tsreport.startTime.tv_usec = mSettings->accept_time.tv_usec;
	    fprintf(stderr, "--triptime set but failed to read sender's timestamp");
	}
    } else {
	now.setnow();
	myReport->info.ts.startTime.tv_sec = now.getSecs();
	myReport->info.ts.startTime.tv_usec = now.getUsecs();
    }
    myReport->info.ts.IPGstart = myReport->startTime;

    if (!TimeZero(reporthdr->intervalTime))
	TimeAdd(myReport->info.tsnextTime, myReport->info.tsintervalTime);
}

void Server::InitTrafficLoop (void) {
    //  copy the thread drop socket to this object such
    //  that the destructor can close it if needed
#if defined(HAVE_LINUX_FILTER_H) && defined(HAVE_AF_PACKET)
    if (mSettings->mSockDrop > 0)
        myDropSocket = mSettings->mSockDrop;
#endif
    // full duplex sockets need to be traffic synchronized
    int barrier_flag=0;
    if (isBidir(mSettings))
	barrier_flag = bidir_start_barrier(&mSettings->bidir_startstop);
    SetReportStartTime();
    if (barrier_flag && (myReport->FullDuplexReport != NULL)) {
	myReport>FullDuplexReport.info.ts.startTime = myReport->info.ts.startTime;
	myReport>FullDuplexReport.info.ts.nextTime = myReport->info.ts.nextTime;
    }
    // Initialze the reportstruct scratchpad
    reportstruct = &myJob->packetring->metapacket;
    reportstruct->packetID = 0;
    reportstruct->l2len = 0;
    reportstruct->l2errors = 0x0;

    // This will only have the connection report
    PostReport(myJob);

    if (isServerModeTime(mSettings) || (isModeTime(mSettings) && isServerReverse(mSettings))) {
#ifdef HAVE_SETITIMER
        int err;
        struct itimerval it;
	memset (&it, 0, sizeof (it));
	it.it_value.tv_sec = (int) (mSettings->mAmount / 100.0);
	it.it_value.tv_usec = (int) (10000 * (mSettings->mAmount -
					      it.it_value.tv_sec * 100.0));
	err = setitimer(ITIMER_REAL, &it, NULL);
	FAIL_errno( err != 0, "setitimer", mSettings );
#endif
        mEndTime.setnow();
        mEndTime.add( mSettings->mAmount / 100.0 );
    }
}

inline int Server::ReadWithRxTimestamp (int *readerr) {
    long currLen;
    int tsdone = 0;

#if HAVE_DECL_SO_TIMESTAMP
    cmsg = (struct cmsghdr *) &ctrl;
    currLen = recvmsg( mSettings->mSock, &message, mSettings->recvflags );
    if (currLen > 0) {
	if (cmsg->cmsg_level == SOL_SOCKET &&
	    cmsg->cmsg_type  == SCM_TIMESTAMP &&
	    cmsg->cmsg_len   == CMSG_LEN(sizeof(struct timeval))) {
	    memcpy(&(reportstruct->packetTime), CMSG_DATA(cmsg), sizeof(struct timeval));
	    tsdone = 1;
	}
    }
#else
    currLen = recv( mSettings->mSock, mBuf, mSettings->mBufLen, mSettings->recvflags);
#endif
    if (currLen <=0) {
	// Socket read timeout or read error
	reportstruct->emptyreport=1;
	// End loop on 0 read or socket error
	// except for socket read timeout
	if (currLen == 0 ||
#ifdef WIN32
	    (WSAGetLastError() != WSAEWOULDBLOCK)
#else
	    (errno != EAGAIN && errno != EWOULDBLOCK)
#endif
	    ) {
	    WARN_errno( currLen, "recvmsg");
	    *readerr = 1;
	}
	currLen= 0;
    }

    if (!tsdone) {
	now.setnow();
	reportstruct->packetTime.tv_sec = now.getSecs();
	reportstruct->packetTime.tv_usec = now.getUsecs();
    }
    return currLen;
}

// Returns true if the client has indicated this is the final packet
inline bool Server::ReadPacketID (void) {
    bool terminate = false;
    struct UDP_datagram* mBuf_UDP  = (struct UDP_datagram*) (mBuf + mSettings->l4payloadoffset);

    // terminate when datagram begins with negative index
    // the datagram ID should be correct, just negated

    if (isSeqNo64b(mSettings)) {
      // New client - Signed PacketID packed into unsigned id2,id
      reportstruct->packetID = ((uint32_t)ntohl(mBuf_UDP->id)) | ((uintmax_t)(ntohl(mBuf_UDP->id2)) << 32);

#ifdef SHOW_PACKETID
      printf("id 0x%x, 0x%x -> %" PRIdMAX " (0x%" PRIxMAX ")\n",
	     ntohl(mBuf_UDP->id), ntohl(mBuf_UDP->id2), reportstruct->packetID, reportstruct->packetID);
#endif
    } else {
      // Old client - Signed PacketID in Signed id
      reportstruct->packetID = (int32_t)ntohl(mBuf_UDP->id);
#ifdef SHOW_PACKETID
      printf("id 0x%x -> %" PRIdMAX " (0x%" PRIxMAX ")\n",
	     ntohl(mBuf_UDP->id), reportstruct->packetID, reportstruct->packetID);
#endif
    }
    if (reportstruct->packetID < 0) {
      reportstruct->packetID = - reportstruct->packetID;
      terminate = true;
    }
    // read the sent timestamp from the rx packet
    reportstruct->sentTime.tv_sec = ntohl( mBuf_UDP->tv_sec  );
    reportstruct->sentTime.tv_usec = ntohl( mBuf_UDP->tv_usec );
    return terminate;
}

void Server::L2_processing (void) {
#if defined(HAVE_LINUX_FILTER_H) && defined(HAVE_AF_PACKET)
    eth_hdr = (struct ether_header *) mBuf;
    ip_hdr = (struct iphdr *) (mBuf + sizeof(struct ether_header));
    // L4 offest is set by the listener and depends upon IPv4 or IPv6
    udp_hdr = (struct udphdr *) (mBuf + mSettings->l4offset);
    // Read the packet to get the UDP length
    int udplen = ntohs(udp_hdr->len);
    //
    // in the event of an L2 error, double check the packet before passing it to the reporter,
    // i.e. no reason to run iperf accounting on a packet that has no reasonable L3 or L4 headers
    //
    reportstruct->packetLen = udplen - sizeof(struct udphdr);
    reportstruct->expected_l2len = reportstruct->packetLen + mSettings->l4offset + sizeof(struct udphdr);
    if (reportstruct->l2len != reportstruct->expected_l2len) {
	reportstruct->l2errors |= L2LENERR;
	if (L2_quintuple_filter() != 0) {
	    reportstruct->l2errors |= L2UNKNOWN;
	    reportstruct->l2errors |= L2CSUMERR;
	    reportstruct->emptyreport = 1;
	}
    }
    if (!(reportstruct->l2errors & L2UNKNOWN)) {
	// perform UDP checksum test, returns zero on success
	int rc;
	rc = udpchecksum((void *)ip_hdr, (void *)udp_hdr, udplen, (isIPV6(mSettings) ? 1 : 0));
	if (rc) {
	    reportstruct->l2errors |= L2CSUMERR;
	    if ((!(reportstruct->l2errors & L2LENERR)) && (L2_quintuple_filter() != 0)) {
		reportstruct->emptyreport = 1;
		reportstruct->l2errors |= L2UNKNOWN;
	    }
	}
    }
#endif // HAVE_AF_PACKET
}

// Run the L2 packet through a quintuple check, i.e. proto/ip src/ip dst/src port/src dst
// and return zero is there is a match, otherwize return nonzero
int Server::L2_quintuple_filter(void) {
#if defined(HAVE_LINUX_FILTER_H) && defined(HAVE_AF_PACKET)

#define IPV4SRCOFFSET 12  // the ipv4 source address offset from the l3 pdu
#define IPV6SRCOFFSET 8 // the ipv6 source address offset

    // Get the expected values from the sockaddr structures
    // Note: it's expected the initiating socket has aready "connected"
    // and the sockaddr structs have been populated
    // 2nd Note:  sockaddr structs are in network byte order
    struct sockaddr *p = (sockaddr *)&mSettings->peer;
    struct sockaddr *l = (sockaddr *)&mSettings->local;
    // make sure sa_family is coherent for both src and dst
    if (!(((l->sa_family == AF_INET) && (p->sa_family == AF_INET)) || ((l->sa_family == AF_INET6) && (p->sa_family == AF_INET6)))) {
	return -1;
    }

    // check the L2 ethertype
    struct ether_header *l2hdr = (struct ether_header *)mBuf;

    if (!isIPV6(mSettings)) {
	if (ntohs(l2hdr->ether_type) != ETHERTYPE_IP)
	    return -1;
    } else {
	if (ntohs(l2hdr->ether_type) != ETHERTYPE_IPV6)
	    return -1;
    }
    // check the ip src/dst
    const uint32_t *data;
    udp_hdr = (struct udphdr *) (mBuf + mSettings->l4offset);

    // Check plain old v4 using v4 addr structs
    if (l->sa_family == AF_INET) {
	data = (const uint32_t *) (mBuf + sizeof(struct ether_header) + IPV4SRCOFFSET);
	if (((struct sockaddr_in *)(p))->sin_addr.s_addr != *data++)
	    return -1;
	if (((struct sockaddr_in *)(l))->sin_addr.s_addr != *data)
	    return -1;
	if (udp_hdr->source != ((struct sockaddr_in *)(p))->sin_port)
	    return -1;
	if (udp_hdr->dest != ((struct sockaddr_in *)(l))->sin_port)
	    return -1;
    } else {
	// Using the v6 addr structures
#  ifdef HAVE_IPV6
	struct in6_addr *v6peer = SockAddr_get_in6_addr(&mSettings->peer);
	struct in6_addr *v6local = SockAddr_get_in6_addr(&mSettings->local);
	if (isIPV6(mSettings)) {
	    int i;
	    data = (const uint32_t *) (mBuf + sizeof(struct ether_header) + IPV6SRCOFFSET);
	    // check for v6 src/dst address match
	    for (i = 0; i < 4; i++) {
		if (v6peer->s6_addr32[i] != *data++)
		    return -1;
	    }
	    for (i = 0; i < 4; i++) {
		if (v6local->s6_addr32[i] != *data++)
		    return -1;
	    }
	} else { // v4 addr in v6 family struct
	    data = (const uint32_t *) (mBuf + sizeof(struct ether_header) + IPV4SRCOFFSET);
	    if (v6peer->s6_addr32[3] != *data++)
		return -1;
	    if (v6peer->s6_addr32[3] != *data)
		return -1;
	}
	// check udp ports
	if (udp_hdr->source != ((struct sockaddr_in6 *)(p))->sin6_port)
	    return -1;
	if (udp_hdr->dest != ((struct sockaddr_in6 *)(l))->sin6_port)
	    return -1;
#  endif // HAVE_IPV6
    }
#endif // HAVE_AF_PACKET
    // made it through all the checks
    return 0;
}

inline void Server::Isoch_processing (int rxlen) {
    // Ignore runt sized isoch packets
    if (rxlen < (int) (sizeof(UDP_datagram) +  sizeof(client_hdr_v1) + sizeof(client_hdr_udp_isoch_tests))) {
	reportstruct->burstsize = 0;
	reportstruct->remaining = 0;
	reportstruct->frameID = 0;
    } else {
	struct client_hdr_udp_isoch_tests *testhdr = (client_hdr_udp_isoch_tests *)(mBuf + sizeof(client_hdr_v1) + sizeof(UDP_datagram));
	struct UDP_isoch_payload* mBuf_isoch = &(testhdr->isoch);
	reportstruct->isochStartTime.tv_sec = ntohl(mBuf_isoch->start_tv_sec);
	reportstruct->isochStartTime.tv_usec = ntohl(mBuf_isoch->start_tv_usec);
	reportstruct->frameID = ntohl(mBuf_isoch->frameid);
	reportstruct->prevframeID = ntohl(mBuf_isoch->prevframeid);
	reportstruct->burstsize = ntohl(mBuf_isoch->burstsize);
	reportstruct->burstperiod = ntohl(mBuf_isoch->burstperiod);
	reportstruct->remaining = ntohl(mBuf_isoch->remaining);
    }
}

/* -------------------------------------------------------------------
 * Receive UDP data from the (connected) socket.
 * Sends termination flag several times at the end.
 * Does not close the socket.
 * ------------------------------------------------------------------- */
void Server::RunUDP( void ) {
    int rxlen;
    int readerr = 0;
    bool lastpacket = 0;
    struct timeval prevsend = {.tv_sec = 0, .tv_usec = 0};

    InitTrafficLoop();

    // Exit loop on three conditions
    // 1) Fatal read error
    // 2) Last packet of traffic flow sent by client
    // 3) -t timer expires
    while (InProgress() && !readerr && !lastpacket) {
	// The emptyreport flag can be set
	// by any of the packet processing routines
	// If it's set the iperf reporter won't do
	// bandwidth accounting, basically it's indicating
	// that the reportstruct itself couldn't be
	// completely filled out.
	reportstruct->emptyreport=1;
	reportstruct->packetLen=0;
	// read the next packet with timestamp
	// will also set empty report or not
	rxlen=ReadWithRxTimestamp(&readerr);
	if (!readerr && (rxlen > 0)) {
	    reportstruct->emptyreport = 0;
	    reportstruct->packetLen = rxlen;
	    if (isL2LengthCheck(mSettings)) {
		reportstruct->l2len = rxlen;
		// L2 processing will set the reportstruct packet length with the length found in the udp header
		// and also set the expected length in the report struct.  The reporter thread
		// will do the compare and account and print l2 errors
		reportstruct->l2errors = 0x0;
		L2_processing();
	    }
	    if (!(reportstruct->l2errors & L2UNKNOWN)) {
		// ReadPacketID returns true if this is the last UDP packet sent by the client
		// aslo sets the packet rx time in the reportstruct
		reportstruct->prevSentTime = prevsend;
		lastpacket = ReadPacketID();
		prevsend = reportstruct->sentTime;
		if (isIsochronous(mSettings)) {
		    Isoch_processing(rxlen);
		}
	    }
	}
	ReportPacket(myJob, reportstruct);
    }
    CloseReport(myJob, reportstruct);

    if (!isMulticast(mSettings) && !isNoUDPfin(mSettings)) {
	// send a UDP acknowledgement back except when:
	// 1) we're NOT receiving multicast
	// 2) the user requested no final exchange
	write_UDP_AckFIN((struct TransferInfo *)&myJob->report.info);
    }

}
// end Recv


// A reverse server thread needs to block on a read being ready
void Server::FirstReadBarrier() {
    fd_set readSet;
    FD_ZERO( &readSet );
    struct timeval timeout;
    // wait until the socket is readable, or our timeout expires
    FD_SET( mSettings->mSock, &readSet );
    if (isModeTime(mSettings)) {
	timeout.tv_sec = (int) (mSettings->mAmount / 100.0);
	timeout.tv_usec = (int) (10000 * (mSettings->mAmount - timeout.tv_sec * 100.0));
	if ((timeout.tv_sec -= SLOPSECS) < SLOPSECS)
	    timeout.tv_sec = SLOPSECS;
    } else {
	timeout.tv_sec  = SLOPSECS;
	timeout.tv_usec = 0;
    }
#ifdef HAVE_THREAD_DEBUG
    thread_debug("Server reverse block on first read with timeout %ld.%ld (sock=%d)", timeout.tv_sec, timeout.tv_usec, mSettings->mSock);
#endif
    int rc = select( mSettings->mSock+1, &readSet, NULL, NULL, &timeout );
    FAIL_errno( rc == SOCKET_ERROR, "select", mSettings );
    if ( rc == 0 ) {
	FAIL_errno( 1, "select timeout", mSettings );
    }
#ifdef HAVE_THREAD_DEBUG
    thread_debug("Server reverse read ready (sock=%d)", mSettings->mSock);
#endif
}
/* -------------------------------------------------------------------
 * Send an AckFIN (a datagram acknowledging a FIN) on the socket,
 * then select on the socket for some time. If additional datagrams
 * come in, probably our AckFIN was lost and they are re-transmitted
 * termination datagrams, so re-transmit our AckFIN.
 * ------------------------------------------------------------------- */
void Server::write_UDP_AckFIN (struct TransferInfo *stats) {
    assert(stats!= NULL);
    int ackpacket_length = (int) (sizeof(UDP_datagram) + sizeof(server_hdr));
    // Make sure the final server report has a large enough packet
    if (mSettings->mBufLen < ackpacket_length) {
	DELETE_ARRAY(mBuf);
	mSettings->mBufLen = ackpacket_length;
	mBuf = new char[mSettings->mBufLen]; // defined in payloads.h
	FAIL_errno(mBuf == NULL, "No memory for buffer per final packet\n", mSettings);
    }

    struct UDP_datagram *UDP_Hdr = (struct UDP_datagram *)mBuf;
    struct server_hdr *hdr = (struct server_hdr *)(UDP_Hdr+1);

    UDP_Hdr = (UDP_datagram*) mBuf;
    int flags = (!isEnhanced(mSettings) ? HEADER_VERSION1 : (HEADER_VERSION1 | HEADER_EXTEND));
#ifdef HAVE_INT64_T
    flags |=  HEADER_SEQNO64B;
#endif
    hdr->base.flags        = htonl((long) flags);
#ifdef HAVE_INT64_T
    hdr->base.total_len1   = htonl((long) (stats->TotalLen >> 32));
#else
    hdr->base.total_len1   = htonl(0x0);
#endif
    hdr->base.total_len2   = htonl((long) (stats->TotalLen & 0xFFFFFFFF));
    hdr->base.stop_sec     = htonl((long) stats->endTime);
    hdr->base.stop_usec    = htonl((long)((stats->endTime - (long)stats->endTime) * rMillion));
    hdr->base.error_cnt    = htonl((long) (stats->cntError & 0xFFFFFFFF));
    hdr->base.outorder_cnt = htonl((long) (stats->cntOutofOrder  & 0xFFFFFFFF));
    hdr->base.datagrams    = htonl((long) (stats->cntDatagrams & 0xFFFFFFFF));
    if (flags & HEADER_SEQNO64B) {
	hdr->extend2.error_cnt2    = htonl((long) (stats->cntError >> 32));
	hdr->extend2.outorder_cnt2 = htonl((long) (stats->cntOutofOrder >> 32) );
	hdr->extend2.datagrams2    = htonl((long) (stats->cntDatagrams >> 32));
    }
    hdr->base.jitter1      = htonl((long) stats->jitter);
    hdr->base.jitter2      = htonl((long) ((stats->jitter - (long)stats->jitter) * rMillion));
    if (flags & HEADER_EXTEND) {
	hdr->extend.minTransit1  = htonl((long) stats->transit.totminTransit);
	hdr->extend.minTransit2  = htonl((long) ((stats->transit.totminTransit - (long)stats->transit.totminTransit) * rMillion));
	hdr->extend.maxTransit1  = htonl((long) stats->transit.totmaxTransit);
	hdr->extend.maxTransit2  = htonl((long) ((stats->transit.totmaxTransit - (long)stats->transit.totmaxTransit) * rMillion));
	hdr->extend.sumTransit1  = htonl((long) stats->transit.totsumTransit);
	hdr->extend.sumTransit2  = htonl((long) ((stats->transit.totsumTransit - (long)stats->transit.totsumTransit) * rMillion));
	hdr->extend.meanTransit1  = htonl((long) stats->transit.totmeanTransit);
	hdr->extend.meanTransit2  = htonl((long) ((stats->transit.totmeanTransit - (long)stats->transit.totmeanTransit) * rMillion));
	hdr->extend.m2Transit1  = htonl((long) stats->transit.totm2Transit);
	hdr->extend.m2Transit2  = htonl((long) ((stats->transit.totm2Transit - (long)stats->transit.totm2Transit) * rMillion));
	hdr->extend.vdTransit1  = htonl((long) stats->transit.totvdTransit);
	hdr->extend.vdTransit2  = htonl((long) ((stats->transit.totvdTransit - (long)stats->transit.totvdTransit) * rMillion));
	hdr->extend.cntTransit   = htonl(stats->transit.totcntTransit);
	hdr->extend.IPGcnt = htonl((long) (stats->cntDatagrams / (stats->endTime - stats->startTime)));
	hdr->extend.IPGsum = htonl(1);
    }

    fd_set readSet;
    FD_ZERO(&readSet);

#define TRYCOUNT 40
    int count = TRYCOUNT;
    boolean success = false;
    while (--count) {
	int rc;
	struct timeval timeout;
        // write data
#if defined(HAVE_LINUX_FILTER_H) && defined(HAVE_AF_PACKET)
	// If in l2mode, use the AF_INET socket to write this packet
	//
	write(((mSettings->mSockDrop > 0) ? mSettings->mSockDrop : mSettings->mSock), mBuf, ackpacket_length);
#else
	write(mSettings->mSock, mBuf, ackpacket_length);
#endif
        // wait until the socket is readable, or our timeout expires
        FD_SET(mSettings->mSock, &readSet);
        timeout.tv_sec  = 0;
        timeout.tv_usec = 250000;
        rc = select(mSettings->mSock+1, &readSet, NULL, NULL, &timeout);
        FAIL_errno(rc == SOCKET_ERROR, "select", mSettings);
        if (rc == 0) {
	    continue; //select timeout
	}
        rc = read(mSettings->mSock, mBuf, mSettings->mBufLen);
        WARN_errno(rc < 0, "read");
	if (rc > 0) {
	    success = true;
	    break;
	}
    }
    if (!success)
	fprintf(stderr, warn_ack_failed, mSettings->mSock, TRYCOUNT);
}
// end write_UDP_AckFIN
