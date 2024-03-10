#!/bin/sh

CcFiles="\
    wbfm.cc \
    WbFmModulator.cc \
    ../Nco/PhaseAccumulator.cc \
    ../Nco/Nco.cc \
    ../Filters/Int16/Interpolator_int16.cc"  

Includes="\
    -I ../Nco \
    -I ../Filters/Int16"

g++ -g -O3 -o wbfm $Includes $CcFiles -lm

exit 0

