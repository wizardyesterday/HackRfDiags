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
static float tunerDecimatorCoefficients[] =
{
   0.0142160,
   0.0172016,
   0.0208329,
   0.0186518,
   0.0092485,
  -0.0061460,
  -0.0235060,
  -0.0371215,
  -0.0417151,
  -0.0348236,
  -0.0183720,
   0.0014735,
   0.0163144,
   0.0188833,
   0.0064515,
  -0.0171519,
  -0.0420162,
  -0.0552787,
  -0.0458543,
  -0.0091367,
   0.0503878,
   0.1195422,
   0.1803717,
   0.2159224,
   0.2159224,
   0.1803717,
   0.1195422,
   0.0503878,
  -0.0091367,
  -0.0458543,
  -0.0552787,
  -0.0420162,
  -0.0171519,
   0.0064515,
   0.0188833,
   0.0163144,
   0.0014735,
  -0.0183720,
  -0.0348236,
  -0.0417151,
  -0.0371215,
  -0.0235060,
  -0.0061460,
   0.0092485,
   0.0186518,
   0.0208329,
   0.0172016,
   0.0142160
};

// These coefficients are used for decimation by 4.
static float postDemodDecimatorCoefficients[] =
{
   0.0022977,
   0.0237042,
   0.0605386,
   0.1127073,
   0.1645167,
   0.1971107,
   0.1971107,
   0.1645167,
   0.1127073,
   0.0605386,
   0.0237042,
   0.0022977
};

// These coefficients are used for decimation by 2.
static float audioDecimatorCoefficients[] =
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
  int numberOfTunerDecimatorTaps;
  int numberOfPostDemodDecimatorTaps;
  int numberOfAudioDecimatorTaps;
  int numberOfDcRemovalNumeratorTaps;
  int numberOfDcRemovalDenominatorTaps;

  // Set to a nominal gain.
  demodulatorGain = 300;

  numberOfTunerDecimatorTaps =
      sizeof(tunerDecimatorCoefficients) / sizeof(float);

  numberOfPostDemodDecimatorTaps =
      sizeof(postDemodDecimatorCoefficients) / sizeof(float);

  numberOfAudioDecimatorTaps =
    sizeof(audioDecimatorCoefficients) / sizeof(float);

  numberOfDcRemovalNumeratorTaps =
      sizeof(dcRemovalNumeratorCoefficients) / sizeof(float);

  numberOfDcRemovalDenominatorTaps =
      sizeof(dcRemovalDenominatorCoefficients) / sizeof(float);

  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
  // This pair of decimators reduce the sample rate from 256000S/s to
  // 64000S/s for use by the AM demodulator.
  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
  // Allocate the decimator for the in-phase component.
  iTunerDecimatorPtr = new Decimator_int16(numberOfTunerDecimatorTaps,
                                          tunerDecimatorCoefficients,
                                          4);

  // Allocate the decimator for the quadrature component.
  qTunerDecimatorPtr = new Decimator_int16(numberOfTunerDecimatorTaps,
                                          tunerDecimatorCoefficients,
                                          4);
  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/

  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
  // The decimator reduces the sample rate of the demodulated data to
  // 16000S/s.  The output of this decimator is used by the next
  // decimator state.
  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
  // Allocate the post demodulator decimator.
  postDemodDecimatorPtr = new Decimator_int16(numberOfPostDemodDecimatorTaps,
                                             postDemodDecimatorCoefficients,
                                             4); 
  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/

  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
  // This decimator has an input sample rate of 16000S/s and an
  // output sample rate of 8000S/s.
  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
  // Allocate the audio decimator.
  audioDecimatorPtr = new Decimator_int16(numberOfAudioDecimatorTaps,
                                         audioDecimatorCoefficients,
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
  delete iTunerDecimatorPtr;
  delete qTunerDecimatorPtr;
  delete postDemodDecimatorPtr;
  delete audioDecimatorPtr;
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
  iTunerDecimatorPtr->resetFilterState();
  qTunerDecimatorPtr->resetFilterState();
  postDemodDecimatorPtr->resetFilterState();
  audioDecimatorPtr->resetFilterState();
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
    sampleAvailable = iTunerDecimatorPtr->decimate((int16_t)bufferPtr[i],
                                                   &sample);
    if (sampleAvailable)
    {
      // Store the decimated sample.
      iData[outputBufferIndex] = sample;

      // Reference the next storage location.
      outputBufferIndex++;
    } // if
  } // for

  // Set to reference the beginning of the quadrature output buffer.
  outputBufferIndex = 0;

  // Decimate the quadrature samples.
  for (i = 1; i < (bufferLength + 1); i += 2)
  {
    sampleAvailable = qTunerDecimatorPtr->decimate((int16_t)bufferPtr[i],
                                                   &sample);
    if (sampleAvailable)
    {
      // Store the decimated sample.
      qData[outputBufferIndex] = sample;

      // Reference the next storage location.
      outputBufferIndex++;
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
  uint32_t outputBufferIndex;
  bool sampleAvailable;
  int16_t sample;

  // Reference the beginning of the PCM output buffer.
  outputBufferIndex = 0;

  for (i = 0; i < bufferLength; i++)
  {
    // Perform the first stage of decimation.
    sampleAvailable = postDemodDecimatorPtr->decimate(demodulatedData[i],
                                                      &sample);
    if (sampleAvailable)
    {
      // Perform the second stage of decimation.
      sampleAvailable = audioDecimatorPtr->decimate(sample,&sample);

      if (sampleAvailable)
      {
        // Store PCM sample with dc offset removed.
        pcmData[outputBufferIndex] = sample;

        // Reference the next storage location.
        outputBufferIndex++;
      } // if
    } // if

  } // for

  return (outputBufferIndex);

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


