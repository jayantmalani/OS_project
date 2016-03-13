#!/bin/bash

R_IP=$1
R_PORT=$2

rm *.log

echo "setup,teardown" > loopback.log
echo "setup,teardown" > remote.log

PORT=$(($RANDOM + 32768))
./server $PORT &
for i in `seq 1 100`; do

	#Test loopback setup and tear-down
	RESULT=$(./client localhost $PORT)
	echo $RESULT >> loopback.log
	
	#Test remote setupd and tear-down
	RESULT=$(./client $R_IP $R_PORT 2> /dev/null)
	echo $RESULT >> remote.log
done
