//************************************************************************
// file name: FirFilter.cc
//************************************************************************
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include "FirFilter.h"

using namespace std;

/*****************************************************************************

  Name: FirFilter

  Purpose: The purpose of this function is to serve as the constructor for
  an instance of an FirFilter.

  Calling Sequence: FirFilter(filterLength,coefficientsPtr)

  Inputs:

    filterLength - The number of taps for the filter.

    coefficientPtr - A pointer to the filter coefficients.

  Outputs:

    None.

*****************************************************************************/
FirFilter::FirFilter(int filterLength,
                     float *coefficientsPtr)
{
  int i;

  // Save for later use.
  this->filterLength = filterLength;

  // Allocate storage for the coefficients.
  coefficientStoragePtr = new float[filterLength];

  // Save the coefficients.
  for (i = 0; i < filterLength; i++)
  {
    coefficientStoragePtr[i] = coefficientsPtr[i];
  } // for

  // Allocate storage for the filter state.
  filterStatePtr = new float[filterLength];

  // Set the filter state to an initial value.
  resetFilterState();

  return;

} // FirFilter

/*****************************************************************************

  Name: ~FirFilter

  Purpose: The purpose of this function is to serve as the destructor for
  an instance of an FirFilter.

  Calling Sequence: ~FirFilter()

  Inputs:

    None.

  Outputs:

    None.

*****************************************************************************/
FirFilter::~FirFilter(void)
{

  // Release resources.
  delete[] coefficientStoragePtr;
  delete[] filterStatePtr;

  return;

} // ~FirFilter

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
void FirFilter::resetFilterState(void)
{
  int i;

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
float FirFilter::filterData(float x)
{
  float *h, y;
  int k, xIndex;

  // Reference the first filter coefficient.
  h = coefficientStoragePtr;

  // Store sample value.
  filterStatePtr[ringBufferIndex] = x;

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

  // Increment the index in a modulo fashion.
  ringBufferIndex++;
  if (ringBufferIndex == filterLength)
  {
    // Wrap the index.
    ringBufferIndex = 0;
  } // if
 
  return (y);

} // filterData

