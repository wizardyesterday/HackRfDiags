#!/bin/sh

CcFiles="\
    fm.cc \
    FmModulator.cc \
    ../Nco/PhaseAccumulator.cc \
    ../Nco/Nco.cc \
    ../Filters/Int16/Interpolator_int16.cc"  

Includes="\
    -I ../Nco \
    -I ../Filters/Int16"

g++ -g -O3 -o fm $Includes $CcFiles -lm

exit 0

