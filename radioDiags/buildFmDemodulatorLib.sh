#!/bin/sh
#*****************************************************************************
# This build script creates a static library that can be used for creating
# HackRf apps.  To run this script, type ./buildFmDemodulatorLib.sh.
# Chris G. 03/22/2018
#*****************************************************************************

CcFiles="\
    Filters/Int16/Decimator_int16.cc \
    FmDemodulator/FmDemodulator.cc"

Includes="\
    -I Filters/Int16 \
    -I FmDemodulator"
 
# Compile string.
Compile="g++ -O3 -c $Includes $CcFiles"

# Build our library.
$Compile

# Create the archive.
ar rcs lib/libFmDemodulator.a *.o

# Cleanup.
rm *.o

# We're done.
exit 0

