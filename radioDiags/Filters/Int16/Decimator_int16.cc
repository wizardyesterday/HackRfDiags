//************************************************************************
// file name: Decimator_int16.cc
//************************************************************************
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>

#include "Decimator_int16.h"

using namespace std;

/*****************************************************************************

  Name: Decimator_int16

  Purpose: The purpose of this function is to serve as the constructor for
  an instance of an Decimator_int16.  One thing should be mentioned.  The
  pipeline size needs to be an integer multiple of the decimation factor
  due to the method of commutation used.  Rather than filtering all samples
  at the higher sample rate and throwing away samples, samples are inserted
  into the pipeline, and then the convolution sum is performed.  This allows
  the filter to run at the decimated sample rate.

  Calling Sequence: Decimator_int16(filterLength,coefficientsPtr,
                                    decimationFactor)

  Inputs:

    filterLength - The number of taps for the filter.

    coefficientPtr - A pointer to the filter coefficients.

    decimationFactor - The decimation factor of the decimator.

  Outputs:

    None.

*****************************************************************************/
Decimator_int16::Decimator_int16(int filterLength,
                                 float *coefficientsPtr,
                                 int decimationFactor)
{
  int i;
  float scaledCoefficient;

  // Save for later use.
  this->filterLength = filterLength;

  // Allocate storage for the coefficients.
  coefficientStoragePtr = new int16_t[filterLength];

  // Save the coefficients.
  for (i = 0; i < filterLength; i++)
  {
    // Scale and round the coefficient to a 16-bit integer.
    scaledCoefficient = coefficientsPtr[i] * 32768;
    scaledCoefficient = round(scaledCoefficient);

    // Now, we can save the scaled, rounded integer representation.
    coefficientStoragePtr[i] = (int16_t)scaledCoefficient;
  } // for

  // Allocate storage for the filter state.
  filterStatePtr = new int16_t[filterLength];

  // Save for later use by the decimator.
  this->decimationFactor = decimationFactor;

  // Allocate storage for the decimation input buffer.
  decimationBufferPtr = new int16_t[decimationFactor];

  // Set the filter state to an initial value.
  resetFilterState();

  return;

} // Decimator_int16

/*****************************************************************************

  Name: ~Decimator_int16

  Purpose: The purpose of this function is to serve as the destructor for
  an instance of an Decimator_int16.

  Calling Sequence: ~Decimator_int16()

  Inputs:

    None.

  Outputs:

    None.

*****************************************************************************/
Decimator_int16::~Decimator_int16(void)
{

  // Release resources.
  delete[] coefficientStoragePtr;
  delete[] filterStatePtr;
  delete[] decimationBufferPtr;

  return;

} // ~Decimator_int16

/*****************************************************************************

  Name: resetFilterState

  Purpose: The purpose of this function is to reset the filter state to its
  initial values.  This includes setting the ring buffer index to the
  beginning of the filter state memory and setting all entries of the
  filter state memory to a value of 0.

  Calling Sequence: resetFilterState()

  Inputs:

    None.

  Outputs:

    None.

*****************************************************************************/
void Decimator_int16::resetFilterState(void)
{
  int i;

  // Set to the beginning of filter state memory.
  ringBufferIndex = 0;

  // Clear the filter state.
  for (i = 0; i < filterLength; i++)
  {
    filterStatePtr[i] = 0;
  } // for

  // Set to the first position in the decimation buffer.  
  decimationBufferIndex = 0;

  // Clear the decimation buffer.
  for (i = 0; i < decimationFactor; i++)
  {
    decimationBufferPtr[i] = 0;
  } // for

  return;

} // resetFilterState

/*****************************************************************************

  Name: filterData

  Purpose: The purpose of this function is to filter one sample of data.
  It uses a circular buffer to avoid the copying of data when the filter
  state memory is updated.

  Calling Sequence: y = filterData(x)

  Inputs:

    x - The data sample to filter.

  Outputs:

    y - The output value of the filter.

*****************************************************************************/
int16_t Decimator_int16::filterData(int16_t x)
{
  int16_t *h, y;
  int k, xIndex;
  int32_t accumulator;

  // Reference the first filter coefficient.
  h = coefficientStoragePtr;

  // Store sample value.
  filterStatePtr[ringBufferIndex] = x;

  // Set current position of index to deal with convolution sum.
  xIndex = ringBufferIndex;

  // Set to the rounding constant.  This is a value of 0.5.
  accumulator = 1 << 14;

  for (k = 0; k < filterLength; k++)
  {
    // Perform multiply-accumulate operation.
    accumulator = accumulator + (h[k] * filterStatePtr[xIndex]);
 
    // Decrement the index in a modulo fashion.
    xIndex--;
    if (xIndex < 0)
    {
      // Wrap the index.
      xIndex = filterLength - 1;
    } // if
  } // for

  // Increment the index in a modulo fashion.
  ringBufferIndex++;
  if (ringBufferIndex == filterLength)
  {
    // Wrap the index.
    ringBufferIndex = 0;
  } // if

  // Transform from Q31 format to Q15 format. 
  y = (int16_t)(accumulator >> 15);
 
  return (y);

} // filterData

/*****************************************************************************

  Name: shiftSampleIn

  Purpose: The purpose of this function is to shift a sample into the
  pipeline.  This function is used for multirate processing.

  Calling Sequence: shiftSampleIn(x)

  Inputs:

    x - The sample to shift into the pipeline.

  Outputs:

    None.

*****************************************************************************/
void Decimator_int16::shiftSampleIn(int16_t x)
{

  // Store sample value.
  filterStatePtr[ringBufferIndex] = x;

  // Increment the index in a modulo fashion.
  ringBufferIndex++;
  if (ringBufferIndex == filterLength)
  {
    // Wrap the index.
    ringBufferIndex = 0;
  } // if

  return;

} // shiftSampleIn

/*****************************************************************************

  Name:  decimate

  Purpose: The purpose of this function is to perform the function of a
  decimator.  Here's how things work.  In order to decimate by M, one pushes
  M samples into the FIR filter pipeline, and instructs the filter to perform
  it's convolution sum on its pipeline.  In an ideal world, that's all one
  needs.  In the real world, the sample input buffer is not always an integer
  multiple of the decimation factor.  The simplest way to handle this is to
  buffer M samples up (this function is only getting invoked once per sample
  to keep complexity down), and then invoke the FIR filter function.  Thus,
  we have an easy to maintain commutator.

  Calling Sequence:  outputSampleAvailable = decimate(inputSample,
                                                      outputSamplePtr)

  Inputs:

    numberOfSamples - The number of samples in the input buffer.

    inputSample - A pointer to a buffer to be decimated.

    outputSamplePtr - A pointer to storage that is to accept the decimated
    data.

  Outputs:

    outputSampleAvailable - A flag that indicates whether or not an output
    sample is available.  A value of true indicates that an output sample is
    available, and a value of false indicates that the sample was buffered
    for later use.

*****************************************************************************/
bool Decimator_int16::decimate(int16_t inputSample,int16_t *outputSamplePtr)
{
  bool outputSampleAvailable;
  int i;
  int16_t value;

  // Default to no samples available.
  outputSampleAvailable = false;

  // Store sample in the buffer for later use.
  decimationBufferPtr[decimationBufferIndex] = inputSample;

  // Reference the next position for storage.
  decimationBufferIndex++;

  if (decimationBufferIndex == decimationFactor)
  {
    // Reset the index to the start of the buffer.
    decimationBufferIndex = 0;

    for (i = 0; i < (decimationFactor - 1); i++)
    {
      // Shift the sample into the pipeline.
      shiftSampleIn(decimationBufferPtr[i]);
    } // for

    //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_
    // Filter the sample.  The next sample is automatically
    // shifted into the pipeline.
    //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_
    value = filterData(decimationBufferPtr[decimationFactor - 1]);

    // Return the decimated sample.
    *outputSamplePtr = value;

    // Indicate to the caller that an output sample is available.
    outputSampleAvailable = true;
  } // if

  return (outputSampleAvailable);

} // decimate

