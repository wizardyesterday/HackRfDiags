//**********************************************************************
// file name: FrequencyScanner.cc
//**********************************************************************

#include <stdio.h>
#include <sys/time.h>

#include "FrequencyScanner.h"

extern void nprintf(FILE *s,const char *formatPtr, ...);

// This is needed to avoid race conditions upon instance destruction.
static bool callbackEnabled;

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

  if (callbackEnabled)
  {
    // Reference the context pointer properly.
    thisPtr = (FrequencyScanner *)contextPtr;

    // Peform the processing.
    thisPtr->run(signalPresent);
  } // ig

  return;

} // signalStateCallback

/**************************************************************************

  Name: FrequencyScanner

  Purpose: The purpose of this function is to serve as the constructor
  of a FrequencyScanner object.

  Calling Sequence: FrequencyScanner(radioPtr,
                                     startFrequencyInHertz,
                                     endFrequencyInHertz,
                                     frequencyIncrementInHertz)

  Inputs:

    radioPtr - A pointer to a Radio instance.

    startFrequencyInHertz - The start frequency of the scan.

    endFrequencyInHz - The end frequency of the scan.

    frequencyIncrementInHertz - The increment that is used to generate
    the discrete frequencies. 

  Outputs:

    None.

**************************************************************************/
FrequencyScanner::FrequencyScanner(Radio *radioPtr,
                                   uint64_t startFrequencyInHertz,
                                   uint64_t endFrequencyInHertz,
                                   uint64_t frequencyIncrementInHertz)
{
  bool success;

  // Save for later use.
  this->radioPtr = radioPtr;
  this->startFrequencyInHertz = startFrequencyInHertz;
  this->endFrequencyInHertz = endFrequencyInHertz;
  this->frequencyIncrementInHertz = frequencyIncrementInHertz;

  // Start at the beginning.
  currentFrequencyInHertz = startFrequencyInHertz;

  // Select initial frequency.
  success = radioPtr->setReceiveFrequency(currentFrequencyInHertz);

  // Indicate that the system is scanning.
  scannerState = Scanning;

  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
  // Register the callback to the IqDataProcessor.
  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
  // Enable the callback function.
  callbackEnabled = true;

  // Retrieve the object instance.
  dataProcessorPtr = radioPtr->getIqProcessor();

  // Register the callback.
  dataProcessorPtr->registerSignalStateCallback(signalStateCallback,this);

  // Allow callback processing.
  dataProcessorPtr->enableSignalNotification();
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
  callbackEnabled = false;

  // Turn off nofication.
  dataProcessorPtr->disableSignalNotification();

  // Unregister the callback.
  dataProcessorPtr->registerSignalStateCallback(NULL,NULL);

  return;

} // ~FrequencyScanner

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
    success = radioPtr->setReceiveFrequency(startFrequencyInHertz);
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
