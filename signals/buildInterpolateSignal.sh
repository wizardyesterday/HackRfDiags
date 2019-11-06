#!/bin/sh

CcFiles="\
    interpolateSignal.cc \
    ../radioDiags/Filters/Int16/Interpolator_int16.cc"  

Includes="\
    -I ../radioDiags/Filters/Int16"

g++ -g -O3 -o interpolateSignal $Includes $CcFiles -lm

exit 0

