//**************************************************************************
// file name: Decimator.h
//**************************************************************************
//_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
// This class implements a signal processing block known as a
// A decimator consists of an antialiasing filter followd by a sampling
// rate compressor.  In its most naive implementation, the input signal
// is presented to the filter, and samples are discarded as dictated by
// the decimation factor.  Unfortunately, the filter is operating at the
// pre-decimation sample rate (the higher sample rate).
// We can do better.  Let M be the decimation factor.  The more efficient
// approach is to insert M samples into the pipeline, and run the
// convolution sum between the filter coefficients, h(n), and the
// pipeline values.  The result is that the filter is operating at the
// decimated sampling rate.
// The only constraint is that the pipeline be an integer multiple of
// the decimation factor due to the manner in which the commutator is
// operating.  We have avoided using a polyphase filter and still get
// the same performance.  This alternative method merely costs more
// memory.
//_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/

#ifndef __DECIMATOR__
#define __DECIMATOR__

#include <stdint.h>

class Decimator
{
  //***************************** operations **************************

  public:

  Decimator(int filterLength,
            float *coefficientsPtr,
            int decimationFactor);

  ~Decimator(void);

  void resetFilterState(void);

  bool decimate(float inputSample,float *outputSamplePtr);

  private:

  float filterData(float x);
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

  // Pointer to the storage for buffered input samples.
  float *decimationBufferPtr;

  // Next decimation buffer postion.
  int decimationBufferIndex;

  // Decimation factor.
  int decimationFactor;

};

#endif // __DECIMATOR__
