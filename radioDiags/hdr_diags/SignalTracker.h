//**************************************************************************
// file name: SignalTracker.h
//**************************************************************************
//_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
// This class implements a signal-operated squelch function.
//_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/

#ifndef __SIGNALTRACKER__
#define __SIGNALTRACKER__

#include <stdint.h>

#include "SignalDetector.h"

#define SIGNALTRACKER_NOISE (0)
#define SIGNALTRACKER_STARTOFSIGNAL (1)
#define SIGNALTRACKER_SIGNALPRESENT (2)
#define SIGNALTRACKER_ENDOFSIGNAL (3)

class SignalTracker
{
  //***************************** operations **************************

  public:

  enum TrackerState {NoSignal, Tracking};

  SignalTracker(int32_t threshold);

  ~SignalTracker(void);

  void reset(void);
  void setThreshold(int32_t threshold);
  int32_t getThreshold(void);
  uint32_t getSignalMagnitude(void);
  uint16_t run(int8_t *bufferPtr,uint32_t bufferLength);

  //***************************** attributes **************************
  private:

  TrackerState state;

  // The threshold is in linear units.
  int32_t threshold;

  // Signal detection support.
  SignalDetector *detectorPtr;
};

#endif // __SIGNALTRACKER__
