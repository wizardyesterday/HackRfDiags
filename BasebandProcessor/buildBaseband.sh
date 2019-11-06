#!/bin/sh

CcFiles="\
    bb.cc \
    BasebandDataProcessor.cc \
    ../radioDiags/AmModulator/AmModulator.cc \
    ../radioDiags/FmModulator/FmModulator.cc \
    ../radioDiags/SsbModulator/SsbModulator.cc \
    ../radioDiags/Filters/Int16/FirFilter_int16.cc \
    ../radioDiags/Filters/Int16/Interpolator_int16.cc"

Includes="\
    -I ../radioDiags/AmModulator \
    -I ../radioDiags/FmModulator \
    -I ../radioDiags/SsbModulator \
    -I ../radioDiags/Filters/Int16"

#echo $Includes
#echo $CcFiles

g++ -g -O3 -o bb $Includes $CcFiles -lm -lpthread

exit 0

