//******************************************************************
//******************************************************************
// This program generates the required filter coefficients so that
// an input signal, that is sampled at 256000 S/s can be decimated
// to 64000 S/s.  This set of coefficients is used for the first 
// stage of a 3-stage decimator.  Due to the nature of a multi-stage
// decimator, the transition width can be relaxed since the final
// stage of decimation will filter out any aliased components.
// filter to Hz.  The filter coefficients are computed by the
// minimax function, and the result is an equiripple linear phase
// FIR filter.
// The filter specifications are listed below.
//
// Pass Band: 0 <= F <= 2400 Hz.
// Transition Band: 2400 < F <= 60000 Hz.
// Stop Band: 60000 < F < 64000 Hz.
// Passband Ripple: 0.1
// Stopband Ripple: 0.005
//
// Note that the filter length will be automatically  calculated
// from the filter parameters.
// Chris G. 08/17/2017
//******************************************************************

// Include the common code.
exec('../Common/utils.sci',-1);

//******************************************************************
// Set up parameters.
//******************************************************************
// Sample rate is 256000 S/s.
Fsample = 256000;

// Passband edge.
Fp = 2400;

// Stopband edge.
Fs = 60000;

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

