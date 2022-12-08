//**************************************************************************
// file name: DbfsCalculator.h
//**************************************************************************
//_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
// This class implements a signal processing block that computes decibles
// below a full scale value of a finite word length quantity.
//_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/

#ifndef __DBFSCALCULATOR__
#define __DBFSCALCULATOR__

#include <stdint.h>

class DbfsCalculator
{
  //***************************** operations **************************

  public:

  DbfsCalculator(uint32_t wordLengthInBits);

  ~DbfsCalculator(void);

  int32_t convertMagnitudeToDbFs(uint32_t signalMagnitude);

  //***************************** attributes **************************
  private:

  //*****************************************
  // Utility functions.
  //*****************************************

  //*****************************************
  // Attributes.
  //*****************************************
  // The full scale value is 2^(number of bits).
  uint32_t fullScaleValue;

  // This is used for dBFs computations.
  uint32_t fullScaleValueInDb;

  uint8_t magnitudeBuffer[16384];

  // This table is used to compute decibels for values [0,255].
  int32_t dbTable[257];
};

#endif // __DBFSCALCULATOR__
