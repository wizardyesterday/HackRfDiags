//******************************************************************
// This program generates the required filter coefficients so that
// an input signal, that is sampled at 256000 S/s can be decimated
// to 64000 S/s.  We want to limit the bandwidth of the aliasing
// filter to 27200 Hz.  The filter coefficients are computed by
// the minimax function, and the result is an equiripple linear
// phase FIR filter.
// The filter specifications are listed below.
//
// Pass Band: 0 <= F <= 4000 Hz.
// Transition Band: 4000 < F <= 16000 Hz.
// Stop Band: 16000 < F < 128000 Hz.
// Passband Ripple: 0.1
// Stopband Ripple: 0.005
//
// Note that the filter length will be automatically  calculated
// from the filter parameters.
// Chris G. 06/24/2017
//******************************************************************

// Include the common code.
exec('../Common/utils.sci',-1);

//******************************************************************
// Set up parameters.
//******************************************************************
// Sample rate is 256000 S/s.
Fsample = 256000;

// Passband edge.
Fp = 4000;

// Stopband edge.
Fs = 16000;

// The desired demodulator bandwidth.
F = [0 Fp; Fs Fsample/2];

// Bandwidth of transition band.
deltaF = (Fs - Fp) / Fsample;

// Passband ripple
deltaP = 0.1;

// Stopband ripple.
deltaS = 0.005;

// Number of taps for our filter.
n = computeFilterOrder(deltaP,deltaS,deltaF,Fs)

// Ensure that the filter order is a multiple of 4.
n = computeNextMultiple(n,4);

//******************************************************************
// Generate the FIR filter coefficients and magnitude of frequency
// response..  
//******************************************************************

//------------------------------------------------------------------
// This will be an antialiasing filter the preceeds the 4:1
// compressor.
//------------------------------------------------------------------
h = eqfir(n,F/Fsample,[1 0],[1/deltaP 1/deltaS]);

// Compute magnitude and frequency points.
[hm,fr] = frmag(h,1024);

//******************************************************************
// Plot magnitudes.
//******************************************************************
set("figure_style","new");

title("Antialiasing Filter - Sample Rate: 256000 S/s  (non-decimated)");
a = gca();
a.margins = [0.225 0.1 0.125 0.2];
a.grid = [1 1];
a.x_label.text = "F, Hz";
a.y_label.text = "|H|";
plot2d(fr*Fsample,hm);

