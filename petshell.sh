#!/bin/bash
# Starts a login prompt on /dev/ttyUSB0
# accepts baud rate as a parameter
# update with password
echo
if [[ $1 == '' ]]; then
	echo " petshell.sh  -  starts a shell on ttyUSB0"
	echo " usage  -  petshell.sh <baud>"
	echo " eg  -  petshell.sh 9600" 
	echo
else
echo Starting shell on /dev/ttyUSB0 at $1 baud
echo If you experience echoed characters, try running 
echo "  stty -echo"
echo
echo Use 'screen' when in adm3a terminal mode  
echo password|sudo -S stty -F /dev/ttyUSB0 -echo -crtscts -cstopb -parodd $1
echo password|sudo agetty -L ttyUSB0 adm3a $1
echo
echo Closed remote shell on /dev/ttyUSB0
echo
fi

