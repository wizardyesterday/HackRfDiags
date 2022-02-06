//**********************************************************************
// file name: FrequencyScanner.h
//**********************************************************************

#ifndef _FREQUENCYSCANNER_H_
#define _FREQUENCYSCANNER_H_

#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <pthread.h>

#include "Radio.h"

class FrequencyScanner
{
  public:

  enum state_t {Idle, Scanning};

  FrequencyScanner(Radio *radioPtr,
                   uint64_t startFrequencyInHertz,
                   uint64_t endFrequencyInHertz,
                   uint64_t frequencyIncrementInHertz);

  ~FrequencyScanner(void);

  void updateSignalState(bool signalPresent);
  bool getSignalState(void);

  void displayInternalInformation(void);

  private:

  //*****************************************
  // Utility functions.
  //*****************************************
  void signal(void);
  int wait(void);

  static void scanProcedure(void *arg);

  //*****************************************
  // Attributes.
  //*****************************************

  // The scanner state.
  state_t scannerState;
   
  // Used to tell the sweep thread that it is time to exit.
  bool timeToExit; 

  // The start frequency of the sweep.
  uint64_t startFrequencyInHertz;

  // The end frequency of the sweep.
  uint64_t endFrequencyInHertz;

  // The current frequency for display purposes.
  uint64_t currentFrequencyInHertz;

  // The frequency step size to take during a scan.
  uint64_t frequencyIncrementInHertz;

  // Thread support.
  pthread_t scanThread;
  pthread_mutex_t energyWakeupLock;
  pthread_cond_t energyWakeupCondition;

  // An indicator that a signal has exceeded the squelch threshold.
  bool signalPresent;

  // Pointer to a radio instance.
  Radio *radioPtr;

  IqDataProcessor *dataProcessorPtr;
};

#endif // _FREQUENCYSCANNER_H_
