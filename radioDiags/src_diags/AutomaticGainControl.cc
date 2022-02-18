//**********************************************************************
// file name: AutomaticGainControl.cc
//**********************************************************************

#include <stdio.h>
#include <math.h>
#include <sys/time.h>

#include "AutomaticGainControl.h"

#define FREQUENCY_THRESHOLD_FOR_FRONT_END_AMP (200000000)
#define FREQUENCY_THRESHOLD_FOR_MAX_IF_GAIN (200000000)

extern void nprintf(FILE *s,const char *formatPtr, ...);

static bool callbackEnabled;

/**************************************************************************

  Name: signalMagnitudeCallback

  Purpose: The purpose of this function is to serve as the callback that
  accepts signal state information.

  Calling Sequence: signalMagnitudeCallback(signalMagnitude,contextPtr)

  Inputs:

    signalMagnitude - The average magnitude of the IQ data block that
    was received.

    contextPtr - A pointer to private context data.

  Outputs:

    None.

**************************************************************************/
static void signalMagnitudeCallback(uint32_t signalMagnitude,
                                   void *contextPtr)
{
  AutomaticGainControl *thisPtr;

  if (callbackEnabled)
  {
    // Reference the context pointer properly.
    thisPtr = (AutomaticGainControl *)contextPtr;

    // Process the signal.
    thisPtr->run(signalMagnitude);
  } // if

  return;

} // signalMagnitudeCallback

/************************************************************************

  Name: AutomaticGainControl

  Purpose: The purpose of this function is to serve as the constructor
  of an AutomaticGainControl object.

  Calling Sequence: AutomaticGainControl(radioPtr,operatingPointInDbFs)

  Inputs:

    radioPtr - A pointer to a Radio instance.

    operatingPointInDbFs - The AGC operating point in decibels referenced
    to full scale.  Full scale represents 0dBFs, otherwise, all other
    values will be negative.

  Outputs:

    None.

**************************************************************************/
AutomaticGainControl::AutomaticGainControl(Radio *radioPtr,
                                           int32_t operatingPointInDbFs)
{
  uint32_t i;
  uint32_t maximumMagnitude;
  float dbFsLevel;
  float maximumDbFsLevel;

  // Reference this for later use.
  this->radioPtr = radioPtr;

  // Reference the set point to the antenna input.
  this->operatingPointInDbFs = operatingPointInDbFs;

  // Set nominal values.
  rfGainInDb = 0;
  ifGainInDb = 40;
  basebandGainInDb = 40;
  filteredBasebandGainInDb = 40;
  signalMagnitude = 0;
 
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

  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
  // Register the callback to the IqDataProcessor.
  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
  // Enable the callback function.
  callbackEnabled = true;

  // Retrieve the object instance.  
  dataProcessorPtr = radioPtr->getIqProcessor();

  // Register the callback.
  dataProcessorPtr->registerSignalMagnitudeCallback(signalMagnitudeCallback,
                                                    this);

  // Allow callback notification.
  dataProcessorPtr->enableSignalMagnitudeNotification();
  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/

  return;
 
} // AutomaticGainControl

/**************************************************************************

  Name: ~AutomaticGainControl

  Purpose: The purpose of this function is to serve as the destructor
  of an AutomaticGainControl object.

  Calling Sequence: ~AutomaticGainControl()

  Inputs:

    None.

  Outputs:

    None.

**************************************************************************/
AutomaticGainControl::~AutomaticGainControl(void)
{

  // Disable callback function processing.
  callbackEnabled = false;

  // Turn off notification.
  dataProcessorPtr->disableSignalMagnitudeNotification();

  // Unregister the callback.
  dataProcessorPtr->registerSignalMagnitudeCallback(NULL,NULL);

  return;

} // ~AutomaticGainControl

/**************************************************************************

  Name: setOperatingPoint

  Purpose: The purpose of this function is to set the operating point
  of the AGC.

  Calling Sequence: setOperatingPoint(operatingPointInDbFs)
  Inputs:

    operatingPointInDbFs - The operating point in decibels referenced to
    the full scale value.

  Outputs:

    None.

**************************************************************************/
void AutomaticGainControl::setOperatingPoint(int32_t operatingPointInDbFs)
{

  // Update operating point.
  this->operatingPointInDbFs = operatingPointInDbFs;

  return;

} // setOperatingPoint

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
int32_t AutomaticGainControl::convertMagnitudeToDbFs(
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

/**************************************************************************

  Name: run

  Purpose: The purpose of this function is to run the automatic gain
  control.

  Note: There are some tweaks that must be done to this system.  For
  example, experiments must be performed to determine the amount of
  gain that is needed in the three amplifiers in order to hear
  anything.  In my HackRF (using a simple indoor whip antenna), I
  need at least 32dB gain in the IF amp. When set to lower gains, the
  receiver is somewhat deaf.  I usually set the baseband amplifier
  gain to about 44dB.  The frontend RF amp needs to be disabled in the
  VHF frequencies, but on UHF (for example police on the 460MHz
  frequencies), the amplifier needs to be enabled.  Once the receiver is
  characterized, this AGC system (in theory) should work just fine.

  Calling Sequence: run(signalMagnitude )

  Inputs:

    signalMagnitude - The average magnitude of the IQ data block that
    was received.

  Outputs:

    None.

**************************************************************************/
void AutomaticGainControl::run(uint32_t signalMagnitude)
{
  bool success;
  bool frontEndAmpEnabled;
  int32_t deltaGainInDb;
  int32_t adjustedBasebandGainInDb;
  int32_t signalInDbFs;
  uint64_t frequencyInHertz;

  // Update for display purposes.
  this->signalMagnitude = signalMagnitude;

  // Convert to decibels referenced to full scale.
  signalInDbFs = convertMagnitudeToDbFs(signalMagnitude);

  // Get the receiver frequency for further evaluation.
  frequencyInHertz = radioPtr->getReceiveFrequency();

  if (frequencyInHertz >= FREQUENCY_THRESHOLD_FOR_FRONT_END_AMP)
  {
    // Enable the frontend amp for 14dB of extra gain.
    rfGainInDb = 14;

    // Indicate that the frontend amp is enabled.
    frontEndAmpEnabled = true;
  } // if
  else
  {
    // Disable the frontend amp for 14dB of less gain.
    rfGainInDb = 0;

    // Indicate that the frontend amp is disabled.
    frontEndAmpEnabled = false;
  } // else

  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
  // Allocate gains appropriately.  Here is what we
  // have to work with:
  //
  //   1. The front end RF amp provides either 0
  //    to 14dB of gain in 14dB steps.
  //
  //   3. The IF amp provides gains from 0 to 40dB
  //   in 8dB steps.
  //
  //   3. The baseband amp provides gains from 0 to
  //   62dB of gain in 2dB steps.
  //
  // Let's try this first:
  //
  //   1a. For frequencies below 200MHz disable the front end
  //   RF amp.
  //   1b. For frequencies above 200MHz, enable the front end
  //   RF amp.
  //
  //   2. Adjust the baseband gain as appropriate to achieve
  //   the operating point referenced at the antenna input.
  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
  // Compute the gain adjustment. This might not be correct.
  deltaGainInDb = operatingPointInDbFs - signalInDbFs;

  // Adjust the gain.
  adjustedBasebandGainInDb = basebandGainInDb + deltaGainInDb;

  //+++++++++++++++++++++++++++++++++++++++++++
  // Limit the gain to valid values.
  //+++++++++++++++++++++++++++++++++++++++++++
  if (adjustedBasebandGainInDb > 62)
  {
    adjustedBasebandGainInDb = 62;
  } // if
  else
  {
    if (adjustedBasebandGainInDb < 0)
    {
      adjustedBasebandGainInDb = 0;
    } // if
  } // else

  //*****************************************************************
  // Run the computation through a lowpass filter.
  //*****************************************************************
  filteredBasebandGainInDb = (0.1 * (float)adjustedBasebandGainInDb)
                           + (0.9 * filteredBasebandGainInDb);
  //*****************************************************************

  // Update the attribute.
  basebandGainInDb = (uint32_t)filteredBasebandGainInDb;

  //+++++++++++++++++++++++++++++++++++++++++++

  //+++++++++++++++++++++++++++++++++++++++++++++++++++
  // Update the receiver gain parameters.
  //+++++++++++++++++++++++++++++++++++++++++++++++++++
  if (frontEndAmpEnabled)
  {
    // Enable the front end RF amp.
    success = radioPtr->enableReceiveFrontEndAmplifier();
  } // if
  else
  {
    // Disable the front end RF amp.
    success = radioPtr->disableReceiveFrontEndAmplifier();
  } // else

  success = radioPtr->setReceiveIfGainInDb(ifGainInDb);
  success = radioPtr->setReceiveBasebandGainInDb(basebandGainInDb);
  //+++++++++++++++++++++++++++++++++++++++++++++++++++
  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/

  return;

} // run

/**************************************************************************

  Name: displayInternalInformation

  Purpose: The purpose of this function is to display information of the
  frequency sweeper.

  Calling Sequence: displayInternalInformation()

  Inputs:

    None.

  Outputs:

    None.

**************************************************************************/
void AutomaticGainControl::displayInternalInformation(void)
{

  nprintf(stderr,"\n--------------------------------------------\n");
  nprintf(stderr,"AGC Internal Information\n");
  nprintf(stderr,"--------------------------------------------\n");

  nprintf(stderr,"Signal Magnitude          : %u\n",
          signalMagnitude);

  nprintf(stderr,"Operating Point           : %d dBFs\n",
          operatingPointInDbFs);

  nprintf(stderr,"RF Gain                   : %u dB\n",
          rfGainInDb);

  nprintf(stderr,"IF Gain                   : %u dB\n",
          ifGainInDb);

  nprintf(stderr,"Baseband Gain             : %u dB\n",
          basebandGainInDb);

  return;

} // displayInternalInformation

