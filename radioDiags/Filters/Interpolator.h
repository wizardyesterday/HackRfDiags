//**************************************************************************
// file name: Interpolator.h
//**************************************************************************
//_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
// This class implements a signal processing block known as an
// interpolator.  An interpolator consists of a sampline rate expander
// followed by an anti-imaging filter.  To make things more efficient,
// a polyphase filter structure is used.  Essentially, one starts out
// with a prototype filter, designed with the interpolated sample rate
// used as the sampling frequency.  Call this h(n).  The coefficients of
// h(n) and then permuted in a certain way, and the results are written
// to a polyphase coefficient array, such that when filtering, the
// coefficients are accessed in a sequential manner.  The polyphase
// coefficient array is arranged as follows:
//
// p0,p1,p2,p3.....,
//
// Now, pi is the ith polyphase filter, and each has the same number of
// coefficients.  Now consider that we have a coefficient array called p.
// Then to access p0, the address of p[0] is rereferenced.  To access p1,
// the address of p[q] is referenced.  The symbol, q, is the number of
// coefficients in each polyphase filter.
// This causes a constraint on the number of taps in the prototype filter.
// They must be an integer mutiple of the decimation factor.  That is,
// given a prototype filter length of N, and a decimation factor of L,
// q = N/L must be an integer since that is the number of taps in each
// polyphase filter.
//_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/

#ifndef __INTERPOLATOR__
#define __INTERPOLATOR__

#include <stdint.h>

class Interpolator
{
  //***************************** operations **************************

  public:

  Interpolator(int filterLength,
               float *coefficientsPtr,
               int interpolationFactor);

  ~Interpolator(void);

  void resetFilterState(void);

  void interpolate(float inputSample,float *outputBufferPtr);

  private:

  float filterData(float *coefficientsPtr);
  void advancePipeline(void);

  void createPolyphaseCoefficients(int filterLength,
                                   float *coefficientsPtr,
                                   int interpolationFactor);

  //***************************** attributes **************************
  private:

  // The number of taps each polyphase filter.
  int polyphaseFilterLength;

  // Pointer to the storage for the polyphase filter coefficients.
  float *coefficientStoragePtr;

  // Pointer to the filter state (previous samples).
  float *filterStatePtr;

  // Current ring buffer index.
  int ringBufferIndex;

  // Interpolation factor.
  int interpolationFactor;

};

#endif // __INTERPOLATOR__
