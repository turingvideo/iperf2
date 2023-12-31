# common test setup
set -o pipefail
#set -x

lo=localhost
ip=127.0.0.1
ip6=::1

# Some versions of expr can't handle can't mix regexp and math
test=$(expr $0 : '.*/t\([0-9]*\)_')
port=$(expr 5000 + $test)
lport=$(expr 6000 - $test)

@() { echo "+ $@"; "$@"; }

run_iperf() {
    mode=server
    server=(-s)
    client=(-c)
    match=""
    # Split server and client args lists
    while [ $# -gt 0 ]; do
	case $1 in
	    (-c) mode=client;;
	    (-s) mode=server;;
	    (-match) shift; match=$1;;
	    (*)
		case $mode in
		    (server) server+=($1);;
		    (client) client+=($1);;
		esac
	esac
	shift
    done
    # Start server
    # Wait for "listening"
    # Start client
    # Merge server and client output
    # Store results for additional processing and also copy to stderr for progress
    results=$(@ src/iperf -p $port "${server[@]}" 2>&1 | {
	    awk '{print};/listening/{exit 0}';
	    @ src/iperf -p $port "${client[@]}"; cat;
	} 2>&1 | tee /dev/stderr)

    # Check for known error messages
    if [[ "$results" =~ unrecognized|ignoring|failed|not\ valid ]]; then
	exit 1
    fi
    if [[ -n "$match" && ! ("$results" =~ "$match") ]]; then
       exit 1
    fi
    exit 0
}
