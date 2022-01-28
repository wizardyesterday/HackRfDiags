//******************************************************************
// This program generates the required filter coefficients so that
// an input signal, that is sampled at 128000 S/s can be
// interpolated to 256000S/s.  We want to limit the bandwidth of
// the imaging filter to 27200 Hz.  The filter coefficients are
// computed by the minimax function, and the result is an equiripple
// linear phase FIR filter.
// The filter specifications are listed below.
//
// Stopband Attenuation: 45dB.
// Pass Band: 0 <= F <= 27200 Hz.
// Transition Band: 27200 < F <= 100800 Hz.
// Stop Band: 100800 < F < 128000 Hz.
// Passband Ripple: 10^(-Attenuation/20).
// Stopband Ripple: 10^(-Attenuation/20).
//
// Note that the filter length will be automatically  calculated
// from the filter parameters.
// Chris G. 03/03/2018
//******************************************************************

// Include the common code.
exec('../Common/utils.sci',-1);

//******************************************************************
// Set up parameters.
//******************************************************************
// Sample rate is 256000 S/s.
Fsample = 256000;

// Passband edge.
Fp = 27200;

// Stopband edge.
Fs = (Fsample / 2) - Fp;

// The desired demodulator bandwidth.
F = [0 Fp; Fs Fsample/2];

// Bandwidth of transition band.
deltaF = (Fs - Fp) / Fsample;

// Stopband attenuation is 45dB.
Attenuation = 45;

// Passband ripple
deltaP = 10^(-Attenuation/20);

// Stopband ripple.
deltaS = deltaP;

// Number of taps for our filter.
//n = computeFilterOrder(deltaP,deltaS,deltaF,Fs)
n = 7;

//******************************************************************
// Generate the FIR filter coefficients and magnitude of frequency
// response..  
//******************************************************************

//------------------------------------------------------------------
// This will be an antialiasing filter the preceeds the 4:1
// compressor.
//------------------------------------------------------------------
h = eqfir(n,F/Fsample,[1 0],[1 1]);

// Let's make the number of taps even.
h(8) = 0;

// Compute magnitude and frequency points.
[hm,fr] = frmag(h,1024);

//******************************************************************
// Plot magnitudes.
//******************************************************************
set("figure_style","new");

title("Antialiasing Filter - Sample Rate: 256000 S/s  (interpolated)");
a = gca();
a.margins = [0.225 0.1 0.125 0.2];
a.grid = [1 1];
a.x_label.text = "F, Hz";
a.y_label.text = "|H|";
plot2d(fr*Fsample,hm);

