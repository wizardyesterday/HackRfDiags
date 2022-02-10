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

    // Update the signal state information.
    thisPtr->updateSignalState(signalPresent);
  } // ig

  return;

} // signalStateCallback

/**************************************************************************

  Name: FrequencyScanner

  Purpose: The purpose of this function is to serve as the constructor
  of a FrequencyScanner object.

  Calling Sequence: FrequencyScanner(radioPtr,
                                     startFrequencyInHertz,
                                     endFrequencyInHz,
                                     frequencyIncrementInHertz)

  Inputs:

    radioPtr - A pointer to a Radio instance.

    startFrequencyInHertz - The start frequency of the sweep.

    frequencyIncrementInHertz - The increment that is used to generate
    the discrete frequencies. 

    numberOfFrequencySteps - The number of discrete frequency steps in
    the sweep.

    dwellTimeInMilliseconds - The time that the system dwells on each
    discrete frequency.

  Outputs:

    None.

**************************************************************************/
FrequencyScanner::FrequencyScanner(Radio *radioPtr,
                                   uint64_t startFrequencyInHertz,
                                   uint64_t endFrequencyInHertz,
                                   uint64_t frequencyIncrementInHertz)
{

 // Perform mutex initialization.
 pthread_mutex_init(&energyWakeupLock,NULL);

  // Perform condition variable initialization.
  pthread_cond_init(&energyWakeupCondition,NULL);

  // Save for later use.
  this->radioPtr = radioPtr;
  this->startFrequencyInHertz = startFrequencyInHertz;
  this->endFrequencyInHertz = endFrequencyInHertz;
  this->frequencyIncrementInHertz = frequencyIncrementInHertz;

  // Save for display purposes.
  currentFrequencyInHertz = startFrequencyInHertz;

  // Indicate that a signal is not present.
  signalPresent = false;

  // Allow things to run.
  timeToExit = false;

  pthread_create(&scanThread,0,
                 (void *(*)(void *))scanProcedure,this);

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
  of a FrequencySweeper object.

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

  // Notify the sweeper thread to terminate.
  timeToExit = true;

  // We're done ... wait for the sweeper thread to terminate.
  pthread_join(scanThread,0);

  // Destroy condition variable and mutex.
  pthread_mutex_destroy(&energyWakeupLock);
  pthread_cond_destroy(&energyWakeupCondition);

  return;

} // ~FrequencyScanner

/**************************************************************************

  Name: updateSignalState

  Purpose: The purpose of this function is to signal the scanner thread
  that new signal state information is available.

  Calling Sequence: updateSignalState(signalPresent)

  Inputs:

    signalPresent - A flag that indicates whether or not a signal is
    present.  A value of true indicates that a signal is present, and
    a value of false indicates that a signal is not present.

  Outputs:

    None.

**************************************************************************/
 void FrequencyScanner::updateSignalState(bool signalPresent)
{

  // Update the attribute.
  this->signalPresent = signalPresent;

  // Notify the scanner thread of new signal state information.
  signal();

  return;

} // updateSignalState

/**************************************************************************

  Name: getSignalState

  Purpose: The purpose of this function is to signal the scanner thread
  that new signal state information is available.

  Calling Sequence: signalPresent = getSignalState()

  Inputs:

    None.

  Outputs:

    signalPresent - A flag that indicates whether or not a signal is
    present.  A value of true indicates that a signal is present, and
    a value of false indicates that a signal is not present.

**************************************************************************/
bool FrequencyScanner::getSignalState(void)
{

  return (signalPresent);

} // getSignalState

/**************************************************************************

  Name: signal

  Purpose: The purpose of this function is to signal the scanner thread
  that new signal state information is available.

  Calling Sequence: signal()

  Inputs:

    None.

  Outputs:

    None.

**************************************************************************/
void FrequencyScanner::signal(void)
{

  // Grab the mutex to avoid race conditions.
  pthread_mutex_lock(&energyWakeupLock);

  // Signal the scanner thread.
  pthread_cond_signal(&energyWakeupCondition);

  // We're done.
  pthread_mutex_unlock(&energyWakeupLock);

  return;

} // signal

/**************************************************************************

  Name: wait

  Purpose: The purpose of this function is to allow the scanner thread
  to wait for new signal state informaiton.

  Calling Sequence: result = wait()

  Inputs:

    None.

  Outputs:

    result - The result of the wait.  A value of zero indicates that a
    wakeup occurred as a result of a signal to the condition variable.  A
    nonzero value indicates a timeout or something else occurred.

**************************************************************************/
int FrequencyScanner::wait(void)
{
  struct timeval now;
  struct timespec timeout;
  int result;

  // Grab the mutex to avoid race conditions.
  pthread_mutex_lock(&energyWakeupLock);

  // Compute a time 1 second into the future.
  gettimeofday(&now,NULL);
  timeout.tv_sec = now.tv_sec + 1;
  timeout.tv_nsec = now.tv_usec * 1000;

  // Wait on the condition variable.
  result = pthread_cond_timedwait(&energyWakeupCondition,
                                  &energyWakeupLock,
                                  &timeout);

  // We're done.
  pthread_mutex_unlock(&energyWakeupLock);

  return (result);

} // wait

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

/**************************************************************************

  Name: sweepProcedure

  Purpose: The purpose of this function is to provide frequency sweep
  functionality of the system.  This function is the entry point to the
  user interface thread.

  Calling Sequence: sweepProcedure(arg)

  Inputs:

    arg - A pointer to an argument.

  Outputs:

    None.


**************************************************************************/
void FrequencyScanner::scanProcedure(void *arg)
{
  FrequencyScanner *thisPtr;
  uint64_t currentFrequency;
  uint64_t frequencyIncrement;
  uint64_t startFrequency;
  uint64_t endFrequency;
  bool outcome;
  int result;
  bool signalPresent;

  fprintf(stderr,"Entering scan procedure\n");

  // Retrieve the pointer to the class instance.
  thisPtr = (FrequencyScanner *)arg;

  // Retrieve parameters.
  startFrequency = thisPtr->startFrequencyInHertz;
  endFrequency = thisPtr->endFrequencyInHertz;
  frequencyIncrement = thisPtr->frequencyIncrementInHertz;

  // Start at the beginning.
  currentFrequency = startFrequency;

  // Select initial frequency.
  outcome = thisPtr->radioPtr->setReceiveFrequency(currentFrequency);

  // Indicate that the system is scanning.
  thisPtr->scannerState = Scanning;

  while (!thisPtr->timeToExit)
  {
    //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
    // Sleep until notified of new signal state information.
    // Note that if the result is equal to zero, new signal
    // state information is available, otherwise, a timeout
    // on the wait() function occurred.
    //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
    result = thisPtr->wait();
    //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/

    // Retrieve signal state.
    signalPresent = thisPtr->getSignalState();

    // Dwell on the frequency as long as a signal is present.
    while ((signalPresent) & (!thisPtr->timeToExit))
    {
      //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
      // Sleep until notified of new signal state information.
      // Note that if the result is equal to zero, new signal
      // state information is available, otherwise, a timeout
      // on the wait() function occurred.
      //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
      result = thisPtr->wait();
      //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/

      // Retrieve signal state.
      signalPresent = thisPtr->getSignalState();
    } // while

    if (!thisPtr->timeToExit)
    {
      // If a block of IQ data woke us up.
      if (result == 0)
      {
        // Step to the next frequency.
        currentFrequency = currentFrequency + frequencyIncrement;

        if (currentFrequency > endFrequency)
        {
          // Wrap back to the beginning.
          currentFrequency = startFrequency;
        } // if

        // Update the information for display purposes.
        thisPtr->currentFrequencyInHertz = currentFrequency;

        // Select new frequency.
        outcome = thisPtr->radioPtr->setReceiveFrequency(currentFrequency);
      } // if
    } // if
  } // while

  // Indicate that the system not sweeping.
  thisPtr->scannerState = Idle;

  fprintf(stderr,"Leaving scan procedure\n");

  pthread_exit(0);

} // scanProcedure

