//**************************************************************************
// file name: WbFmDemodulator.h
//**************************************************************************
//_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
// This class implements a signal processing block known as a wideband  FM
// demodulator.  This class has a configurable demodulator gain.
///_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/

#ifndef __WBFMDEMODULATOR__
#define __WBFMDEMODULATOR__

#include <stdint.h>

#include "IirFilter.h"
#include "Decimator_int16.h"

class WbFmDemodulator
{
  //***************************** operations **************************

  public:

  WbFmDemodulator(
      void (*pcmCallbackPtr)(int16_t *bufferPtr,uint32_t bufferLength));

  ~WbFmDemodulator(void);

  void resetDemodulator(void);
  void setDemodulatorGain(float gain);
  void acceptIqData(int8_t *bufferPtr,uint32_t bufferLength);
  void displayInternalInformation(void);

  private:

  //*******************************************************************
  // Utility functions.
  //*******************************************************************
  uint32_t demodulateSignal(int8_t *bufferPtr,uint32_t bufferLength);
  uint32_t createPcmData(uint32_t bufferLength);
  void sendPcmData(uint32_t bufferLength);

  //*******************************************************************
  // Attributes.
  //*******************************************************************
  // This gain maps d(phi)/dt to an information signal.
  float demodulatorGain;

  // Storage for last phase angle.
  float previousTheta;

  // Demodulated data is the demodulator gain times delta theta.
  float demodulatedData[16384];

  // Demodulated data is converted into PCM data for listening.
  int16_t pcmData[512];

  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
  // These three decimators represent a three-stage decimator that
  // decimates a signal from a sample rate of 256000S/s down to 8000S/s
  // that is needed for 8000S/s PCM audio.
  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
  Decimator_int16 *postDemodDecimator1Ptr;
  Decimator_int16 *postDemodDecimator2Ptr;
  Decimator_int16 *audioDecimatorPtr;

  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
  // This filter represents an FM deemphasis filter that follows the FM
  // demodulator when the system is used to receive broadcasts on the FM
  // broadcast band.
  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
  IirFilter *fmDeemphasisFilterPtr;

  // Client callback support.
  void (*pcmCallbackPtr)(int16_t *bufferPtr,uint32_t bufferLength);
};

#endif // _WB_FMDEMODULATOR__
