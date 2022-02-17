//**************************************************************************
// file name: SignalDetector.h
//**************************************************************************
//_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
// This class implements a signal processing block known as a signal
// detector.
//_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/

#ifndef __SIGNALDETECTOR__
#define __SIGNALDETECTOR__

#include <stdint.h>

class SignalDetector
{
  //***************************** operations **************************

  public:

  SignalDetector(uint32_t threshold);

  ~SignalDetector(void);

  void setThreshold(uint32_t threshold);
  uint32_t getThreshold(void);
  uint32_t getSignalMagnitude(void);
  bool detectSignal(int8_t *bufferPtr,uint32_t bufferLength);

  //***************************** attributes **************************
  private:

  // The threshold is in linear units.
  uint32_t threshold;

  // The average magnitude of the last IQ data block processed.
  uint32_t signalMagnitude;

  uint8_t magnitudeBuffer[16384];
};

#endif // __SIGNALDETECTOR__
