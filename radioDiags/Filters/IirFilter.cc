//************************************************************************
// file name: IirFilter.cc
//************************************************************************
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include "IirFilter.h"

using namespace std;

/*****************************************************************************

  Purpose: The purpose of this function is to serve as the constructor for
  an instance of an IirFilter.

  Calling Sequence: IirFilter(numeratorFilterLength,
                              numeratorCoefficientsPtr,
                              denominatorFilterLength,
                              denominatorCoefficientPtr)

  Inputs:

    numeratorFilterLength - The number of taps for the nonrecursive filter.

    numeratorCoefficientsPtr - A pointer to the nonrecursivefilter
    coefficients.

    denominatorFilterLength - The number of taps for the recursive filter.

    denominatorCoefficientPtr - A pointer to the recursive filter
    coefficients.

  Outputs:

    None.

*****************************************************************************/
IirFilter::IirFilter(int numeratorFilterLength,
                    float *numeratorCoefficientsPtr,
                    int denominatorFilterLength,
                    float *denominatorCoefficientPtr)
{
  int i;

  // Create an instance of the filter for the numerator.
  firFilterPtr = new FirFilter(numeratorFilterLength,
                               numeratorCoefficientsPtr);

  // Save for later use.
  this->filterLength = denominatorFilterLength;

  // Allocate storage for the coefficients.
  coefficientStoragePtr = new float[denominatorFilterLength];

  // Save the coefficients.
  for (i = 0; i < denominatorFilterLength; i++)
  {
    coefficientStoragePtr[i] = denominatorCoefficientPtr[i];
  } // for

  // Allocate storage for the filter state.
  filterStatePtr = new float[denominatorFilterLength];

  // Set the filter state to an initial value.
  resetFilterState();

  return;

} // IirFilter

/*****************************************************************************

  Name: ~IirFilter

  Purpose: The purpose of this function is to serve as the destructor for
  an instance of an IirFilter.

  Calling Sequence: ~IirFilter()

  Inputs:

    None.

  Outputs:

    None.

*****************************************************************************/
IirFilter::~IirFilter(void)
{

  // Release resources.
  delete firFilterPtr;
  delete[] coefficientStoragePtr;
  delete[] filterStatePtr;

  return;

} // ~IirFilter

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
void IirFilter::resetFilterState(void)
{
  int i;

  // Reset the nonrecursive filter state.
  firFilterPtr->resetFilterState();

  // Set to the beginning of filter state memory.
  ringBufferIndex = 0;

  // Clear the filter state.
  for (i = 0; i < filterLength; i++)
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

  Calling Sequence: y = filterData(x)

  Inputs:

    x - The data sample to filter.

  Outputs:

    y - The output value of the filter.

*****************************************************************************/
float IirFilter::filterData(float x)
{
  float y;

  // Perform nonrecursive filtering.
  y = firFilterPtr->filterData(x);

  // Perform recursive filtering.
  y -= filterRecursive();

  // Save the new output in the pipeline.
  shiftSampleIn(y);
 
  return (y);

} // filterData

/*****************************************************************************

  Name: filterRecursive

  Purpose: The purpose of this function is to filter data by convolving
  the coefficients with the pipeline.  The processing is used for past
  output aemples that are stored in the pipeline.
  It uses a circular buffer to avoid the copying of data when the filter
  state memory is updated.

  Calling Sequence: y = filterRecursive(x)

  Inputs:

    None.

  Outputs:

    y - The output value of the filter.

*****************************************************************************/
float IirFilter::filterRecursive(void)
{
  float *h, y;
  int k, xIndex;

  // Reference the first filter coefficient.
  h = coefficientStoragePtr;

  // Set current position of index to deal with convolution sum.
  xIndex = ringBufferIndex;

  // Clear the accumulator.
  y = 0;
  
  for (k = 0; k < filterLength; k++)
  {
    // Perform multiply-accumulate operation.
    y = y + (h[k] * filterStatePtr[xIndex]);

    // Decrement the index in a modulo fashion.
    xIndex--;
    if (xIndex < 0)
    {
      // Wrap the index.
      xIndex = filterLength - 1;
    } // if
  } // for

  return (y);

} // filterRecursive

/*****************************************************************************

  Name: shiftSampleIn

  Purpose: The purpose of this function is to shift a sample into the
  pipeline.  This function is used for the recursive aspect of the
  processing.

  Calling Sequence: shiftSampleIn(x)

  Inputs:

    x - The sample to shift into the pipeline.

  Outputs:

    None.

*****************************************************************************/
void IirFilter::shiftSampleIn(float x)
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
