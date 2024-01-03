//**************************************************************************
// file name: MessageQueue.h
//**************************************************************************
//_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
// This class implements a signal processing block known as a signal
// analyzer.  Given 8-bit IQ samples from an SDR, plots can be displayed
// of the magnitude of the signal or the power spectrum of the signal.
///_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/

#ifndef __MESSAGEQUEUE__
#define __MESSAGEQUEUE__

#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <pthread.h>

class MessageQueue
{
  //***************************** operations **************************

  public:

  MessageQueue(uint32_t numberOfEntries);
 ~MessageQueue(void);

  void enqueueEntry(void *entryPtr);
  void *dequeueEntry(void);

  private:

  //*******************************************************************
  // Utility functions.
  //*******************************************************************
  void signal(void);
  void wait(void);

  //*******************************************************************
  // Attributes.
  //*******************************************************************
  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
  // Queue support.
  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
  uint32_t numberOfEntries;
  uint32_t head;
  uint32_t tail;
  void **queuePtr;
  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/

  pthread_mutex_t queueLock;
  pthread_cond_t dataAvailable;  
};

#endif // __MESSAGEQUEUE__
