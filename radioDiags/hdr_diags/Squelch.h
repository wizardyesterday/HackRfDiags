//**************************************************************************
// file name: Squelch.h
//**************************************************************************
//_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
// This class implements a signal-operated squelch function.
//_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/

#ifndef __SQUELCH__
#define __SQUELCH__

#include <stdint.h>

#include "SignalTracker.h"
#include "SignalDetector.h"

class Squelch
{
  //***************************** operations **************************

  public:

  Squelch(int32_t threshold);
  ~Squelch(void);

  void reset(void);
  void setThreshold(int32_t threshold);
  int32_t getThreshold(void);
  uint32_t getSignalMagnitude(void);
  bool run(uint32_t gainInDb,int8_t *bufferPtr,uint32_t bufferLength);

  //***************************** attributes **************************
  private:

  // The threshold is in dBFs units.
  int32_t threshold;

  // Signal tracking support.
  SignalTracker *trackerPtr;

  // Signal detection support.
  SignalDetector *detectorPtr;
};

#endif // __SQUELCH__
