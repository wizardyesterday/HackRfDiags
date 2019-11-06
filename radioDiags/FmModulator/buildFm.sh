#!/bin/sh

CcFiles="\
    fm.cc \
    FmModulator.cc \
    ../Filters/Int16/Interpolator_int16.cc"  

Includes="\
    -I ../Filters/Int16"

g++ -g -O3 -o fm $Includes $CcFiles -lm

exit 0

