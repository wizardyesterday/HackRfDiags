//************************************************************************
// file name: SsbModulator.cc
//************************************************************************
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <ctype.h>

#include "SsbModulator.h"

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

// These coefficients provide a delay line function.
static float delayLineCoefficients[] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1};

// Hilbert transform coefficients smoothed by a Hamming window.
static float phaseShifterCoefficients[] =
{
  -0.0033953,
   0,
  -0.0058652,
   0,
  -0.0134385,
   0,
  -0.0281423,
   0,
  -0.0534836,
   0,
  -0.0980394,
   0,
  -0.1935638,
   0,
  -0.6302204,
   0,
   0.6302204,
   0,
   0.1935638,
   0,
   0.0980394,
   0,
   0.0534836,
   0,
   0.0281423,
   0,
   0.0134385,
   0,
   0.0058652,
   0,
   0.0033953
};

extern void nprintf(FILE *s,const char *formatPtr, ...);

/*****************************************************************************

  Name: SsbModulator

  Purpose: The purpose of this function is to serve as the contructor for
  an instance of an SsbModulator.

  Calling Sequence: SsbModulator()

  Inputs:

    None.

 Outputs:

    None.

*****************************************************************************/
SsbModulator::SsbModulator(void)
{
  int filter1Length, filter2Length, filter3Length, filter4Length;
  int filter5Length, filter6Length, filter7Length, filter8Length;
  int numberOfDelayLineTaps;
  int numberOfPhaseShifterTaps;

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

  numberOfDelayLineTaps =
      sizeof(delayLineCoefficients) / sizeof(float);

  numberOfPhaseShifterTaps =
      sizeof(phaseShifterCoefficients) / sizeof(float);

  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
  // These interpolators are used to convert the sample rate from 8000S/s
  // to 2048000S/s.  The first stage uses a conventional FIR filter,
  // whereas, the remaining stages are realized with half band FIR filters.
  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
  iInterpolator1Ptr = new Interpolator_int16(filter1Length,
                                             interpolator1Coefficients,
                                             2);
  iInterpolator2Ptr = new Interpolator_int16(filter2Length,
                                             interpolator2Coefficients,
                                             2);
  iInterpolator3Ptr = new Interpolator_int16(filter3Length,
                                             interpolator3Coefficients,
                                             2);
  iInterpolator4Ptr = new Interpolator_int16(filter4Length,
                                             interpolator4Coefficients,
                                             2);
  iInterpolator5Ptr = new Interpolator_int16(filter5Length,
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
  qInterpolator1Ptr = new Interpolator_int16(filter1Length,
                                             interpolator1Coefficients,
                                             2);
  qInterpolator2Ptr = new Interpolator_int16(filter2Length,
                                             interpolator2Coefficients,
                                             2);
  qInterpolator3Ptr = new Interpolator_int16(filter3Length,
                                             interpolator3Coefficients,
                                            2);
  qInterpolator4Ptr = new Interpolator_int16(filter4Length,
                                             interpolator4Coefficients,
                                             2);
  qInterpolator5Ptr = new Interpolator_int16(filter5Length,
                                             interpolator5Coefficients,
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

  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
  // This pair of filters provide the required processing so that the
  // SSB signal can be modulated.
  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
  // Allocate the delay line for the in-phase component.
  delayLinePtr = new FirFilter_int16(numberOfDelayLineTaps,
                                     delayLineCoefficients);

  // Allocate the phase shifter for the quadrature component.
  phaseShifterPtr = new FirFilter_int16(numberOfPhaseShifterTaps,
                                        phaseShifterCoefficients);
  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/

  // Default to modulating lower sideband signals.
  lsbModulationMode = true;

  return;

} // SsbModulator

/*****************************************************************************

  Name: ~SsbModulator

  Purpose: The purpose of this function is to serve as the destructor for
  an instance of an SsbModulator.

  Calling Sequence: ~SsbModulator()

  Inputs:

    None.

  Outputs:

    None.

*****************************************************************************/
SsbModulator::~SsbModulator(void)
{

  // Release resources.
  delete iInterpolator1Ptr;
  delete iInterpolator2Ptr;
  delete iInterpolator3Ptr;
  delete iInterpolator4Ptr;
  delete iInterpolator5Ptr;
  delete iInterpolator6Ptr;
  delete iInterpolator7Ptr;
  delete iInterpolator8Ptr;
  delete qInterpolator1Ptr;
  delete qInterpolator2Ptr;
  delete qInterpolator3Ptr;
  delete qInterpolator4Ptr;
  delete qInterpolator5Ptr;
  delete qInterpolator6Ptr;
  delete qInterpolator7Ptr;
  delete qInterpolator8Ptr;
  delete delayLinePtr;
  delete phaseShifterPtr;

  return;

} // ~SsbModulator

/*****************************************************************************

  Name: resetModulator

  Purpose: The purpose of this function is to reset the resetModulator to its
  initial condition.

  Calling Sequence: resetModulator()

  Inputs:

    None.

  Outputs:

    None.

*****************************************************************************/
void SsbModulator::resetModulator(void)
{

  // Reset all filter structures to their initial conditions.
  iInterpolator1Ptr->resetFilterState();
  iInterpolator2Ptr->resetFilterState();
  iInterpolator3Ptr->resetFilterState();
  iInterpolator4Ptr->resetFilterState();
  iInterpolator5Ptr->resetFilterState();
  iInterpolator6Ptr->resetFilterState();
  iInterpolator7Ptr->resetFilterState();
  iInterpolator8Ptr->resetFilterState();
  qInterpolator1Ptr->resetFilterState();
  qInterpolator2Ptr->resetFilterState();
  qInterpolator3Ptr->resetFilterState();
  qInterpolator4Ptr->resetFilterState();
  qInterpolator5Ptr->resetFilterState();
  qInterpolator6Ptr->resetFilterState();
  qInterpolator7Ptr->resetFilterState();
  qInterpolator8Ptr->resetFilterState();
  delayLinePtr->resetFilterState();
  phaseShifterPtr->resetFilterState();

  return;

} // resetModulator 

/*****************************************************************************

  Name: setLsbModulationMode

  Purpose: The purpose of this function is to set the demodulator mode so
  that lower sideband modulation is performed.

  Calling Sequence: setLsbModulationMode()

  Inputs:

    None.

  Outputs:

    None.

*****************************************************************************/
void SsbModulator::setLsbModulationMode(void)
{

  // Indicate that lower sideband signals are to be modulated.
  lsbModulationMode = true;

  return;

} // setLsbModulationMode

/*****************************************************************************

  Name: setUsbModulationMode

  Purpose: The purpose of this function is to set the demodulator mode so
  that upper sideband modulation is performed.

  Calling Sequence: setUsbModulationMode()

  Inputs:

    None.

  Outputs:

    None.

*****************************************************************************/
void SsbModulator::setUsbModulationMode(void)
{

  // Indicate that upper sideband signals are to be modulated.
  lsbModulationMode = false;

  return;

} // setUsbModulationMode

/*****************************************************************************

  Name: acceptData

  Purpose: The purpose of this function is to perform the high level
  processing that is associated with the modulation of a signal.

  Calling Sequence: acceptData(bufferPtr,bufferLength,outputBufferPtr,
                               outputBufferLengthPtr)

  Inputs:

    bufferPtr - A pointer to data to be modulated.

    bufferLength - The number of bytes contained in the buffer that is
    in the buffer.

  Outputs:

   outputBufferPtr - A pointer to storage to the modulated interpolated data.

   outputBufferLength - A pointer to storage for the length of the data
   that is referenced by outputBufferPtr.  The units are in bytes.

*****************************************************************************/
void SsbModulator::acceptData(int16_t *bufferPtr,
                             uint32_t bufferLength,
                             int8_t *outputBufferPtr,
                             uint32_t *outputBufferLengthPtr)
{
  uint32_t sampleCount;

  // Modulate the signal.
  sampleCount = modulateSignal(bufferPtr,bufferLength);

  // Interpolate the modulated signal to the system sample rate.
  *outputBufferLengthPtr = increaseSampleRate(outputBufferPtr,sampleCount);

  return;

} // acceptData

/*****************************************************************************

  Name: increaseSampleRate

  Purpose: The purpose of this function is to accept modulated IQ data
  and interpolate the data by a factor of 256.  The interpolated data will
  be stored 
  in the iData[] and qData[] arrays.

  Calling Sequence: sampleCount increaseSampleRate(bufferPtr,bufferLength)

  Inputs:

    bufferPtr - A pointer to the interpolated output data.

    bufferLength - The number of bytes contained in the buffer.

  Outputs:

    sampleCount - The number of samples stored in the iData[] and qData[]
    arrays.

*****************************************************************************/
uint32_t SsbModulator::increaseSampleRate(int8_t *bufferPtr,
                                         uint32_t bufferLength)
{
  uint32_t i, j;
  uint32_t sampleCount;

  // Set return value do indicate an interpolation factor of 256.
  sampleCount = bufferLength << 9;

  for (j = 0; j < bufferLength; j++)
  {
    //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
    // Interpolate the in-phase samples.
    //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
    // Perform the first stage of interpolation (1:2).
    iInterpolator1Ptr->interpolate(iModulatedData[j],iStage1Buffer);

    // Perform the second stage of interpolation (cascade: 1:4).
    for (i = 0; i < 2; i++)
    {
      iInterpolator2Ptr->interpolate(iStage1Buffer[i],&iStage2Buffer[i * 2]);
    } // for

    // Perform the third stage of interpolation (cascade: 1:8).
    for (i = 0; i < 4; i++)
    {
      iInterpolator3Ptr->interpolate(iStage2Buffer[i],&iStage3Buffer[i * 2]);
    } // for

    // Perform the fourth stage of interpolation (cascade: 1:16).
    for (i = 0; i < 8; i++)
    {
      iInterpolator4Ptr->interpolate(iStage3Buffer[i],&iStage4Buffer[i * 2]);
    } // for

    // Perform the fifth stage of interpolation (cascade: 1:32).
    for (i = 0; i < 16; i++)
    {
      iInterpolator5Ptr->interpolate(iStage4Buffer[i],&iStage5Buffer[i * 2]);
    } // for

    // Perform the sixth stage of interpolation (cascade: 1:64).
    for (i = 0; i < 32; i++)
    {
      iInterpolator6Ptr->interpolate(iStage5Buffer[i],&iStage6Buffer[i * 2]);
    } // for

    // Perform the seventh stage of interpolation (cascade: 1:128).
    for (i = 0; i < 64; i++)
    {
      iInterpolator7Ptr->interpolate(iStage6Buffer[i],&iStage7Buffer[i * 2]);
    } // for

    // Perform the eighth stage of interpolation (cascade: 1:256).
    for (i = 0; i < 128; i++)
    {
      iInterpolator8Ptr->interpolate(iStage7Buffer[i],&iStage8Buffer[i * 2]);
    } // for
    //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/

    //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
    // Interpolate the quadrature samples.
    //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
    // Perform the first stage of interpolation (now at 1:2).
    qInterpolator1Ptr->interpolate(qModulatedData[j],qStage1Buffer);

    // Perform the second stage of interpolation (cascade: 1:4).
    for (i = 0; i < 2; i++)
    {
      qInterpolator2Ptr->interpolate(qStage1Buffer[i],&qStage2Buffer[i * 2]);
    } // for

    // Perform the third stage of interpolation (cascade: 1:8).
    for (i = 0; i < 4; i++)
    {
      qInterpolator3Ptr->interpolate(qStage2Buffer[i],&qStage3Buffer[i * 2]);
    } // for

    // Perform the fourth stage of interpolation (cascade: 1:16).
    for (i = 0; i < 8; i++)
    {
      qInterpolator4Ptr->interpolate(qStage3Buffer[i],&qStage4Buffer[i * 2]);
    } // for

    // Perform the fifth stage of interpolation (cascade: 1:32).
    for (i = 0; i < 16; i++)
    {
      qInterpolator5Ptr->interpolate(qStage4Buffer[i],&qStage5Buffer[i * 2]);
    } // for

    // Perform the sixth stage of interpolation (cascade: 1:64).
    for (i = 0; i < 32; i++)
    {
      qInterpolator6Ptr->interpolate(qStage5Buffer[i],&qStage6Buffer[i * 2]);
    } // for

    // Perform the seventh stage of interpolation (cascade: 1:128).
    for (i = 0; i < 64; i++)
    {
      qInterpolator7Ptr->interpolate(qStage6Buffer[i],&qStage7Buffer[i * 2]);
    } // for

    // Perform the eighth stage of interpolation (cascade: 1:256).
    for (i = 0; i < 128; i++)
    {
      qInterpolator8Ptr->interpolate(qStage7Buffer[i],&qStage8Buffer[i * 2]);
    } // for
    //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/

    // Construct the output buffer.
    for (i = 0; i < 256; i++)
    {
      // Save the interpolated data to the output buffer.
      *bufferPtr++ = (int8_t)iStage8Buffer[i];
      *bufferPtr++ = (int8_t)qStage8Buffer[i];
    } // for
  } // for

  return (sampleCount);

} // reduceSampleRate

/*****************************************************************************

  Name: modulateSignal

  Purpose: The purpose of this function is to single sideband modulate a
  signal whose samples are stored in memory that is referenced by bufferPtr.
  Here's how things work.

    1. An incoming sample is scaled so that an overflow condition does
    not occur in the transmitter chain.  Let's call this the scaled sample.

    2. The scaled sample is delayed by a delay line whose delay accounts
    for the group delay of the Hilbert transformer which is discussed
    next.  Let's call this I delayed.

    3. The scaled sample is presented to a Hilbert transformer so that
    the required phase quadrature is realized.  Let's call this
   Q phase shifted.

    4. If upper sideband transmission is desired, the value of
    Q phase shifted is negated, otherwise it is not modified.

    5. The I delayed value is stored in the in-phase component of the
    output buffer.

    6. The value of Q phase shifted is stored in the quadrature component
    of the output buffer.

  The result will be a single sideband signal whose amplitude is determined
  by the information signal.     

  Calling Sequence: sampleCount = modulateSignal(bufferPtr,bufferLength)

  Inputs:

    bufferPtr - The baseband signal that is to be modulated.

    bufferLength - The number of entries contained iData[] and qData[]
    buffers.

  Outputs:

    sampleCount - The number of samples stored in the iModulatedData[] and
    qModulatedData[] arrays.

*****************************************************************************/
uint32_t SsbModulator::modulateSignal(int16_t *bufferPtr,
                                     uint32_t bufferLength)
{
  uint32_t i;
  float scaledSample;
  int16_t iDelayed, qPhaseShifted;

  for (i = 0; i < bufferLength; i++)
  {
    // Use these variables to make things easier.
    scaledSample = (float)bufferPtr[i];

    // Scale this to avoid overflow.
    scaledSample /= 4;

    // Delay in-phase component.
    iDelayed = delayLinePtr->filterData((int16_t)scaledSample);

    // Phase shift quadrature component.
    qPhaseShifted = phaseShifterPtr->filterData((int16_t)scaledSample);

    if (!lsbModulationMode)
    {
      // Negate for the USB case.
      qPhaseShifted = (-qPhaseShifted);
    } // if
    
    // Save modulated sample.
    iModulatedData[i] = iDelayed;
    qModulatedData[i] = qPhaseShifted;
  } // for

  return (i);

} // modulateSignal

/**************************************************************************

  Name: displayInternalInformation

  Purpose: The purpose of this function is to display internal information
  in the AM demodulator.

  Calling Sequence: displayInternalInformation()

  Inputs:

    None.

  Outputs:

    None.

**************************************************************************/
void SsbModulator::displayInternalInformation(void)
{

  nprintf(stderr,"\n--------------------------------------------\n");
  nprintf(stderr,"SSB Modulator Internal Information\n");
  nprintf(stderr,"--------------------------------------------\n");

  nprintf(stderr,"Modulation Mode        : ");

  if (lsbModulationMode)
  {
    nprintf(stderr,"LSB\n");
  } // if
  else
  {
    nprintf(stderr,"USB\n");
  } // else

  return;

} // displayInternalInformation


