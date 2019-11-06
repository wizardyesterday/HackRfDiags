#!/bin/sh
#**********************************************************************
# This script sets up a netcat listener and pipes audio camples to
# aplay.  The information source is output from the rtl_fm program.
# An rtl-sdr dongle is used for the IQ samples that are presented to
# the rtl_fm program.
#**********************************************************************
netcat -l -u -p 8000 | aplay -f S16_LE -r 12000

