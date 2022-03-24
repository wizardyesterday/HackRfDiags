//**********************************************************************
// file name: AutomaticGainControl.cc
//**********************************************************************

#include <stdio.h>
#include <math.h>
#include <sys/time.h>

#include "Radio.h"
#include "IqDataProcessor.h"

#include "AutomaticGainControl.h"

#define FREQUENCY_THRESHOLD_FOR_FRONT_END_AMP (200000000)
#define FREQUENCY_THRESHOLD_FOR_MAX_IF_GAIN (200000000)

extern void nprintf(FILE *s,const char *formatPtr, ...);

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

  // Reference the context pointer properly.
  thisPtr = (AutomaticGainControl *)contextPtr;

  if (thisPtr->isEnabled())
  {
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
AutomaticGainControl::AutomaticGainControl(void *radioPtr,
                                           int32_t operatingPointInDbFs)
{
  uint32_t i;
  uint32_t maximumMagnitude;
  float dbFsLevel;
  float maximumDbFsLevel;
  Radio * RadioPtr;
  IqDataProcessor *DataProcessorPtr;

  // Reference this for later use.
  this->radioPtr = radioPtr;

  // Reference the set point to the antenna input.
  this->operatingPointInDbFs = operatingPointInDbFs;

  // Reference the pointer in the proper context.
  RadioPtr = (Radio *)radioPtr;

  // Default to snappier system.
  agcType = AGC_TYPE_HARRIS;

  // Start with a reasonable deadband.
  deadbandInDb = 1;

  // Set this to the midrange.
  signalMagnitude = 64;

  // Default to disabled.
  enabled = false;
 
  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
  // Sometimes, adjustments need to be avoided when a transient in the
  // hardware occurs as a result of a gain adjustment.  In the rtlsdr,
  // it was initially thought that the transient was occurring in the
  // tuner chip.  This was not the case.  Instead, a transient in the
  // demodulated data was occurring as a result of the IIC repeater
  // being enabled (and/or disabled) in the Realtek 2832U chip.  The
  // simplest thing to do in software is to perform a transient
  // avoidance strategy.  While it is true that the performance of the
  // AGC becomes less than optimal, it is still better than experiencing
  // limit cycles.  The blankingLimit is configurable so that the
  // user can change the value to suit the needs of the application.
  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
  blankingCounter = 1;
  blankingLimit = 0;
  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/

  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
 // This is a good starting point for the receiver gain
  // values.
  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
  rfGainInDb = 0;
  ifGainInDb = 40;
  basebandGainInDb = 40;

  normalizedSignalLevelInDbFs =  -basebandGainInDb;
  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/

  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
  // AGC filter initialization.  The filter is implemented
  // as a first-order difference equation.
  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
  // Initial condition of filter memory.
  filteredBasebandGainInDb = 40;

  //+++++++++++++++++++++++++++++++++++++++++++++++++
  // Set the AGC time constant such that gain
  // adjustment occurs rapidly while maintaining
  // system stability.
  //+++++++++++++++++++++++++++++++++++++++++++++++++
  alpha = 0.8;
  //+++++++++++++++++++++++++++++++++++++++++++++++++
  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/

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
  // Retrieve the object instance.  
  dataProcessorPtr = RadioPtr->getIqProcessor();

  // Reference the pointer in the proper context.
  DataProcessorPtr = (IqDataProcessor *)dataProcessorPtr;

  // Turn off notification.
  DataProcessorPtr->disableSignalMagnitudeNotification();

  // Register the callback.
  DataProcessorPtr->registerSignalMagnitudeCallback(signalMagnitudeCallback,
                                                    this);
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
  IqDataProcessor *DataProcessorPtr;

  // Reference the pointer in the proper context.
  DataProcessorPtr = (IqDataProcessor *)dataProcessorPtr;

  // Disable the AGC.
  enabled = false;

  // Turn off notification.
  DataProcessorPtr->disableSignalMagnitudeNotification();

  // Unregister the callback.
  DataProcessorPtr->registerSignalMagnitudeCallback(NULL,NULL);

  return;

} // ~AutomaticGainControl

/**************************************************************************

  Name: setType

  Purpose: The purpose of this function is to set the operating point
  of the AGC.

  Calling Sequence: success = setType(type)

  Inputs:

    type - The type of AGC.  A value of 0 indicates that the lowpass AGC
    is to be used, and a value of 1 indicates that the Harris AGC is to
    be used.

  Outputs:

    success - A flag that indicates whether the the AGC type was set.
    A value of true indicates that the AGC type was successfully set,
    and a value of false indicates that the AGC type was not set due
    to an invalid value for the AGC type was specified.

**************************************************************************/
bool AutomaticGainControl::setType(uint32_t type)
{
  bool success;

  // Default to success.
  success = true;

  switch (type)
  {
    case AGC_TYPE_LOWPASS:
    {
      // Update the attribute.
      agcType = type;
      break;
    } // case

    case AGC_TYPE_HARRIS:
    {
      // Update the attribute.
      agcType = type;
      break;
    } // case

    default:
    {
      // Indicate failure.
      success = false;
      break;
    } // case
  } // switch

  return (success);

} // setType

/**************************************************************************

  Name: setDeadband

  Purpose: The purpose of this function is to set the deadband of the
  AGC.  This presents gain setting oscillations

  Calling Sequence: success = setDeadband(deadbandInDb)

  Inputs:

    deadbandInDb - The deadband, in decibels, used to prevent unwanted
    gain setting oscillations.  A window is created such that a gain
    adjustment will not occur if the current gain is within the window
    new gain <= current gain += deadband.

  Outputs:

    success - A flag that indicates whether or not the deadband parameter
    was updated.  A value of true indicates that the deadband was
    updated, and a value of false indicates that the parameter was not
    updated due to an invalid specified deadband value.

**************************************************************************/
bool  AutomaticGainControl::setDeadband(uint32_t deadbandInDb)
{
  bool success;

  // Default to failure.
  success = false;

  if ((deadbandInDb >= 0) && (deadbandInDb <= 10))
  {
    // Update the attribute.
    this->deadbandInDb = deadbandInDb;

    // Indicate success.
    success = true;
  } // if

  return (success);

} // setDeadband

/**************************************************************************

  Name: setBlankingLimit

  Purpose: The purpose of this function is to set the blanking limit of
  the AGC.  This presents gain setting oscillations.  What happens if
  transients exist after an adjustment is made is that the AGC becomes
  blind to the actual signal that is being received since the transient
  can swamp the system.

  Calling Sequence: success = setBlankingLimit(blankingLimit)

  Inputs:

    blankingLimit - The number of measurements to ignore before making
    the next gain adjustment.

  Outputs:

    success - A flag that indicates whether or not the blanking limit
    parameter was updated.  A value of true indicates that the blanking
    limit was updated, and a value of false indicates that the parameter
    was not updated due to an invalid specified blanking limit value.

**************************************************************************/
bool  AutomaticGainControl::setBlankingLimit(uint32_t blankingLimit)
{
  bool success;

  // Default to failure.
  success = false;

  if ((blankingLimit >= 0) && (blankingLimit <= 10))
  {
    // Update the attributes.
    this->blankingLimit = blankingLimit;

    // Ensure that the next measurement can be made.    
    blankingCounter = blankingLimit + 1;

    // Indicate success.
    success = true;
  } // if

  return (success);

} // setBlankingLimit

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

  Name: setAgcFilterCoefficient

  Purpose: The purpose of this function is to set the coefficient of
  the first order lowpass filter that filters the baseband gain value.
  In effect, the time constant of the filter is set.

  Calling Sequence: success = setAgcFilterCoefficient(coefficient)

  Inputs:

    coefficient - The filter coefficient for the lowpass filter that
    filters the baseband gain value.

  Outputs:

    success - A flag that indicates whether the filter coefficient was
    updated.  A value of true indicates that the coefficient was
    updated, and a value of false indicates that the coefficient was
    not updated due to an invalid coefficient value.

**************************************************************************/
bool  AutomaticGainControl::setAgcFilterCoefficient(float coefficient)
{
  bool success;

  // Default to failure.
  success = false;

  if ((coefficient >= 0.001) && (coefficient < 0.999))
  {
    // Update the attribute.
    alpha = coefficient;

    // Indicate success.
    success = true;
  } // if

  return (success);

} // setAgcFilterCoefficient

/**************************************************************************

  Name: enable

  Purpose: The purpose of this function is to enable the AGC.

  Calling Sequence: success = enable();

  Inputs:

    None.

  Outputs:

    success - A flag that indicates whether or not the operation was
    successful.  A value of true indicates that the operation was
    successful, and a value of false indicates that the AGC was already
    enabled.

**************************************************************************/
bool AutomaticGainControl::enable(void)
{
  bool success;
  IqDataProcessor *DataProcessorPtr;
  Radio *RadioPtr;

  // Reference the pointers in the proper context.
  DataProcessorPtr = (IqDataProcessor *)dataProcessorPtr;
  RadioPtr = (Radio *)radioPtr;

  // Default to failure.
  success = false;

  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
  // Ensure that the radio is already in the receiving state so
  // that AGC transients do not occur when the receiver is
  // first enabled.
  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
  if (RadioPtr->isReceiving())
  {
    if (!enabled)
    {
      // Enable the AGC.
      enabled = true;

      // Ensure that the next measurement can be made.    
      blankingCounter = blankingLimit + 1;

      // Allow callback notification.
      DataProcessorPtr->enableSignalMagnitudeNotification();

      // Indicate success.
      success = true;
    } // if
  } // if

  return (success);

} // enable

/**************************************************************************

  Name: disable

  Purpose: The purpose of this function is to disable the AGC.

  Calling Sequence: success = disable();

  Inputs:

    None.

  Outputs:

    success - A flag that indicates whether or not the operation was
    successful.  A value of true indicates that the operation was
    successful, and a value of false indicates that the AGC was already
    disabled.

**************************************************************************/
bool AutomaticGainControl::disable(void)
{
  bool success;
  IqDataProcessor *DataProcessorPtr;

  // Reference the pointer in the proper context.
  DataProcessorPtr = (IqDataProcessor *)dataProcessorPtr;

  // Default to failure.
  success = false;

  if (enabled)
  {
    // Disable the AGC.
    enabled = false;

    // Turn off notification.
    DataProcessorPtr->disableSignalMagnitudeNotification();

    // Indicate success.
    success = true;
  } // if

  return (success);

} // disable

/**************************************************************************

  Name: isEnabled

  Purpose: The purpose of this function is to determine whether or not
  the AGC is enabled.

  Calling Sequence: status = isEnabled()

  Inputs:

    None.

  Outputs:

    success - A flag that indicates whether or not the AGC is enabled.
    A value of true indicates that the AGC is enabled, and a value of
    false indicates that the AGC is disabled.

**************************************************************************/
bool AutomaticGainControl::isEnabled(void)
{

  return (enabled);

} // isEnabled

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

  Purpose: The purpose of this function is to run the selected automatic
  gain control algorithm.
 
  Calling Sequence: run(signalMagnitude )

  Inputs:

    signalMagnitude - The average magnitude of the IQ data block that
    was received in units of dBFs.

  Outputs:

    None.

**************************************************************************/
void AutomaticGainControl::run(uint32_t signalMagnitude)
{

  if (blankingCounter < blankingLimit)
  {
    // We're not ready to make an adjustment.
    blankingCounter++;
  } // if
  else
  {
    // Reset the blanking counter.
    blankingCounter = 1;

    switch (agcType)
    {
      case AGC_TYPE_LOWPASS:
      {
        runLowpass(signalMagnitude);
        break;
      } // case

      case AGC_TYPE_HARRIS:
      {
        runHarris(signalMagnitude);
        break;
      } // case

      default:
      {
        runLowpass(signalMagnitude);
        break;
      } // case
    } // switch
  } // else

  return;

} // run

/**************************************************************************

  Name: runLowpass

  Purpose: The purpose of this function is to run the automatic gain
  control.

  Note:  A large gain error will result in a more rapid convergence
  to the operating point versus a small gain error.
 
  Calling Sequence: runLowpass(signalMagnitude )

  Inputs:

    signalMagnitude - The average magnitude of the IQ data block that
    was received in units of dBFs.

  Outputs:

    None.

**************************************************************************/
void AutomaticGainControl::runLowpass(uint32_t signalMagnitude)
{
  bool success;
  bool frontEndAmpEnabled;
  int32_t gainError;
  int32_t adjustedGain;
  int32_t signalInDbFs;
  uint64_t frequencyInHertz;
  Radio * RadioPtr;

  // Reference the pointer in the proper context.
  RadioPtr = (Radio *)radioPtr;

  // Update for display purposes.
  this->signalMagnitude = signalMagnitude;

  // Convert to decibels referenced to full scale.
  signalInDbFs = convertMagnitudeToDbFs(signalMagnitude);

  // Update for display purposes.
  normalizedSignalLevelInDbFs = signalInDbFs - basebandGainInDb;

  // Get the receiver frequency for further evaluation.
  frequencyInHertz = RadioPtr->getReceiveFrequency();

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

  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
  // Compute the gain adjustment.
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
  //   2. Max out the IF gain to 40dB.
  //
  //   3. Adjust the baseband gain as appropriate to achieve
  //   the operating point referenced at the antenna input.
  //
  // This provides a dynamic range of 62dB (since the baseband
  // amplifier has an adjustable gain from 0 to 62dB), of the
  // samples that drive the A/D convertor.  If a dynamic range of
  // 62dB is inadequate, a more sophisticated algorithm can be
  // devised that utilitizes the adjustment range of the IF amplifier.
  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
  // Compute the gain adjustment.
  gainError = operatingPointInDbFs - signalInDbFs;

  // Apply deadband to eliminate gain oscillations.
  if (abs(gainError) <= deadbandInDb)
  {
    gainError = 0;
  } // if

  adjustedGain = basebandGainInDb + gainError;

  //*******************************************************************
  // Filter the baseband gain.
  //*******************************************************************
  filteredBasebandGainInDb = (alpha * (float)adjustedGain) +
    ((1 - alpha) * filteredBasebandGainInDb);

  //+++++++++++++++++++++++++++++++++++++++++++
  // Limit the gain to valid values.
  //+++++++++++++++++++++++++++++++++++++++++++
  if (filteredBasebandGainInDb > 62)
  {
    filteredBasebandGainInDb = 62;
  } // if
  else
  {
    if (filteredBasebandGainInDb < 0)
    {
      filteredBasebandGainInDb = 0;
    } // if
  } // else
  //*******************************************************************

  // Update the attribute.
  basebandGainInDb = (uint32_t)filteredBasebandGainInDb;

  //+++++++++++++++++++++++++++++++++++++++++++

  //+++++++++++++++++++++++++++++++++++++++++++++++++++
  // Update the receiver gain parameters.
  //+++++++++++++++++++++++++++++++++++++++++++++++++++
  if (frontEndAmpEnabled)
  {
    // Enable the front end RF amp.
    success = RadioPtr->enableReceiveFrontEndAmplifier();
  } // if
  else
  {
    // Disable the front end RF amp.
    success = RadioPtr->disableReceiveFrontEndAmplifier();
  } // else

  success = RadioPtr->setReceiveIfGainInDb(ifGainInDb);

  // There is no need to update the gain if no change has occurred.
  // This way, we're nicer to the hardware.
  if (gainError != 0)
  {
    success = RadioPtr->setReceiveBasebandGainInDb(basebandGainInDb);
  } // if
  //+++++++++++++++++++++++++++++++++++++++++++++++++++
  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/

  return;

} // runLowpass

/**************************************************************************

  Name: runHarris

  Purpose: The purpose of this function is to run the automatic gain
  control.  This is the implementation of the algorithm described by
  Fredric Harris et. al. in the paper entitled, "On the Design,
  Implementation, and Performance of a Microprocessor-Controlled AGC
  System for a Digital Receiver".  This AGC is also described in
  "Understanding Digital Signal Processing, Third Edition", Section 13.30,
  "Automatic Gain Control (AGC)" by Richard G. Lyons (same AGC as described
  in the previously mentioned paper).

  Note:  A large gain error will result in a more rapid convergence
  to the operating point versus a small gain error.  Let the subsystem
  parameters be defined as follows:

    R - The reference point in dBFs.

    alpha - The system time constant.  This parameter dictates how
    quickly the AGC will converge to the reference point, R, as a
    function of the received signal magnitude (in dBFs). 

    x(n) - The received signal magnitude, in dBFs, referenced at the
    point before adjustable amplification.

    y(n) - The received signal that has been amplified by the adjustable
    gain amplifier in units of dBFs.

    e(n) - The deviation from the reference point in decibels.

    g(n) - The gain of the adjustable gain amplifier in decibels.

  The equations for the AGC algorithm are as follows.

    y(n) = x(n) + g(n).

    e(n) = R - y(n).

    g(n+1) = g(n) + [alpha * e(n)]

 
  Calling Sequence: runHarris(signalMagnitude )

  Inputs:

    signalMagnitude - The average magnitude of the IQ data block that
    was received in units of dBFs.

  Outputs:

    None.

**************************************************************************/
void AutomaticGainControl::runHarris(uint32_t signalMagnitude)
{
  bool success;
  bool frontEndAmpEnabled;
  float gainError;
  int32_t deltaGain;
  int32_t signalInDbFs;
  uint64_t frequencyInHertz;
  Radio * RadioPtr;

  // Reference the pointer in the proper context.
  RadioPtr = (Radio *)radioPtr;

  // Update for display purposes.
  this->signalMagnitude = signalMagnitude;

  // Convert to decibels referenced to full scale.
  signalInDbFs = convertMagnitudeToDbFs(signalMagnitude);

  // Update for display purposes.
  normalizedSignalLevelInDbFs = signalInDbFs - basebandGainInDb;

  // Get the receiver frequency for further evaluation.
  frequencyInHertz = RadioPtr->getReceiveFrequency();

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

  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
  // Compute the gain adjustment.
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
  //   2. Max out the IF gain to 40dB.
  //
  //   3. Adjust the baseband gain as appropriate to achieve
  //   the operating point referenced at the antenna input.
  //
  // This provides a dynamic range of 62dB (since the baseband
  // amplifier has an adjustable gain from 0 to 62dB), of the
  // samples that drive the A/D convertor.  If a dynamic range of
  // 62dB is inadequate, a more sophisticated algorithm can be
  // devised that utilitizes the adjustment range of the IF amplifier.
  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
  // Compute the gain adjustment.
  deltaGain = operatingPointInDbFs - signalInDbFs;
  gainError = (float)deltaGain;

  // Apply deadband to eliminate gain oscillations.
  if (abs(deltaGain) <= deadbandInDb)
  {
    gainError = 0;
    deltaGain = 0;
  } // if

  //*******************************************************************
  // Run the AGC algorithm.
  //*******************************************************************
  filteredBasebandGainInDb = filteredBasebandGainInDb +
    (alpha * gainError);

  //+++++++++++++++++++++++++++++++++++++++++++
  // Limit the gain to valid values.
  //+++++++++++++++++++++++++++++++++++++++++++
  if (filteredBasebandGainInDb > 62)
  {
    filteredBasebandGainInDb = 62;
  } // if
  else
  {
    if (filteredBasebandGainInDb < 0)
    {
      filteredBasebandGainInDb = 0;
    } // if
  } // else
  //*******************************************************************

  // Update the attribute.
  basebandGainInDb = (uint32_t)filteredBasebandGainInDb;

  //+++++++++++++++++++++++++++++++++++++++++++

  //+++++++++++++++++++++++++++++++++++++++++++++++++++
  // Update the receiver gain parameters.
  //+++++++++++++++++++++++++++++++++++++++++++++++++++
  if (frontEndAmpEnabled)
  {
    // Enable the front end RF amp.
    success = RadioPtr->enableReceiveFrontEndAmplifier();
  } // if
  else
  {
    // Disable the front end RF amp.
    success = RadioPtr->disableReceiveFrontEndAmplifier();
  } // else

  success = RadioPtr->setReceiveIfGainInDb(ifGainInDb);

  // There is no need to update the gain if no change has occurred.
  // This way, we're nicer to the hardware.
  if (deltaGain != 0)
  {
    success = RadioPtr->setReceiveBasebandGainInDb(basebandGainInDb);
  } // if
  //+++++++++++++++++++++++++++++++++++++++++++++++++++
  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/

  return;

} // runHarris

/**************************************************************************

  Name: displayInternalInformation

  Purpose: The purpose of this function is to display information in the
  AGC.

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

  if (enabled)
  {
    nprintf(stderr,"AGC Emabled               : Yes\n");
  } // if
  else
  {
    nprintf(stderr,"AGC Emabled               : No\n");
  } // else

  switch (agcType)
  {
    case AGC_TYPE_LOWPASS:
    {
      nprintf(stderr,"AGC Type                  : Lowpass\n");
      break;
    } // case

    case AGC_TYPE_HARRIS:
    {
      nprintf(stderr,"AGC Type                  : Harris\n");
      break;
    } // case

    default:
    {
      nprintf(stderr,"AGC Type                  : Lowpass\n");
      break;
    } // case
  } // switch

  nprintf(stderr,"Blanking Counter:           %u ticks\n",
          blankingCounter);

  nprintf(stderr,"Blanking Limit:             %u ticks\n",
          blankingLimit);

  nprintf(stderr,"Lowpass Filter Coefficient: %0.3f\n",
          alpha);

  nprintf(stderr,"Deadband                  : %u dB\n",
          deadbandInDb);

  nprintf(stderr,"Operating Point           : %d dBFs\n",
          operatingPointInDbFs);

 nprintf(stderr,"IF Gain                   : %u dB\n",
          ifGainInDb);

  nprintf(stderr,"/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/\n");
  nprintf(stderr,"Signal Magnitude          : %u\n",
          signalMagnitude);

  nprintf(stderr,"RSSI (After Mixer)        : %d dBFs\n",
          normalizedSignalLevelInDbFs);
  nprintf(stderr,"/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/\n");

  return;

} // displayInternalInformation

