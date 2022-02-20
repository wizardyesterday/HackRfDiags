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
  bool setAgcFilterCoefficient(float coefficient);
  void run(uint32_t signalMagnitude);

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
  int32_t operatingPointInDbFs;

  // AGC filter lowpass filter coefficient for baseband gain filtering.
  float alpha;

  // System gains.
  uint32_t rfGainInDb;
  uint32_t ifGainInDb;
  uint32_t basebandGainInDb;

  // Filtered gain.
  float filteredBasebandGainInDb;

  // Magnitude of the latest signal.
  uint32_t signalMagnitude;

  Radio *radioPtr;
  IqDataProcessor *dataProcessorPtr;

  // This assumes 8-bit quantities for signal level.
  int32_t dbFsTable[257];
};

#endif // _AUTOMATICGAINCONTROL_H_