#!/bin/bash

g++ am.cc
sh generateBaseband.sh am

g++ fm.cc
sh generateBaseband.sh fm

g++ pm.cc
sh generateBaseband.sh pm

g++ dsb.cc
sh generateBaseband.sh dsb

rm a.out

exit 0

