#!/bin/sh

CcFiles="\
    wbfm.cc \
    WbFmModulator.cc \
    ../Filters/Int16/Interpolator_int16.cc"  

Includes="\
    -I ../Filters/Int16"

g++ -g -O3 -o wbfm $Includes $CcFiles -lm

exit 0

