//******************************************************************
// This program generates the required filter coefficients for a
// 90 degree phase shifter so that a single sideband signal can be
// demodulated.  The filter has an odd number of taps, and the group
// delay is G = (N - 1)/2, where N is the number of taps in the
// filter.  When processing an IQ signal pair, the Q component of
// the signal is passed through the phase shifter, and the I
// component is delayed by G/2 samples.  This can be realized by a
// FIR filter that has G delay elements.
// The filter specifications are listed below.
//
// Filter Type: Hilbert transformer.
// Window Function: Hamming.
//
// Chris G. 08/17/2017
///******************************************************************

//******************************************************************
// Function declarations.
//******************************************************************

//******************************************************************
// Set up parameters.
//******************************************************************
// Sample rate is 8000 S/s.
Fsample = 8000;

// Number of taps for our filter.
n = 31;

//******************************************************************
// Generate the FIR filter coefficients and magnitude of frequency
// response..  
//******************************************************************

//------------------------------------------------------------------
// This will be a Hilbert transformer with a Hamming window
// applied to the coefficients.
//------------------------------------------------------------------
h = hilb(n,'hm');

// Compute magnitude and frequency points.
[hm,fr] = frmag(h,1024);

//******************************************************************
// Plot magnitudes.
//******************************************************************
set("figure_style","new");

title("Phase Shifter Filter");
a = gca();
a.margins = [0.225 0.1 0.125 0.2];
a.grid = [1 1];
a.x_label.text = "F, Hz";
a.y_label.text = "|H|";
plot2d(fr*Fsample,hm);

