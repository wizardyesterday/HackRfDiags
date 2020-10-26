//**************************************************************************
// file name: WbFmModulator.h
//**************************************************************************
//_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
// This class implements a signal processing block known as a wideband FM
// modulator.  This class has a configurable FM deviation.
///_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/

#ifndef __WBFMMODULATOR__
#define __WBFMMODULATOR__

#include <stdint.h>

#include "Interpolator_int16.h"

class WbFmModulator
{
  //***************************** operations **************************

  public:

  WbFmModulator(void);
  ~WbFmModulator(void);

  void resetModulator(void);
  void setFrequencyDeviation(float deviaton);

  void acceptData(int16_t *bufferPtr,
                  uint32_t bufferLength,
                  int8_t *outputBufferPtr,
                  uint32_t *outputBufferLengthPtr);

  void displayInternalInformation(void);

  private:

  //*******************************************************************
  // Utility functions.
  //*******************************************************************
  uint32_t increasePcmSampleRate(int16_t *bufferPtr,uint32_t bufferLength);
  uint32_t increaseModulatedSampleRate(int8_t *bufferPtr,uint32_t bufferLength);
  uint32_t modulateSignal(int16_t *bufferPtr,uint32_t bufferLength);

  //*******************************************************************
  // Attributes.
  //*******************************************************************
  // Allowable frequency deviation.
  float frequencyDeviation;

  // Phase accumulator support.
  float phaseAccumulator;

  // This is the interpolated PCM data presented to the modulator.
  int16_t interpolatedPcmData[16384];

  // This is the complex output data that is created by the modulator.
  int16_t iModulatedData[16384];
  int16_t qModulatedData[16384];

  // Sine and cosine lookup tables.
  float Sin[16384];
  float Cos[16384];

  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
  // Working buffers used for the interpolation process.
  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
  int16_t stage1Buffer[2];
  int16_t stage2Buffer[4];
  int16_t stage3Buffer[8];
  int16_t stage4Buffer[16];
  int16_t stage5Buffer[32];
  int16_t iStage6Buffer[64];
  int16_t iStage7Buffer[128];
  int16_t iStage8Buffer[256];
  int16_t qStage6Buffer[64];
  int16_t qStage7Buffer[128];
  int16_t qStage8Buffer[256];

  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
  // These interpolators are used to convert the sample rate from 8000S/s
  // to 2048000S/s.
  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
  Interpolator_int16 *interpolator1Ptr;
  Interpolator_int16 *interpolator2Ptr;
  Interpolator_int16 *interpolator3Ptr;
  Interpolator_int16 *interpolator4Ptr;
  Interpolator_int16 *interpolator5Ptr;
  Interpolator_int16 *iInterpolator6Ptr;
  Interpolator_int16 *iInterpolator7Ptr;
  Interpolator_int16 *iInterpolator8Ptr;
  Interpolator_int16 *qInterpolator6Ptr;
  Interpolator_int16 *qInterpolator7Ptr;
  Interpolator_int16 *qInterpolator8Ptr;
};

#endif // __WBFMMODULATOR__
