******************************************************************************
This directory contains all of the files, build scripts, and Scilab filter
design scripts for implementing an AM demodulator.
The file descriptions are listed below.	
Chris G. 07/02/2017

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

decimateBy4.sci - A Scilab script that designs the 4:1 pre-demodulator
decimator filter. 

decimateBy4_2.sci - A Scilab script that designs the 4:1 post-demodulator
decimator filter.
******************************************************************************

/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
 Design data.
/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
DecimateBy2.pdf - A frequency response plot of the audio decimator filter.
DecimateBy4.pdf - A frequency response plot of the pre-democulator filter.

PostDemodDecimator.pdf - A frequency response plot of the post-demodulator
filter.

Audio1stSecond.pdf - A time domain plot of the first second of the audio
data.
******************************************************************************

/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
 Filter classes.
/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
AmDemodulator.cc - The implementation fo the AM demodulator.
AmDemodulator.h - The interface of the AM demodulator.
******************************************************************************

