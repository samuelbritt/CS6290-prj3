#!/bin/bash

PROG=./sim_trace
PROCS="4 8 16"
ALL_PROTOCOLS="MI MSI MOSI MESI MOESI MOESIF"
OUTPUT_DIR=./runs/validation
DIFF="diff -u"

PROTOCOLS=`echo $@ | awk '{print toupper($0)}'`
if [[ $# = 0 ]]
then
	PROTOCOLS=$ALL_PROTOCOLS
fi

mkdir -p $OUTPUT_DIR
for PROC_COUNT in $PROCS
do
	VALID_DIR=traces/${PROC_COUNT}proc_validation
	for PROTOCOL in $PROTOCOLS
	do
		echo "Validating $PROTOCOL on $PROC_COUNT processors"
		OUTPUT_FILE=$OUTPUT_DIR/trace_${PROTOCOL}_$PROC_COUNT
		VALID_FILE=$VALID_DIR/${PROTOCOL}_validation.txt
		$PROG -t $VALID_DIR -p$PROTOCOL >$OUTPUT_FILE 2>&1
		$DIFF ${VALID_FILE} $OUTPUT_FILE
	done
done
