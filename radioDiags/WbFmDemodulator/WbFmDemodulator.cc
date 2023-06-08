//************************************************************************
// file name: WbFmDemodulator.cc
//************************************************************************
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <ctype.h>

#include "WbFmDemodulator.h"

using namespace std;

// This provides a quick method for computing the atan2() function.
static float atan2LookupTable[256][256];

// These coefficients are used for the first stage decimation by 4.
static float postDemodDecimator1Coefficients[] =
{
   0.0243699,
   0.0769537,
   0.1463572,
   0.1967096,
   0.1967096,
   0.1463572,
   0.0769537,
   0.0243699
};

// These coefficients are used for the second stage decimation by 4.
static float postDemodDecimator2Coefficients[] =
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

// These coefficients are used for the third stage decimation by 2.
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

// These coefficients are used in the deemphasis filter that follows the
// demodulator.  The time constant of this filter is 75 microseconds.
static float deemphasisFilterNumeratorCoefficients[] =
{
  0.0253863,
  0.0253863
};

static float deemphasisFilterDenominatorCoefficients[] =
{
  -0.9492274
};

extern void nprintf(FILE *s,const char *formatPtr, ...);

/*****************************************************************************

  Name: WbFmDemodulator

  Purpose: The purpose of this function is to serve as the contructor for
  an instance of an WbFmDemodulator.

  Calling Sequence: WbFmDemodulator(pcmCallbackPtr)

  Inputs:

    pcmCallbackPtr - A pointer to a callback function that is to process
    demodulated data.

 Outputs:

    None.

*****************************************************************************/
WbFmDemodulator::WbFmDemodulator(
    void (*pcmCallbackPtr)(int16_t *bufferPtr,uint32_t bufferLength))
{
  int numberOfStage1DecimatorTaps;
  int numberOfStage2DecimatorTaps;
  int numberOfAudioDecimatorTaps;
  int numberOfDeemphasisFilterNumeratorTaps;
  int numberOfDeemphasisFilterDenominatorTaps;
  int x, y;
  double xArg, yArg;

  // Construct lookup table.
  for (x = 0; x < 256; x++)
  {
    for (y = 0; y < 256; y++)
    {
      // Set arguments into the proper range.
      xArg = (double)x - 128;
      yArg = (double)y - 128;

      // Save entry into the table.
      atan2LookupTable[y][x] = atan2(yArg,xArg);
    } // for
  } // for

  // Set to a nominal gain.
  demodulatorGain = 64000/(2 * M_PI);

  numberOfDeemphasisFilterNumeratorTaps =
    sizeof(deemphasisFilterNumeratorCoefficients) / sizeof(float);

  numberOfDeemphasisFilterDenominatorTaps =
    sizeof(deemphasisFilterDenominatorCoefficients) / sizeof(float);

  numberOfStage1DecimatorTaps =
      sizeof(postDemodDecimator1Coefficients) / sizeof(float);

  numberOfStage2DecimatorTaps =
      sizeof(postDemodDecimator2Coefficients) / sizeof(float);

  numberOfAudioDecimatorTaps =
    sizeof(audioDecimatorCoefficients) / sizeof(float);

  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
  // The deemphasis filter provides the appropriate rolloff to
  // compensate for the preemphasis that a broadcast FM station uses.
  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
  fmDeemphasisFilterPtr = 
    new IirFilter(numberOfDeemphasisFilterNumeratorTaps,
                  deemphasisFilterNumeratorCoefficients,
                  numberOfDeemphasisFilterDenominatorTaps,
                  deemphasisFilterDenominatorCoefficients);
  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/


  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
  // The decimator reduces the sample rate of the demodulated data to
  // 16000S/s.  The output of this decimator is used by the next
  // decimator state.
  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
  // Allocate the first post demodulator decimator.
  postDemodDecimator1Ptr = new Decimator_int16(numberOfStage1DecimatorTaps,
                                               postDemodDecimator1Coefficients,
                                               4);

  // Allocate the second post demodulator decimator.
  postDemodDecimator2Ptr = new Decimator_int16(numberOfStage2DecimatorTaps,
                                               postDemodDecimator2Coefficients,
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

  // Initial phase angle for d(theta)/dt computation.
  previousTheta = 0;

  // This is needed for outputting of PCM data.
  this->pcmCallbackPtr = pcmCallbackPtr;

  return;

} // WbFmDemodulator

/*****************************************************************************

  Name: ~WbFmDemodulator

  Purpose: The purpose of this function is to serve as the destructor for
  an instance of an WbFmDemodulator.

  Calling Sequence: ~WbFmDemodulator()

  Inputs:

    None.

  Outputs:

    None.

*****************************************************************************/
WbFmDemodulator::~WbFmDemodulator(void)
{

  // Release resources.
  delete fmDeemphasisFilterPtr;
  delete postDemodDecimator1Ptr;
  delete postDemodDecimator2Ptr;
  delete audioDecimatorPtr;

  return;

} // ~WbFmDemodulator

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
void WbFmDemodulator::resetDemodulator(void)
{

  // Reset all filter structures to their initial conditions.
  postDemodDecimator1Ptr->resetFilterState();
  postDemodDecimator2Ptr->resetFilterState();
  audioDecimatorPtr->resetFilterState();

  // Initial phase angle for d(theta)/dt computation.
  previousTheta = 0;

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
void WbFmDemodulator::setDemodulatorGain(float gain)
{

  this->demodulatorGain = gain;

  return;

} // setDemodulatorGain

/*****************************************************************************

  Name: acceptIqData

  Purpose: The purpose of this function is to perform the high level
  processing that is associated with the demodulation of a signal.
  The signal processing that is performed is now described.  First, the
  phase angle of the signal is computed by the equation:
  theta(n) = atan[Q(n) / i(n)].  Next, the phase angle of the previous IQ
  sample is subtracted from theta(n).  This approximates the derivative of
  the phase angle with respect to time.  Note that omega(n) = dtheta(n)/dn
  represents the instantaneous frequency of the signal.  Due to the branch
  cut of the atan() function at PI radians, processing is performed to
  ensure that the phase difference satisfies the equation,
  -PI < dtheta < PI.  Finally, the demodulated signal is multiplied by the
  demodulator gain, and the result is a scaled demodulated signal.  For
  speed of processing, a table lookup is performed for computation of the
  atan() function.

  Calling Sequence: acceptIqData(bufferPtr,bufferLength)

  Inputs:

    bufferPtr - A pointer to IQ data.

    bufferLength - The number of bytes contained in the buffer that is
    in the buffer.

  Outputs:

    None.

*****************************************************************************/
void WbFmDemodulator::acceptIqData(int8_t *bufferPtr,uint32_t bufferLength)
{
  uint32_t sampleCount;

  // Demodulate the signal.
  sampleCount = demodulateSignal(bufferPtr,bufferLength);

  // Create the PCM data stream.
  sampleCount = createPcmData(sampleCount);

  // Send the PCM data to the information sink.
  sendPcmData(sampleCount);

  return;

} // acceptIqData

/*****************************************************************************

  Name: demodulateSignal

  Purpose: The purpose of this function is to demodulate an FM signal that
  is stored in the iData[] and qData[] arrays.

  Calling Sequence: demodulateSignal(bufferPtr,bufferLength)

  Inputs:

    bufferPtr - A pointer to IQ data.

    bufferLength - The number of bytes contained in the buffer that is
    in the buffer.

  Outputs:

    sampleCount - The number of samples stored in the demodulatedData[]
    array.

*****************************************************************************/
uint32_t WbFmDemodulator::demodulateSignal(int8_t *bufferPtr,
                                       uint32_t bufferLength)
{
  uint32_t sampleCount;
  uint32_t i;
  uint8_t iData, qData;
  float theta;
  float deltaTheta;

  // We're mapping interleaved data into two separate buffers.
  sampleCount = bufferLength / 2;

  for (i = 0; i < sampleCount; i++)
  {
    // Compute lookup table indices.
    iData = (uint8_t)bufferPtr[2 * i] + 128;
    qData = (uint8_t)bufferPtr[(2 * i) + 1] + 128;

    // Compute phase angle.
    theta = atan2LookupTable[qData][iData];

    //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
    // Compute d(theta)/dt.
    //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
    deltaTheta = theta - previousTheta;

    //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
    // We want -M_PI <= deltaTheta <= M_PI.
    //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
    while (deltaTheta > M_PI)
    {
      deltaTheta -= (2 * M_PI);
    } // while

    while (deltaTheta < (-M_PI))
    {
      deltaTheta += (2 * M_PI);
    } // while
    //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/

    // Store the demodulated data.
    demodulatedData[i] =
      fmDeemphasisFilterPtr->filterData(demodulatorGain * deltaTheta);

    // Update our last phase angle for the next iteration.
    previousTheta = theta;

  } // for

  return (sampleCount);

} // demodulateSignal

/*****************************************************************************

  Name: createPcmData

  Purpose: The purpose of this function is to convert the signal, contained
  in demodulatedData[], into 16-bit little endian PCM data.

  Calling Sequence: sampleCount - createPcmData(bufferLength)

  Inputs:

    bufferLength - The number of entries contained in the demodulatedData[]
    buffer.

  Outputs:

    sampleCount - The number of samples stored in the pcmData[] array.

*****************************************************************************/
uint32_t WbFmDemodulator::createPcmData(uint32_t bufferLength)
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
    sampleAvailable =
      postDemodDecimator1Ptr->decimate((int16_t)demodulatedData[i],
                                       &sample);
    if (sampleAvailable)
    {
      // Perform the second stage of decimation.
      sampleAvailable = postDemodDecimator2Ptr->decimate(sample,&sample);

      if (sampleAvailable)
      {
        // Perform the second stage of decimation.
        sampleAvailable = audioDecimatorPtr->decimate(sample,&sample);

        if (sampleAvailable)
        {
          // Store PCM sample.
          pcmData[outputBufferIndex] = sample;

          // Reference the next storage location.
          outputBufferIndex++;
        } // if
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
void WbFmDemodulator::sendPcmData(uint32_t bufferLength)
{
  uint32_t i;

  // Send the PCM samples to the client callback.
  pcmCallbackPtr(pcmData,bufferLength);

  return;

} // sendPcmData

/**************************************************************************

  Name: displayInternalInformation

  Purpose: The purpose of this function is to display internal information
  in the wideband FM demodulator.

  Calling Sequence: displayInternalInformation()

  Inputs:

    None.

  Outputs:

    None.

**************************************************************************/
void WbFmDemodulator::displayInternalInformation(void)
{

  nprintf(stderr,"\n--------------------------------------------\n");
  nprintf(stderr,"Wideband FM Demodulator Internal Information\n");
  nprintf(stderr,"--------------------------------------------\n");

  nprintf(stderr,"Demodulator Gain         : %f\n",demodulatorGain);

  return;

} // displayInternalInformation


