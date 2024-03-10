//**************************************************************************
// file name: PhaseAccumulator.h
//**************************************************************************
//_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
// This class implements a signal processing block that performs a
// phase accumulator function.
//_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/

#ifndef __PHASEACCUMULATOR__
#define __PHASEACCUMULATOR__

#include <stdint.h>

class PhaseAccumulator
{
  //***************************** operations **************************

  public:

  PhaseAccumulator(float sampleRate,float frequency);

  ~PhaseAccumulator(void);

  void setFrequency(float frequency);
  void reset(void);
  float run(void);

  //***************************** attributes **************************
  private:

  // The sample rate is needed when performing frequency changes.
  float sampleRate;

  // The operating frequency of the phase accumulator.
  float frequency;

  // This is the lag that will be used for accumulator update.
  float phaseStepSize;

  // This is the current phase bounded by (0,2*PI).
  float phaseAccumulator;
};

#endif // __PHASEACCUMULATOR__
