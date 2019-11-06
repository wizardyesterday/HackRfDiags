******************************************************************************
This directory contains all of the files, build scripts, and Scilab filter
design scripts for implementing and testing all of the filter structures
that have been implemented.
The file descriptions are listed below.	
Chris G. 07/07/2017
******************************************************************************

/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
 Scilab filter design programs.
/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
hDecimate.sci - A Scilab script that designs a 4:1 decimator filter.
hInterpolate.sci - A Scilab script that designs a 1:2 interpolator filter.
******************************************************************************

/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
 Design data.
/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
AntiImagingFilter.pdf - A frequency response plot of the interpolator filter.
AntiAliasingFilter.pdf - A frequency response plot of the decimator filter.
******************************************************************************

/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
 Filter classes.
/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
IirFilter.cc - The implementation of the IIR filter.
IirFilter.h - The interface of the IIR filter.
FirFilter.cc - The implementation of the FIR filter.
FirFilter.h - The interface of the FIR filter.
Decimator.cc - The implementation of the  polyphase decimator.
Decimator.h - The interface of the polyphase decimator.
Interpolator.cc - The implementaton of the polyphase interpolator.
Interpolator.h - The interface of the polyphase interpolator.
******************************************************************************

/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
 Test applications.
/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
testIirFilter.cc A test app that validates the IIR filter.
testFirFilter.cc A test app that validates the FIR filter.

decimateAudio.cc - A test app that accepts a 32000S/s audio file and
outputs an 8000S/s audio file.

interpolateAudio.cc - A test app that accepts a 8000S/s audio file and
outputs a 16000S/s audio file.

testDecimator.cc - A test app that validates the decimator.
testInterpolator.cc - A test app that validates the interpolator.
******************************************************************************

/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
 Build scripts.
/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
buildTestIirFilter.sh - The build script for the IIR filter test app.
buildTestFirFilter.sh - The build script for the FIR filter test app.
buildDecimateAAudio.sh - The build script for the audio decimator app.
buildInterpolateAudio.sh - The build script for the audio interpolator app.
buildTestDecimator.sh - The build script for the decimator test app.
buildTestInterpolator.sh - The build script for the interpolator test app.
buildTestInterpolator.sh - The build script for the interpolator test app.
******************************************************************************

/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
 Audio capture and playback scripts.
/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
capture.sh - A script that uses arecord to capture 8000S/s audio.
play.sh - A script that uses aplay to play captured 8000S/s audio.
******************************************************************************

/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
 Audio files.
/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
original32000.raw - The 32000S/s audio file that is input to the
decimateAudio app.

decimated8000.raw - The 8000S/s audio file that is output by the
decimateAudio app.

original8000.raw - The 8000S/s audio file that is input to the
interpolateAudio app.

interpolated16000.raw - The 16000S/s audio file that is output by the
interpolateAudio app.
******************************************************************************

