#!/bin/sh
#*****************************************************************************
# This build script creates a static library that can be used for creating
# HackRf apps.  To run this script, type ./buildSsbModulatorLib.sh.
# Chris G. 03/22/2018
#*****************************************************************************

CcFiles="\
    Filters/Int16/FirFilter_int16.cc \
    Filters/Int16/Interpolator_int16.cc \
    SsbModulator/SsbModulator.cc"

Includes="\
    -I Filters \
    -I Filters/Int16 \
    -I SsbModulator"
 
# Compile string.
Compile="g++ -O3 -c $Includes $CcFiles"

# Build our library.
$Compile

# Create the archive.
ar rcs lib/libSsbModulator.a *.o

# Cleanup.
rm *.o

# We're done.
exit 0

