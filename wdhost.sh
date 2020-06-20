#!/bin/bash 
#
# Starts HOSTCM server in the current working directory
#

if [[ $1 == '' ]] ; then
	echo
	echo " wdhost.sh  -  starts HOSTCM server in the current working directory on ttyUSB0"
	echo " usage -  wdhost.sh <baud> "
	echo " eg wdhost.sh 9600"
	echo
else

echo Starting HOSTCM server on /dev/ttyUSB0 at $1 baud E 1
echo
echo -n "The 'host.' directory is "
pwd
ls
echo
echo To exit HOSTCM server from the SuperPET,
echo "enter passthrough or talk mode and type 'q' <return>"
echo
~/bin/hostcm/hostcm -p even -s $1 /dev/ttyUSB0
echo 
echo HOSTCM closed.
echo
fi

