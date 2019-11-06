//******************************************************************
// This program generates the required filter coefficients so that
// an input signal, that is sampled at 16000 S/s can be decimated
// to 80000 S/s.  This set of coefficients is used for the third 
// stage of a 3-stage decimator.  Due to the nature of a multi-stage
// decimator, the transition width can be relaxed since the final
// stage of decimation will filter out any aliased components.
// filter to Hz.  The filter coefficients are computed by the
// minimax function, and the result is an equiripple linear phase
// FIR filter.
// The filter specifications are listed below.
//
// Pass Band: 0 <= F <= 2400 Hz.
// Transition Band: 2400 < F <= 3990 Hz.
// Stop Band: 3990 < F < 4000 Hz.
// Passband Ripple: 0.1
// Stopband Ripple: 0.005
//
// Note that the filter length will be automatically  calculated
// from the filter parameters.
// Chris G. 08/17/2017
///******************************************************************

//******************************************************************
// Function declarations.
//******************************************************************

//******************************************************************
//
// Name dInf
//
// Purpose: The purpose of this function is to compute the measure,
// D-infinity, that is used for computation of the required filter
// order for an equal ripple filter using the minimax algorithm.
// The origin of this algorithm is from the book, "Multirate
// Digital Signal Processing" by Ronald E. Crochiere and Lawrence
// R. Rabiner.
//
// Calling sequence: x = dInf(deltaP,deltaF)
//
// Inputs:
//
//    deltaP - The passband ripple.
//
//    deltaS - The stopband ripple.
//
// Outputs:
//
//    x - The value of D-infinity.
// 
//******************************************************************
function x = dInf(deltaP,deltaS)

// Set parameters.
a1 = 0.005309;
a2 = 0.07114;
a3 = -0.4761;
a4 = -0.00266;
a5 = -0.5941;
a6 = -0.4278;

//_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
// Break up the computation to make things easier to
// read.
//_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
// Compute first term.
x1 = a1 * log10(deltaP)^2 + a2 * log10(deltaP) + a3;
x1 = log10(deltaS) * x1;

// Compute second term.
x2 = a4 * log10(deltaP)^2 + a5 * log10(deltaP) + a6;
//_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/

// Sum up the two terms.
x = x1 + x2;

endfunction

//******************************************************************
//
// Name f
//
// Purpose: The purpose of this function is to compute the measure,
// f, that is used for computation of the required filter
// order for an equal ripple filter using the minimax algorithm.
// The origin of this algorithm is from the book, "Multirate
// Digital Signal Processing" by Ronald E. Crochiere and Lawrence
// R. Rabiner.
//
// Calling sequence: x = f(deltaP,deltaF)
//
// Inputs:
//
//    deltaP - The passband ripple.
//
//    deltaS - The stopband ripple.
//
// Outputs:
//
//    x - The value of f.
// 
//******************************************************************
function x = f(deltaP,deltaS)

x = 11.012 + 0.512 * (log10(deltaP) - log10(deltaS));

endfunction

//******************************************************************
//
// Name computeFilterOrder
//
// Purpose: The purpose of this function is to compute the filter
// order for an equiripple FIR filter.
// The origin of this algorithm is from the book, "Multirate
// Digital Signal Processing" by Ronald E. Crochiere and Lawrence
// R. Rabiner.
//
// Calling sequence: n = computeFilterOrder(deltaP,
//                                          deltaS,
//                                          deltaF,
//                                          Fs)
//
// Inputs:
//
//    deltaP - The passband ripple.
//
//    deltaS - The stopband ripple.
//
//    deltaF - The bandwidth of the transition band in the
//    normalized frequency domain.
//
//    Fs - The sampling frequency in sample per second.
//
// Outputs:
//
//    n - The filter order.
// 
//******************************************************************
function n = computeFilterOrder(deltaP,deltaS,deltaF,Fs)

// Compute number of taps.
n = dInf(deltaP,deltaS) / deltaF - f(deltaP,deltaS) * deltaF + 1;

// Round up to the next highest integer.
n = ceil(n);

endfunction
//******************************************************************
// Set up parameters.
//******************************************************************
// Sample rate is 16000 S/s.
Fsample = 16000

// Passband edge.
Fp = 2400;

// Stopband edge.
Fs = 3990;

// The desired audio bandwidth.
F = [0 Fp; Fs Fsample/2];

// Bandwidth of transition band.
deltaF = (Fs - Fp) / Fsample;

// Passband ripple
deltaP = 0.1;

// Stopband ripple.
deltaS = 0.005;

// Number of taps for our filter.
n = computeFilterOrder(deltaP,deltaS,deltaF,Fs)

// Ensure that the filter order is a multiple of 2.
n = n + 2 - modulo(n,2);

//******************************************************************
// Generate the FIR filter coefficients and magnitude of frequency
// response..  
//******************************************************************

//------------------------------------------------------------------
// This will be an antialiasing filter the preceeds the 2:1
// compressor.
//------------------------------------------------------------------
h = eqfir(n,F/Fsample,[1 0],[1/deltaP 1/deltaS]);

// Compute magnitude and frequency points.
[hm,fr] = frmag(h,1024);

//******************************************************************
// Plot magnitudes.
//******************************************************************
set("figure_style","new");

title("Antialiasing Filter - Sample Rate: 16000 S/s  (non-decimated)");
a = gca();
a.margins = [0.225 0.1 0.125 0.2];
a.grid = [1 1];
a.x_label.text = "F, Hz";
a.y_label.text = "|H|";
plot2d(fr*Fsample,hm);

