//**********************************************************************
// file name: FrequencySweeper.h
//**********************************************************************

#ifndef _FREQUENCYSWEEPER_H_
#define _FREQUENCYSWEEPER_H_

#include <pthread.h>
#include "Radio.h"

class FrequencySweeper
{
  public:

  enum state_t {Idle, Sweeping};

  FrequencySweeper(Radio *radioPtr,
              uint64_t startFrequencyInHertz,
              double frequencyIncrementInHertz,
              uint64_t numberOfFrequencySteps,
              uint32_t dwellTimeInMilliseconds);

  ~FrequencySweeper(void);

  void displayInternalInformation(void);

  private:

  //*****************************************
  // Utility functions.
  //*****************************************

  static void sweepProcedure(void *arg);

  //*****************************************
  // Attributes.
  //*****************************************

  // The sweeper state.
  state_t sweepState;
   
  // Used to tell the sweep thread that it is time to exit.
  bool timeToExit; 

  // The start frequency of the sweep.
  uint64_t startFrequencyInHertz;

  // The current frequency for display purposes.
  uint64_t currentFrequencyInHertz;

  // The frequency step size to take during a sweep.
  double frequencyIncrementInHertz;

  // This indicates how many steps to make before restarting the sweep.
  uint64_t numberOfFrequencySteps;

  // This specifies how long to park on a given frequency during a sweep.
  uint32_t dwellTimeInMilliseconds;

  // Thread support.
  pthread_t sweepThread;

  // Pointer to a radio instance.
  Radio *radioPtr;
};

#endif // _FREQUENCYSWEEPER_H_
