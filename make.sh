#!/bin/sh

if [ ! -z "$1" ]
then
obj=$1
gcc -DHAVE_CONFIG_H  -g -I. -I. -I.. -I.. -I/sw/include -DG_LOG_DOMAIN=\"lavtools\" -DLAVPLAY_VERSION=\"1.6.2\" -I/usr/X11R6/include -I /usr/X11R6/include -I../utils -I/sw/include -I/sw/include -Wall -Wunused -c -o ${obj}.o ${obj}.c 

gcc  -I/sw/include -Wall -Wunused -g  -L/sw/lib -o ${obj} xgeom.o  ${obj}.o ../utils/libmjpegutils.a 
else
	echo "usage: make objname"
fi
