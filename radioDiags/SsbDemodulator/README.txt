******************************************************************************
This directory contains all of the files, build scripts, and Scilab filter
design scripts for implementing an SSB demodulator.
The file descriptions are listed below.	
Chris G. 08/18/2017

Filter coefficients have been updated in the Scilab files for the decimator
filters to increase the stopband attenuation.  I finally realized that the
values of the weight vector, that is passed to the eqfir() function should
have the reciprocal of the passband ripple and stopband ripple rather unity
as the examples in some DSP books have liberally used.
Chris G. 10/26/2019
******************************************************************************

/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
 Scilab filter design programs.
/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
decimateBy4_1.sci - A scilab script that designs the first stage of the
three stage decimator filter.  The decimation ration is 4:1.

decimateBy4_2.sci - A scilab script that designs the second stage of the
three stage decimator filter.  The decimation ration is 4:1.

decimateBy2_3.sci - A scilab script that designs the third stage of the
three stage decimator filter.  The decimation ration is 2:1.

phaseShifter.sci - A scilab script that designs the phase shifter.
******************************************************************************

/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
 Design data.
/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
Stage1DecimatorFilter.pdf - A frequency response plot of the first stage of
the decimator.

Stage2DecimatorFilter.pdf - A frequency response plot of the second stage of
the decimator.

Stage3DecimatorFilter.pdf - A frequency response plot of the third stage of
the decimator.

PhaseShifter.pdf - A frequency response plot of the phase shifter.
******************************************************************************

/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
 Filter classes.
/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
SsbDemodulator.cc - The implementation fo the SSB demodulator.
SsbDemodulator.h - The interface of the SSB demodulator.
******************************************************************************

/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
 Test applications.
/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
testSsDemodulator.cc - A test app that validates the SSB demodulator.
******************************************************************************

/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
 Build scripts.
/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
buildTestSsbDemodulator.sh - The build script for the SSB demodulator test
app.
******************************************************************************

/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
 Audio capture and playback scripts.
/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
play.sh - A script that uses aplay to play demodulated 8000S/s audio.
******************************************************************************

/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
 Iq files.
/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
lsb.iq - The sample file for the lower sideband signal.
usb.iq - The sample file for the upper sideband signal.
******************************************************************************

/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
 Audio files.
/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
audio.raw - The 8000S/s audio file that contains the demodulated data.
******************************************************************************
