//**************************************************************************
// file name: FmModulator.h
//**************************************************************************
//_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
// This class implements a signal processing block known as an FM
// modulator.
///_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/

#ifndef __FMMODULATOR__
#define __FMMODULATOR__

#include <stdint.h>

#include "Interpolator_int16.h"

class FmModulator
{
  //***************************** operations **************************

  public:

  FmModulator(void);
  ~FmModulator(void);

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
  uint32_t increaseSampleRate(int8_t *bufferPtr,uint32_t bufferLength);
  uint32_t modulateSignal(int16_t *bufferPtr,uint32_t bufferLength);

  //*******************************************************************
  // Attributes.
  //*******************************************************************
  // Allowable frequency deviation.
  float frequencyDeviation;

  // Phase accumulator support.
  float phaseAccumulator;

  // This is the complex output data that is created by the modulator.
  int16_t iModulatedData[512];
  int16_t qModulatedData[512];

  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
  // Working buffers used for the interpolation process.
  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
  int16_t iStage1Buffer[2];
  int16_t iStage2Buffer[4];
  int16_t iStage3Buffer[8];
  int16_t iStage4Buffer[16];
  int16_t iStage5Buffer[32];
  int16_t iStage6Buffer[64];
  int16_t iStage7Buffer[128];
  int16_t iStage8Buffer[256];
  int16_t qStage1Buffer[2];
  int16_t qStage2Buffer[4];
  int16_t qStage3Buffer[8];
  int16_t qStage4Buffer[16];
  int16_t qStage5Buffer[32];
  int16_t qStage6Buffer[64];
  int16_t qStage7Buffer[128];
  int16_t qStage8Buffer[256];

  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
  // These interpolators are used to convert the sample rate from 8000S/s
  // to 2048000S/s.
  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
  Interpolator_int16 *iInterpolator1Ptr;
  Interpolator_int16 *iInterpolator2Ptr;
  Interpolator_int16 *iInterpolator3Ptr;
  Interpolator_int16 *iInterpolator4Ptr;
  Interpolator_int16 *iInterpolator5Ptr;
  Interpolator_int16 *iInterpolator6Ptr;
  Interpolator_int16 *iInterpolator7Ptr;
  Interpolator_int16 *iInterpolator8Ptr;
  Interpolator_int16 *qInterpolator1Ptr;
  Interpolator_int16 *qInterpolator2Ptr;
  Interpolator_int16 *qInterpolator3Ptr;
  Interpolator_int16 *qInterpolator4Ptr;
  Interpolator_int16 *qInterpolator5Ptr;
  Interpolator_int16 *qInterpolator6Ptr;
  Interpolator_int16 *qInterpolator7Ptr;
  Interpolator_int16 *qInterpolator8Ptr;
};

#endif // __FMMODULATOR__
