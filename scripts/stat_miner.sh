#!/bin/bash

#
# Use this to compute the opt-traces directory, which will contain the results
# of traces with varying optimization flags
#

OPTFLAGS="O2 O3 O4 Os O1 O g"
for optflag in $OPTFLAGS; do
	echo $optflag
	mkdir -p tests/traces-{out,bin,obj}
	make tests-clean
	TRACE_CFLAGS="-$optflag" VEXLLVM_DUMP_XLATESTATS=1 make test-built-traces
	rm -rf opt-traces/"$optflag"
	mkdir -p opt-traces/"$optflag"
	mv tests/traces-{out,bin,obj}  opt-traces/"$optflag"
done