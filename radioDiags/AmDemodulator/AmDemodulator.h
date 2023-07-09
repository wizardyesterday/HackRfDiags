//**************************************************************************
// file name: AmDemodulator.h
//**************************************************************************
//_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
// This class implements a signal processing block known as an AM
// demodulator.  This class has a configurable demodulator gain.
///_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/

#ifndef __AMDEMODULATOR__
#define __AMDEMODULATOR__

#include <stdint.h>

#include "Decimator_int16.h"
#include "IirFilter.h"

class AmDemodulator
{
  //***************************** operations **************************

  public:

  AmDemodulator(
      void (*pcmCallbackPtr)(int16_t *bufferPtr,uint32_t bufferLength));

 ~AmDemodulator(void);

  void resetDemodulator(void);
  void setDemodulatorGain(float gain);
  void acceptIqData(int8_t *bufferPtr,uint32_t bufferLength);
  void displayInternalInformation(void);

  private:

  //*******************************************************************
  // Utility functions.
  //*******************************************************************
  uint32_t reduceSampleRate(int8_t *bufferPtr,uint32_t bufferLength);
  uint32_t demodulateSignal(uint32_t bufferLength);
  uint32_t createPcmData(uint32_t bufferLength);
  void sendPcmData(uint32_t bufferLength);

  //*******************************************************************
  // Attributes.
  //*******************************************************************
  // This gain the envelope to an information signal.
  float demodulatorGain;

  // Decimated in-phase and quadrature data samples.
  int16_t iData[512];
  int16_t qData[512];

  // Demodulated data is the demodulator gain times envelope.
  int16_t demodulatedData[512];

  // Demodulated data is converted into PCM data for listening.
  int16_t pcmData[512];

  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
  // The first decimator pair is used to lower the sample rate to 64000S/s.
  // The second decimator pair is used to lower the sample rate to 16000S/s.
  // The third decimator pair is used to lower the sample rate to 8000S/s.
  // The demodulation process is applied to the signal at the final sample
  // rate.
  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
  Decimator_int16 *stage1IDecimatorPtr;
  Decimator_int16 *stage1QDecimatorPtr;
  Decimator_int16 *stage2IDecimatorPtr;
  Decimator_int16 *stage2QDecimatorPtr;
  Decimator_int16 *stage3IDecimatorPtr;
  Decimator_int16 *stage3QDecimatorPtr;

  IirFilter *dcRemovalFilterPtr;

  // Client callback support.
  void (*pcmCallbackPtr)(int16_t *bufferPtr,uint32_t bufferLength);
};

#endif // __AMDEMODULATOR__
