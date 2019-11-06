#!/bin/sh
#*****************************************************************************
# This build script creates a static library that can be used for creating
# HackRf apps.  To run this script, type ./buildAmDemodulatorLib.sh.
# Chris G. 03/22/2018
#*****************************************************************************

CcFiles="\
    Filters/Int16/Decimator_int16.cc \
    Filters/FirFilter.cc \
    Filters/IirFilter.cc \
    AmDemodulator/AmDemodulator.cc"

Includes="\
    -I Filters \
    -I Filters/Int16 \
    -I AmDemodulator"
 
# Compile string.
Compile="g++ -O3 -c $Includes $CcFiles"

# Build our library.
$Compile

# Create the archive.
ar rcs lib/libAmDemodulator.a *.o

# Cleanup.
rm *.o

# We're done.
exit 0

