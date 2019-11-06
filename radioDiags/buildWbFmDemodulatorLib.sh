#!/bin/sh
#*****************************************************************************
# This build script creates a static library that can be used for creating
# HackRf apps.  To run this script, type ./buildWbFmDemodulatorLib.sh.
# Chris G. 03/22/2018
#*****************************************************************************

CcFiles="\
    Filters/FirFilter.cc \
    Filters/IirFilter.cc \
    Filters/Int16/Decimator_int16.cc \
    WbFmDemodulator/WbFmDemodulator.cc"

Includes="\
    -I Filters \
    -I Filters/Int16 \
    -I WbFmDemodulator"
 
# Compile string.
Compile="g++ -O3 -c $Includes $CcFiles"

# Build our library.
$Compile

# Create the archive.
ar rcs lib/libWbFmDemodulator.a *.o

# Cleanup.
rm *.o

# We're done.
exit 0

