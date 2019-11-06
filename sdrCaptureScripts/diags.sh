#!/bin/sh
#**********************************************************************
# This script runs the radioDaigs  program and pipes its output to
# netcat so that an information sink, running netcat and piping its
# output to an audio player, allows one to listen to the audio on
# another computer.
# To run this script, type: # './diag.sh ipAddress', where ipAddress
# is the IP address of the the computer that is to receive the data
# that is sent # to stdout.
#**********************************************************************
bin/radioDiags | netcat -u $1 8000
