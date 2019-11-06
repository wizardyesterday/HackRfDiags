#!/bin/sh
#**********************************************************************
# This script runs the rtl_fm program and pipes its output to netcat
# so that an information sink, running netcat and piping its output
# to an audio player, allows one to listen to the audio on another
# computer.
#**********************************************************************
rtl_fm -p 24 -M wbfm -s 200000 -f $1 -r 20000 | netcat -u $2 8000
