#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include "SignalTracker.h"

/*****************************************************************************

  Name: SignalTracker

  Purpose: The purpose of this function is to serve as the contructor for
  an instance of a SignalTracker.

  Calling Sequence: SignalTracker()

  Inputs:

    None.

 Outputs:

    None.

*****************************************************************************/
SignalTracker::SignalTracker(void)
{

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

  Name: run

  Purpose: The purpose of this function is to track the envelope of a
  signal.

  Calling Sequence: signalPresenceIndicator = run(signalIsPresent)

  Inputs:

    signalIsPresent - An indicator as to whether or not a signal is
    present.  A value of true indicates that a signal is present, and a
    value of false indicates that a signal is not present.

  Outputs:

    signalPresenceIndicator - The event associated with a signal.

*****************************************************************************/
uint16_t SignalTracker::run(bool signalIsPresent)
{
  uint16_t signalPresenceIndicator;

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

