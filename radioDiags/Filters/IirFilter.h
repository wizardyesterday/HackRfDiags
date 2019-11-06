//**************************************************************************
// file name: IirFilter.h
//**************************************************************************
//_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
// This class implements a signal processing block known as a IIR filter.
// The structure of this filter is that of a Direct Form I filter.  This
// structure was chosen because it is least sensitive to finite word
// length effects, although, floating point values are being used.  The
// difference equation that is implemented is,
//
//   y(n) = b0*x(n) + b1*x(n-1) + ... - a1 * y(n-1) - a2*y(n-2) - ...
//
// The transfer function of this filter is,
//
//         (bo + b1/z + b2/z^2 + ...)
//  H(z) = --------------------------
//         (1 + a1/z + a2/z^2 + ... )
//
// The coefficients of the numerator are greater than or equal to zero,
// and the coefficients of the denominator are also greater than or equal
// to zero.  This is compatible with the results that occur when following
// the design procedures in DSP textbooks.  In this case, you do not have
// to flip the signs of the coefficients.  When using Scilab, Matlab, etc.,
// see what their documentation states with respect to the signs of
// coefficients that are generated when using their IIR filter design
// functions.
//
// A circular buffer is used to maintain filter state so that data does
// not actually need to be copied when advancing the pipeline.
///_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/

#ifndef __IIRFILTER__
#define __IIRFILTER__

#include <stdint.h>

#include "FirFilter.h"

class IirFilter
{
  //***************************** operations **************************

  public:

  IirFilter(int numeratorFilterLength,
            float *numeratorCoefficientsPtr,
            int denominatorFilterLength,
            float *denominatorCoefficientPtr);

  ~IirFilter(void);

  void resetFilterState(void);

  float filterData(float x);

  private:

  float filterRecursive(void);
  void shiftSampleIn(float x);

  //***************************** attributes **************************
  private:

  // The number of taps in the filter.
  int filterLength;

  // Pointer to the storage for the filter coefficients.
  float *coefficientStoragePtr;

  // Pointer to the filter state (previous samples).
  float *filterStatePtr;

  // Current ring buffer index.
  int ringBufferIndex;

  // Nonrecursive filter support.
  FirFilter *firFilterPtr;
};

#endif // __IIRFILTER__
