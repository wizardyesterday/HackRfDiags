******************************************************************************
This directory contains all of the files, build scripts, and Scilab filter
design scripts for implementing and testing all of the filter structures
that have been implemented.
The file descriptions are listed below.	
Chris G. 01/23/2018
******************************************************************************

/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
 Scilab filter design programs.
/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
decimateHalfband1.sci - A Scilab script that designs a 2:1 decimator filter
that decimates the sample rate from 2048000S/s to 1024000S/s.

decimateHalfband2.sci - A Scilab script that designs a 2:1 decimator filter
that decimates the sample rate from 1024000S/s to 512000S/s.

decimateHalfband3.sci - A Scilab script that designs a 2:1 decimator filter
that decimates the sample rate from 512000S/s to 256000S/s.
******************************************************************************

/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
 Design data.
/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
halfbandStage1.pdf - The frequency response plot of stage 1 of the 8:1
decimator.

halfbandStage2.pdf - The frequency response plot of stage 2 of the 8:1
decimator.

halfbandStage3.pdf - The frequency response plot of stage 3 of the 8:1
decimator.
******************************************************************************

/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
 Filter classes.
/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
Decimator_int16.cc - The implementation of the  polyphase decimator.
Decimator_int16.h - The interface of the polyphase decimator.
FirFilter_int16.cc - The implementation of the  Fir filter.
Firfilter_int16.h - The interface of the Fir filter.
Interpolator_int16.cc - The implementation of the polyphase interpolator.
Interpolator_int16.8 - The interface of the polyphase interpolator.
******************************************************************************

/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
 Test applications.
/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
decimateAudio.cc - A test app that accepts a 32000S/s audio file and
outputs an 8000S/s audio file.
******************************************************************************

/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
 Build scripts.
/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
buildDecimateAAudio.sh - The build script for the audio decimator app.
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
******************************************************************************

