#!/bin/sh
#*****************************************************************************
# This build script creates a HackRf app.  It assumes that all the libraries
# have been built already.  If they have not been built type ./buildLibs.sh.
# Chris G. 03/22/2018
#*****************************************************************************
Executable="bin/radioDiags"

CcFiles="\
    src_diags/radioApp.cc \
    src_diags/DataConsumer.cc \
    src_diags/DataProvider.cc \
    src_diags/FrequencySweeper.cc \
    Filters/Int16/Decimator_int16.cc \
    src_diags/IqDataProcessor.cc \
    src_diags/BasebandDataProcessor.cc \
    src_diags/Radio.cc \
    src_diags/console.cc \
    src_diags/diagUi.cc"

Includes="\
    -I hackRf \
    -I pfkUtils \
    -I hdr_diags \
    -I Filters \
    -I Filters/Int16 \
    -I AmDemodulator \
    -I FmDemodulator \
    -I WbFmDemodulator \
    -I SsbDemodulator \
    -I AmModulator \
    -I FmModulator \
    -I WbFmModulator \
    -I SsbModulator"
 
# Compile string.
Compile="g++ -O3 -o $Executable $Includes $CcFiles"

# Link options
LinkOptions="\
    -O3 \
    -L lib -lhackrf \
    -L lib -lmsgq \
    -L lib -lAmDemodulator \
    -L lib -lFmDemodulator \
    -L lib -lWbFmDemodulator \
    -L lib -lSsbDemodulator \
    -L lib -lAmModulator \
    -L lib -lFmModulator \
    -L lib -lWbFmModulator \
    -L lib -lSsbModulator \
    -lusb-1.0 \
    -lm \
    -lpthread \
    -lrt"

# Build our application.
$Compile  $LinkOptions

# We're done.
exit 0

