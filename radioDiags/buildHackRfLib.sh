#!/bin/sh
#*****************************************************************************
# This build script creates a static library that can be used for creating
# HackRf  apps.  To run this script, type ./buildHackRfLib.sh"
# Chris G. 12/26/2017
#*****************************************************************************
Compile="gcc -c -O3 -Iinclude -I/usr/include/libusb-1.0"

# First compile the files of interest.
$Compile hackRf/hackrf.c

# Create the archive.
ar rcs lib/libhackrf.a *.o

# Cleanup.
rm *.o

# We're done.
exit 0

