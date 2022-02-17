//**********************************************************************
// file name: AutomaticGainControl.h
//**********************************************************************

#ifndef _AUTOMATICGAINCONTROL_H_
#define _AUTOMATICGAINCONTROL_H_

#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>

#include "Radio.h"
#include "IqDataProcessor.h"

class AutomaticGainControl
{
  public:

  AutomaticGainControl(Radio *radioPtr,int32_t operatingPointInDbFs);

  ~AutomaticGainControl(void);

  void setOperatingPoint(int32_t operatingPointInDbFs);
  void updateSignalMagnitude(uint32_t signalMagnitude);
  void run(uint32_t signalMagnitude);

  void displayInternalInformation(void);

  private:

  //*****************************************
  // Utility functions.
  //*****************************************
  int32_t convertMagnitudeToDbFs(uint32_t signalMagnitude);

  void signal(void);
  int wait(void);

  static void agcProcedure(void *arg);

  //*****************************************
  // Attributes.
  //*****************************************
  // The goal.
  int32_t operatingPointInDbFs;

  // System gains.
  uint32_t rfGainInDb;
  uint32_t ifGainInDb;
  uint32_t basebandGainInDb;

  // Filtered gain.
  float filteredBasebandGainInDb;

  Radio *radioPtr;
  IqDataProcessor *dataProcessorPtr;

  // This assumes 8-bit quantities for signal level.
  int32_t dbFsTable[257];
};

#endif // _AUTOMATICGAINCONTROL_H_
