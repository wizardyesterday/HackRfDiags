#!/bin/sh
#*****************************************************************************
# This compile script is used to compile rtlsdr apps for the purpose of
# finding syntax errors.  A copy of the libusb-1.0 directory has been placed
# in the include directory so that compiles may occur without syntax errors.
# This allows software development to occur on a system that doesn't have
# the USB development libraries.  Ultimately, system will be built on the
# target system.  To compile, type,
# ./compile.sh file.cc.
# Chris G. 08/18/2017
#*****************************************************************************

Includes="\
    -I include \
    -I src/convenience \
    -I libmh_msgq/hdr \
    -I hdr_diags \
    -I pfkUtils \
    -I Filters \
    -I AmDemodulator \
    -I FmDemodulator \
    -I WbFmDemodulator \
    -I SsbDemodulator \
    -I include/libusb-1.0"
 
# Compile our file.
g++ -c $Includes $1

# We're done.
exit 0

# We're done.
exit 0

