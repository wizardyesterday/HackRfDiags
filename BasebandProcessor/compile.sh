#!/bin/bash

Includes="\
    -I ../radioDiags/Filters/Int16 \
    -I ../radioDiags/AmModulator \
    -I ../radioDiags/FmModulator \
    -I ../radioDiags/SsbModulator"

g++ -c $Includes BasebandDataProcessor.cc

exit 0
