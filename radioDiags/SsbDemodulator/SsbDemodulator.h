//**************************************************************************
// file name: SsbDemodulator.h
//**************************************************************************
//_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
// This class implements a signal processing block known as a SSB
// demodulator.  This class has a configurable demodulator gain.
///_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/

#ifndef __SSBDEMODULATOR__
#define __SSBDEMODULATOR__

#include <stdint.h>

#include "Decimator_int16.h"
#include "FirFilter_int16.h"
#include "IirFilter.h"

class SsbDemodulator
{
  //***************************** operations **************************

  public:

  SsbDemodulator(
      void (*pcmCallbackPtr)(int16_t *bufferPtr,uint32_t bufferLength));

  ~SsbDemodulator(void);

  void resetDemodulator(void);
  void setLsbDemodulationMode(void);
  void setUsbDemodulationMode(void);
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
  // An indicator that LSB signals are to be demodulated.
  bool lsbDemodulationMode;

  // This gain maps the envelope  to an information signal.
  float demodulatorGain;

  // Decimated in-phase and quadrature data samples.
  int16_t iData[512];
  int16_t qData[512];

  // Demodulated data is the demodulator gain times delta theta.
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

  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
  // The second filter performs a Hilbert transform operation, and the
  // first filter performs a delay line function that is used to compensate
  // for the group delay of the Hilbert transformer.  Note that I'm using
  // a decimator that will decimate by 1, thus, it'll be have like a
  // normal FIR filter.  This saves me the time of creating an integer-based
  // FIR filter.
  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
  FirFilter_int16 *delayLinePtr;
  FirFilter_int16 *phaseShifterPtr;

  IirFilter *dcRemovalFilterPtr;

  // Client callback support.
  void (*pcmCallbackPtr)(int16_t *bufferPtr,uint32_t bufferLength);
};

#endif // __SSBDEMODULATOR__
