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

  SignalDetector(int32_t threshold);

  ~SignalDetector(void);

  void setThreshold(int32_t threshold);
  int32_t getThreshold(void);
  uint32_t getSignalMagnitude(void);
  bool detectSignal(int8_t *bufferPtr,uint32_t bufferLength);

  //***************************** attributes **************************
  private:

  //*****************************************
  // Utility functions.
  //*****************************************
  int32_t convertMagnitudeToDbFs(uint32_t signalMagnitude);

  //*****************************************
  // Attributes.
  //*****************************************
  // The threshold is in dBFs units.
  int32_t threshold;

  // The average magnitude of the last IQ data block processed.
  uint32_t signalMagnitude;

  uint8_t magnitudeBuffer[16384];

  // This assumes 8-bit quantities for signal level.
  int32_t dbFsTable[257];
};

#endif // __SIGNALDETECTOR__
