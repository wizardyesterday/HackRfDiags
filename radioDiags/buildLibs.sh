#!/bin/sh
#*****************************************************************************
# This build script creates all of static libraries that can be used for
# creating HackRf apps.  To run this script, type ./buildLibs.sh.
# Chris G. 03/22/2018
#*****************************************************************************
sh buildAmDemodulatorLib.sh
sh buildFmDemodulatorLib.sh
sh buildWbFmDemodulatorLib.sh
sh buildSsbDemodulatorLib.sh
sh buildAmModulatorLib.sh
sh buildFmModulatorLib.sh
sh buildWbFmModulatorLib.sh
sh buildSsbModulatorLib.sh
sh buildHackRfLib.sh

exit 0
