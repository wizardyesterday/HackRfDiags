//************************************************************************
// file name: AmDemodulator.cc
//************************************************************************
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <ctype.h>

#include "AmDemodulator.h"

using namespace std;

// These coefficients are used for decimation by 4.
static float stage1DecimatorCoefficients[] =
{
   0.0242683,
   0.0766338,
   0.1457589,
   0.1959036,
   0.1959036,
   0.1457589,
   0.0766338,
   0.0242683
};

// These coefficients are used for decimation by 4.
static float stage2DecimatorCoefficients[] =
{
   0.0057496,
   0.0263853,
   0.0605301,
   0.1074406,
   0.1523486,
   0.1804951,
   0.1804951,
   0.1523486,
   0.1074406,
   0.0605301,
   0.0263853,
   0.0057496
};

// These coefficients are used for decimation by 2.
static float stage3DecimatorCoefficients[] =
{
   0.0116487,
   0.0152694,
  -0.0109804,
  -0.0611915,
  -0.0736143,
   0.0187617,
   0.1988190,
   0.3481364,
   0.3481364,
   0.1988190,
   0.0187617,
  -0.0736143,
  -0.0611915,
  -0.0109804,
   0.0152694,
   0.0116487
};

//_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
// Coefficients for dc removal IIR filter.
//_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
static float dcRemovalNumeratorCoefficients[] = {1, -1};
static float dcRemovalDenominatorCoefficients[] = {-0.95};
//_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/

extern void nprintf(FILE *s,const char *formatPtr, ...);

/*****************************************************************************

  Name: AmDemodulator

  Purpose: The purpose of this function is to serve as the contructor for
  an instance of an AmDemodulator.

  Calling Sequence: AmDemodulator(pcmCallbackPtr)

  Inputs:

    pcmCallbackPtr - A pointer to a callback function that is to process
    demodulated data.

 Outputs:

    None.

*****************************************************************************/
AmDemodulator::AmDemodulator(
    void (*pcmCallbackPtr)(int16_t *bufferPtr,uint32_t bufferLength))
{
  int numberOfStage1DecimatorTaps;
  int numberOfStage2DecimatorTaps;
  int numberOfStage3DecimatorTaps;
  int numberOfDcRemovalNumeratorTaps;
  int numberOfDcRemovalDenominatorTaps;

  // Set to a nominal gain.
  demodulatorGain = 300;

  numberOfStage1DecimatorTaps =
      sizeof(stage1DecimatorCoefficients) / sizeof(float);

  numberOfStage2DecimatorTaps =
      sizeof(stage2DecimatorCoefficients) / sizeof(float);

  numberOfStage3DecimatorTaps =
      sizeof(stage3DecimatorCoefficients) / sizeof(float);

  numberOfDcRemovalNumeratorTaps =
      sizeof(dcRemovalNumeratorCoefficients) / sizeof(float);

  numberOfDcRemovalDenominatorTaps =
      sizeof(dcRemovalDenominatorCoefficients) / sizeof(float);

  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
  // This pair of decimators reduce the sample rate from 256000S/s to
  // 64000S/s.
  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
  // Allocate the decimator for the in-phase component.
  stage1IDecimatorPtr = new Decimator_int16(numberOfStage1DecimatorTaps,
                                            stage1DecimatorCoefficients,
                                            4);

  // Allocate the decimator for the quadrature component.
  stage1QDecimatorPtr = new Decimator_int16(numberOfStage1DecimatorTaps,
                                            stage1DecimatorCoefficients,
                                            4);
  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/

  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
  // This pair of decimators reduce the sample rate from 64000S/s to
  // 16000S/s.
  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
  // Allocate the decimator for the in-phase component.
  stage2IDecimatorPtr = new Decimator_int16(numberOfStage2DecimatorTaps,
                                            stage2DecimatorCoefficients,
                                            4);

  // Allocate the decimator for the quadrature component.
  stage2QDecimatorPtr = new Decimator_int16(numberOfStage2DecimatorTaps,
                                            stage2DecimatorCoefficients,
                                            4);
  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/

  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
  // This pair of decimators reduce the sample rate from 16000S/s to
  // 8000S/s.
  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
  // Allocate the decimator for the in-phase component.
  stage3IDecimatorPtr = new Decimator_int16(numberOfStage3DecimatorTaps,
                                            stage3DecimatorCoefficients,
                                            2);

  // Allocate the decimator for the quadrature component.
  stage3QDecimatorPtr = new Decimator_int16(numberOfStage3DecimatorTaps,
                                            stage3DecimatorCoefficients,
                                            2);
  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/

  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
  // Instantiate IIR filter state for dc removal.
  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
  dcRemovalFilterPtr = new IirFilter(numberOfDcRemovalNumeratorTaps,
                                     dcRemovalNumeratorCoefficients,
                                     numberOfDcRemovalDenominatorTaps,
                                     dcRemovalDenominatorCoefficients);
  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/

  // This is needed for outputting of PCM data.
  this->pcmCallbackPtr = pcmCallbackPtr;

  return;

} // AmDemodulator

/*****************************************************************************

  Name: ~AmDemodulator

  Purpose: The purpose of this function is to serve as the destructor for
  an instance of an AmDemodulator.

  Calling Sequence: ~AmDemodulator()

  Inputs:

    None.

  Outputs:

    None.

*****************************************************************************/
AmDemodulator::~AmDemodulator(void)
{

  // Release resources.
  delete stage1IDecimatorPtr;
  delete stage1QDecimatorPtr;
  delete stage2IDecimatorPtr;
  delete stage2QDecimatorPtr;
  delete stage3IDecimatorPtr;
  delete stage3QDecimatorPtr;
  delete dcRemovalFilterPtr;

  return;

} // ~AmDemodulator

/*****************************************************************************

  Name: resetDemodulator

  Purpose: The purpose of this function is to reset the demodulator to its
  initial condition.

  Calling Sequence: resetDemodulator()

  Inputs:

    None.

  Outputs:

    None.

*****************************************************************************/
void AmDemodulator::resetDemodulator(void)
{

  // Reset all filter structures to their initial conditions.
  stage1IDecimatorPtr->resetFilterState();
  stage1QDecimatorPtr->resetFilterState();
  stage2IDecimatorPtr->resetFilterState();
  stage2QDecimatorPtr->resetFilterState();
  stage3IDecimatorPtr->resetFilterState();
  stage3QDecimatorPtr->resetFilterState();
  dcRemovalFilterPtr->resetFilterState();

  return;

} // resetDemodulator

/*****************************************************************************

  Name: setDemodulatorGain

  Purpose: The purpose of this function is to set the gain of the
  demodulator.  This gain is used to convert the phase differences of the
  signal values into numbers that are later converted into PCM values.

  Calling Sequence: setDemodulatorGain(gain)

  Inputs:

    gain - The demodulator gain.

  Outputs:

    None.

*****************************************************************************/
void AmDemodulator::setDemodulatorGain(float gain)
{

  this->demodulatorGain = gain;

  return;

} // setDemodulatorGain

/*****************************************************************************

  Name: acceptIqData

  Purpose: The purpose of this function is to perform the high level
  processing that is associated with the demodulation of a signal.

  Calling Sequence: acceptIqData(bufferPtr,bufferLength)

  Inputs:

    bufferPtr - A pointer to IQ data.

    bufferLength - The number of bytes contained in the buffer that is
    in the buffer.

  Outputs:

    None.

*****************************************************************************/
void AmDemodulator::acceptIqData(int8_t *bufferPtr,uint32_t bufferLength)
{
  uint32_t sampleCount;

  // Decimate the signal to the demodulator sample rate.
  sampleCount = reduceSampleRate(bufferPtr,bufferLength);

  // Demodulate the decimated signal.
  demodulateSignal(sampleCount);

  // Create the PCM data stream.
  sampleCount = createPcmData(sampleCount);

  // Send the PCM data to the information sink.
  sendPcmData(sampleCount);

  return;

} // acceptIqData

/*****************************************************************************

  Name: reduceSampleRate

  Purpose: The purpose of this function is to accept interleaved IQ data
  and decimate the data by a factor of 4.  The decimated data will be stored
  in the iData[] and qData[] arrays.

  Calling Sequence: sampleCount reduceSampleRate(bufferPtr,bufferLength)

  Inputs:

    bufferPtr - A pointer to IQ data.

    bufferLength - The number of bytes contained in the buffer.

  Outputs:

    sampleCount - The number of samples stored in the iData[] and qData[]
    arrays.

*****************************************************************************/
uint32_t AmDemodulator::reduceSampleRate(
    int8_t *bufferPtr,
    uint32_t bufferLength)
{
  uint32_t i;
  uint32_t outputBufferIndex;
  bool sampleAvailable;
  int16_t sample;

  // Set to reference the beginning of the in-phase output buffer.
  outputBufferIndex = 0;

  // Decimate the in-phase samples.
  for (i = 0; i < bufferLength; i += 2)
  {
    sampleAvailable = stage1IDecimatorPtr->decimate((int16_t)bufferPtr[i],
                                                    &sample);

    if (sampleAvailable)
    {
      sampleAvailable = stage2IDecimatorPtr->decimate(sample,&sample);

      if (sampleAvailable)
      {
        sampleAvailable = stage3IDecimatorPtr->decimate(sample,&sample);

        if (sampleAvailable)
        {
          // Store the decimated sample.
          iData[outputBufferIndex] = sample;

          // Reference the next storage location.
          outputBufferIndex++;
        } // if
      } // if
    } // if
  } // for

  // Set to reference the beginning of the quadrature output buffer.
  outputBufferIndex = 0;

  // Decimate the quadrature samples.
  for (i = 1; i < (bufferLength + 1); i += 2)
  {
    sampleAvailable = stage1QDecimatorPtr->decimate((int16_t)bufferPtr[i],
                                                    &sample);

    if (sampleAvailable)
    {
      sampleAvailable = stage2QDecimatorPtr->decimate(sample,&sample);

      if (sampleAvailable)
      {
        sampleAvailable = stage3QDecimatorPtr->decimate(sample,&sample);

        if (sampleAvailable)
        {
          // Store the decimated sample.
          qData[outputBufferIndex] = sample;

          // Reference the next storage location.
          outputBufferIndex++;
        } // if
      } // if
    } // if
  } // for

  return (outputBufferIndex);

} // reduceSampleRate

/*****************************************************************************

  Name: demodulateSignal

  Purpose: The purpose of this function is to demodulate an AM signal that
  is stored in the iData[] and qData[] arrays.  The signal processing that
  is performed is now described.  First, the envelope of the signal is
  computed by the equation: m(n) = sqrt[I(n)*I(n) + Q(n)*Q(n)].  Next,
  the dc component of m(n) is removed.  Finally, m(t) is multiplied by the
  demodulator gain, and the result is a scaled demodulated signal.  It
  should be noted in the code that an approximation of sqrt(.) is used.

  Calling Sequence: sampleCount = demodulateSignal(bufferLength)

  Inputs:

    bufferLength - The number of entries contained iData[] and qData buffers.

  Outputs:

    sampleCount - The number of samples stored in the demodulatedData[]
    array.

*****************************************************************************/
uint32_t AmDemodulator::demodulateSignal(uint32_t bufferLength)
{
  uint32_t i;
  int16_t iMagnitude, qMagnitude;
  int16_t inputMagnitude;
  float outputMagnitude;

  for (i = 0; i < bufferLength; i++)
  {
    // Grab these values for the magnitude estimator.
    iMagnitude = abs(iData[i]);
    qMagnitude = abs(qData[i]);

    //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
    // This block of code performs a magnitude estimation of
    // the complex signal.
    //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
    if (iMagnitude > qMagnitude)
    {
      inputMagnitude = iMagnitude  + (qMagnitude >> 1);
    } // if
    else
    {
      inputMagnitude = qMagnitude + (iMagnitude >> 1);
    } // else
    //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/

    // Remove dc from the signal.
    outputMagnitude = dcRemovalFilterPtr->filterData((float)inputMagnitude);

    // Store the demodulated data.
    demodulatedData[i] = (int16_t)(demodulatorGain * outputMagnitude);

  } // for

  return (bufferLength);

} // demodulateSignal

/*****************************************************************************

  Name: createPcmData

  Purpose: The purpose of this function is to convert the signal, contained
  in demodulatedData[], into 16-bit little endian PCM data.

  Calling Sequence: sampleCount = createPcmData(bufferLength)

  Inputs:

    bufferLength - The number of entries contained in the demodulatedData[]
    buffer.

  Outputs:

    sampleCount - The number of samples stored in the pcmData[] array.

*****************************************************************************/
uint32_t AmDemodulator::createPcmData(uint32_t bufferLength)
{
  uint32_t i;

  for (i = 0; i < bufferLength; i++)
  {
    // Store PCM sample with dc offset removed.
    pcmData[i] = demodulatedData[i];
  } // for

  return (bufferLength);

} // createPcmData

/*****************************************************************************

  Name: sendPcmData

  Purpose: The purpose of this function is to send the PCM data to the
  appropriate information sink.

  Calling Sequence: sendPcmData(bufferLength)

  Inputs:

    bufferLength - The number of entries contained in the pcmData[] buffer.

  Outputs:

    None.

*****************************************************************************/
void AmDemodulator::sendPcmData(uint32_t bufferLength)
{
  uint32_t i;

  // Send the PCM samples to the client callback.
  pcmCallbackPtr(pcmData,bufferLength);

  return;

} // sendPcmData

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
void AmDemodulator::displayInternalInformation(void)
{

  nprintf(stderr,"\n--------------------------------------------\n");
  nprintf(stderr,"AM Demodulator Internal Information\n");
  nprintf(stderr,"--------------------------------------------\n");

  nprintf(stderr,"Demodulator Gain         : %f\n",demodulatorGain);

  return;

} // displayInternalInformation


