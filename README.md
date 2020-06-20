# hostcm

Commodore SuperPET file server HOSTCM compiled for the Raspberry Pi 4

Reverse engineered from SuperPET ROMS by Robert Ferguson of www.seefigure1.com
Be sure to check out his very thorough README and digression.txt,
as well as his webpage which has a great history lesson on the SuperPET!

Also included are some shell scripts to streamline execution. 

 petshell.sh - starts shell on ttyUSB0 at user specified baud rate, even parity, 1 stop bit
  * needs to be edited to update password! 

 pethost.sh  -  starts HOSTCM in ~/pethost directory at user specified baud rate
 wdhost.sh  -  starts HOSTCM in the current working directory at user specified baud rate
  * these scripts assume connection on ttyUSB0
  * the host scripts assume hostcm installation at ~/bin/hostcm
  * pethost.sh assumes the existence of ~/pethost directory

 


