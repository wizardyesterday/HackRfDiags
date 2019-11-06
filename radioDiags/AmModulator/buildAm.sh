#!/bin/sh

CcFiles="\
    am.cc \
    AmModulator.cc \
    ../Filters/Int16/Interpolator_int16.cc"  

Includes="\
    -I ../Filters/Int16"

g++ -g -O3 -o am $Includes $CcFiles -lm

exit 0

