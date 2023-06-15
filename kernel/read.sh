#!/bin/bash

TIMEOUT=$1

i=1
while true
do
	echo -n "core $i: "	
	cat /sys/class/hwmon/hwmon5/energy"$i"_input
	i=$((i+1))

	if [ $i -gt 8 ]
	then
		i=1
		echo ""
	fi
	sleep $TIMEOUT
done
