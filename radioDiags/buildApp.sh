#!/bin/sh
#*****************************************************************************
# This build script creates an HackRf app.  It assumes that all the libraries
# have been built already.  If they have not been built type ./buildLibs.sh.
# Chris G. 12/26/2017
#*****************************************************************************
Executable="bin/app.bin"

CcFiles="\
    app.cc"

Includes="\
    -I hackRf"
 
# Compile string.
Compile="g++ -O3 -o $Executable $Includes $CcFiles"

# Link options
LinkOptions="\
    -O3 \
    -L lib -lhackrf \
    -lusb-1.0 \
    -lm \
    -lpthread \
    -lrt"

# Build our application.
$Compile  $LinkOptions

# We're done.
exit 0

