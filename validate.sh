#!/bin/sh

PROG=./sim_trace
PROCS="4 8 16"
PROTOCOLS="MI MSI" # MESI MOSI"
TMP=/tmp
DIFF="diff -u"

PROC_COUNT=4
PROTOCOL="MI"

for PROC_COUNT in $PROCS
do
	VALID_DIR=traces/${PROC_COUNT}proc_validation
	for PROTOCOL in $PROTOCOLS
	do
		echo "Validating $PROTOCOL on $PROC_COUNT processors"
		OUTPUT_FILE=$TMP/trace_${PROTOCOL}_$PROC_COUNT
		VALID_FILE=$VALID_DIR/${PROTOCOL}_validation.txt
		$PROG -t $VALID_DIR -p$PROTOCOL >$OUTPUT_FILE 2>&1
		$DIFF ${VALID_FILE} $OUTPUT_FILE
	done
done
