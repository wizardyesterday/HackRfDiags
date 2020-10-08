******************************************************************************
This directory contains all of the files, build scripts, and Scilab filter
design scripts for implementing an FM modulator.
The file descriptions are listed below.	
Chris G. 03/12/2018

Filter coefficients have been updated in the Scilab files for the interpolator
filters to increase the stopband attenuation.  I finally realized that the
values of the weight vector, that is passed to the eqfir() function should
have the reciprocal of the passband ripple and stopband ripple rather unity
as the examples in some DSP books have liberally used.
Note that the halfband filters didn't need to be modified since the passband
and stopband ripples are equal in that case.
Chris G. 10/26/2019

The modulation function has been updated so that it more intuitive to use.
Rather than using a modulator gain, the FM deviation is used.  A more way
of computing the phase increment is now performed.  I didn't like variable 
names like theta and newTheta.  Those terms have been replaced with
phaseAccumulator and phaseIncrement.  Basically, the FM modulator is
implemented with an NCO (numerically controlled oscillator), and I wanted
this to be clear.
Chris G. 09/30/2020 
******************************************************************************

/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
 Scilab filter design programs.
/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
decimateBy2.sci - A Scilab script that designs a 2:1 decimator filter.

decimateBy4.sci - A Scilab script that designs the 4:1 pre-demodulator
decimator filter. 

decimateBy4_2.sci - A Scilab script that designs the 4:1 post-demodulator
decimator filter.

interpolateBy2.sci - A Scilab script that designes stage 1 of the 1:256
interpolator filter.

interpolateHalfBand1.sci - A Scilab script that designes stage 2 of the 1:256
interpolator filter.

interpolateHalfBand2.sci  - A Scilab script that designes stage 3 of the 1:256
interpolator filter.

interpolateHalfBand3.sci - A Scilab script that designes stage 4 of the 1:256
interpolator filter.

interpolateHalfBand4.sci - A Scilab script that designes stage 5 of the 1:256
interpolator filter.

interpolateHalfBand5.sci - A Scilab script that designes stage 6 of the 1:256
interpolator filter.

interpolateHalfBand6.sci - A Scilab script that designes stage 7 of the 1:256
interpolator filter.

interpolateHalfBand7.sci - A Scilab script that designes stage 8 of the 1:256
interpolator filter.
******************************************************************************

/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
 Design data.
/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
Stage1Interpolator.pdf - A frequency response plot of stage 1 of the 1:256
interpolator filter.

Stage2Interpolator.pdf - A frequency response plot of stage 2 of the 1:256
interpolator filter.

Stage3Interpolator.pdf - A frequency response plot of stage 3 of the 1:256
interpolator filter.

Stage4Interpolator.pdf - A frequency response plot of stage 4 of the 1:256
interpolator filter.

Stage5Interpolator.pdf - A frequency response plot of stage 5 of the 1:256
interpolator filter.

Stage6Interpolator.pdf - A frequency response plot of stage 6 of the 1:256
interpolator filter.

Stage7Interpolator.pdf - A frequency response plot of stage 7 of the 1:256
interpolator filter.

Stage8Interpolator.pdf - A frequency response plot of stage 8 of the 1:256
interpolator filter.
******************************************************************************

/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
 Filter classes.
/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/  /_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
FmModulator.cc - The implementation fo the FM modulator.
FmModulator.h - The interface of the FM modulator.
******************************************************************************

/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
 Test applications.
/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
fm.cc - A test app that validates the FM modulator.
******************************************************************************

/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
 Build scripts.
/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
buildAm.sh - The build script for the AM modulator test app.
******************************************************************************

/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
Test scripts.
/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
sendAudioStream.sh - A script that allows the user, on the host system, to
send realtime audio data to the system running the modulator and transmitter.

receiveAudioStream.sh - A script that allows the user to receive realtime
audio data from the host computer and modulate and transmit the modulated
signal over the radio.
******************************************************************************



