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
 * Settings.cpp
 * by Mark Gates <mgates@nlanr.net>
 * & Ajay Tirumala <tirumala@ncsa.uiuc.edu>
 * -------------------------------------------------------------------
 * Stores and parses the initial values for all the global variables.
 * -------------------------------------------------------------------
 * headers
 * uses
 *   <stdlib.h>
 *   <stdio.h>
 *   <string.h>
 *
 *   <unistd.h>
 * ------------------------------------------------------------------- */

#define HEADERS()

#include "headers.h"
#include "Settings.hpp"
#include "Locale.h"
#include "SocketAddr.h"
#include "util.h"
#include "version.h"
#include "gnu_getopt.h"
#include "isochronous.hpp"
#include "pdfs.h"
#include "payloads.h"

static int reversetest = 0;
static int fullduplextest = 0;
static int rxhistogram = 0;
static int l2checks = 0;
static int incrdstip = 0;
static int txstarttime = 0;
static int noconnectsync = 0;
static int txholdback = 0;
static int fqrate = 0;
static int triptime = 0;
static int writeack = 0;
static int infinitetime = 0;
static int connectonly = 0;
static int connectretry = 0;
static int burstipg = 0;
static int isochronous = 0;
static int noudpfin = 0;
static int numreportstructs = 0;
static int sumonly = 0;

void Settings_Interpret(char option, const char *optarg, struct thread_Settings *mExtSettings);
// apply compound settings after the command line has been fully parsed
void Settings_ModalOptions(struct thread_Settings *mExtSettings);


/* -------------------------------------------------------------------
 * command line options
 *
 * The option struct essentially maps a long option name (--foobar)
 * or environment variable ($FOOBAR) to its short option char (f).
 * ------------------------------------------------------------------- */
#define LONG_OPTIONS()

const struct option long_options[] =
{
{"singleclient",     no_argument, NULL, '1'},
{"bandwidth",  required_argument, NULL, 'b'},
{"client",     required_argument, NULL, 'c'},
{"dualtest",         no_argument, NULL, 'd'},
{"enhanced",         no_argument, NULL, 'e'},
{"format",     required_argument, NULL, 'f'},
{"help",             no_argument, NULL, 'h'},
{"interval",   required_argument, NULL, 'i'},
{"len",        required_argument, NULL, 'l'},
{"print_mss",        no_argument, NULL, 'm'},
{"num",        required_argument, NULL, 'n'},
{"output",     required_argument, NULL, 'o'},
{"port",       required_argument, NULL, 'p'},
{"tradeoff",         no_argument, NULL, 'r'},
{"server",           no_argument, NULL, 's'},
{"time",       required_argument, NULL, 't'},
{"udp",              no_argument, NULL, 'u'},
{"version",          no_argument, NULL, 'v'},
{"window",     required_argument, NULL, 'w'},
{"reportexclude", required_argument, NULL, 'x'},
{"reportstyle",required_argument, NULL, 'y'},
{"realtime",         no_argument, NULL, 'z'},

// more esoteric options
{"awdl",             no_argument, NULL, 'A'},
{"bind",       required_argument, NULL, 'B'},
{"compatibility",    no_argument, NULL, 'C'},
{"daemon",           no_argument, NULL, 'D'},
{"file_input", required_argument, NULL, 'F'},
{"ssm-host", required_argument, NULL, 'H'},
{"stdin_input",      no_argument, NULL, 'I'},
{"mss",        required_argument, NULL, 'M'},
{"nodelay",          no_argument, NULL, 'N'},
{"listenport", required_argument, NULL, 'L'},
{"parallel",   required_argument, NULL, 'P'},
#ifdef WIN32
{"remove",           no_argument, NULL, 'R'},
#else
{"reverse",          no_argument, NULL, 'R'},
#endif
{"tos",        required_argument, NULL, 'S'},
{"ttl",        required_argument, NULL, 'T'},
{"single_udp",       no_argument, NULL, 'U'},
{"ipv6_domain",      no_argument, NULL, 'V'},
{"suggest_win_size", no_argument, NULL, 'W'},
{"peer-detect",      no_argument, NULL, 'X'},
{"tcp-congestion", required_argument, NULL, 'Z'},
{"histograms", optional_argument, &rxhistogram, 1},
{"l2checks", no_argument, &l2checks, 1},
{"incr-dstip", no_argument, &incrdstip, 1},
{"txstart-time", required_argument, &txstarttime, 1},
{"txdelay-time", required_argument, &txholdback, 1},
{"fq-rate", required_argument, &fqrate, 1},
{"trip-times", no_argument, &triptime, 1},
{"write-ack", optional_argument, &writeack, 1},
{"no-udp-fin", no_argument, &noudpfin, 1},
{"connect-only", optional_argument, &connectonly, 1},
{"connect-retries", required_argument, &connectretry, 1},
{"no-connect-sync", no_argument, &noconnectsync, 1},
{"full-duplex", no_argument, &fullduplextest, 1},
{"ipg", required_argument, &burstipg, 1},
{"isochronous", optional_argument, &isochronous, 1},
{"sum-only", no_argument, &sumonly, 1},
{"NUM_REPORT_STRUCTS", required_argument, &numreportstructs, 1},
#ifdef WIN32
{"reverse", no_argument, &reversetest, 1},
#endif
{0, 0, 0, 0}
};

#define ENV_OPTIONS()

const struct option env_options[] =
{
{"IPERF_IPV6_DOMAIN",      no_argument, NULL, 'V'},
{"IPERF_SINGLECLIENT",     no_argument, NULL, '1'},
{"IPERF_BANDWIDTH",  required_argument, NULL, 'b'},
{"IPERF_CLIENT",     required_argument, NULL, 'c'},
{"IPERF_DUALTEST",         no_argument, NULL, 'd'},
{"IPERF_ENHANCEDREPORTS",  no_argument, NULL, 'e'},
{"IPERF_FORMAT",     required_argument, NULL, 'f'},
// skip help
{"IPERF_INTERVAL",   required_argument, NULL, 'i'},
{"IPERF_LEN",        required_argument, NULL, 'l'},
{"IPERF_PRINT_MSS",        no_argument, NULL, 'm'},
{"IPERF_NUM",        required_argument, NULL, 'n'},
{"IPERF_PORT",       required_argument, NULL, 'p'},
{"IPERF_TRADEOFF",         no_argument, NULL, 'r'},
{"IPERF_SERVER",           no_argument, NULL, 's'},
{"IPERF_TIME",       required_argument, NULL, 't'},
{"IPERF_UDP",              no_argument, NULL, 'u'},
// skip version
{"TCP_WINDOW_SIZE",  required_argument, NULL, 'w'},
{"IPERF_REPORTEXCLUDE", required_argument, NULL, 'x'},
{"IPERF_REPORTSTYLE",required_argument, NULL, 'y'},

// more esoteric options
{"IPERF_BIND",       required_argument, NULL, 'B'},
{"IPERF_COMPAT",           no_argument, NULL, 'C'},
{"IPERF_DAEMON",           no_argument, NULL, 'D'},
{"IPERF_FILE_INPUT", required_argument, NULL, 'F'},
{"IPERF_STDIN_INPUT",      no_argument, NULL, 'I'},
{"IPERF_MSS",        required_argument, NULL, 'M'},
{"IPERF_NODELAY",          no_argument, NULL, 'N'},
{"IPERF_LISTENPORT", required_argument, NULL, 'L'},
{"IPERF_PARALLEL",   required_argument, NULL, 'P'},
{"IPERF_TOS",        required_argument, NULL, 'S'},
{"IPERF_TTL",        required_argument, NULL, 'T'},
{"IPERF_SINGLE_UDP",       no_argument, NULL, 'U'},
{"IPERF_SUGGEST_WIN_SIZE", required_argument, NULL, 'W'},
{"IPERF_PEER_DETECT", required_argument, NULL, 'X'},
{"IPERF_CONGESTION_CONTROL",  required_argument, NULL, 'Z'},
{0, 0, 0, 0}
};

#define SHORT_OPTIONS()

const char short_options[] = "1b:c:def:hi:l:mn:o:p:rst:uvw:x:y:zAB:CDF:H:IL:M:NP:RS:T:UVWXZ:";

/* -------------------------------------------------------------------
 * defaults
 * ------------------------------------------------------------------- */
#define DEFAULTS()

const long kDefault_UDPRate = 1024 * 1024; // -u  if set, 1 Mbit/sec
const int  kDefault_UDPBufLen = 1470;      // -u  if set, read/write 1470 bytes
// v4: 1470 bytes UDP payload will fill one and only one ethernet datagram (IPv4 overhead is 20 bytes)
const int  kDefault_UDPBufLenV6 = 1450;      // -u  if set, read/write 1470 bytes
// v6: 1450 bytes UDP payload will fill one and only one ethernet datagram (IPv6 overhead is 40 bytes)
const int kDefault_TCPBufLen = 128 * 1024; // TCP default read/write size

/* -------------------------------------------------------------------
 * Initialize all settings to defaults.
 * ------------------------------------------------------------------- */
void Settings_Initialize (struct thread_Settings *main) {
    // Everything defaults to zero or NULL with
    // this memset. Only need to set non-zero values
    // below.
    memset(main, 0, sizeof(struct thread_Settings));
    main->mSock = INVALID_SOCKET;
    main->mReportMode = kReport_Default;
    // option, defaults
    main->flags         = FLAG_MODETIME | FLAG_STDOUT; // Default time and stdout
    main->flags_extend  = 0x0;           // Default all extend flags to off
    //main->mAppRate      = 0;           // -b,  offered (or rate limited) load (both UDP and TCP)
    main->mAppRateUnits = kRate_BW;
    //main->mHost         = NULL;        // -c,  none, required for client
    main->mMode         = kTest_Normal;  // -d,  mMode == kTest_DualTest
    main->mFormat       = 'a';           // -f,  adaptive bits
    // skip help                         // -h,
    //main->mBufLenSet  = false;         // -l,
    main->mBufLen       = kDefault_TCPBufLen; // -l,  Default to TCP read/write size
    //main->mInterval     = 0;           // -i,  ie. no periodic bw reports
    //main->mPrintMSS   = false;         // -m,  don't print MSS
    // mAmount is time also              // -n,  N/A
    //main->mOutputFileName = NULL;      // -o,  filename
    main->mPort         = 5001;          // -p,  ttcp port
    main->mBindPort     = 0;             // -B,  default port for bind
    // mMode    = kTest_Normal;          // -r,  mMode == kTest_TradeOff
    main->mThreadMode   = kMode_Unknown; // -s,  or -c, none
    main->mAmount       = 1000;          // -t,  10 seconds
    main->mIntervalMode = kInterval_None;// -i   none, time, packets, or bursts
    // skip version                      // -v,
    //main->mTCPWin       = 0;           // -w,  ie. don't set window

    // more esoteric options
    //main->mLocalhost    = NULL;        // -B,  none
    //main->mCompat     = false;         // -C,  run in Compatibility mode
    //main->mDaemon     = false;         // -D,  run as a daemon
    //main->mFileInput  = false;         // -F,
    //main->mFileName     = NULL;        // -F,  filename
    //main->mStdin      = false;         // -I,  default not stdin
    //main->mListenPort   = 0;           // -L,  listen port
    //main->mMSS          = 0;           // -M,  ie. don't set MSS
    //main->mNodelay    = false;         // -N,  don't set nodelay
    //main->mThreads      = 0;           // -P,
    //main->mRemoveService = false;      // -R,
    //main->mTOS          = 0;           // -S,  ie. don't set type of service
    main->mTTL          = -1;            // -T,  link-local TTL
    //main->mDomain     = kMode_IPv4;    // -V,
    //main->mSuggestWin = false;         // -W,  Suggest the window size.

} // end Settings

void Settings_Copy (struct thread_Settings *from, struct thread_Settings **into, int copyall) {
    *into = new struct thread_Settings;
    memset(*into, 0, sizeof(struct thread_Settings));
    memcpy(*into, from, sizeof(struct thread_Settings));
    (*into)->mSumReport = NULL;
#ifdef HAVE_THREAD_DEBUG
    thread_debug("Copy thread settings (malloc) from/to=%p/%p report/sum/fullduplex %p/%p/%p", \
		 (void *)from, (void *)*into, (void *)(*into)->reporthdr, (void *)(*into)->mSumReport, (void *)(*into)->mFullDuplexReport);
#endif
    // Some settings don't need to be copied and will confuse things. Don't copy them unless copyall is set
    if (copyall) {
	// Don't allocate memory for these if this is a reverse client
	if (from->mHost != NULL) {
	    (*into)->mHost = new char[ strlen(from->mHost) + 1];
	    strcpy((*into)->mHost, from->mHost);
	}
	if (from->mOutputFileName != NULL) {
	    (*into)->mOutputFileName = new char[ strlen(from->mOutputFileName) + 1];
	    strcpy((*into)->mOutputFileName, from->mOutputFileName);
	}
	if (from->mLocalhost != NULL) {
	    (*into)->mLocalhost = new char[ strlen(from->mLocalhost) + 1];
	    strcpy((*into)->mLocalhost, from->mLocalhost);
	}
	if (from->mFileName != NULL) {
	    (*into)->mFileName = new char[ strlen(from->mFileName) + 1];
	    strcpy((*into)->mFileName, from->mFileName);
	}
	if (from->mRxHistogramStr != NULL) {
	    (*into)->mRxHistogramStr = new char[ strlen(from->mRxHistogramStr) + 1];
	    strcpy((*into)->mRxHistogramStr, from->mRxHistogramStr);
	}
	if (from->mSSMMulticastStr != NULL) {
	    (*into)->mSSMMulticastStr = new char[ strlen(from->mSSMMulticastStr) + 1];
	    strcpy((*into)->mSSMMulticastStr, from->mSSMMulticastStr);
	}
	if (from->mIfrname != NULL) {
	    (*into)->mIfrname = (char *) calloc(1, strlen(from->mIfrname) + 1);
	    strcpy((*into)->mIfrname, from->mIfrname);
	}
	if (from->mIfrnametx != NULL) {
	    (*into)->mIfrnametx = (char *) calloc(1, strlen(from->mIfrnametx) + 1);
	    strcpy((*into)->mIfrnametx, from->mIfrnametx);
	}
	if (from->mIsochronousStr != NULL) {
	    (*into)->mIsochronousStr = new char[ strlen(from->mIsochronousStr) + 1];
	    strcpy((*into)->mIsochronousStr, from->mIsochronousStr);
	}
	if (from->mCongestion != NULL) {
	    (*into)->mCongestion = new char[ strlen(from->mCongestion) + 1];
	    strcpy((*into)->mCongestion, from->mCongestion);
	}
    } else {
	(*into)->mHost = NULL;
	(*into)->mOutputFileName = NULL;
	(*into)->mLocalhost = NULL;
	(*into)->mFileName = NULL;
	(*into)->mRxHistogramStr = NULL;
	(*into)->mSSMMulticastStr = NULL;
	(*into)->mIfrname = NULL;
	(*into)->mIfrnametx = NULL;
	(*into)->mIsochronousStr = NULL;
	(*into)->mCongestion = NULL;
	(*into)->mTransferIDStr = NULL;
	// apply the server side congestion setting to reverse clients
	if (from->mIsochronousStr != NULL) {
	    (*into)->mIsochronousStr = new char[ strlen(from->mIsochronousStr) + 1];
	    strcpy((*into)->mIsochronousStr, from->mIsochronousStr);
	}
    }

    (*into)->txstart_epoch = from->txstart_epoch;
    (*into)->mSumReport = from->mSumReport;
    (*into)->mFullDuplexReport = from->mFullDuplexReport;

    // Zero out certain entries
    (*into)->mTID = thread_zeroid();
    (*into)->runNext = NULL;
    (*into)->runNow = NULL;
#if defined(HAVE_LINUX_FILTER_H) && defined(HAVE_AF_PACKET)
    (*into)->mSockDrop = INVALID_SOCKET;
#endif
    Condition_Initialize(&(*into)->awake_me);
    // default copied settings to no reporter reporting
    unsetReport((*into));
}

/* -------------------------------------------------------------------
 * Delete memory: Does not clean up open file pointers or ptr_parents
 * ------------------------------------------------------------------- */

void Settings_Destroy (struct thread_Settings *mSettings) {
#if HAVE_THREAD_DEBUG
    thread_debug("Free thread settings=%p", mSettings);
#endif
    Condition_Destroy(&mSettings->awake_me);
    DELETE_ARRAY(mSettings->mHost);
    DELETE_ARRAY(mSettings->mLocalhost);
    DELETE_ARRAY(mSettings->mFileName);
    DELETE_ARRAY(mSettings->mOutputFileName);
    DELETE_ARRAY(mSettings->mRxHistogramStr);
    DELETE_ARRAY(mSettings->mSSMMulticastStr);
    DELETE_ARRAY(mSettings->mCongestion);
    FREE_ARRAY(mSettings->mIfrname);
    FREE_ARRAY(mSettings->mIfrnametx);
    FREE_ARRAY(mSettings->mTransferIDStr);
    DELETE_ARRAY(mSettings->mIsochronousStr);
    DELETE_PTR(mSettings);
} // end ~Settings

/* -------------------------------------------------------------------
 * Parses settings from user's environment variables.
 * ------------------------------------------------------------------- */
void Settings_ParseEnvironment (struct thread_Settings *mSettings) {
    char *theVariable;

    int i = 0;
    while (env_options[i].name != NULL) {
        theVariable = getenv(env_options[i].name);
        if (theVariable != NULL) {
            Settings_Interpret(env_options[i].val, theVariable, mSettings);
        }
        i++;
    }
} // end ParseEnvironment

/* -------------------------------------------------------------------
 * Parse settings from app's command line.
 * ------------------------------------------------------------------- */

void Settings_ParseCommandLine (int argc, char **argv, struct thread_Settings *mSettings) {
    int option;
    gnu_opterr = 1; // Fail on an unrecognized command line option
    while ((option =
             gnu_getopt_long(argc, argv, short_options,
                              long_options, NULL)) != EOF) {
        Settings_Interpret(option, gnu_optarg, mSettings);
    }

    for (int i = gnu_optind; i < argc; i++) {
        fprintf(stderr, "%s: ignoring extra argument -- %s\n", argv[0], argv[i]);
    }
    // Determine the modal or compound settings now that the full command line has been parsed
    Settings_ModalOptions(mSettings);

} // end ParseCommandLine

/* -------------------------------------------------------------------
 * Interpret individual options, either from the command line
 * or from environment variables.
 * ------------------------------------------------------------------- */

void Settings_Interpret (char option, const char *optarg, struct thread_Settings *mExtSettings) {
    char *results;
    switch (option) {
        case '1': // Single Client
            setSingleClient(mExtSettings);
            break;

        case 'b': // UDP bandwidth
	    {
		char *tmp= new char [strlen(optarg) + 1];
		strcpy(tmp, optarg);
		// scan for PPS units, just look for 'p' as that's good enough
		if ((((results = strtok(tmp, "p")) != NULL) && strcmp(results,optarg)) \
		    || (((results = strtok(tmp, "P")) != NULL)  && strcmp(results,optarg))) {
		    mExtSettings->mAppRateUnits = kRate_PPS;
		    mExtSettings->mAppRate = byte_atoi(results);
		} else {
		    mExtSettings->mAppRateUnits = kRate_BW;
		    mExtSettings->mAppRate = byte_atoi(optarg);
		    if (((results = strtok(tmp, ",")) != NULL) && strcmp(results,optarg)) {
			setVaryLoad(mExtSettings);
			mExtSettings->mVariance = byte_atoi(optarg);
		    }
		}
		delete [] tmp;
	    }
	    setBWSet(mExtSettings);
	    break;
        case 'c': // client mode w/ server host to connect to
            mExtSettings->mHost = new char[ strlen(optarg) + 1 ];
            strcpy(mExtSettings->mHost, optarg);

            if (mExtSettings->mThreadMode == kMode_Unknown) {
                mExtSettings->mThreadMode = kMode_Client;
                mExtSettings->mThreads = 1;
            }
            break;

        case 'd': // Dual-test Mode
            if (mExtSettings->mThreadMode != kMode_Client) {
                fprintf(stderr, warn_invalid_server_option, option);
                break;
            }
            if (isCompat(mExtSettings)) {
                fprintf(stderr, warn_invalid_compatibility_option, option);
            }
#ifdef HAVE_THREAD
            mExtSettings->mMode = kTest_DualTest;
#else
            fprintf(stderr, warn_invalid_single_threaded, option);
            mExtSettings->mMode = kTest_TradeOff;
#endif
            break;
        case 'e': // Use enhanced reports
            setEnhanced(mExtSettings);
            break;
        case 'f': // format to print in
            mExtSettings->mFormat = (*optarg);
            break;

        case 'h': // print help and exit
	    fprintf(stderr, "%s", usage_long1);
            fprintf(stderr, "%s", usage_long2);
            exit(1);
            break;

        case 'i': // specify interval between periodic bw reports
	    {
		char framechar;
		char *tmp= new char [strlen(optarg) + 1];
		strcpy(tmp, optarg);
		// scan for frames as units
		if ((sscanf(optarg,"%c", &framechar)) && ((framechar == 'f') || (framechar == 'F'))) {
		    mExtSettings->mIntervalMode = kInterval_Frames;
		    setEnhanced(mExtSettings);
		    setFrameInterval(mExtSettings);
		} else {
		    char *end;
		    strcpy(tmp, optarg);
		    double itime = strtof(optarg, &end);
		    if (*end != '\0') {
			fprintf (stderr, "Invalid value of '%s' for -i interval\n", optarg);
			exit(1);
		    }
		    if (itime > (UINT_MAX / 1e6)) {
			fprintf (stderr, "Too large value of '%s' for -i interval, max is %f\n", optarg, (UINT_MAX / 1e6));
			exit(1);
		    }
		    mExtSettings->mInterval = (unsigned int) (itime * 1e6);
		    if (!mExtSettings->mInterval) {
			fprintf (stderr, "Interval per -i cannot be zero\n");
			exit(1);
		    }
		    mExtSettings->mIntervalMode = kInterval_Time;
		    if (mExtSettings->mInterval < SMALLEST_INTERVAL) {
			mExtSettings->mInterval = SMALLEST_INTERVAL;
#ifndef HAVE_FASTSAMPLING
			fprintf (stderr, report_interval_small, mExtSettings->mInterval);
#endif
		    }
		}
		delete [] tmp;
	    }
            break;

        case 'l': // length of each buffer
            mExtSettings->mBufLen = byte_atoi(optarg);
            setBuflenSet(mExtSettings);
            break;

        case 'm': // print TCP MSS
            setPrintMSS(mExtSettings);
            break;

        case 'n': // bytes of data
            // amount mode (instead of time mode)
            unsetModeTime(mExtSettings);
            mExtSettings->mAmount = byte_atoi(optarg);
	    if (!(mExtSettings->mAmount > 0)) {
		fprintf (stderr, "Invalid value for -n amount of '%s'\n", optarg);
	        exit(1);
	    }
            break;

        case 'o' : // output the report and other messages into the file
            unsetSTDOUT(mExtSettings);
            mExtSettings->mOutputFileName = new char[strlen(optarg)+1];
            strcpy(mExtSettings->mOutputFileName, optarg);
            break;

        case 'p': // server port
            mExtSettings->mPort = atoi(optarg);
            break;

        case 'r': // test mode tradeoff
            if (mExtSettings->mThreadMode != kMode_Client) {
                fprintf(stderr, warn_invalid_server_option, option);
                break;
            }
            if (isCompat(mExtSettings)) {
                fprintf(stderr, warn_invalid_compatibility_option, option);
            }
            mExtSettings->mMode = kTest_TradeOff;
            break;

        case 's': // server mode
            if (mExtSettings->mThreadMode != kMode_Unknown) {
                fprintf(stderr, warn_invalid_client_option, option);
                break;
            }

            mExtSettings->mThreadMode = kMode_Listener;
            break;

        case 't': // seconds to run the client, server, listener
            // time mode (instead of amount mode), units is 10 ms
            setModeTime(mExtSettings);
            setServerModeTime(mExtSettings);
	    if (atof(optarg) > 0.0)
                mExtSettings->mAmount = (size_t) (atof(optarg) * 100.0);
	    else
	        infinitetime = 1;
            break;

        case 'u': // UDP instead of TCP
	    setUDP(mExtSettings);
            break;

        case 'v': // print version and exit
	    fprintf(stderr, "%s", version);
            exit(1);
            break;

        case 'w': // TCP window size (socket buffer size)
            mExtSettings->mTCPWin = byte_atoi(optarg);
            if (mExtSettings->mTCPWin < 2048) {
                fprintf(stderr, warn_window_small, mExtSettings->mTCPWin);
            }
            break;

        case 'x': // Limit Reports
            while (*optarg != '\0') {
                switch (*optarg) {
                    case 's':
                    case 'S':
                        setNoSettReport(mExtSettings);
                        break;
                    case 'c':
                    case 'C':
                        setNoConnReport(mExtSettings);
                        break;
                    case 'd':
                    case 'D':
                        setNoDataReport(mExtSettings);
                        break;
                    case 'v':
                    case 'V':
                        setNoServReport(mExtSettings);
                        break;
                    case 'm':
                    case 'M':
                        setNoMultReport(mExtSettings);
                        break;
                    default:
                        fprintf(stderr, warn_invalid_report, *optarg);
                }
                optarg++;
            }
            break;
#if HAVE_SCHED_SETSCHEDULER
        case 'z': // Use realtime scheduling
	    setRealtime(mExtSettings);
            break;
#endif

        case 'y': // Reporting Style
            switch (*optarg) {
	    case 'c':
	    case 'C':
		mExtSettings->mReportMode = kReport_CSV;
		setNoSettReport(mExtSettings);
		setNoConnReport(mExtSettings);
		break;
	    default:
		fprintf(stderr, warn_invalid_report_style, optarg);
            }
            break;

            // more esoteric options

        case 'B': // specify bind address
	    if (mExtSettings->mLocalhost == NULL) {
		mExtSettings->mLocalhost = new char[ strlen(optarg) + 1 ];
		strcpy(mExtSettings->mLocalhost, optarg);
	    }
            break;

        case 'C': // Run in Compatibility Mode, i.e. no intial nor final header messaging
            setCompat(mExtSettings);
            if (mExtSettings->mMode != kTest_Normal) {
                fprintf(stderr, warn_invalid_compatibility_option,
                        (mExtSettings->mMode == kTest_DualTest ?
                          'd' : 'r'));
                mExtSettings->mMode = kTest_Normal;
            }
            break;

        case 'D': // Run as a daemon
            setDaemon(mExtSettings);
            break;

        case 'F' : // Get the input for the data stream from a file
            if (mExtSettings->mThreadMode != kMode_Client) {
                fprintf(stderr, warn_invalid_server_option, option);
                break;
            }

            setFileInput(mExtSettings);
            mExtSettings->mFileName = new char[strlen(optarg)+1];
            strcpy(mExtSettings->mFileName, optarg);
            break;

        case 'H' : // Get the SSM host (or Source per the S,G)
            if (mExtSettings->mThreadMode == kMode_Client) {
                fprintf(stderr, warn_invalid_client_option, option);
                break;
            }
            mExtSettings->mSSMMulticastStr = new char[strlen(optarg)+1];
            strcpy(mExtSettings->mSSMMulticastStr, optarg);
            setSSMMulticast(mExtSettings);
            break;

        case 'I' : // Set the stdin as the input source
            if (mExtSettings->mThreadMode != kMode_Client) {
                fprintf(stderr, warn_invalid_server_option, option);
                break;
            }

            setFileInput(mExtSettings);
            setSTDIN(mExtSettings);
            mExtSettings->mFileName = new char[strlen("<stdin>")+1];
            strcpy(mExtSettings->mFileName,"<stdin>");
            break;

        case 'L': // Listen Port (bidirectional testing client-side)
            if (mExtSettings->mThreadMode != kMode_Client) {
                fprintf(stderr, warn_invalid_server_option, option);
                break;
            }
            mExtSettings->mListenPort = atoi(optarg);
            break;

        case 'M': // specify TCP MSS (maximum segment size)
            mExtSettings->mMSS = byte_atoi(optarg);
            setPrintMSS(mExtSettings);
            break;

        case 'N': // specify TCP nodelay option (disable Jacobson's Algorithm)
            setNoDelay(mExtSettings);
            break;

        case 'P': // number of client threads
#ifdef HAVE_THREAD
            mExtSettings->mThreads = atoi(optarg);
#else
            if (mExtSettings->mThreadMode != kMode_Server) {
                fprintf(stderr, warn_invalid_single_threaded, option);
            } else {
                mExtSettings->mThreads = atoi(optarg);
            }
#endif
            break;
#ifdef WIN32
        case 'R':
            setRemoveService(mExtSettings);
            break;
#else
        case 'R':
	    setReverse(mExtSettings);
            break;
#endif

        case 'S': // IP type-of-service
            // TODO use a function that understands base-2
            // the zero base here allows the user to specify
            // "0x#" hex, "0#" octal, and "#" decimal numbers
            mExtSettings->mTOS = strtol(optarg, NULL, 0);
            break;

        case 'T': // time-to-live for both unicast and multicast
            mExtSettings->mTTL = atoi(optarg);
            break;

        case 'U': // single threaded UDP server
	    setUDP(mExtSettings);
            setSingleUDP(mExtSettings);
            setSingleClient(mExtSettings);
            break;

        case 'V': // IPv6 Domain
#ifdef HAVE_IPV6
            setIPV6(mExtSettings);
#else
	    fprintf(stderr, "The --ipv6_domain (-V) option is not enabled in this build.\n");
	    exit(1);
#endif
            break;

        case 'W' :
            setSuggestWin(mExtSettings);
            fprintf(stderr, "The -W option is not available in this release\n");
            break;

        case 'X' :
            setPeerVerDetect(mExtSettings);
            break;

        case 'Z':
#ifdef TCP_CONGESTION
	    setCongestionControl(mExtSettings);
	    mExtSettings->mCongestion = new char[strlen(optarg)+1];
	    strcpy(mExtSettings->mCongestion, optarg);
#else
            fprintf(stderr, "The -Z option is not available on this operating system\n");
#endif
	    break;

        case 0:
	    if (incrdstip) {
		incrdstip = 0;
		setIncrDstIP(mExtSettings);
	    }
	    if (txstarttime) {
		long seconds;
		long usecs;
		int match = 0;
		txstarttime = 0;
		setTxStartTime(mExtSettings);
		setEnhanced(mExtSettings);
		match = sscanf(optarg,"%ld.%6ld", &seconds, &usecs);
		mExtSettings->txstart_epoch.tv_usec = 0;
		switch (match) {
		case 2:
		    mExtSettings->txstart_epoch.tv_usec = usecs;
		case 1:
		    mExtSettings->txstart_epoch.tv_sec = seconds;
		    break;
		default:
		    unsetTxStartTime(mExtSettings);
		    fprintf(stderr, "WARNING: invalid --txstart-time format\n");
		}
	    }
	    if (noconnectsync) {
#ifdef HAVE_THREAD
		noconnectsync = 0;
		setNoConnectSync(mExtSettings);
#else
	        fprintf(stderr, "WARNING: --no-connect-sync requires thread support and not supported\n");
#endif
	    }
	    if (txholdback) {
		txholdback = 0;
	        char *end;
		Timestamp holdbackdelay;
		double delay = strtof(optarg, &end);
		if (*end != '\0') {
		    fprintf (stderr, "Invalid value of '%s' for --tcp-holdback time\n", optarg);
		} else {
		    holdbackdelay.set(delay);
		    mExtSettings->txholdback_timer.tv_sec = holdbackdelay.getSecs();
		    mExtSettings->txholdback_timer.tv_usec = (holdbackdelay.getUsecs());
		    setTxHoldback(mExtSettings);
		}
	    }
	    if (triptime) {
		triptime = 0;
		setTripTime(mExtSettings);
	    }
	    if (writeack) {
		writeack = 0;
		setWriteAck(mExtSettings);
		if (optarg) {
		    mExtSettings->mWriteAckLen = byte_atoi(optarg);
		}
	    }
	    if (noudpfin) {
		noudpfin = 0;
		setNoUDPfin(mExtSettings);
	    }
	    if (connectonly) {
		connectonly = 0;
		setConnectOnly(mExtSettings);
		unsetNoConnReport(mExtSettings);
		if (optarg) {
		  mExtSettings->connectonly_count = atoi(optarg);
		} else {
		  mExtSettings->connectonly_count = -1;
		}
	    }
	    if (connectretry) {
		connectretry = 0;
		mExtSettings->mConnectRetries = atoi(optarg);
	    }
	    if (sumonly) {
		sumonly = 0;
		setSumOnly(mExtSettings);
	    }
	    if (rxhistogram) {
		rxhistogram = 0;
		setRxHistogram(mExtSettings);
		setEnhanced(mExtSettings);
		// set default histogram settings, milliseconds bins between 0 and 1 secs
		mExtSettings->mRXbins = 1000;
		mExtSettings->mRXbinsize = 1;
		mExtSettings->mRXunits = 3;
		mExtSettings->mRXci_lower = 5;
		mExtSettings->mRXci_upper = 95;
		if (optarg) {
		    mExtSettings->mRxHistogramStr = new char[ strlen(optarg) + 1 ];
		    strcpy(mExtSettings->mRxHistogramStr, optarg);
		}
	    }
	    if (reversetest) {
		reversetest = 0;
		setReverse(mExtSettings);
	    }
	    if (fullduplextest) {
		fullduplextest = 0;
		setFullDuplex(mExtSettings);
	    }
	    if (fqrate) {
#if defined(HAVE_DECL_SO_MAX_PACING_RATE)
	        fqrate=0;
		setFQPacing(mExtSettings);
		mExtSettings->mFQPacingRate = (uintmax_t) (bitorbyte_atoi(optarg) / 8);
#else
		fprintf(stderr, "WARNING: The --fq-rate option is not supported\n");
#endif
	    }
	    if (isochronous) {
		isochronous = 0;
		setEnhanced(mExtSettings);
		setIsochronous(mExtSettings);
		// The following are default values which
		// may be overwritten during modal parsing
		mExtSettings->mFPS = 60.0;
		mExtSettings->mMean = 20000000.0;
		mExtSettings->mVariance = 0.0;
		mExtSettings->mBurstIPG = 5e-6;
		if (optarg) {
		    mExtSettings->mIsochronousStr = new char[ strlen(optarg) + 1 ];
		    strcpy(mExtSettings->mIsochronousStr, optarg);
		}
	    }
	    if (burstipg) {
		burstipg = 0;
		setIPG(mExtSettings);
		char *end;
		mExtSettings->mBurstIPG = strtof(optarg,&end);
		if (*end != '\0') {
		    fprintf (stderr, "ERRPORE: Invalid value of '%s' for --ipg\n", optarg);
		    exit(1);
		}
	    }
	    if (numreportstructs) {
		numreportstructs = 0;
		mExtSettings->numreportstructs = byte_atoi(optarg);
	    }
	    break;
        default: // ignore unknown
            break;
    }
} // end Interpret

static void strip_v6_brackets (char *v6addr) {
    char * results;
    if (v6addr && (*v6addr ==  '[') && ((results = strtok(v6addr, "]")) != NULL)) {
	int len = strlen(v6addr);
	for (int jx = 0; jx < len; jx++) {
	    v6addr[jx]= v6addr[jx + 1];
	}
    }
}

static char * isv6_bracketed_port (char *v6addr) {
    char *results = NULL;
    if (v6addr && (*v6addr ==  '[') && ((results = strtok(v6addr, "]")) != NULL)) {
	strip_v6_brackets(v6addr);
	if (results[0]==':') {
	    return ++results;
	}
    }
    return NULL;
}
static char * isv4_port (char *v4addr) {
    char *results = NULL;
    if (((results = strtok(v4addr, ":")) != NULL) && ((results = strtok(NULL, ":")) != NULL)) {
	return results;
    }
    return NULL;
}


//  The commmand line options are position independent and hence some settings become "modal"
//  i.e. two passes are required to get all the final settings correct.
//  For example, -V indicates use IPv6 and -u indicates use UDP, and the default socket
//  read/write (UDP payload) size is different for ipv4 and ipv6.
//  So in the Settings_Interpret pass there is no guarantee to know all three of (-u and -V and not -l)
//  while parsing them individually.
//
//  Since Settings_Interpret() will set all the *individual* options and flags
//  then the below code (per the example UDP, v4 or v6, and not -l) can set final
//  values, e.g. a correct default mBufLen. Other examples that need this are multicast
//  socket or not,-B local bind port parsing, and when to use the default UDP offered load
//
//  Also apply the bail out or exit conditions if the user requested mutually exclusive
//  or incompatabile options
void Settings_ModalOptions (struct thread_Settings *mExtSettings) {
    char *results;
    // Handle default read/write sizes based on v4, v6, UDP or TCP
    if (!isBuflenSet(mExtSettings)) {
	if (isUDP(mExtSettings)) {
	    if (isIPV6(mExtSettings) && mExtSettings->mThreadMode == kMode_Client) {
		mExtSettings->mBufLen = kDefault_UDPBufLenV6;
	    } else {
		mExtSettings->mBufLen = kDefault_UDPBufLen;
	    }
	} else {
	    mExtSettings->mBufLen = kDefault_TCPBufLen;
	}
    }
    // Handle default UDP offered load (TCP will be max, i.e. no read() or write() rate limiting)
    if (!isBWSet(mExtSettings) && isUDP(mExtSettings)) {
	mExtSettings->mAppRate = kDefault_UDPRate;
    }
    if (isTripTime(mExtSettings) && (isReverse(mExtSettings) || \
				     isFullDuplex(mExtSettings) || \
				     (mExtSettings->mMode != kTest_Normal))) {
	setEnhanced(mExtSettings);
    }
    // Warnings
    if (mExtSettings->mThreadMode == kMode_Client) {
	if (isModeTime(mExtSettings) && infinitetime) {
	    unsetModeTime(mExtSettings);
	    setModeInfinite(mExtSettings);
	    fprintf(stderr, "WARNING: client will send traffic forever or until an external signal (e.g. SIGINT or SIGTERM) occurs to stop it\n");
	}
	if (isFullDuplex(mExtSettings) && isCongestionControl(mExtSettings)) {
	    fprintf(stderr, "WARNING: tcp congestion control will only be applied on transmit traffic, use -Z on the server\n");
	}
    }
    // Bail outs
    bool bail = false;
    // compat mode doesn't support these test settings
    int compat_nosupport = (isReverse(mExtSettings) | isFullDuplex(mExtSettings) | isTripTime(mExtSettings) | isVaryLoad(mExtSettings) \
			    | isRxHistogram(mExtSettings) |  isIsochronous(mExtSettings) \
			    | isEnhanced(mExtSettings) | (mExtSettings->mMode != kTest_Normal));
    if (isCompat(mExtSettings) && compat_nosupport) {
	fprintf(stderr, "ERROR: compatibility mode not supported with the requested with options\n");
	bail = true;
    }
    if (mExtSettings->mThreadMode == kMode_Client) {
	if (isSumOnly(mExtSettings) && !(mExtSettings->mThreads > 1)) {
	    fprintf(stderr, "ERROR: option of --sum-only requires -P greater than 1\n");
	    bail = true;
	}
        if (isTxStartTime(mExtSettings)) {
	    Timestamp now;
	    long nowsecs = now.getSecs();
	    if (((nowsecs - mExtSettings->txstart_epoch.tv_sec) > 0) \
		|| ((nowsecs == mExtSettings->txstart_epoch.tv_sec) && (now.getUsecs() > mExtSettings->txstart_epoch.tv_usec))) {
		fprintf(stderr, "WARNING: --txstart-time of before now ignored\n");
		unsetTxStartTime(mExtSettings);
	    }
	}
	if (isTxHoldback(mExtSettings) && isTxStartTime(mExtSettings)) {
	    fprintf(stdout,"ERROR: options of --txstart-time and --txdelay-time are mutually exclusive\n");
	    bail = true;
	}
	if (isUDP(mExtSettings)) {
	    if (isPeerVerDetect(mExtSettings)) {
		fprintf(stderr, "ERROR: option of -X or --peer-detect not supported with -u UDP\n");
		bail = true;
	    }
	    if (isConnectOnly(mExtSettings)) {
		fprintf(stderr, "ERROR: option of --connect-only not supported with -u UDP\n");
		bail = true;
	    }
	    if (isIPG(mExtSettings) && isBWSet(mExtSettings)) {
		fprintf(stderr, "ERROR: options of --b and --ipg cannot be applied together\n");
		bail = true;
	    }
	    if (mExtSettings->mBurstIPG < 0.0) {
		fprintf(stderr, "ERROR: option --ipg must be a positive value\n");
		bail = true;
	    }
	    if (mExtSettings->mConnectRetries > 0) {
		fprintf(stderr, "ERROR: option --connect-retries not supported with -u UDP\n");
		bail = true;
	    }
	    {
		double delay_target;
		if (isIPG(mExtSettings)) {
		    delay_target = mExtSettings->mBurstIPG * 1e9;  // convert from seconds to nanoseconds
		} else {
		    // compute delay target in units of nanoseconds
		    if (mExtSettings->mAppRateUnits == kRate_BW) {
			// compute delay for bandwidth restriction, constrained to [0,max] seconds
			delay_target = (mExtSettings->mBufLen * 8e9) / mExtSettings->mAppRate;
		    } else {
			delay_target = 1e9 / mExtSettings->mAppRate;
		    }
		}
		if (delay_target < 0  ||
		    delay_target > MAXIPGSECS * 1e9) {
		    fprintf(stderr, "ERROR: IPG delay target of %.1f secs too large (max value is %d secs)\n", (delay_target / 1e9), MAXIPGSECS);
		    bail = true;
		}
	    }
	} else {
	    if (isBWSet(mExtSettings) && ((mExtSettings->mAppRate / 8) < (uintmax_t) mExtSettings->mBufLen)) {
		fprintf(stderr, "ERROR: option -b and -l of %d are incompatible, consider setting -l to %d or lower\n", \
			mExtSettings->mBufLen, (int) (mExtSettings->mAppRate / 8));
		bail = true;
	    }
	    if (isIPG(mExtSettings)) {
		fprintf(stderr, "ERROR: option --ipg requires -u UDP\n");
		bail = true;
	    }
	    if (isNoUDPfin(mExtSettings)) {
		fprintf(stderr, "ERROR: option --no-udp-fin requires -u UDP\n");
		bail = true;
	    }
	    if (isTxHoldback(mExtSettings)) {
		Timestamp now;
		if (mExtSettings->txholdback_timer.tv_sec > MAXDIFFTXDELAY) {
		    fprintf(stdout,"ERROR: Fail because --txdelay-time is not within %d seconds of now\n", MAXDIFFTXDELAY);
		    bail = true;
		}
		if (isConnectOnly(mExtSettings)) {
		    fprintf(stdout,"ERROR: Fail because --txdelay-time and --connect-only cannot be applied together\n");
		    bail = true;			;
		}
	    }
	}
	if (isTxStartTime(mExtSettings)) {
	    Timestamp now;
	    if ((mExtSettings->txstart_epoch.tv_sec- now.getSecs()) > MAXDIFFTXSTART) {
		fprintf(stdout,"ERROR: Fail because --txstart-time is not within %d seconds of now\n", MAXDIFFTXSTART);
		bail = true;
	    }
	}
	if (isRxHistogram(mExtSettings)) {
	    fprintf(stderr, "ERROR: option of --histograms is not supported on the client\n");
	    bail = true;
	}
	if (isCongestionControl(mExtSettings) && isReverse(mExtSettings)) {
	    fprintf(stderr, "ERROR: tcp congestion control -Z and --reverse cannot be applied together\n");
	    bail = true;
	}
	{
	    int one_only = 0;
	    if (isFullDuplex(mExtSettings))
		one_only++;
	    if (isReverse(mExtSettings))
		one_only++;
	    if (mExtSettings->mMode != kTest_Normal)
		one_only++;
	    if (one_only > 1) {
		fprintf(stderr, "ERROR: options of --full-duplex, --reverse, -d and -r are mutually exclusive\n");
		bail = true;
	    }
	}
	if (isBWSet(mExtSettings) && isIsochronous(mExtSettings)) {
	    fprintf(stderr, "ERROR: options of --b and --isochronous cannot be applied together\n");
	    bail = true;
	}
    } else {
        if (isTripTime(mExtSettings)) {
            fprintf(stderr, "ERROR: setting of option --trip-times is not supported on the server\n");
	    bail = true;
	}
	if (isVaryLoad(mExtSettings)) {
	    fprintf(stderr, "ERROR: option of variance per -b is not supported on the server\n");
	    bail = true;
	}
	if (isTxStartTime(mExtSettings)) {
	    fprintf(stderr, "ERROR: option of --txstart-time is not supported on the server\n");
	    bail = true;
	}
	if (isTxHoldback(mExtSettings)) {
	    fprintf(stderr, "ERROR: option of --txdelay-time is not supported on the server\n");
	    bail = true;
	}
        if (isConnectOnly(mExtSettings)) {
	    fprintf(stderr, "ERROR: option of --connect-only is not supported on the server\n");
	    bail = true;
	}
	if (isIPG(mExtSettings)) {
	    fprintf(stderr, "ERROR: option of --ipg is not supported on the server\n");
	    bail = true;
	}
	if (isIsochronous(mExtSettings)) {
	    fprintf(stderr, "ERROR: option of --isochronous is not supported on the server\n");
	    bail = true;
	}
	if (isFullDuplex(mExtSettings)) {
	    fprintf(stderr, "ERROR: option of --full-duplex is not supported on the server\n");
	    bail = true;
	}
	if (isReverse(mExtSettings)) {
	    fprintf(stderr, "ERROR: option of --reverse is not supported on the server\n");
	    bail = true;
	}
	if (isIncrDstIP(mExtSettings)) {
	    fprintf(stderr, "ERROR: option of --incr-dstpip is not supported on the server\n");
	    bail = true;
	}
	if (isFQPacing(mExtSettings)) {
	    fprintf(stderr, "ERROR: option of --fq-rate is not supported on the server\n");
	    bail = true;
	}
	if (isNoUDPfin(mExtSettings)) {
	    fprintf(stderr, "ERROR: option of --no-udp-fin is not supported on the server\n");
	    bail = true;
	}
	if (isPeerVerDetect(mExtSettings)) {
	    fprintf(stderr, "ERROR: option of -X or --peer-detect not supported on the server\n");
	    bail = true;
	}
	if (mExtSettings->mConnectRetries > 0) {
	    fprintf(stderr, "ERROR: option --connect-retries not supported on the server\n");
	    bail = true;
	}
    }
    if (bail)
	exit(1);
    // UDP histogram optional settings
    if (isRxHistogram(mExtSettings) && (mExtSettings->mThreadMode != kMode_Client) && mExtSettings->mRxHistogramStr) {
	// check for optional arguments to change histogram settings
	if (((results = strtok(mExtSettings->mRxHistogramStr, ",")) != NULL) && !strcmp(results,mExtSettings->mRxHistogramStr)) {
	    // scan for unit specifier
	    char *tmp = new char [strlen(results) + 1];
	    strcpy(tmp, results);
	    if ((strtok(tmp, "u") != NULL) && strcmp(results,tmp)) {
		mExtSettings->mRXunits = 6;  // units is microseconds
	    } else {
		strcpy(tmp, results);
		if ((strtok(tmp, "m") != NULL) && strcmp(results,tmp)) {
		    mExtSettings->mRXunits = 3;  // units is milliseconds
		}
	    }
	    mExtSettings->mRXbinsize = atoi(tmp);
	    delete [] tmp;
	    if ((results = strtok(results+strlen(results)+1, ",")) != NULL) {
		mExtSettings->mRXbins = byte_atoi(results);
		if ((results = strtok(NULL, ",")) != NULL) {
		    mExtSettings->mRXci_lower = atof(results);
		    if ((results = strtok(NULL, ",")) != NULL) {
			mExtSettings->mRXci_upper = atof(results);
		    }
		}
	    }
	}
    }
    if (isUDP(mExtSettings)) {
	// L2 settings
	if (l2checks && isUDP(mExtSettings)) {
	    l2checks = 0;
	    // Client controls hash or not
	    if (mExtSettings->mThreadMode == kMode_Client) {
		setL2LengthCheck(mExtSettings);
	    } else {
#if defined(HAVE_LINUX_FILTER_H) && defined(HAVE_AF_PACKET)
		// Request server to do length checks
		setL2LengthCheck(mExtSettings);
#else
		fprintf(stderr, "WARNING: option --l2checks not supported on this platform\n");
#endif
	    }
	}
	if (isFullDuplex(mExtSettings)) {
	    setNoUDPfin(mExtSettings);
	}
    }
    if (isIsochronous(mExtSettings) && mExtSettings->mIsochronousStr) {
	// parse client isochronous field,
	// format is --isochronous <int>:<float>,<float> and supports
	// human suffixes, e.g. --isochronous 60:100m,5m
	// which is frames per second, mean and variance
	if (mExtSettings->mThreadMode == kMode_Client) {
	    if (((results = strtok(mExtSettings->mIsochronousStr, ":")) != NULL) && !strcmp(results,mExtSettings->mIsochronousStr)) {
		mExtSettings->mFPS = atof(results);
		if ((results = strtok(NULL, ",")) != NULL) {
		    mExtSettings->mMean = bitorbyte_atof(results);
		    if (mExtSettings->mMean < 0) {
		        mExtSettings->mMean *= -8 * mExtSettings->mBufLen * mExtSettings->mFPS;
		    }
		    if ((results = strtok(NULL, ",")) != NULL) {
		        mExtSettings->mVariance = bitorbyte_atof(results);
		    }
		} else {
		    mExtSettings->mMean = 20000000.0;
		    mExtSettings->mVariance = 0.0;
		}
	    } else {
		fprintf(stderr, "WARNING: Invalid --isochronous value, format is <fps>:<mean>,<variance> (e.g. 60:18M,1m)\n");
	    }
	}
    }
    // See if the Write ack size should equal the write size (vs a configured or a burst size)
    if (isWriteAck(mExtSettings) && !mExtSettings->mWriteAckLen && \
	(mExtSettings->mThreadMode == kMode_Client) && !isIsochronous(mExtSettings))
	mExtSettings->mWriteAckLen = mExtSettings->mBufLen;

    // Check for further mLocalhost (-B) and <dev> requests
    // full addresses look like 192.168.1.1:6001%eth0 or [2001:e30:1401:2:d46e:b891:3082:b939]:6001%eth0
    iperf_sockaddr tmp;
    // Parse -B addresses
    if (mExtSettings->mLocalhost) {
	if (((results = strtok(mExtSettings->mLocalhost, "%")) != NULL) && ((results = strtok(NULL, "%")) != NULL)) {
	    mExtSettings->mIfrname = (char *) calloc(1,strlen(results) + 1);
	    strcpy(mExtSettings->mIfrname, results);
	    if (mExtSettings->mThreadMode == kMode_Client) {
	        fprintf(stderr, "WARNING: Client cannot set bind device %s via -B consider using -c\n", mExtSettings->mIfrname);
		free(mExtSettings->mIfrname);
		mExtSettings->mIfrname = NULL;
	    }
	}
	if (isIPV6(mExtSettings)) {
	    results = isv6_bracketed_port(mExtSettings->mLocalhost);
	} else {
	    results = isv4_port(mExtSettings->mLocalhost);
	}
	if (results) {
	    if (mExtSettings->mThreadMode == kMode_Client) {
		mExtSettings->mBindPort = atoi(results);
	    } else {
		fprintf(stderr, "WARNING: port %s ignored - set receive port on server via -p or -L\n", results);
	    }
	}
	// Check for multicast per the -B
	SockAddr_setHostname(mExtSettings->mLocalhost, &tmp,
			     (isIPV6(mExtSettings) ? 1 : 0));
	if ((mExtSettings->mThreadMode != kMode_Client) && SockAddr_isMulticast(&tmp)) {
	    setMulticast(mExtSettings);
	} else if (SockAddr_isMulticast(&tmp)) {
	    if (mExtSettings->mIfrname) {
		free(mExtSettings->mIfrname);
		mExtSettings->mIfrname = NULL;
	    }
	    fprintf(stderr, "WARNING: Client src addr (per -B) must be ip unicast\n");
	}
    }
    // Parse client (-c) addresses for multicast, link-local and bind to device
    if (mExtSettings->mThreadMode == kMode_Client) {
	mExtSettings->mIfrnametx = NULL; // default off SO_BINDTODEVICE
	if (((results = strtok(mExtSettings->mHost, "%")) != NULL) && ((results = strtok(NULL, "%")) != NULL)) {
	    mExtSettings->mIfrnametx = (char *) calloc(1,strlen(results) + 1);
	    strcpy(mExtSettings->mIfrnametx, results);
	}
	if (isIPV6(mExtSettings))
	    strip_v6_brackets(mExtSettings->mHost);
	// get the socket address settings from the host, needed for link-local and multicast tests
	SockAddr_zeroAddress(&mExtSettings->peer);
	SockAddr_remoteAddr(mExtSettings);
	if (isIPV6(mExtSettings) && SockAddr_isLinklocal(&mExtSettings->peer)) {
	    // link-local doesn't use SO_BINDTODEVICE but includes it in the host string
	    // so stitch things back together and null the bind to name
	    if (mExtSettings->mIfrnametx) {
		strcat(mExtSettings->mHost, "%");
		strcat(mExtSettings->mHost, mExtSettings->mIfrnametx);
		free(mExtSettings->mIfrnametx);
		mExtSettings->mIfrnametx = NULL;
	    } else {
		fprintf(stderr, "WARNING: usage of ipv6 link-local requires a device specifier, e.g. %s%%eth0\n", mExtSettings->mHost);
	    }
	}
	if (SockAddr_isMulticast(&mExtSettings->peer)) {
	    bail = false;
	    if ((mExtSettings->mThreads > 1) && !isIncrDstIP(mExtSettings)) {
		fprintf(stderr, "ERROR: client option of -P greater than 1 not supported with multicast address\n");
		bail = true;
	    } else if (isFullDuplex(mExtSettings) || isReverse(mExtSettings) || (mExtSettings->mMode != kTest_Normal)) {
		fprintf(stderr, "ERROR: options of --full-duplex, --reverse, -d and -r not supported with multicast addresses\n");
		bail = true;
	    }
	    if (bail)
		exit(1);
	    else
		setMulticast(mExtSettings);
	}
#ifndef HAVE_DECL_SO_BINDTODEVICE
	if (mExtSettings->mIfrnametx) {
	    fprintf(stderr, "bind to device will be ignored because not supported\n");
	    free(mExtSettings->mIfrnametx);
	    mExtSettings->mIfrnametx=NULL;
	}
#endif
    }
}

void Settings_GetUpperCaseArg (const char *inarg, char *outarg) {
    int len = strlen(inarg);
    strcpy(outarg,inarg);

    if ((len > 0) && (inarg[len-1] >='a')
         && (inarg[len-1] <= 'z'))
        outarg[len-1]= outarg[len-1]+'A'-'a';
}

void Settings_GetLowerCaseArg (const char *inarg, char *outarg) {
    int len = strlen(inarg);
    strcpy(outarg,inarg);

    if ((len > 0) && (inarg[len-1] >='A')
         && (inarg[len-1] <= 'Z'))
        outarg[len-1]= outarg[len-1]-'A'+'a';
}

/*
 * Settings_GenerateListenerSettings
 * Called to generate the settings to be passed to the Listener
 * instance that will handle dual testings from the client side
 * this should only return an instance if it was called on
 * the struct thread_settings instance generated from the command line
 * for client side execution
 */
#define DUALTIMER_MS 300
void Settings_GenerateListenerSettings (struct thread_Settings *client, struct thread_Settings **listener) {
    if ((client->mMode == kTest_DualTest) || (client->mMode == kTest_TradeOff)) {
	Settings_Copy(client, listener, 0);
        unsetDaemon((*listener));
        setCompat((*listener));
        if (client->mListenPort != 0) {
            (*listener)->mPort   = client->mListenPort;
        } else {
            (*listener)->mPort   = client->mPort;
        }
	if (client->mMode == kTest_TradeOff) {
	    (*listener)->mAmount   = client->mAmount + DUALTIMER_MS;
	} else if (client->mMode == kTest_DualTest) {
	    (*listener)->mAmount   = client->mAmount + (SLOPSECS * 100);
	}
	if ((client->mMode != kTest_Normal) && ((*listener)->mAmount  < DUALTIMER_MS)) {
	    (*listener)->mAmount   = DUALTIMER_MS;
	}
        (*listener)->mFileName   = NULL;
        (*listener)->mHost       = NULL;
        (*listener)->mLocalhost  = NULL;
        (*listener)->mOutputFileName = NULL;
        (*listener)->mMode       = kTest_Normal;
        (*listener)->mThreadMode = kMode_Listener;
        if (client->mHost != NULL) {
            (*listener)->mHost = new char[strlen(client->mHost) + 1];
            strcpy((*listener)->mHost, client->mHost);
        }
        if (client->mLocalhost != NULL) {
            (*listener)->mLocalhost = new char[strlen(client->mLocalhost) + 1];
            strcpy((*listener)->mLocalhost, client->mLocalhost);
        }
	if (client->mBufLen <= 0) {
	    if (isUDP((*listener))) {
		(*listener)->mBufLen = kDefault_UDPBufLen;
	    } else {
		(*listener)->mBufLen = kDefault_TCPBufLen;
	    }
	} else {
	    (*listener)->mBufLen = client->mBufLen;
	}
	setReport((*listener));
    } else {
        *listener = NULL;
    }
}

void Settings_ReadClientSettingsIsoch (struct thread_Settings **client, struct client_hdrext_isoch_settings *hdr) {
    (*client)->mFPS = ntohl(hdr->FPSl);
    (*client)->mFPS += ntohl(hdr->FPSu) / (double)rMillion;
    (*client)->mMean = ntohl(hdr->Meanl);
    (*client)->mMean += ntohl(hdr->Meanu) / (double)rMillion;
    (*client)->mVariance = ntohl(hdr->Variancel);
    (*client)->mVariance += ntohl(hdr->Varianceu) / (double)rMillion;
    (*client)->mBurstIPG = ntohl(hdr->BurstIPGl);
    (*client)->mBurstIPG += ntohl(hdr->BurstIPGu) / (double)rMillion;
}

void Settings_ReadClientSettingsV1 (struct thread_Settings **client, struct client_hdr_v1 *hdr) {
    (*client)->mTID = thread_zeroid();
    (*client)->mPort = (unsigned short) ntohl(hdr->mPort);
    (*client)->mThreads = 1;
    if (hdr->mBufLen != 0) {
	(*client)->mBufLen = ntohl(hdr->mBufLen);
    }
    (*client)->mAmount = ntohl(hdr->mAmount);
    if (((*client)->mAmount & 0x80000000) > 0) {
	setModeTime((*client));
#ifndef WIN32
	(*client)->mAmount |= 0xFFFFFFFF00000000LL;
#else
	(*client)->mAmount |= 0xFFFFFFFF00000000;
#endif
	(*client)->mAmount = -(*client)->mAmount;
    } else {
	unsetModeTime((*client));
    }
}

/*
 * Settings_GenerateClientSettings
 *
 * Called by the Listener to generate the settings to be used by clients
 * per things like dual tests. Set client pointer to null if a client isn't needed
 *
 * Note: mBuf should already be filled out per the Listener's apply_client_settings
 */
void Settings_GenerateClientSettings (struct thread_Settings *server, struct thread_Settings **client, void *mBuf) {
    assert(server != NULL);
    assert(mBuf != NULL);
    uint32_t flags = isUDP(server) ? ntohl(*(uint32_t *)((char *)mBuf + sizeof(struct UDP_datagram))) : ntohl(*(uint32_t *)mBuf);
    uint16_t upperflags = 0;
    thread_Settings *reversed_thread = NULL;
    *client = NULL;
    bool v1test = (flags & HEADER_VERSION1) && !(flags & HEADER_VERSION2);
#ifdef HAVE_THREAD_DEBUG
    if (v1test)
	thread_debug("header set for a version 1 test");
#endif
    if (isFullDuplex(server) || isServerReverse(server))
	setTransferID(server, 0);
    if (isFullDuplex(server) || v1test) {
	Settings_Copy(server, client, 0);
	reversed_thread = *client;
	if (isFullDuplex(server) && !(flags & HEADER_VERSION1)) {
	    setFullDuplex(reversed_thread);
	} else {
	    unsetFullDuplex(reversed_thread);
	}
    } else if (isServerReverse(server)) {
	reversed_thread = server;
    } else {
	assert(0);
	return;
    }
    reversed_thread->mThreadMode = kMode_Client;

    if (isUDP(server)) { // UDP test information passed in every packet per being stateless
	struct client_udp_testhdr *hdr = (struct client_udp_testhdr *) mBuf;
	Settings_ReadClientSettingsV1(&reversed_thread, &hdr->base);
	if (isFullDuplex(server) || v1test) {
	    server->mAmount = reversed_thread->mAmount + (SLOPSECS * 100);
	}
	if (v1test) {
	    setServerReverse(reversed_thread);
	    if (flags & RUN_NOW) {
		reversed_thread->mMode = kTest_DualTest;
	    } else {
		reversed_thread->mMode = kTest_TradeOff;
	    }
	}
	if (flags & HEADER_EXTEND) {
	    reversed_thread->mAppRate = ntohl(hdr->extend.lRate);
#ifdef HAVE_INT64_T
	    reversed_thread->mAppRate |= ((uint64_t)(ntohl(hdr->extend.uRate) >> 8) << 32);
#endif
	    upperflags = ntohs(hdr->extend.upperflags);
	    if (upperflags & HEADER_NOUDPFIN) {
		setNoUDPfin(reversed_thread);
	    }
	    if ((upperflags & HEADER_UNITS_PPS) == HEADER_UNITS_PPS) {
		reversed_thread->mAppRateUnits = kRate_PPS;
	    } else {
		reversed_thread->mAppRateUnits = kRate_BW;
	    }
	    reversed_thread->mTOS = ntohs(hdr->extend.tos);
	    if (isIsochronous(server)) {
		Settings_ReadClientSettingsIsoch(&reversed_thread, &hdr->isoch_settings);
	    }
	    if (upperflags & HEADER_FQRATESET) {
		setFQPacing(reversed_thread);
		reversed_thread->mFQPacingRate = ntohl(hdr->start_fq.fqratel);
#ifdef HAVE_INT64_T
		reversed_thread->mFQPacingRate |= ((uint64_t)(ntohl(hdr->start_fq.fqrateu)) << 32);
#endif
	    }
	}
    } else { //tcp first payload
	struct client_tcp_testhdr *hdr = (struct client_tcp_testhdr *) mBuf;
	Settings_ReadClientSettingsV1(&reversed_thread, &hdr->base);
	if (isFullDuplex(server) || v1test) {
	    server->mAmount = reversed_thread->mAmount + (SLOPSECS * 100);
	}
	if (v1test) {
	    setServerReverse(reversed_thread);
	    if (flags & RUN_NOW) {
		reversed_thread->mMode = kTest_DualTest;
	    } else {
		reversed_thread->mMode = kTest_TradeOff;
	    }
	}
	if (flags & HEADER_EXTEND) {
	    reversed_thread->mAppRate = ntohl(hdr->extend.lRate);
#ifdef HAVE_INT64_T
	    reversed_thread->mAppRate |= ((uint64_t)(ntohl(hdr->extend.uRate) >> 8) << 32);
#endif
	    upperflags = ntohs(hdr->extend.upperflags);
	    reversed_thread->mTOS = ntohs(hdr->extend.tos);

	    if (isIsochronous(server)) {
		Settings_ReadClientSettingsIsoch(&reversed_thread, &hdr->isoch_settings);
	    }
	    if (upperflags & HEADER_FQRATESET) {
		setFQPacing(reversed_thread);
		reversed_thread->mFQPacingRate = ntohl(hdr->start_fq.fqratel);
#ifdef HAVE_INT64_T
		reversed_thread->mFQPacingRate |= ((uint64_t)(ntohl(hdr->start_fq.fqrateu)) << 32);
#endif
	    }
	}
    }
    unsetTxHoldback(reversed_thread);
    setNoSettReport(reversed_thread);
    setNoConnectSync(reversed_thread);
    // for legacy -d and -r need so set the reversed threads mHost
    if (v1test) {
	reversed_thread->mHost = new char[REPORT_ADDRLEN];
	if (((sockaddr*)&server->peer)->sa_family == AF_INET) {
	    inet_ntop(AF_INET, &((sockaddr_in*)&server->peer)->sin_addr,
		      reversed_thread->mHost, REPORT_ADDRLEN);
	}
#ifdef HAVE_IPV6
	else {
	    inet_ntop(AF_INET6, &((sockaddr_in6*)&server->peer)->sin6_addr,
		      reversed_thread->mHost, REPORT_ADDRLEN);
	}
#endif
    } else {
	reversed_thread->mMode = kTest_Normal;
#if HAVE_DECL_SO_MAX_PACING_RATE
	if (isFQPacing(reversed_thread)) {
	    int rc = setsockopt(reversed_thread->mSock, SOL_SOCKET, SO_MAX_PACING_RATE, \
				&reversed_thread->mFQPacingRate, sizeof(reversed_thread->mFQPacingRate));
	    WARN_errno(rc == SOCKET_ERROR, "setsockopt SO_MAX_PACING_RATE");
  #ifdef HAVE_THREAD_DEBUG
    #ifdef HAVE_INT64_T
	    thread_debug("Set socket %d pacing rate to %ld byte/sec", reversed_thread->mSock, reversed_thread->mFQPacingRate);
    #else
	    thread_debug("Set socket %d pacing rate to %d byte/sec", reversed_thread->mSock, reversed_thread->mFQPacingRate);
    #endif
  #endif
	}
#endif // MAX_PACING_RATE
    }
}

int Settings_GenerateClientHdrV1 (struct thread_Settings *client, struct client_hdr_v1 *hdr) {
    if (isBuflenSet(client)) {
	hdr->mBufLen = htonl(client->mBufLen);
    } else {
	hdr->mBufLen = 0;
    }
    if (client->mListenPort != 0) {
	hdr->mPort  = htonl(client->mListenPort);
    } else {
	hdr->mPort  = htonl(client->mPort);
    }
    hdr->numThreads = htonl(client->mThreads);
    if (isModeTime(client)) {
	hdr->mAmount = htonl(-(long)client->mAmount);
    } else {
	hdr->mAmount = htonl((long)client->mAmount);
	hdr->mAmount &= htonl(0x7FFFFFFF);
    }
    return (sizeof(struct client_hdr_v1));
}

/*
 * Settings_GenerateClientHdr
 *
 * Called to generate the client header to be passed to the listener/server
 *
 * This will handle:
 * o) dual testings from the listener/server side
 * o) advanced udp test settings
 *
 * Returns size of header in bytes
 */
int Settings_GenerateClientHdr (struct thread_Settings *client, void *testhdr, struct timeval startTime) {
    uint16_t len = 0;
    uint16_t upperflags = 0;
    uint16_t lowerflags = 0;
    uint32_t flags = 0;

    // flags common to both TCP and UDP
    if (isReverse(client) && !isCompat(client)) {
	upperflags |= HEADER_REVERSE;
    }
    if (isFullDuplex(client) && !isCompat(client)) {
	upperflags |= HEADER_FULLDUPLEX;
    }
    if (isTxStartTime(client) && !TimeZero(startTime)) {
	upperflags |= HEADER_EPOCH_START;
    }
    // Now setup UDP and TCP specific passed settings from client to server
    if (isUDP(client)) { // UDP test information passed in every packet per being stateless
	struct client_udp_testhdr *hdr = (struct client_udp_testhdr *) testhdr;
	memset(hdr, 0, sizeof(struct client_udp_testhdr));
	flags |= HEADER_SEQNO64B; // use 64 bit by default
	flags |= HEADER_EXTEND;
	hdr->extend.version_u = htonl(IPERF_VERSION_MAJORHEX);
	hdr->extend.version_l = htonl(IPERF_VERSION_MINORHEX);
	hdr->extend.tos = htons(client->mTOS & 0xFF);
	if (isBWSet(client)) {
	    hdr->extend.lRate = htonl((uint32_t)(client->mAppRate));
#ifdef HAVE_INT64_T
	    hdr->extend.uRate = htonl(((uint32_t)(client->mAppRate >> 32)) << 8);
#endif
	} else {
	    hdr->extend.lRate = htonl(kDefault_UDPRate);
	    hdr->extend.uRate = 0x0;
	}
	len += sizeof(struct client_hdrext);
	len += Settings_GenerateClientHdrV1(client, &hdr->base);
	if (!isCompat(client) && (client->mMode != kTest_Normal)) {
	    flags |= HEADER_VERSION1;
	    if (client->mMode == kTest_DualTest)
		flags |= RUN_NOW;
	    hdr->base.flags = htonl(flags);
	}
	/*
	 * set the default offset where underlying "inline" subsystems can write into the udp payload
	 */
	if (isL2LengthCheck(client)) {
	    flags |= HEADER_UDPTESTS;
	    if (isL2LengthCheck(client)) {
		upperflags |= HEADER_L2LENCHECK;
		if (isIPV6(client))
		    upperflags |= HEADER_L2ETHPIPV6;
	    }
	}
	if (isIsochronous(client)) {
	    flags |= (HEADER_UDPTESTS | HEADER_EXTEND) ;
	    upperflags |= HEADER_ISOCH;
	    if (isFullDuplex(client) || isReverse(client)) {
		upperflags |= HEADER_ISOCH_SETTINGS;
		hdr->isoch_settings.FPSl = htonl((long)(client->mFPS));
		hdr->isoch_settings.FPSu = htonl(((client->mFPS - (long)(client->mFPS)) * rMillion));
		hdr->isoch_settings.Meanl = htonl((long)(client->mMean));
		hdr->isoch_settings.Meanu = htonl((((client->mMean) - (long)(client->mMean)) * rMillion));
		hdr->isoch_settings.Variancel = htonl((long)(client->mVariance));
		hdr->isoch_settings.Varianceu = htonl(((client->mVariance - (long)(client->mVariance)) * rMillion));
		hdr->isoch_settings.BurstIPGl = htonl((long)(client->mBurstIPG));
		hdr->isoch_settings.BurstIPGu = htonl(((client->mBurstIPG - (long)(client->mBurstIPG)) * rMillion));
		len += sizeof(struct client_hdrext_isoch_settings);
	    }
	}
	if (isReverse(client) || isFullDuplex(client)) {
	    flags |= (HEADER_UDPTESTS | HEADER_VERSION2);
	}
	if (isNoUDPfin(client)) {
	    flags |= (HEADER_UDPTESTS | HEADER_EXTEND);
	    upperflags |= HEADER_NOUDPFIN;
	}
	if (isTripTime(client) || isFQPacing(client) || isTxStartTime(client)) {
	    flags |= HEADER_UDPTESTS;
	    if (isTripTime(client) || isTxStartTime(client)) {
		hdr->start_fq.start_tv_sec = htonl(startTime.tv_sec);
		hdr->start_fq.start_tv_usec = htonl(startTime.tv_usec);
		if (isTripTime(client))
		    upperflags |= HEADER_TRIPTIME;
	    }
	    if (isFQPacing(client)) {
		upperflags |= HEADER_FQRATESET;
		hdr->start_fq.fqratel = htonl((uint32_t) client->mFQPacingRate);
#ifdef HAVE_INT64_T
		hdr->start_fq.fqrateu = htonl((uint32_t) (client->mFQPacingRate >> 32));
#endif
	    }
	}
	// Write flags to header so the listener can determine the tests requested
	hdr->extend.upperflags = htons(upperflags);
	hdr->extend.lowerflags = htons(lowerflags);

	// isoch payload is an enclave field between v 0.13 and v0.14
	// so figure out if it's there now - UDP only
	// will be filled in by the client write
	if (isTripTime(client) || isFQPacing(client) || isIsochronous(client))
	    len += sizeof (struct isoch_payload);

	if (len > 0) {
	    len += sizeof(struct UDP_datagram);
	    flags |= HEADER_LEN_BIT;
	}
	hdr->base.flags = htonl(flags | ((len << 1) & 0xFFFE));
    } else { // TCP first write with test information
	struct client_tcp_testhdr *hdr = (struct client_tcp_testhdr *) testhdr;
	memset(hdr, 0, sizeof(struct client_tcp_testhdr));
	flags |= HEADER_EXTEND;
	hdr->extend.version_u = htonl(IPERF_VERSION_MAJORHEX);
	hdr->extend.version_l = htonl(IPERF_VERSION_MINORHEX);
	hdr->extend.tos = htons(client->mTOS & 0xFF);
	if (isBWSet(client)) {
	    hdr->extend.lRate = htonl((uint32_t)(client->mAppRate));
#ifdef HAVE_INT64_T
	    hdr->extend.uRate = htonl(((uint32_t)(client->mAppRate >> 32)) << 8);
#endif
	}
	len += sizeof(struct client_hdrext);
	len += Settings_GenerateClientHdrV1(client, &hdr->base);
	if (!isCompat(client) && (client->mMode != kTest_Normal)) {
	    flags |= HEADER_VERSION1;
	    if (client->mMode == kTest_DualTest)
		flags |= RUN_NOW;
	}
	if (isPeerVerDetect(client)) {
	    flags |= (HEADER_V2PEERDETECT | HEADER_VERSION2);
	}
	if (isTripTime(client) || isFQPacing(client) || isIsochronous(client) || isTxStartTime(client)) {
	    hdr->start_fq.start_tv_sec = htonl(startTime.tv_sec);
	    hdr->start_fq.start_tv_usec = htonl(startTime.tv_usec);
	    hdr->start_fq.fqratel = htonl((uint32_t) client->mFQPacingRate);
#ifdef HAVE_INT64_T
	    hdr->start_fq.fqrateu = htonl((uint32_t) (client->mFQPacingRate >> 32));
#endif
	    len += sizeof(struct client_hdrext_starttime_fq);
	    // Set flags on
	    if (isTripTime(client)) {
		upperflags |= HEADER_TRIPTIME;
	    }
	    if (isFQPacing(client)) {
		upperflags |= HEADER_FQRATESET;
	    }
	}
	if (isIsochronous(client)) {
	    upperflags |= HEADER_ISOCH;
	    if (isFullDuplex(client) || isReverse(client)) {
		upperflags |= HEADER_ISOCH_SETTINGS;
		hdr->isoch_settings.FPSl = htonl(client->mFPS);
		hdr->isoch_settings.FPSu = htonl(((long)(client->mFPS) - (long)client->mFPS * rMillion));
		hdr->isoch_settings.Meanl = htonl(client->mMean);
		hdr->isoch_settings.Meanu = htonl(((long)(client->mMean) - (long)client->mMean * rMillion));
		hdr->isoch_settings.Variancel = htonl(client->mVariance);
		hdr->isoch_settings.Varianceu = htonl(((long)(client->mVariance) - (long)client->mVariance * rMillion));
		hdr->isoch_settings.BurstIPGl = htonl(client->mBurstIPG);
		hdr->isoch_settings.BurstIPGu = htonl(((long)(client->mBurstIPG) - (long)client->mBurstIPG * rMillion));
		len += sizeof(struct client_hdrext_isoch_settings);
	    }
	}
	if (isReverse(client) || isFullDuplex(client)) {
	    flags |= HEADER_VERSION2;
	}
	hdr->extend.upperflags = htons(upperflags);
	hdr->extend.lowerflags = htons(lowerflags);
	if (len > 0) {
	    flags |= HEADER_LEN_BIT;
	}
	hdr->base.flags = htonl((flags | ((len << 1) & 0xFFFE)));
    }
    return (len);
}

int Settings_ClientHdrPeekLen (uint32_t flags) {
    //* determine peek length
    int peeklen = 0;
    if (flags & HEADER_LEN_BIT) {
	peeklen = (flags & 0xFFFE) >> 1;
	if (peeklen <= 0)
	    fprintf(stderr, "WARN: header length bit set and length invalid\n");
    } else {
	peeklen = 0;
	if (flags & (HEADER_VERSION1 | HEADER_EXTEND)) {
	    peeklen = sizeof(struct client_hdr_v1);
	}
	if (flags & (HEADER_VERSION2 | HEADER_EXTEND)) {
	    peeklen += sizeof(struct client_hdrext);
	}
    }
    return peeklen;
}
