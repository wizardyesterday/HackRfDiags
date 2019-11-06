//************************************************************************
// file name: Interpolator.cc
//************************************************************************
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include "Interpolator.h"

using namespace std;

/*****************************************************************************

  Name: Interpolator

  Purpose: The purpose of this function is to serve as the constructor for
  an instance of an Interpolator.  One thing should be mentioned.  The
  filter size needs to be an integer multiple of the interpolation factor
  so that the polyphase L polyphase filters have the same number of taps.
  The advantage of using polyphase filters is that, rather than stuffing
  zeros in the pipeline and filtering at interpolated sample rate, due to
  the commutation method, filtering occurs at the pre-interpolation (lower)
  sample rate.

  Calling Sequence: Interpolator(filterLength,
                                 coefficientsPtr,
                                 interpolationFactor)

  Inputs:

    filterLength - The number of taps for the prototype filter.

    coefficientPtr - A pointer to the prototype filter coefficients.

    interpolationFactor - The interpolation factor of the interpolator.

  Outputs:

    None.

*****************************************************************************/
Interpolator::Interpolator(int filterLength,
                           float *coefficientsPtr,
                           int interpolationFactor)
{
  int i;


  // Save for later use by the decimator.
  this->interpolationFactor = interpolationFactor;

  // Let's make that polyphase filter.
  createPolyphaseCoefficients(filterLength,
                              coefficientsPtr,
                              interpolationFactor);

  // Set the filter state to an initial value.
  resetFilterState();

  return;

} // Interpolator

/*****************************************************************************

  Name: ~Interpolator

  Purpose: The purpose of this function is to serve as the destructor for
  an instance of an Interpolator.

  Calling Sequence: ~Interpolator()

  Inputs:

    None.

  Outputs:

    None.

*****************************************************************************/
Interpolator::~Interpolator(void)
{

  // Release resources.
  delete[] coefficientStoragePtr;
  delete[] filterStatePtr;

  return;

} // ~Interpolator

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
void Interpolator::resetFilterState(void)
{
  int i;

  // Set to the beginning of filter state memory.
  ringBufferIndex = 0;

  // Clear the filter state.
  for (i = 0; i < polyphaseFilterLength; i++)
  {
    filterStatePtr[i] = 0;
  } // for

  return;

} // resetFilterState

/*****************************************************************************

  Name: filterData

  Purpose: The purpose of this function is to filter one sample of data.
  It uses a circular buffer to avoid the copying of data when the filter
  state memory is updated.

  Calling Sequence: y = filterData(coefficientsPtr)

  Inputs:

    coefficientsPtr - A pointer to the filter coefficients.

  Outputs:

    y - The output value of the filter.

*****************************************************************************/
float Interpolator::filterData(float *coefficientsPtr)
{
  float *h, y;
  int k, xIndex;

  // Reference the first filter coefficient.
  h = coefficientsPtr;

  // Set current position of index to deal with convolution sum.
  xIndex = ringBufferIndex;

  // Clear the accumulator.
  y = 0;
  
  for (k = 0; k < polyphaseFilterLength; k++)
  {
    // Perform multiply-accumulate operation.
    y = y + (h[k] * filterStatePtr[xIndex]);

    // Decrement the index in a modulo fashion.
    xIndex--;
    if (xIndex < 0)
    {
      // Wrap the index.
      xIndex = polyphaseFilterLength - 1;
    } // if
  } // for
 
  return (y);

} // filterData

/*****************************************************************************

  Name: createPolyphaseCoefficients

  Purpose: The purpose of this function is to create the polyphase filter
  array given the interpolation factor and an array of coefficients that
  represent the prototype filter.  Let's state some definitions so that we
  can describe how the data is formatted in the polyphase coefficient array.

  N - Prototype filter number of taps.
  L - Interpolation factor.
  q - Polyphase filter number of taps: N/L
  h - Prototype filter coefficient array.
  pi - Polyphase filter cofficient array for ith subfilter.

  The aggregate polyphase filter array is arranged as illustrated below.

  p0,p1,...pL-1.

  Each pi is arranged as illustrated below.

  p0  : h(0),h(L),h(2L)....
  p1  : h(1),h(L+1),h(2L+2)...
  .
  .
  pL-1: h(L-1),h(2L-1),h(3L-1)....

  Let's provide an example.  Consider that the prototype filter has 8
  taps, and the interpolation factor is 4.  Our pi polyphase filters
  will have the coefficients as illustrated below.

  p0: h(0),h(4)
  p1: h(1),h(5)
  p2: h(2),h(6)
  p3: h(3),h(7)  

  Calling Sequence: createPolyphaseCoefficients(filterLength,
                                                coefficientPtr,
                                                interpolationFactor)

  Inputs:

    filterLength - The number of taps for the filter.

    coefficientPtr - A pointer to the prototype filter coefficients.

    interpolationFactor - The interpolation factor of the interpolator.


  Outputs:

    None.

*****************************************************************************/
void Interpolator::createPolyphaseCoefficients(int filterLength,
                                               float *coefficientsPtr,
                                               int interpolationFactor)
{
  int i, j, index;
  int lookupIndex;

  // Polyphase filter lengths.
  polyphaseFilterLength = filterLength / interpolationFactor;
 
  // Allocate storage for polyphase filter coefficients.
  coefficientStoragePtr = new float[filterLength];

  // Allocate storage for the filter state.
  filterStatePtr = new float[polyphaseFilterLength];

 // Reference the beginning of the polyphase coefficients.
  index = 0;

  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
  // The outer loop increments through the polyphase
  // filter number, and the inner loop fills in the
  // coefficients of a particular polyphase filter.  This
  // is performed so that the control structure that
  // invokes the filter method need not be concerned with
  // dealing with indexing through the prototype filter
  // array.  Thus, everything is permuted over here, so
  // that the appropriate polyphase filter array is
  // passed to the filter method.
  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
  for (i = 0; i < interpolationFactor; i++)
  {
    for (j = 0; j < polyphaseFilterLength; j++)
    {
      // Make this easier to follow.
      lookupIndex = i + (j * interpolationFactor);

      // Permute the coefficients.
      coefficientStoragePtr[index] = coefficientsPtr[lookupIndex];

      // Reference the next location for storage of the permuted coefficients.
      index++;
    } // for
  } // for

  return;

} // createPolyphaseCoefficients

/*****************************************************************************

  Name: advancePipeline

  Purpose: The purpose of this function is to shift a sample into the
  pipeline.  This function is used for multirate processing.

  Calling Sequence: advancePipeline()

  Inputs:

    None.

  Outputs:

    None.

*****************************************************************************/
void Interpolator::advancePipeline(void)
{

  // Increment the index in a modulo fashion.
  ringBufferIndex++;
  if (ringBufferIndex == polyphaseFilterLength)
  {
    // Wrap the index.
    ringBufferIndex = 0;
  } // if

  return;

} // advancePipeline

/*****************************************************************************

  Name:  interpolate

  Purpose: The purpose of this function is to perform the function of an
  interpolator.  Here's how things work.  First, the sample is pushed into
  the pipeline.  Next, the FIR filter is invoked using the appropriate
  polyphase filter and the filtered output is stored in next location in
  the output buffer.  This filtering operation is performed the number of
  times as dictated by the value of the interpolation factor.  For example,
  if the interpolation factor were equal to 4, there would exist 4 sets of
  polyphase coefficients that would be presented to the FIR filter.
  The bottom line is that for every input sample, L output samples are
  generated, given that L is the interpolation factor.

  Calling Sequence:  outputSampleAvailable = interpolate(inputSample,
                                                         outputBufferPtr)

  Inputs:

    inputSample - A pointer to a buffer to be decimated.

    outputBufferPtr - A pointer to storage that is to accept the
    interpolated data.

  Outputs:

    None.

*****************************************************************************/
void Interpolator::interpolate(float inputSample,float *outputBufferPtr)
{
  int i;

  // Store the sample into the pipeline.
  filterStatePtr[ringBufferIndex] = inputSample;

  // Perform the interpolation operation.
  for (i = 0; i < interpolationFactor; i++)
  {
    outputBufferPtr[i] =
        filterData(&coefficientStoragePtr[i * polyphaseFilterLength]);
  } // for

  // We're done, so advance the pipeline.
  advancePipeline();

  return;

} // interpolate

