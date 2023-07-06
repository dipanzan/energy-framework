#!/bin/bash

TIMEOUT=$1

while true
do

	echo -n "core 0: " && cat /sys/class/hwmon/hwmon5/energy1_input
	echo -n "core 1: " && cat /sys/class/hwmon/hwmon5/energy2_input
  echo -e "\n"

	echo -n "core 2: " && cat /sys/class/hwmon/hwmon5/energy3_input
	echo -n "core 3: " && cat /sys/class/hwmon/hwmon5/energy4_input
  echo -e "\n"

	echo -n "core 4: " && cat /sys/class/hwmon/hwmon5/energy5_input
	echo -n "core 5: " && cat /sys/class/hwmon/hwmon5/energy6_input
  echo -e "\n"

	echo -n "core 6: " && cat /sys/class/hwmon/hwmon5/energy7_input
	echo -n "core 7: " && cat /sys/class/hwmon/hwmon5/energy8_input
  echo -e "\n"
	echo "----------------------------------------------------------"
	sleep $TIMEOUT
	clear
done
