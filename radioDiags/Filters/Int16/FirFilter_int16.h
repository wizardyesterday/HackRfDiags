//**************************************************************************
// file name: FirFilter_int16.h
//**************************************************************************
//_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
// This class implements a signal processing block known as a FIR filter.
// A circular buffer is used to maintain filter state so that data does
// not actually need to be copied when advancing the pipeline.
//_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/

#ifndef __FIRFILTERINT16__
#define __FIRFILTERINT16__

#include <stdint.h>

class FirFilter_int16
{
  //***************************** operations **************************

  public:

  FirFilter_int16(int filterLength,
                  float *coefficientsPtr);

  ~FirFilter_int16(void);

  void resetFilterState(void);
  int16_t filterData(int16_t x);

  //***************************** attributes **************************
  private:

  // The number of taps in the filter.
  int filterLength;

  // Pointer to the storage for the filter coefficients.
  int16_t *coefficientStoragePtr;

  // Pointer to the filter state (previous samples).
  int16_t *filterStatePtr;

  // Current ring buffer index.
  int ringBufferIndex;

};

#endif // __FIRFILTERINT16__
