#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include "SignalTracker.h"

/*****************************************************************************

  Name: SignalTracker

  Purpose: The purpose of this function is to serve as the contructor for
  an instance of a SignalTracker.

  Calling Sequence: SignalTracker(threshold)

  Inputs:

    threshold - The detection threshold that is used to determine signal
    presence or absense.

 Outputs:

    None.

*****************************************************************************/
SignalTracker::SignalTracker(int32_t threshold)
{

  // Associate the signal tracker with this object.
  detectorPtr = new SignalDetector(threshold);

  // Keep this around for queries.
  this->threshold = threshold;

  // Initially, there is no signal.
  state = NoSignal;

} // SignalTracker

/*****************************************************************************

  Name: ~SignalTracker

  Purpose: The purpose of this function is to serve as the destructor for
  an instance of a SignalTracker.

  Calling Sequence: ~SignalTracker()

  Inputs:

    detectorPtr - A pointer to an instance of a signal detector.

 Outputs:

    None.

*****************************************************************************/
SignalTracker::~SignalTracker(void)
{

  // Release resources.
  if (detectorPtr != NULL)
  {
    delete detectorPtr;
  } // if

} // ~SignalTracker

/*****************************************************************************

  Name: reset

  Purpose: The purpose of this function is to reset the system so that it
  is ready to track the next signal.
 
  Calling Sequence: reset()

  Inputs:

    None.

  Outputs:

    None.

*****************************************************************************/
void SignalTracker::reset(void)
{

  // Get ready for the next signal.
  state = NoSignal;

  return;

} // reset

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
void SignalTracker::setThreshold(int32_t threshold)
{

  // This is probably not done too often.
  this->threshold = threshold;

  // Inform th detector of the new threshold.
  detectorPtr->setThreshold(threshold);

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

    threshold - The threshold, dBFs units, that determines whether
    or not a signal is present.

*****************************************************************************/
int32_t SignalTracker::getThreshold(void)
{

  return (threshold);

} // getThreshold

/*****************************************************************************

  Name: getSignalMagnitude

  Purpose: The purpose of this function is to retrieve average magnitude
  of the last IQ data block that was processed by the SignalDetector.

  Calling Sequence: magnitude = getSignalMagnitude()

  Inputs:

    None.

  Outputs:

    magnitude - The average magnitude of the last IQ data block that
    was processed.

*****************************************************************************/
uint32_t SignalTracker::getSignalMagnitude(void)
{
  uint32_t magnitude;

  // Retrieve the average magnitude of the last IQ data block processed.
  magnitude = detectorPtr->getSignalMagnitude();

  return (magnitude);

} // getSignalMagnitude

/*****************************************************************************

  Name: run

  Purpose: The purpose of this function is to set the detection threshold
  of the signal detector. The format of the signal, within the input buffer,
  appears below.

  I0,Q0,I1,Q1,...

  In is the in-phase component of the signal, and Qn is the quadrature
  component of the signal.  Each component is an 8-bit 2's complement
  value.

  Calling Sequence: signalPresenceIndicator = run(bufferPtr,bufferLength)

  Inputs:

    bufferPtr - A pointer to IQ data.  The representation is described
    above.

    bufferLength - The number of samples contained in the buffer pointed
    to by bufferPtr.

  Outputs:

    signalPresenceIndicator - The event associated with a signal.

*****************************************************************************/
uint16_t SignalTracker::run(int8_t *bufferPtr,uint32_t bufferLength)
{
  uint16_t signalPresenceIndicator;
  bool signalIsPresent;

  // Determine signal presence.
  signalIsPresent = detectorPtr->detectSignal(bufferPtr,bufferLength);

  switch (state)
  {
    case NoSignal:
    {
      if (signalIsPresent)
      {
        state = Tracking;
        signalPresenceIndicator = SIGNALTRACKER_STARTOFSIGNAL;
      } // if
      else
      {
        state = NoSignal;
        signalPresenceIndicator = SIGNALTRACKER_NOISE;
      } // else

      break;
    } // case

    case Tracking:
    {
      if (signalIsPresent)
      {
        state = Tracking;
        signalPresenceIndicator = SIGNALTRACKER_SIGNALPRESENT;
      } // if
      else
      {
        state = NoSignal;
        signalPresenceIndicator = SIGNALTRACKER_ENDOFSIGNAL;
      } // if

      break;
    } // case
  } // switch

  return (signalPresenceIndicator);

} // run

