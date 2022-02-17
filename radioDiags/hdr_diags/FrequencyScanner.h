//**********************************************************************
// file name: FrequencyScanner.h
//**********************************************************************

#ifndef _FREQUENCYSCANNER_H_
#define _FREQUENCYSCANNER_H_

#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>

#include "Radio.h"
#include "IqDataProcessor.h"

class FrequencyScanner
{
  public:

  enum state_t {Idle, Scanning};

  FrequencyScanner(Radio *radioPtr,
                   uint64_t startFrequencyInHertz,
                   uint64_t endFrequencyInHertz,
                   uint64_t frequencyIncrementInHertz);

  ~FrequencyScanner(void);

  void run(bool signalPresent);

  void displayInternalInformation(void);

  private:

  //*****************************************
  // Utility functions.
  //*****************************************

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

  // Pointer to a radio instance.
  Radio *radioPtr;

  IqDataProcessor *dataProcessorPtr;
};

#endif // _FREQUENCYSCANNER_H_
