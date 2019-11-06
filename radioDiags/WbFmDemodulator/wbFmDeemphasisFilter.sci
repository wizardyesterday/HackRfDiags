//******************************************************************
// This program generates the required filter coefficients for a
// wideband FM (FM broadcast band) deemphasis filter.  The time
// constant, tau, is 75 microseconds, and the 3-db frequency of the
// passband of this lowpass filter is F-3dB = 1/(2*PI*tau).  The
// prototype analog lowpass filter is a simple one pole filter.  The
// filter will be operated at a sample rate of 256000S/s.
//
// Chris G. 07/24/2017
//******************************************************************

//******************************************************************
// Set up parameters.
//******************************************************************
// Filter time constant.
tau = 75e-6;

// Sample rate is 256000 S/s.
Fsample = 256000;

// Passband edge.
Fp = 1/(2*%pi*tau);

// Stopband edge - not used for iir() function.
Fs = Fp;

//******************************************************************
// Generate the IIR filter coefficients and magnitude of frequency
// response.  This is our deemphasis filter.
//******************************************************************
hz = iir(1,'lp','butt',[Fp Fs]/Fsample,[1,1])

// Compute magnitude and frequency points.
[hm,fr] = frmag(hz,1024);

//******************************************************************
// Plot magnitudes.
//******************************************************************
set("figure_style","new");

title("Fm Deemphasis Filter - Sample Rate: 256000 S/s");
a = gca();
a.margins = [0.225 0.1 0.125 0.2];
a.grid = [1 1];
a.x_label.text = "F, Hz";
a.y_label.text = "|H|";
plot2d(fr*Fsample,hm);

