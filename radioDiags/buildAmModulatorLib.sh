#!/bin/sh
#*****************************************************************************
# This build script creates a static library that can be used for creating
# HackRf apps.  To run this script, type ./buildAmModulatorLib.sh.
# Chris G. 03/22/2018
#*****************************************************************************

CcFiles="\
    Filters/Int16/FirFilter_int16.cc \
    Filters/Int16/Interpolator_int16.cc \
    AmModulator/AmModulator.cc"

Includes="\
    -I Filters \
    -I Filters/Int16 \
    -I AmModulator"
 
# Compile string.
Compile="g++ -O3 -c $Includes $CcFiles"

# Build our library.
$Compile

# Create the archive.
ar rcs lib/libAmModulator.a *.o

# Cleanup.
rm *.o

# We're done.
exit 0

