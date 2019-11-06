//**********************************************************************
// file name: DataProvider.h
//**********************************************************************

#ifndef _DATAPROVIDER_H_
#define  _DATAPROVIDER_H_

#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

//#include "Radio.h"

class DataProvider
{
  public:

  DataProvider(void);
  ~DataProvider(void);

  void getIqData(int8_t *bufferPtr,uint32_t bufferLength);

  bool loadIqFile(char *fileNamePtr);
  void displayInternalInformation(void);

  private:

  // Utility functions.
  // We need this for when new files are loaded.
  void initializeStreamingParameters(void);

  // This retrieves data from our stored IQ samples.
  void retrieveIqDataFromBuffer(int8_t *bufferPtr,uint32_t byteCount);

  // Attributes.
  // IQ file support.
  char iqFileName[256];

  // IQ data ring buffer support.
  int8_t *iqSampleBufferPtr;
  uint32_t iqSampleBufferLength;
  uint32_t iqSampleBufferIndex;

};

#endif //  _DATAPROVIDER_H_
