//************************************************************************
// file name: WbFmModulator.cc
//************************************************************************
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <ctype.h>

#include "WbFmModulator.h"

using namespace std;

static float interpolator1Coefficients[] =
{
   0.0015969,
  -0.0111080,
  -0.0270501,
  -0.0265610,
  -0.0023190,
   0.0180618,
   0.0065495,
  -0.0183409,
  -0.0133345,
   0.0184489,
   0.0230891,
  -0.0161248,
  -0.0363745,
   0.0091343,
   0.0550219,
   0.0070312,
  -0.0862280,
  -0.0497761,
   0.1793543,
   0.4145808,
   0.4145808,
   0.1793543,
  -0.0497761,
  -0.0862280,
   0.0070312,
   0.0550219,
   0.0091343,
  -0.0363745,
  -0.0161248,
   0.0230891,
   0.0184489,
  -0.0133345,
  -0.0183409,
   0.0065495,
   0.0180618,
  -0.0023190,
  -0.0265610,
  -0.0270501,
  -0.0111080,
   0.0015969
};

static float interpolator2Coefficients[] =
{
  -0.0440934,
   0,
   0.2913764,
   0.5000000,
   0.2913764,
   0,
  -0.0440934,
   0
};

static float interpolator3Coefficients[] =
{
   0.2570951,
   0.5000000,
   0.2570951,
   0
};

static float interpolator4Coefficients[] =
{
  -0.0440934,
   0,
   0.2913764,
   0.5000000,
   0.2913764,
   0,
  -0.0440934,
  0
};

static float interpolator5Coefficients[] =
{
  -0.0440934,
   0,
   0.2913764,
   0.5000000,
   0.2913764,
   0,
  -0.0440934,
   0
};

static float interpolator6Coefficients[] =
{
   0.2570951,
   0.5000000,
   0.2570951,
   0
};

static float interpolator7Coefficients[] =
{
   0.2517491,
   0.4999998,
   0.2517491,
   0
};

static float interpolator8Coefficients[] =
{
   0.2504357,
   0.5000000,
   0.2504357,
   0
};

extern void nprintf(FILE *s,const char *formatPtr, ...);

/*****************************************************************************

  Name: WbFmModulator

  Purpose: The purpose of this function is to serve as the contructor for
  an instance of an WbFmModulator.

  Calling Sequence: WbFmModulator()

  Inputs:

    None.

 Outputs:

    None.

*****************************************************************************/
WbFmModulator::WbFmModulator(void)
{
  int filter1Length, filter2Length, filter3Length, filter4Length;
  int filter5Length, filter6Length, filter7Length, filter8Length;
  int i;

  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
  // Compute the number of taps in the prototype filter.
  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
  filter1Length = sizeof(interpolator1Coefficients)/sizeof(float);
  filter2Length = sizeof(interpolator2Coefficients)/sizeof(float);
  filter3Length = sizeof(interpolator3Coefficients)/sizeof(float);
  filter4Length = sizeof(interpolator4Coefficients)/sizeof(float);
  filter5Length = sizeof(interpolator5Coefficients)/sizeof(float);
  filter6Length = sizeof(interpolator6Coefficients)/sizeof(float);
  filter7Length = sizeof(interpolator7Coefficients)/sizeof(float);
  filter8Length = sizeof(interpolator8Coefficients)/sizeof(float);

  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
  // These interpolators are used to convert the sample rate from 8000S/s
  // to 2048000S/s.  The first stage uses a conventional FIR filter,
  // whereas, the remaining stages are realized with half band FIR filters.
  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
  interpolator1Ptr = new Interpolator_int16(filter1Length,
                                             interpolator1Coefficients,
                                             2);
  interpolator2Ptr = new Interpolator_int16(filter2Length,
                                             interpolator2Coefficients,
                                             2);
  interpolator3Ptr = new Interpolator_int16(filter3Length,
                                             interpolator3Coefficients,
                                             2);
  interpolator4Ptr = new Interpolator_int16(filter4Length,
                                             interpolator4Coefficients,
                                             2);
  interpolator5Ptr = new Interpolator_int16(filter5Length,
                                             interpolator5Coefficients,
                                             2);
  iInterpolator6Ptr = new Interpolator_int16(filter6Length,
                                             interpolator6Coefficients,
                                             2);
  iInterpolator7Ptr = new Interpolator_int16(filter7Length,
                                             interpolator7Coefficients,
                                             2);
  iInterpolator8Ptr = new Interpolator_int16(filter8Length,
                                             interpolator8Coefficients,
                                             2);
  qInterpolator6Ptr = new Interpolator_int16(filter6Length,
                                             interpolator6Coefficients,
                                             2);
  qInterpolator7Ptr = new Interpolator_int16(filter7Length,
                                             interpolator7Coefficients,
                                             2);
  qInterpolator8Ptr = new Interpolator_int16(filter8Length,
                                             interpolator8Coefficients,
                                             2);
  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/

  // The frequency deviation must be less than Fs/2 (Fs = 256000S/s).
  frequencyDeviation = 70000;

  // Inztantiate an NCO operating at a sample rate of 256000S/s.
  ncoPtr = new Nco(256000,0);

  return;

} // WbFmModulator

/*****************************************************************************

  Name: ~WbFmModulator

  Purpose: The purpose of this function is to serve as the destructor for
  an instance of an WbFmModulator.

  Calling Sequence: ~WbFmModulator()

  Inputs:

    None.

  Outputs:

    None.

*****************************************************************************/
WbFmModulator::~WbFmModulator(void)
{

  // Release resources.
  delete interpolator1Ptr;
  delete interpolator2Ptr;
  delete interpolator3Ptr;
  delete interpolator4Ptr;
  delete interpolator5Ptr;
  delete iInterpolator6Ptr;
  delete iInterpolator7Ptr;
  delete iInterpolator8Ptr;
  delete qInterpolator6Ptr;
  delete qInterpolator7Ptr;
  delete qInterpolator8Ptr;
  delete ncoPtr;

  return;

} // ~WbFmModulator

/*****************************************************************************

  Name: resetModulator

  Purpose: The purpose of this function is to reset the modulator to its
  initial condition.

  Calling Sequence: resetModulator()

  Inputs:

    None.

  Outputs:

    None.

*****************************************************************************/
void WbFmModulator::resetModulator(void)
{

  // Reset all filter structures to their initial conditions.
  interpolator1Ptr->resetFilterState();
  interpolator2Ptr->resetFilterState();
  interpolator3Ptr->resetFilterState();
  interpolator4Ptr->resetFilterState();
  interpolator5Ptr->resetFilterState();
  iInterpolator6Ptr->resetFilterState();
  iInterpolator7Ptr->resetFilterState();
  iInterpolator8Ptr->resetFilterState();
  qInterpolator6Ptr->resetFilterState();
  qInterpolator7Ptr->resetFilterState();
  qInterpolator8Ptr->resetFilterState();

  return;

} // resetModulator 

/*****************************************************************************

  Name: setFrequencyDeviation

  Purpose: The purpose of this function is to set the frequency deviation
  of the modulator.  Additionally, it is ensured that the frequency deviation 
  is constrained to be less than half of the modulator sample rate.

  Calling Sequence: setFrequencyDeviation(deviation)

  Inputs:

    deviation - The maximum frequency deviation that the information
    signal can cause.

  Outputs:

    None.

*****************************************************************************/
void WbFmModulator::setFrequencyDeviation(float deviation)
{

  if ((frequencyDeviation >= 0) && (frequencyDeviation <= 112000))
  {
    this->frequencyDeviation = deviation;
  } // if

  return;

} // setFrequencyDeviation

/*****************************************************************************

  Name: acceptData

  Purpose: The purpose of this function is to perform the high level
  processing that is associated with the modulation of a signal.

  Calling Sequence: acceptData(bufferPtr,bufferLength,outputBufferPtr,
                               outputBufferLengthPtr)

  Inputs:

    bufferPtr - A pointer to data to be modulated.

    bufferLength - The number of samples contained in the buffer that is
    in the buffer.

  Outputs:

   outputBufferPtr - A pointer to storage to the modulated interpolated data.

   outputBufferLength - A pointer to storage for the length of the data
   that is referenced by outputBufferPtr.  The units are in bytes.

*****************************************************************************/
void WbFmModulator::acceptData(int16_t *bufferPtr,
                               uint32_t bufferLength,
                               int8_t *outputBufferPtr,
                               uint32_t *outputBufferLengthPtr)
{
  uint32_t sampleCount;

  // Increase the PCM data rate to match the modulator data rate.
  sampleCount = increasePcmSampleRate(bufferPtr,bufferLength);

  // Modulate the signal.
  sampleCount = modulateSignal(interpolatedPcmData,sampleCount);

  // Interpolate the modulated signal to the system sample rate.
  *outputBufferLengthPtr = increaseModulatedSampleRate(outputBufferPtr,sampleCount);

  return;

} // acceptData

/*****************************************************************************

  Name: increasePcmSampleRate

  Purpose: The purpose of this function is to accept PCM data
  and interpolate the data by a factor of 32.  The interpolated data will
  be stored in the interpolatedPcmData[].

  Calling Sequence: sampleCount increaseSampleRate(bufferPtr,bufferLength)

  Inputs:

    bufferPtr - A pointer to the interpolated output data.

    bufferLength - The number of samples contained in the buffer.

  Outputs:

    sampleCount - The number of samples stored in the iData[] and qData[]
    arrays.

*****************************************************************************/
uint32_t WbFmModulator::increasePcmSampleRate(int16_t *bufferPtr,
                                              uint32_t bufferLength)
{
  uint32_t i, j;
  uint32_t sampleCount;
  int16_t *interpolatedPcmPtr;

  // Reference the beginning of the output buffer.
  interpolatedPcmPtr = interpolatedPcmData;

  // Set return value do indicate an interpolation factor of 32.
  sampleCount = bufferLength * 32;

  for (j = 0; j < bufferLength; j++)
  {
    // Perform the first stage of interpolation (1:2).
    interpolator1Ptr->interpolate(bufferPtr[j],stage1Buffer);

    // Perform the second stage of interpolation (cascade: 1:4).
    for (i = 0; i < 2; i++)
    {
      interpolator2Ptr->interpolate(stage1Buffer[i],&stage2Buffer[i * 2]);
    } // for

    // Perform the third stage of interpolation (cascade: 1:8).
    for (i = 0; i < 4; i++)
    {
      interpolator3Ptr->interpolate(stage2Buffer[i],&stage3Buffer[i * 2]);
    } // for

    // Perform the fourth stage of interpolation (cascade: 1:16).
    for (i = 0; i < 8; i++)
    {
      interpolator4Ptr->interpolate(stage3Buffer[i],&stage4Buffer[i * 2]);
    } // for

    // Perform the fifth stage of interpolation (cascade: 1:32).
    for (i = 0; i < 16; i++)
    {
      interpolator5Ptr->interpolate(stage4Buffer[i],&stage5Buffer[i * 2]);
    } // for

    // Construct the output buffer.
    for (i = 0; i < 32; i++)
    {
      // Save the interpolated data to the output buffer.
      *interpolatedPcmPtr++ = stage5Buffer[i];
    } // for
  } // for

  return (sampleCount);

} // increasePcmSampleRate

/*****************************************************************************

  Name: increaseModulatedSampleRate

  Purpose: The purpose of this function is to accept modulated IQ data
  and interpolate the data by a factor of 8.  The interpolated data will
  be stored in the iData[] and qData[] arrays.

  Note that the modulator scales the sample values appropriately so that
  after interpolation, the sample values will like in the range of -128
  to 127 inclusive.  This avoids overflows when type casting the
  interpolated result to an 8-bit signed quantity.

  Calling Sequence: sampleCount increaseModulatedSampleRate(bufferPtr,
                                                            bufferLength)

  Inputs:

    bufferPtr - A pointer to the interpolated output data.

    bufferLength - The number of bytes contained in the buffer.

  Outputs:

    sampleCount - The number of samples stored in the iData[] and qData[]
    arrays.

*****************************************************************************/
uint32_t WbFmModulator::increaseModulatedSampleRate(int8_t *bufferPtr,
                                                    uint32_t bufferLength)
{
  uint32_t i, j;
  uint32_t sampleCount;

  // Set return value do indicate an interpolation factor of 8.
  sampleCount = bufferLength << 4;

  for (j = 0; j < bufferLength; j++)
  {
    //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
    // Interpolate the in-phase samples.
    //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
    // Perform the sixth stage of interpolation (1:2).
    iInterpolator6Ptr->interpolate(iModulatedData[j],iStage6Buffer);

    // Perform the seventh stage of interpolation (cascade: 1:4).
    for (i = 0; i < 2; i++)
    {
      iInterpolator7Ptr->interpolate(iStage6Buffer[i],&iStage7Buffer[i * 2]);
    } // for

    // Perform the eighth stage of interpolation (cascade: 1:8).
    for (i = 0; i < 4; i++)
    {
      iInterpolator8Ptr->interpolate(iStage7Buffer[i],&iStage8Buffer[i * 2]);
    } // for
    //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/

    //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
    // Interpolate the quadrature samples.
    //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
    // Perform the sixth stage of interpolation (now at 1:2).
    qInterpolator6Ptr->interpolate(qModulatedData[j],qStage6Buffer);

    // Perform the seventh stage of interpolation (cascade: 1:4).
    for (i = 0; i < 2; i++)
    {
      qInterpolator7Ptr->interpolate(qStage6Buffer[i],&qStage7Buffer[i * 2]);
    } // for

    // Perform the eighth stage of interpolation (cascade: 1:8).
    for (i = 0; i < 4; i++)
    {
      qInterpolator8Ptr->interpolate(qStage7Buffer[i],&qStage8Buffer[i * 2]);
    } // for
   //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/

    // Construct the output buffer.
    for (i = 0; i < 8; i++)
    {
      // Save the interpolated data to the output buffer.
      *bufferPtr++ = (int8_t)iStage8Buffer[i];
      *bufferPtr++ = (int8_t)qStage8Buffer[i];
    } // for
  } // for

  return (sampleCount);

} // increaseModulatedSampleRate

/*****************************************************************************

  Name: modulateSignal

  Purpose: The purpose of this function is to frequency modulate a signal
  whose samples are stored in memory that is referenced by bufferPtr.
  Here's how things work.

    1. An incoming sample is scaled so that it is normalized to a value of
    unity.  Let's call this the phase increment.

    2. The phase increment is then multipled by the frequency deviation.

    3. The phase increment (currently a frequency deviation) is divided
    by the sampling frequency so that the frequency deviation is a
    fractional value.

    4. The phase increment is converted to radians.

    3. The phase increment is added to the phase accumulator.

    4. The value of the phase accumulator is processed to ensure that
    -PI < phase accumulator < Pi.

    5. The cosine of the phase accumulator is stored in the in-phase
    component of the output buffer.

    6. The sine of the phase accumulator is stored in the quadrature
    component of the output buffer.

  This results in a transmitted waveform that is frequency modulated by the
  information signal.

  Note that a lookup table approach is used for computation of the sin()
  and cos() functions to ease the processing load.

  Calling Sequence: sampleCount = modulateSignal(bufferPtr,bufferLength)

  Inputs:

    bufferPtr - The baseband signal that is to be modulated.

    bufferLength - The number of entries contained iData[] and qData buffers.

  Outputs:

    sampleCount - The number of samples stored in the iModulatedData[] and
    qModulatedData[] arrays.

*****************************************************************************/
uint32_t WbFmModulator::modulateSignal(int16_t *bufferPtr,
                                       uint32_t bufferLength)
{
  uint32_t i;
  float iSample, qSample;
  float ncoFrequency;

  for (i = 0; i < bufferLength; i++)
  {
    //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
    // We have to scale the interpolated PCM sample to unity.  Now,
    // the raw PCM samples [-32768,32767] have been upsampled by a
    // factor of 32.  This means that the range of the interpolated
    // samples is the original range divided by 32.  Hence the
    // maximum magnitude is 32768 / 32 = 1024.  This is the scale
    // factor that we need.  The result will be a range of [-1,1).
    //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
    // Scale the PCM sample to unity and multiply by the max deviation.
    ncoFrequency = frequencyDeviation * (float)bufferPtr[i] / 1024;

    // Set the frequency deviation.
    ncoPtr->setFrequency(ncoFrequency);

    // Retrieve the new IQ sample pair.
    ncoPtr->runFast(&iSample ,&qSample);

    //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
    // Since, the modulated data will be
    // interpolated by a factor of 8, and due
    // to the fact that the samples lie in the
    // range between -128 to 127 inclusive, the
    // samples could be scalled from unity to
    // 1024 to account for the interpolation
    // factor.  Let's be on the safe side and
    // scale the samples by a little less than
    // 1024.
    //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
    // Create I and Q signals.
    iSample *= 900;
    qSample *= 900;

    // Map from float to integer.
    iModulatedData[i] = (int16_t)iSample;
    qModulatedData[i] = (int16_t)qSample;
    //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
  } // for

  return (i);

} // modulateSignal

/**************************************************************************

  Name: displayInternalInformation

  Purpose: The purpose of this function is to display internal information
  in the AM modulator.

  Calling Sequence: displayInternalInformation()

  Inputs:

    None.

  Outputs:

    None.

**************************************************************************/
void WbFmModulator::displayInternalInformation(void)
{

  nprintf(stderr,"\n--------------------------------------------\n");
  nprintf(stderr,"Wideband FM Modulator Internal Information\n");
  nprintf(stderr,"--------------------------------------------\n");

  nprintf(stderr,"Frequency Deviation:      : %fHz\n",frequencyDeviation);

  return;

} // displayInternalInformation

