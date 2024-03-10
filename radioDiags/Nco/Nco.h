//**************************************************************************
// file name: Nco.h
//**************************************************************************
//_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
// This class implements a signal processing block that performs a
// numerically controlled oscillator (NCO) function.
//_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/

#ifndef __NCO__
#define __NCO__

#include <stdint.h>
#include "PhaseAccumulator.h"

class Nco
{
  //***************************** operations **************************

  public:

  Nco(float sampleRate,float frequency);

  ~Nco(void);

  void setFrequency(float frequency);
  void reset(void);
  void run(float *iValuePtr,float *qValuePtr);
  void runFast(float *iValuePtr,float *qValuePtr);

  //***************************** attributes **************************
  private:

  // The sample rate is needed when performing frequency changes.
  float sampleRate;

  // The operating frequency of the NCO.
  float frequency;

  // Sine and cosine lookup tables.
  float Sin[16384];
  float Cos[16384];

  PhaseAccumulator *phaseAccumulatorPtr;
};

#endif // __NCO__
