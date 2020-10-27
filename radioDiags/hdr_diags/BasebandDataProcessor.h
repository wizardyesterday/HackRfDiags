//**********************************************************************
// file name: BasebandDataProcessor.h
//**********************************************************************

#ifndef _BASEBANDDATAPROCESSOR_H_
#define _BASEBANDDATAPROCESSOR_H_

#include <stdint.h>
#include <pthread.h>
#include "AmModulator.h"
#include "FmModulator.h"
#include "WbFmModulator.h"
#include "SsbModulator.h"

// This is the size of each PCM buffer.
#define PCM_BLOCK_SIZE (512)

// This should be enough buffers to queue up.
#define PCM_RING_SIZE (16)

class BasebandDataProcessor
{
  public:

  enum modulatorType {None=0, Am=1, Fm=2, WbFm = 3, Lsb = 4, Usb = 5};
  enum streamStateType {Idle, Running};

  BasebandDataProcessor(void);
  ~BasebandDataProcessor(void);

  void setModulatorMode(modulatorType mode);
  void setAmModulator(AmModulator *modulatorPtr);
  void setFmModulator(FmModulator *modulatorPtr);
  void setWbFmModulator(WbFmModulator *modulatorPtr);
  void setSsbModulator(SsbModulator *modulatorPtr);

  void start(void);
  void stop(void);
  void getIqData(int8_t *bufferPtr,int32_t bufferLength);

  void displayInternalInformation(void);

private:

  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
  // Utility functions.
  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
  // The returned buffer will be filled with PCM data.
  int16_t *getNextUnfilledBuffer(void);

  // Retrieve the next buffer of PCM data.
  int16_t *getNextFilledBuffer(void);

  // This method coordinates the modulation process.
  void modulateBasebandData(int8_t *bufferPtr,uint32_t bufferLength);

  static void basebandReaderProcedure(void *arg);

  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
  // Attributes.
  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
  // This indicates that the transmit ring reader is synchronzied.
  bool synchronized;

  // This is used to maintain streaming state.
  streamStateType streamState;

  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
  // This ring of buffers, and the associated set of indices,
  // provides a means for a producer to fill a buffer with data
  // and a consumer the means to retrieve the data at a later
  // time.
  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
  uint32_t pcmWriterIndex;
  uint32_t pcmReaderIndex;
  int16_t *pcmBufferRing[PCM_RING_SIZE];
  int16_t pcmBuffer[PCM_RING_SIZE][PCM_BLOCK_SIZE];
  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/

  // This buffer is used when the live source is idle.
  int16_t zeroPcmBuffer[PCM_BLOCK_SIZE];

  // The ring statistics provide a means to determine system health.
  uint32_t buffersProduced;
  uint32_t buffersConsumed;
  uint32_t pcmBlocksDropped;
  uint32_t pcmBlocksAdded;

  // The modulator operating mode.
  modulatorType modulatorMode;

  // Modulator support.
  AmModulator *amModulatorPtr;
  FmModulator *fmModulatorPtr;
  WbFmModulator *wbFmModulatorPtr;
  SsbModulator *ssbModulatorPtr;

  // Thread support.
  bool timeTostopReaderThread;
  pthread_t basebandReaderThread;
  pthread_mutex_t writerLock;

};

#endif // _BASEBANDDATAPROCESSOR_H_H_
