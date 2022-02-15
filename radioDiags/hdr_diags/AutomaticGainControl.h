//**********************************************************************
// file name: AutomaticGainControl.h
//**********************************************************************

#ifndef _AUTOMATICGAINCONTROL_H_
#define _AUTOMATICGAINCONTROL_H_

#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>

#include "Radio.h"

class AutomaticGainControl
{
  public:

  AutomaticGainControl(Radio *radioPtr,
                       int32_t setPointInDbFs);

  ~AutomaticGainControl(void);

  void setSetPoint(int32_t setPointInDbFs);
  void run(uint32_t signalMagnitude,uint64_t frequencyInHertz);
  void displayInternalInformation(void);

  private:

  //*****************************************
  // Utility functions.
  //*****************************************
  int32_t convertMagnitudeToDbFs(uint32_t signalMagnitude);

  //*****************************************
  // Attributes.
  //*****************************************
  // The goal.
  int32_t setPointInDbFs;

  // System gains.
  uint32_t rfGainInDb;
  uint32_t ifGainInDb;
  uint32_t basebandGainInDb;

  // Filtered gain.
  float filteredBasebandGainInDb;

  Radio *radioPtr;

  // This assumes 8-bit quantities for signal level.
  int32_t dbFsTable[257];
};

#endif // _AUTOMATICGAINCONTROL_H_
