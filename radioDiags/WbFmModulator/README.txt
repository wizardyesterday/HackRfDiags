******************************************************************************
This directory contains all of the files, build scripts, and Scilab filter
design scripts for implementing a wideband FM modulator.
The file descriptions are listed below.	
Chris G. 10/18/2020
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
WbFmModulator.cc - The implementation fo the wideband FM modulator.
WbFmModulator.h - The interface of the wideband FM modulator.
******************************************************************************

/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
 Test applications.
/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
wbfm.cc - A test app that validates the wideband FM modulator.
******************************************************************************

/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
 Build scripts.
/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
buildWbFm.sh - The build script for the wideband FM modulator test app.
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



