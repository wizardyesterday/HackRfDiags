#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include "Squelch.h"

/*****************************************************************************

  Name: Squelch

  Purpose: The purpose of this function is to serve as the contructor for
  an instance of a Squelch.

  Calling Sequence: Squelch(threshold)

  Inputs:

    threshold - The detection threshold that is used to determine signal
    presence or absense.

 Outputs:

    None.

*****************************************************************************/
Squelch::Squelch(int32_t threshold)
{

  // Keep this around for queries.
  this->threshold = threshold;

  // Sddovisyr the signal tracker with this object.
  trackerPtr = new SignalTracker;

  // Associate the signal detector with this object.
  detectorPtr = new SignalDetector(threshold);

} // Squelch

/*****************************************************************************

  Name: ~Squelch

  Purpose: The purpose of this function is to serve as the destructor for
  an instance of a Squelch.

  Calling Sequence: ~Squelch()

  Inputs:

    detectorPtr - A pointer to an instance of a signal detector.

 Outputs:

    None.

*****************************************************************************/
Squelch::~Squelch(void)
{

  // Release resources.
  if (trackerPtr != NULL)
  {
    delete trackerPtr;
  } // if

  if (detectorPtr != NULL)
  {
    delete detectorPtr;
  } // if

} // ~Squelch

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
void Squelch::reset(void)
{

  // Reset the signal tracker to its idle state.
  trackerPtr->reset();

  return;

} // reset

/*****************************************************************************

  Name: setThreshold

  Purpose: The purpose of this function is to set the detection threshold
  of the squelch.

 
  Calling Sequence: setThreshold(threshold)

  Inputs:

    threshold - The threshold, in dBFs, that determines whether
    or not a signal is present.

  Outputs:

    None.

*****************************************************************************/
void Squelch::setThreshold(int32_t threshold)
{

  // This is probably not done too often.
  this->threshold = threshold;

  // Inform the detector of the new threshold.
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
int32_t Squelch::getThreshold(void)
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
uint32_t Squelch::getSignalMagnitude(void)
{
  uint32_t magnitude;

  // Retrieve the average magnitude of the last IQ data block processed.
  magnitude = detectorPtr->getSignalMagnitude();

  return (magnitude);

} // getSignalMagnitude

/*****************************************************************************

  Name: run

  Purpose: The purpose of this function is to provide squelch functionality
  to a system.  The format of the incoming data is described as:

  I0,Q0,I1,Q1,...

  In is the in-phase component of the signal, and Qn is the quadrature
  component of the signal.  Each component is an 8-bit 2's complement
  value.

  Calling Sequence: signalAllowed = run(gainInDb,bufferPtr,bufferLength)

  Inputs:

    gainInDb - The current setting of the system gain.  This is used so
    that the detection threshold can be compared to the signal level
    referenced before the gain element in the system.

    bufferPtr - A pointer to IQ data.  The representation is described
    above.

    bufferLength - The number of samples contained in the buffer pointed
    to by bufferPtr.

  Outputs:

    signalAllowed - A flag that indicates whether or not the signal
    exceeded the squelch threshold.  A value of true indicates that the
    signal exceeded the threshold, and a value of fals indicates that the
    signal did not exceed the threshold.

*****************************************************************************/
bool Squelch::run(uint32_t gainInDb,int8_t *bufferPtr,uint32_t bufferLength)
{
  bool signalAllowed;
  uint16_t signalPresenceIndicator;
  bool signalIsPresent;

  // Determine signal presence.
  signalIsPresent =
    detectorPtr->detectSignal(gainInDb,bufferPtr,bufferLength);

  // Notify the signal tracker of the signal presence state.
  signalPresenceIndicator = trackerPtr->run(signalIsPresent);

  switch (signalPresenceIndicator)
  {
    case SIGNALTRACKER_NOISE:
    {
      // We have no signal.
      signalAllowed = false;
      break;
    } // case

    case SIGNALTRACKER_STARTOFSIGNAL:
    case SIGNALTRACKER_SIGNALPRESENT:
    {
      // We have a signal.
      signalAllowed = true;
      break;
    } // case

    case SIGNALTRACKER_ENDOFSIGNAL:
    {
      // We like a squelch tail.
      signalAllowed = true;
     break;
    } // case

    default:
    {
      signalAllowed = false;
      break;
    } // case
  } // switch

  return (signalAllowed);

} // run
