//**************************************************************************
// file name: DbfsCalculator.h
//**************************************************************************

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>

#include "DbfsCalculator.h"

// Allow a maximum of 31 bit wordlength.
#define MAX_WORD_LENGTH (31)

// This is the size of the magnitude to decibel lookup.
#define MAX_LOOKUP_INDEX (256)

/*****************************************************************************

  Name: DbfsCalculator

  Purpose: The purpose of this function is to serve as the contructor for
  an instance of a DbfsCalculator.

  Calling Sequence: DbfsCalculator(wordLengthInBits)

  Inputs:

    wordLengthInBits - The number of bits in the full scale value.

 Outputs:

    None.

*****************************************************************************/
DbfsCalculator::DbfsCalculator(uint32_t wordLengthInBits)
{
  uint32_t i;
  float dbLevel;

  if (wordLengthInBits > MAX_WORD_LENGTH)
  {
    // Clip it.
    wordLengthInBits = MAX_WORD_LENGTH;
  } // if

  // Save for later use.
  fullScaleValue = (1 << wordLengthInBits) - 1;

  // Note that 2's complement demands (full scale) / 2.
  fullScaleValueInDb = (uint32_t)(20 * log10((double)fullScaleValue));
  
  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
  // Construct the decibel table. The table can allow a
  // lookup that maps magnitudes from zero to 256 into
  // decibels.
  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
  for (i = 1; i <= MAX_LOOKUP_INDEX; i++)
  {
    dbLevel = 20 * log10((float)i);
    dbTable[i] = (int32_t)dbLevel;
  } // for 

  // Avoid minus infinity.
  dbTable[0] = dbTable[1]; 
  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/

} // DbfsCalculator

/*****************************************************************************

  Name: ~DbfsCalculator

  Purpose: The purpose of this function is to serve as the destructor for
  an instance of a DbfsCalculator.

  Calling Sequence: ~DbfsCalculator()

  Inputs:

    None.

 Outputs:

    None.

*****************************************************************************/
DbfsCalculator::~DbfsCalculator(void)
{

} // ~DbfsCalculator

/**************************************************************************

  Name: convertMagnitudeToDbFs

  Purpose: The purpose of this function is to convert a signal magnitude
  to decibels referred to the full scale value.

  Calling Sequence: dbFsValue = convertMagnitudeToDbFs(signalMagnitude)

  Inputs:

    signalMagnitude - The magnitude of the signal.

  Outputs:

    None.

**************************************************************************/
int32_t DbfsCalculator::convertMagnitudeToDbFs(
    uint32_t signalMagnitude)
{
  int32_t decibels;
  int32_t dbFsValue;

  // Initial value of no overhead.
  decibels = 0;

  if (signalMagnitude > fullScaleValue)
  {
    // Clip it.
    signalMagnitude = fullScaleValue;
  } // if

  // Scale the signal magnitude so that it can be used as an index.
  while (signalMagnitude > MAX_LOOKUP_INDEX)
  {
    // Scale it..
    signalMagnitude /= 2;

    // Add 6dB to account for the divide by 2.
    decibels += 6;
  } // while

  // Map to decibels. 
  dbFsValue = dbTable[signalMagnitude];

  // Add in the compensation for any scaling.
  dbFsValue += decibels;

  // Compute to decibels below the full scale value.
  dbFsValue -= fullScaleValueInDb;

  return (dbFsValue);

} // convertMagnitudeToDbFs
