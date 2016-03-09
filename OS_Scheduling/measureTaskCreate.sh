#!/bin/bash

echo "" > taskCreate.csv

for i in `seq 1 1000`;
do
	./measureProcCreate | tee -a taskCreate.csv
	./measureThreadCreate | tee -a taskCreate.csv

#	echo -n "," >> taskCreate.csv
#	echo $(./measureThread) >> taskCreate.csv
#	echo "" >> taskCreate.csv
done

exit
