******************************************************************************
This directory contains all of the files, build scripts, and Scilab filter
design scripts for implementing and testing a wideband FM demodulator.
The file descriptions are listed below.	
Chris G. 07/22/2017

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
decimateBy2.sci - A Scilab script that designs a 2:1 decimator filter.

decimateBy4_1.sci - A Scilab script that designs the first stage of a
multi-stage 4:1 decimator filter. 

decimateBy4_2.sci - A Scilab script that designs the second stage of a
multi-stage 4:1 decimator filter. 

wbDeemphasisFilter.sci - A Scilab script that designs the deemphasis
filter that follows the demodulator in the signal chain.
******************************************************************************

/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
 Design data.
/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
DecimateBy2.pdf - A frequency response plot of the audio decimator filter.

FirstStageDecimator.pdf - A frequency response plot of the first stage
of a multi-stage decimator.

SecondStageDecimator.pdf - A frequency response plot of the second stage
of a multi-stage decimator.

DeemphasisFilter.pdf - A frequency response plot of the deemphasis filter.
******************************************************************************

/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
 Filter classes.
/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
WbFmDemodulator.cc - The implementation fo the wideband FM demodulator.
WbFmDemodulator.h - The interface of the wideband FM demodulator.
******************************************************************************

/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
 Test applications.
/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
testWbFmDemodulator.cc - A test app that validates the wideband FM
demodulator.
******************************************************************************

/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
 Build scripts.
/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
buildTestWbFmDemodulator.sh - The build script for the wideband FM
demodulator test app.
******************************************************************************

/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
 Audio capture and playback scripts.
/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
play.sh - A script that uses aplay to play demodulated 8000S/s audio.
******************************************************************************

/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
 Iq files.
/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
wmbi.iq - The sample file for the WMBI FM radio station,  The frequency of
this station is 90.1MHz.
******************************************************************************

/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
 Audio files.
/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
audio.raw - The 8000S/s audio file that contains the demodulated data.
******************************************************************************

