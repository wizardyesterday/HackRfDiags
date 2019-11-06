#!/bin/sh
#**********************************************************************
# This script runs the rtl_fm program and pipes its output to netcat
# so that an information sink, running netcat and piping its output
# to an audio player, allows one to listen to the audio on another
# computer.
#**********************************************************************
rtl_fm -p 24 -M fm -f $1 -s 30000 -r 12000 | netcat -u $2 8000
