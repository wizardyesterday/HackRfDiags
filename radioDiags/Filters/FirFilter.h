//**************************************************************************
// file name: FirFilter.h
//**************************************************************************
//_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
// This class implements a signal processing block known as a FIR filter.
// A circular buffer is used to maintain filter state so that data does
// not actually need to be copied when advancing the pipeline.
//_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/

#ifndef __FIRFILTER__
#define __FIRFILTER__

#include <stdint.h>

class FirFilter
{
  //***************************** operations **************************

  public:

  FirFilter(int filterLength,
            float *coefficientsPtr);

  ~FirFilter(void);

  void resetFilterState(void);
  float filterData(float x);

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

};

#endif // __FIRFILTER__
