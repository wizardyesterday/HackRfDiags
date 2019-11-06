#!/bin/sh

CcFiles="\
    ssb.cc \
    SsbModulator.cc \
    ../Filters/Int16/FirFilter_int16.cc \
    ../Filters/Int16/Interpolator_int16.cc"

Includes="\
    -I ../Filters/Int16"

g++ -g -O3 -o ssb $Includes $CcFiles -lm

exit 0

