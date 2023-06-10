//************************************************************************
// file name: FirFilter_int16.cc
//************************************************************************
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>

#include "FirFilter_int16.h"

using namespace std;

/*****************************************************************************

  Name: FirFilter_int16

  Purpose: The purpose of this function is to serve as the constructor for
  an instance of an FirFilter_int16.

  Calling Sequence: FirFilter_int16(filterLength,coefficientsPtr)

  Inputs:

    filterLength - The number of taps for the filter.

    coefficientPtr - A pointer to the filter coefficients.

  Outputs:

    None.

*****************************************************************************/
FirFilter_int16::FirFilter_int16(int filterLength,
                                 float *coefficientsPtr)
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

  // Set the filter state to an initial value.
  resetFilterState();

  return;

} // FirFilter

/*****************************************************************************

  Name: ~FirFilter_int16

  Purpose: The purpose of this function is to serve as the destructor for
  an instance of an FirFilter_int16.

  Calling Sequence: ~FirFilter_int16()

  Inputs:

    None.

  Outputs:

    None.

*****************************************************************************/
FirFilter_int16::~FirFilter_int16(void)
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
void FirFilter_int16::resetFilterState(void)
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
int16_t FirFilter_int16::filterData(int16_t x)
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
   accumulator =
      accumulator + ((int32_t)h[k] * (int32_t)filterStatePtr[xIndex]);
 
    //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
    // Saturate the result.
    //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
    if (accumulator > 0x3ffffff)
    {
      accumulator = 0x3ffffff;
    } // if
    else
    {
      if (accumulator < -0x40000000)
      {
        accumulator = -0x40000000;
      } // if
    } // else
    //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
 
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

