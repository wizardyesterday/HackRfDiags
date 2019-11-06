#!/bin/sh

g++ -I ../Filters -g -O0 -o testWbFmDemodulator testWbFmDemodulator.cc WbFmDemodulator.cc ../Filters/IirFilter.cc ../Filters/FirFilter.cc ../Filters/Decimator.cc -lm

exit 0

