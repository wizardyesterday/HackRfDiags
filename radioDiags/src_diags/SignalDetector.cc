#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>

#include "SignalDetector.h"

// The current variable gain setting.
extern int32_t radio_adjustableReceiveGainInDb;

/*****************************************************************************

  Name: SignalDetector

  Purpose: The purpose of this function is to serve as the contructor for
  an instance of a SignalDetector.

  Calling Sequence: SignalDetector(threshold)

  Inputs:

    threshold - The detection threshold that is used to determine signal
    presence or absense..

 Outputs:

    None.

*****************************************************************************/
SignalDetector::SignalDetector(int32_t threshold)
{
  uint32_t i;
  uint32_t maximumMagnitude;
  float dbFsLevel;
  float maximumDbFsLevel;

  // Save for later use.
  this->threshold = threshold;

  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
  // Construct the dBFs table.  The largest expected signal
  // magnitude, under normal conditions, is 128 for a 2's
  // complement 8-bit quantit.  A larger range of values is
  // stored to handle the case of system saturation.  Values
  // can reach a maximum of sqrt(128^2 + 128^2) = 181.02.
  // Staying on the safe side, a maximum value of 256 will
  // be handled.
  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
  maximumMagnitude = 256;
  maximumDbFsLevel = 20 * log10(128);

  for (i = 1; i <= maximumMagnitude; i++)
  {
    dbFsLevel = (20 * log10((float)i)) - maximumDbFsLevel;
    dbFsTable[i] = (int32_t)dbFsLevel;
  } // for 

  // Avoid minus infinity.
  dbFsTable[0] = dbFsTable[1]; 
  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/

} // SignalDetector

/*****************************************************************************

  Name: ~SignalDetector

  Purpose: The purpose of this function is to serve as the destructor for
  an instance of a SignalDetector.

  Calling Sequence: ~SignalDetector()

  Inputs:

    None.

 Outputs:

    None.

*****************************************************************************/
SignalDetector::~SignalDetector(void)
{

} // ~SignalDetector

/*****************************************************************************

  Name: setThreshold

  Purpose: The purpose of this function is to set the detection threshold
  of the signal detector.

 
  Calling Sequence: setThreshold(threshold)

  Inputs:

    threshold - The threshold, in dBFs, that determines whether
    or not a signal is present.

  Outputs:

    None.

*****************************************************************************/
void SignalDetector::setThreshold(int32_t threshold)
{

  // This is probably not done too often.
  this->threshold = threshold;

  return;

} // setThreshold

/*****************************************************************************

  Name: getThreshold

  Purpose: The purpose of this function is to retrieve detection threshold
  of the signal detector.

 
  Calling Sequence: threshold = getThreshold()

  Inputs:

    None.

  Outputs:

    threshold - The threshold, in dBFs units, that determines whether
    or not a signal is present.

*****************************************************************************/
int32_t SignalDetector::getThreshold(void)
{

  return (threshold);

} // getThreshold

/*****************************************************************************

  Name: getSignalMagnitude

  Purpose: The purpose of this function is to retrieve average magnitude
  of the last IQ data block that was processed.
 
  Calling Sequence: magnitude = getSignalMagnitude()

  Inputs:

    None.

  Outputs:

    magnitude - The average magnitude of the last IQ data block that
    was processed.

*****************************************************************************/
uint32_t SignalDetector::getSignalMagnitude(void)
{

  return (signalMagnitude);

} // getSignalMagnitude

/**************************************************************************

  Name: convertMagnitudeToDbFs

  Purpose: The purpose of this function is to convert a signal magnitude
  to decibels referred to the full scale value.

  Calling Sequence: dbFsValue = convertMagnitudeToDbFs(signalMagnitude)

  Inputs:

    signalMagnitude - The magnitude of the signal.

  Outputs:

    None.

**************************************************************************/
int32_t SignalDetector::convertMagnitudeToDbFs(
    uint32_t signalMagnitude)
{
  int32_t dbFsValue;

  if (signalMagnitude > 256)
  {
    // Clip it.
    signalMagnitude = 256;
  } // if

  // Convert to dBFs.
  dbFsValue = dbFsTable[signalMagnitude];

  return (dbFsValue);

} // convertMagnitudeToDbFs

/*****************************************************************************

  Name: detectSignal

  Purpose: The purpose of this function is to determine if a signal is
  present.  The format of the signal, within the input buffer, appears
  below.

  I0,Q0,I1,Q1,...

  In is the in-phase component of the signal, and Qn is the quadrature
  component of the signal.  Each component is an 8-bit 2's complement
  value.

  Here's how the system works:

  1. An average value of the magnitudes in the data block is computed.
  2. The average value is compared to a detection threshold.
  3. The average value is converted to decibels, referenced to the full
  scale value of a 7-bit number.
  4. The adjustable receiver gain is subtracted from the average value
  (in dBFs units) and compared to the detection threshold (also in dBFs
  units).
  3. If the average value (in dBFs units) exceeds the detection threshold,
  a signal detection is indicated, otherwise, the absense of a signal is
  indicated.

  The value of bufferLength is important.  Too small of a value will
  result in a higher detection (false alarm) rate, and too large of
  a value will reject detections that should have been detected.

  Additionally, the detection threshold can be changed at run time.  This
  might be usable in a dynamic (changing over time) signal environment.

  Calling Sequence: result = detectSignal(bufferPtr,bufferLength)

  Inputs:

    bufferPtr - A pointer to IQ data.  The representation is described
    above.

    bufferLength - The number of samples contained in the buffer pointed
    to by bufferPtr.

  Outputs:

    result - A flag that indicates whether or not a signal is present.
    A value of true indicates that a signal is present, and a value of
    false indicates that a signal is absent.

*****************************************************************************/
bool SignalDetector::detectSignal(int8_t *bufferPtr,uint32_t bufferLength)
{
  bool signalIsPresent;
  uint32_t i;
  int32_t signalInDbFs;
  uint32_t magnitude;
  uint8_t *magnitudePtr;
  uint8_t iMagnitude, qMagnitude;
  uint32_t magnitudeBufferLength;

  // Indicate that signal is not present.
  signalIsPresent = false;

  // Reference the beginning of the buffer.
  magnitudePtr = magnitudeBuffer;

  // We will have half the number of samples.
  magnitudeBufferLength = bufferLength / 2;

  // Convert signal to magnitude format.
  for (i = 0; i < bufferLength; i+= 2)
  {
    iMagnitude = abs(*bufferPtr++);
    qMagnitude = abs(*bufferPtr++);

    if (iMagnitude > qMagnitude)
    {
      *magnitudePtr++ = iMagnitude + (qMagnitude >> 1);
    } // if
    else
    {
      *magnitudePtr++ = qMagnitude + (iMagnitude >> 1);
    } // else
  } // for

  // Reference the beginning of the buffer.
  magnitudePtr = magnitudeBuffer;

  // Zero accumulator.
  magnitude = 0;

  // Store magnitude values.
  for (i = 0; i < magnitudeBufferLength; i++)
  {
    magnitude += *magnitudePtr++;
  } // for

  // Finalize the average.
  magnitude /= magnitudeBufferLength;

  // Convert to decibels referenced to full scale.
  signalInDbFs = convertMagnitudeToDbFs(magnitude);

  // Subtract out gain.
  signalInDbFs -= radio_adjustableReceiveGainInDb;

  if (signalInDbFs >= threshold)
  {
    signalIsPresent = true;
  } // if

  // Update the attribute for later retrieval.
  signalMagnitude = magnitude;

  return (signalIsPresent);

} // detectSignal

