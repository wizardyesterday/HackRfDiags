#!/bin/sh

g++ -I ../Filters -g -O0 -o testFmDemodulator testFmDemodulator.cc FmDemodulator.cc ../Filters/Decimator.cc -lm

exit 0

