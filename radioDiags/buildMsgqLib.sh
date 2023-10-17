#!/bin/sh
#*****************************************************************************
# This build script creates a static library that can be used for creating
# RtlSdr apps.  To run this script, type ./buildMsgqLib.sh
# Many thanks to Phil K. for creating all this stuff.  He has a repository
# at https://github.com/flipk/pfkutils.
# Chris G. 06/05/2017
#*****************************************************************************

CcFiles="\
    pfkUtils/LockWait.cc \
    pfkUtils/dll3.cc \
    pfkUtils/thread_slinger.cc"

Includes="\
     -I pfkUtils"
 
# Compile string.
Compile="g++ -std=c++98 -O3 -c $Includes $CcFiles"

# Build our library.
$Compile

# Create the archive.
ar rcs lib/libmsgq.a *.o

# Cleanup.
rm *.o

# We're done.
exit 0

