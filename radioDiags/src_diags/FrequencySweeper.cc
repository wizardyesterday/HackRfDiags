//**********************************************************************
// file name: FrequencySweeper.cc
//**********************************************************************

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "FrequencySweeper.h"

extern void nprintf(FILE *s,const char *formatPtr, ...);

/**************************************************************************

  Name: FrequencySweeper

  Purpose: The purpose of this function is to serve as the constructor
  of a FrequencySweeper object.

  Calling Sequence: FrequencySweeper(radioPtr,
                                     startFrequencyInHertz,
                                     frequencyIncrementInHertz,
                                     numberOfFrequencySteps,
                                     dwellTimeInMilliseconds)

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
FrequencySweeper::FrequencySweeper(Radio *radioPtr,
                         uint64_t startFrequencyInHertz,
                         double frequencyIncrementInHertz,
                         uint64_t numberOfFrequencySteps,
                         uint32_t dwellTimeInMilliseconds)
{

  // Save for later use.
  this->radioPtr = radioPtr;
  this->startFrequencyInHertz = startFrequencyInHertz;
  this->frequencyIncrementInHertz = frequencyIncrementInHertz;
  this->numberOfFrequencySteps = numberOfFrequencySteps;
  this->dwellTimeInMilliseconds = dwellTimeInMilliseconds;

  // Save for display purposes.
  currentFrequencyInHertz = startFrequencyInHertz;

  // Allow things to run.
  timeToExit = false;

  pthread_create(&sweepThread,0,
                 (void *(*)(void *))sweepProcedure,this);

  return;
 
} // FrequencySweeper

/**************************************************************************

  Name: ~FrequencySweeper

  Purpose: The purpose of this function is to serve as the destructor
  of a FrequencySweeper object.

  Calling Sequence: ~FrequencySweeper()

  Inputs:

    None.

  Outputs:

    None.

**************************************************************************/
FrequencySweeper::~FrequencySweeper(void)
{

  // Notify the sweeper thread to terminate.
  timeToExit = true;

  // We're done ... wait for the sweeper thread to terminate.
  pthread_join(sweepThread,0);

  return;

} // ~FrequencySweeper

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
void FrequencySweeper::displayInternalInformation(void)
{

  nprintf(stderr,"\n--------------------------------------------\n");
  nprintf(stderr,"Frequency Sweeper Internal Information\n");
  nprintf(stderr,"--------------------------------------------\n");

  nprintf(stderr,"Sweep State               : ");

  switch (sweepState)
  {
    case Idle:
    {
      nprintf(stderr,"Idle\n");
      break;
    } // case

    case Sweeping:
    {
      nprintf(stderr,"Sweeping\n");
      break;
    } // case

  } // switch

  nprintf(stderr,"Start Frequency           : %llu Hz\n",
          startFrequencyInHertz);

  nprintf(stderr,"Frequency Increment       : %f Hz\n",
          frequencyIncrementInHertz);

  nprintf(stderr,"Number Of Frequency Steps : %llu\n",
          numberOfFrequencySteps);

  nprintf(stderr,"Dwell Time                : %lu ms\n",
          dwellTimeInMilliseconds);

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
void FrequencySweeper::sweepProcedure(void *arg)
{
  FrequencySweeper *thisPtr;
  uint64_t currentFrequency;
  uint64_t frequencyOffset;
  uint64_t i;
  uint32_t dwellTime;
  bool outcome;

  // Retrieve the pointer to the class instance.
  thisPtr = (FrequencySweeper *)arg;

  // Convert to microseconds.
  dwellTime = thisPtr->dwellTimeInMilliseconds * 1000;

  // Start at the beginning.
  currentFrequency = thisPtr->startFrequencyInHertz;

  // Select initial frequency.
  outcome = thisPtr->radioPtr->setReceiveFrequency(currentFrequency);

  // Indicate that the system is sweeping.
  thisPtr->sweepState = Sweeping;

  while (!thisPtr->timeToExit)
  {
    for (i = 0; i < thisPtr->numberOfFrequencySteps; i++)
    {
      if (thisPtr->timeToExit)
      {
        // Bail out.
        break;
      } // if

      // Compute the offset on the fly to avoid accumulated roundoff error.
      frequencyOffset = (double)i * thisPtr->frequencyIncrementInHertz;

      // Compute the next frequency.
      currentFrequency = thisPtr->startFrequencyInHertz + frequencyOffset;

      // Update the information for display purposes.
      thisPtr->currentFrequencyInHertz = currentFrequency;
  
      // Select new frequency.
      outcome = thisPtr->radioPtr->setReceiveFrequency(currentFrequency);

      if (dwellTime != 0)
      {
        // Dwell on the current frequency.
        usleep(dwellTime);
      } // if

    } // for
  } // while

  // Indicate that the system not sweeping.
  thisPtr->sweepState = Idle;

  pthread_exit(0);

} // sweepProcedure

