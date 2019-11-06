#!/bin/sh

g++ -I ../Filters -g -O0 -o testSsbDemodulator testSsbDemodulator.cc SsbDemodulator.cc ../Filters/IirFilter.cc ../Filters/FirFilter.cc ../Filters/Decimator.cc -lm

exit 0

