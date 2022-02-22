//**********************************************************************
// file name: FrequencyScanner.cc
//**********************************************************************

#include <stdio.h>
#include <sys/time.h>

#include "FrequencyScanner.h"

extern void nprintf(FILE *s,const char *formatPtr, ...);

/**************************************************************************

  Name: signalStateCallback

  Purpose: The purpose of this function is to serve as the callback that
  accepts signal state information.

  Calling Sequence: signalStateCallback(signalPresent,contextPtr)

  Inputs:

    signalPresent - A flag that indicates whether or not a signal is
    present.  A value of true indicates that a signal is present, and
    a value of false indicates that a signal is not present.

    contextPtr - A pointer to private context data.

  Outputs:

    None.

**************************************************************************/
static void signalStateCallback(bool signalPresent,void *contextPtr)
{
  FrequencyScanner *thisPtr;

  // Reference the context pointer properly.
  thisPtr = (FrequencyScanner *)contextPtr;

  if (thisPtr->isScanning())
  {
    // Peform the processing.
    thisPtr->run(signalPresent);
  } // if

  return;

} // signalStateCallback

/**************************************************************************

  Name: FrequencyScanner

  Purpose: The purpose of this function is to serve as the constructor
  of a FrequencyScanner object.

  Calling Sequence: FrequencyScanner(radioPtr)

  Inputs:

    radioPtr - A pointer to a Radio instance.

  Outputs:

    None.

**************************************************************************/
FrequencyScanner::FrequencyScanner(Radio *radioPtr)
{
  bool success;

  // Save for later use.
  this->radioPtr = radioPtr;

  // Set the parameters to harmless values.
  startFrequencyInHertz = 162550000;
  endFrequencyInHertz = 162550000;
  frequencyIncrementInHertz = 0;

  // Start at the beginning.
  currentFrequencyInHertz = startFrequencyInHertz;

  // Indicate that a new configuration isn't available.
  newConfigurationAvailable = false;

  // Indicate that the system is idle.
  scannerState = Idle;

  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
  // Register the callback to the IqDataProcessor.
  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
  // Retrieve the object instance.
  dataProcessorPtr = radioPtr->getIqProcessor();

  // Turn off notification.
  dataProcessorPtr->disableSignalNotification();

  // Register the callback.
  dataProcessorPtr->registerSignalStateCallback(signalStateCallback,this);
  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/

  return;
 
} // FrequencyScanner

/**************************************************************************

  Name: ~FrequencyScanner

  Purpose: The purpose of this function is to serve as the destructor
  of a FrequencyScanner object.

  Calling Sequence: ~FrequencyScanner()

  Inputs:

    None.

  Outputs:

    None.

**************************************************************************/
FrequencyScanner::~FrequencyScanner(void)
{

  // Disable callback function processing.
  scannerState = Idle;

  // Turn off nofication.
  dataProcessorPtr->disableSignalNotification();

  // Unregister the callback.
  dataProcessorPtr->registerSignalStateCallback(NULL,NULL);

  return;

} // ~FrequencyScanner

/**************************************************************************

  Name: setScanParameters

  Purpose: The purpose of this function is to set the scan parameters of
  the frequency scanner.
  Note, that the scanner must be in an idle state in order to change the
  parameters.

  Calling Sequence: success = setScanParameters(startFrequencyInHertz,
                                                endFrequencyInHertz,
                                                frequencyIncrementInHertz)

  Inputs:

    startFrequencyInHertz - The start frequency of the scan.

    endFrequencyInHz - The end frequency of the scan.

    frequencyIncrementInHertz - The increment that is used to generate
    the discrete frequencies. 

  Outputs:

    success - A flag that indicates the outcome of the operation.  A
    value of true indicates success, and a value of false indicates
    that the operation was not successful.  A failure would occur if
    the scanner is currently scanning.

**************************************************************************/
bool FrequencyScanner::setScanParameters(uint64_t startFrequencyInHertz,
                                         uint64_t endFrequencyInHertz,
                                         uint64_t frequencyIncrementInHertz)
{
  bool success;
  bool result;

  // Default to failure.
  success = false;

  if (scannerState == Idle)
  {
    // Store the values for use during scanning.
    this->startFrequencyInHertz = startFrequencyInHertz;
    this->endFrequencyInHertz = endFrequencyInHertz;
    this->frequencyIncrementInHertz = frequencyIncrementInHertz;

    //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
    // Indicate that things have possibly changed.
    // The start() method will deal with this later.
    //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
    newConfigurationAvailable = true;
    //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/

    // Indicate success.
    success = true;
  } // if

  return (success);

} // setScanParameters

/**************************************************************************

  Name: start

  Purpose: The purpose of this function is to enable the frequency
  scanner so that it can start scanning.

  Calling Sequence: success = start();

  Inputs:

    None.

  Outputs:

    success - A flag that indicates whether or not the operation was
    successful.  A value of true indicates that the operation was
    successful, and a value of false indicates that the frequency
    scanner was already scanning.

**************************************************************************/
bool FrequencyScanner::start(void)
{
  bool success;
  bool result;

  // Default to failure.
  success = false;

  if (scannerState == Idle)
  {
    if (newConfigurationAvailable)
    {
      // Force this at the edge of the cliff.
      currentFrequencyInHertz = endFrequencyInHertz;

      // Select new frequency.
      result = radioPtr->setReceiveFrequency(currentFrequencyInHertz);


      // Negate the flag so that this processing occurs one time.
      newConfigurationAvailable = false;
    } // if

    // Make state transition.
    scannerState = Scanning;

    // Allow callback notification.
    dataProcessorPtr->enableSignalNotification();

    // Indicate success.
    success = true;
  } // if

  return (success);

} // start

/**************************************************************************

  Name: stop

  Purpose: The purpose of this function is to stop the frequency scanner
  from scanning.

  Calling Sequence: success = stop();

  Inputs:

    None.

  Outputs:

    success - A flag that indicates whether or not the operation was
    successful.  A value of true indicates that the operation was
    successful, and a value of false indicates that the frequency
    scanner was already idle.

**************************************************************************/
bool FrequencyScanner::stop(void)
{
  bool success;

  // Default to failure.
  success = false;

  if (scannerState == Scanning)
  {
    // Make state transition.
    scannerState = Idle;

    // Turn off nofication.
    dataProcessorPtr->disableSignalNotification();

    // Indicate success.
    success = true;
  } // if

  return (success);

} // stop

/**************************************************************************

  Name: isScanning

  Purpose: The purpose of this function is to indicate whether or not
  the frequency scanner is scanning.

  Calling Sequence: status = isScanning()

  Inputs:

    None.

  Outputs:

    success - A flag that indicates whether or not the frequency
    scanner is scanning.  A value of true indicates that the scanner
    is scanning, and a value of false indicates that the scanner is
    idle.

**************************************************************************/
bool FrequencyScanner::isScanning(void)
{
  bool status;

  switch (scannerState)
  {
    case Idle:
    {
      // Indicate that the scanner is idle.
      status = false;
      break;
    } // case

    case Scanning:
    {
      // Indicate that the scanner is scanning.
      status = true;
      break;
    } // case

    default:
    {
      // Indicate that the scanner is idle.
      status = false;
      break;
    } // case
  } // switch

  return (status);

} // isScanning

/**************************************************************************

  Name: run

  Purpose: The purpose of this function is to run the frequency scanner.


  Calling Sequence: run(signalPresent)

  Inputs:

    signalPresent - A flag that indicates whether or not a signal is
    present.  A value of true indicates that a signal is present, and
    a value of false indicates that a signal is not present.

  Outputs:

    None.

**************************************************************************/
void FrequencyScanner::run(bool signalPresent)
{
  bool success;

  if (!signalPresent)
  {
    // Step to the next frequency.
    currentFrequencyInHertz = 
      currentFrequencyInHertz + frequencyIncrementInHertz;

    if (currentFrequencyInHertz > endFrequencyInHertz)
    {
      // Wrap back to the beginning.
      currentFrequencyInHertz = startFrequencyInHertz;
    } // if

    // Select new frequency.
    success = radioPtr->setReceiveFrequency(currentFrequencyInHertz);
  } // if

  return;

} // run

/**************************************************************************

  Name: displayInternalInformation

  Purpose: The purpose of this function is to display information of the
  frequency scanner.

  Calling Sequence: displayInternalInformation()

  Inputs:

    None.

  Outputs:

    None.

**************************************************************************/
void FrequencyScanner::displayInternalInformation(void)
{

  nprintf(stderr,"\n--------------------------------------------\n");
  nprintf(stderr,"Frequency Scanner Internal Information\n");
  nprintf(stderr,"--------------------------------------------\n");

  nprintf(stderr,"Scanner State             : ");

  switch (scannerState)
  {
    case Idle:
    {
      nprintf(stderr,"Idle\n");
      break;
    } // case

    case Scanning:
    {
      nprintf(stderr,"Scanning\n");
      break;
    } // case

  } // switch

  nprintf(stderr,"Start Frequency           : %llu Hz\n",
          startFrequencyInHertz);

  nprintf(stderr,"End Frequency             : %llu Hz\n",
          endFrequencyInHertz);

  nprintf(stderr,"Frequency Increment       : %llu Hz\n",
          frequencyIncrementInHertz);

  nprintf(stderr,"Current Frequency         : %llu Hz\n",
          currentFrequencyInHertz);

  return;

} // displayInternalInformation
