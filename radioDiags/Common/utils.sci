//******************************************************************
// Filename: utils.sci
//******************************************************************

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
x1 = (a1 * log10(deltaP))^2 + a2 * log10(deltaP) + a3;
x1 = log10(deltaS) * x1;

// Compute second term.
x2 = (a4 * log10(deltaP))^2 + a5 * log10(deltaP) + a6;
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
//
// Name computeNextMultiple
//
// Purpose: The purpose of this function is to round a number up
// to the next multiple of another number.
//
// Calling sequence: m = computeNextMultiple(n,k)
//
// Inputs:
//
//    n - The value that is to be rounded up to the next multiple
//    of k.
//
//    k - The divisor used for the rounding up of n.
//
// Outputs:
//
//    m - The rounded up number.
// 
//******************************************************************
function m = computeNextMultiple(n,k)

  if modulo(n,k) <> 0
    // Round up to the next multiple of k.
    m = n + k - modulo(n,k);
  else
    // We are already at a multiple of k.
    m = n;
  end

endfunction
