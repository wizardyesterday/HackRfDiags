//**************************************************************************
// file name: FmDemodulator.h
//**************************************************************************
//_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
// This class implements a signal processing block known as an FM
// demodulator.  This class has a configurable demodulator gain.
///_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/

#ifndef __FMDEMODULATOR__
#define __FMDEMODULATOR__

#include <stdint.h>

#include "Decimator_int16.h"

class FmDemodulator
{
  //***************************** operations **************************

  public:

  FmDemodulator(
    void (*pcmCallbackPtr)(int16_t *bufferPtr,uint32_t bufferLength));

  ~FmDemodulator(void);

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
  // This gain maps d(phi)/dt to an information signal.
  float demodulatorGain;

  // Storage for last phase angle.
  float previousTheta;

  // Decimated in-phase and quadrature data samples.
  int16_t iData[4096];
  int16_t qData[4096];

  // Demodulated data is the demodulator gain times delta theta.
  float demodulatedData[4096];

  // Demodulated data is converted into PCM data for listening.
  int16_t pcmData[512];

  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
  // The first two decimators are used to lower the sample rate to 64000S/s
  // that is useful for narrowband FM demodulation.  Note that we have
  // tuner decimators for both, the I and the Q arm.  The third decimator
  // lowers the sample rate to 16000S/s. The fourth decimator lowers the
  // sample rate to that needed for 8000S/s PCM audio.
  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
  Decimator_int16 *iTunerDecimatorPtr;
  Decimator_int16 *qTunerDecimatorPtr;
  Decimator_int16 *postDemodDecimatorPtr;
  Decimator_int16 *audioDecimatorPtr;

  // Client callback support.
  void (*pcmCallbackPtr)(int16_t *bufferPtr,uint32_t bufferLength);
};

#endif // __FMDEMODULATOR__
