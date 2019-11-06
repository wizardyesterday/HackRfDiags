//************************************************************************
// file name: FmDemodulator.cc
//************************************************************************
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <ctype.h>

#include "FmDemodulator.h"

using namespace std;

// This provides a quick method for computing the atan2() function.
static float atan2LookupTable[256][256];

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

extern void nprintf(FILE *s,const char *formatPtr, ...);

/*****************************************************************************

  Name: FmDemodulator

  Purpose: The purpose of this function is to serve as the contructor for
  an instance of an FmDemodulator.

  Calling Sequence: FmDemodulator()

  Inputs:

    None.

 Outputs:

    None.

*****************************************************************************/
FmDemodulator::FmDemodulator(void)
{
  int numberOfTunerDecimatorTaps;
  int numberOfPostDemodDecimatorTaps;
  int numberOfAudioDecimatorTaps;
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

      // Kludge to prevent 0/0 condition for the atan2() function.
      yArg = yArg + 1e-10;

      // Save entry into the table.
      atan2LookupTable[y][x] = atan2(yArg,xArg);
    } // for
  } // for

  // Set to a nominal gain.
  demodulatorGain = 64000/(2 * M_PI);

  numberOfTunerDecimatorTaps =
      sizeof(tunerDecimatorCoefficients) / sizeof(float);

  numberOfPostDemodDecimatorTaps =
      sizeof(postDemodDecimatorCoefficients) / sizeof(float);

  numberOfAudioDecimatorTaps =
    sizeof(audioDecimatorCoefficients) / sizeof(float);

  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
  // This pair of decimators reduce the sample rate from 256000S/s to
  // 64000S/s for use by the FM demodulator.
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

  // Initial phase angle for d(theta)/dt computation.
  previousTheta = 0;

  return;

} // FmDemodulator

/*****************************************************************************

  Name: ~FmDemodulator

  Purpose: The purpose of this function is to serve as the destructor for
  an instance of an FmDemodulator.

  Calling Sequence: ~FmDemodulator()

  Inputs:

    None.

  Outputs:

    None.

*****************************************************************************/
FmDemodulator::~FmDemodulator(void)
{

  // Release resources.
  delete iTunerDecimatorPtr;
  delete qTunerDecimatorPtr;
  delete postDemodDecimatorPtr;
  delete audioDecimatorPtr;

  return;

} // ~FmDemodulator

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
void FmDemodulator::resetDemodulator(void)
{

  // Reset all filter structures to their initial conditions.
  iTunerDecimatorPtr->resetFilterState();
  qTunerDecimatorPtr->resetFilterState();
  postDemodDecimatorPtr->resetFilterState();
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
void FmDemodulator::setDemodulatorGain(float gain)
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
void FmDemodulator::acceptIqData(int8_t *bufferPtr,uint32_t bufferLength)
{
  uint32_t sampleCount;

  // Decimate the signal to the demodulator sample rate.
  sampleCount = reduceSampleRate(bufferPtr,bufferLength);

  // Demodulate the decimated signal.
  sampleCount = demodulateSignal(sampleCount);

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

  Calling Sequence: sampleCount = reduceSampleRate(bufferPtr,bufferLength)

  Inputs:

    bufferPtr - A pointer to IQ data.

    bufferLength - The number of bytes contained in the buffer.

  Outputs:

    sampleCount - The number of samples stored in the iData[] and qData[]
    arrays.

*****************************************************************************/
uint32_t FmDemodulator::reduceSampleRate(
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

  Purpose: The purpose of this function is to demodulate an FM signal that
  is stored in the iData[] and qData[] arrays. The signal processing that
  is performed is now described.  First, the phase angle of the signal is
  computed by the equation: theta(n) = atan[Q(n) / i(n)].  Next, the
  phase angle of the previous IQ sample is subtracted from theta(n).  This
  approximates the derivative of the phase angle with respect to time.
  Note that omega(n) = dtheta(n)/dn represents the instantaneous frequency
  of the signal.  Due to the branch cut of the atan() function at PI
  radians, processing is performed to ensure that the phase difference
  satisfies the equation, -PI < dtheta < PI.  Finally, the demodulated
  signal is multiplied by the demodulator gain, and the result is a
  scaled demodulated signal.

  Calling Sequence: sampleCount = demodulateSignal(bufferLength)

  Inputs:

    bufferLength - The number of entries contained iData[] and qData buffers.

  Outputs:

    sampleCount - The number of samples stored in the demodulatedData[]
    array.

*****************************************************************************/
uint32_t FmDemodulator::demodulateSignal(uint32_t bufferLength)
{
  uint32_t i;
  uint8_t iIndex, qIndex;
  float theta;
  float deltaTheta;

  for (i = 0; i < bufferLength; i++)
  {
    // Comput lookup table indices.
    iIndex = (uint8_t)iData[i] + 128;
    qIndex = (uint8_t)qData[i] + 128;

    // Compute phase angle.
    theta = atan2LookupTable[qIndex][iIndex];

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
    demodulatedData[i] = demodulatorGain * deltaTheta;

    // Update our last phase angle for the next iteration.
    previousTheta = theta;

  } // for

  return (bufferLength);

} // demodulateSignal

/*****************************************************************************

  Name: createPcmData

  Purpose: The purpose of this function is to convert the signal, contained
  in demodulatedData[], into 16-bit little endian PCM data.

  Calling Sequence: createPcmData(bufferLength)

  Inputs:

    bufferLength - The number of entries contained in the demodulatedData[]
    buffer.

  Outputs:

    sampleCount - The number of samples stored in the pcmData[]
    array.

*****************************************************************************/
uint32_t FmDemodulator::createPcmData(uint32_t bufferLength)
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
      postDemodDecimatorPtr->decimate((int16_t)demodulatedData[i],&sample);
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
void FmDemodulator::sendPcmData(uint32_t bufferLength)
{
  uint32_t i;

  // Send the PCM samples to stdout for now.
  fwrite(pcmData,2,bufferLength,stdout);

  return;

} // sendPcmData

/**************************************************************************

  Name: displayInternalInformation

  Purpose: The purpose of this function is to display internal information
  in the FM demodulator.

  Calling Sequence: displayInternalInformation()

  Inputs:

    None.

  Outputs:

    None.

**************************************************************************/
void FmDemodulator::displayInternalInformation(void)
{

  nprintf(stderr,"\n--------------------------------------------\n");
  nprintf(stderr,"FM Demodulator Internal Information\n");
  nprintf(stderr,"--------------------------------------------\n");

  nprintf(stderr,"Demodulator Gain         : %f\n",demodulatorGain);

  return;

} // displayInternalInformation


