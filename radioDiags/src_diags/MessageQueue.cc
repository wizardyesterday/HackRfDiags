//************************************************************************
// file name: MessageQueue.cc
//************************************************************************
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

#include "MessageQueue.h"

using namespace std;

/*****************************************************************************

  Name: MessageQueue

  Purpose: The purpose of this function is to serve as the constructor for
  an instance of an MessageQueue.

  Calling Sequence: MessageQueue(sampleRate)
 
  Inputs:

    sampleRate - The sample rate of incoming IQ data in units of S/s.

 Outputs:

    None.

*****************************************************************************/
MessageQueue:: MessageQueue(uint32_t numberOfEntries)
{
  pthread_mutexattr_t mutexAttribute;

  // We need this for queue processing.
  this->numberOfEntries = numberOfEntries;

  // Reset the queue.
  head = 0;
  tail = 0;

  // Allocate storage for the queue.
  queuePtr = new void *[numberOfEntries];

  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
  // Initialize the queue lock.  This occurs
  // since some functions that lock the mutex invoke
  // other functions that lock the same mutex.
  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
  // Initialize the lock.
  pthread_mutex_init(&queueLock,0);

  // Set up the attribute.
  pthread_mutexattr_init(&mutexAttribute);
  pthread_mutexattr_settype(&mutexAttribute,PTHREAD_MUTEX_RECURSIVE_NP);

  // Initialize the mutex using the attribute.
  pthread_mutex_init(&queueLock,&mutexAttribute);

  // This is no longer needed.
  pthread_mutexattr_destroy(&mutexAttribute);
  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/

  // Initialize the condition variable.
  pthread_cond_init(&dataAvailable,NULL);


  return;

} // MessageQueue

/*****************************************************************************

  Name: ~MessageQueue

  Purpose: The purpose of this function is to serve as the destructor for
  an instance of an MessageQueue.

  Calling Sequence: ~MessageQueue()

  Inputs:

    None.

  Outputs:

    None.

*****************************************************************************/
MessageQueue::~MessageQueue(void)
{

  // Destroy queue lock.
  pthread_mutex_destroy(&queueLock);

  // Destroy the condition variable.
  pthread_cond_destroy(&dataAvailable);

  // Release resources.
  if (queuePtr != NULL)
  {
    delete [] queuePtr;
  } // if

  return;

} // ~MessageQueue

/*****************************************************************************

  Name: signal

  Purpose: The purpose of this function is to signal the availability
  of data in the queue.

  Calling Sequence: signal()

  Inputs:

    None.

  Outputs:

    None.

*****************************************************************************/
void MessageQueue::signal(void)
{

  // Wake up whoever is interested.
  pthread_cond_signal(&dataAvailable);

  return;

} // signal

/*****************************************************************************

  Name: wait

  Purpose: The purpose of this function is to wait on the condition
  variable that is used for data availability.

  Calling Sequence: wait()

  Inputs:

    None.

  Outputs:

    None.

*****************************************************************************/
void MessageQueue::wait(void)
{
  struct timeval now;
  struct timespec timeout;

  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
  // All this setup needs to be done because the timeout value for
  // pthread_cond_wait_timeout() is not a relative time.  It is an
  // absolute time.  The only reason that I use the timedwait
  // function is to avoid the system being stuck in a waiting state.
  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
  // Retrieve the current time.
  gettimeofday(&now,NULL);

  // Set the timeout to be 1 second into the future.
  timeout.tv_sec = now.tv_sec + 1;
  timeout.tv_nsec = now.tv_usec * 1000;
  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/

  // Wait until either a signal has occurred or the time has expired.
  pthread_cond_timedwait(&dataAvailable,&queueLock,&timeout);

  return;

} // wait

/*****************************************************************************

  Name: enqueueEntry

  Purpose: The purpose of this function is to add an entry to the queue.

  Calling Sequence: enqueueEntry(entryPtr)

  Inputs:

    entryPtr - A pointer to the entry that is to be entered into the queue.

 Outputs:

    None.

*****************************************************************************/
void MessageQueue::enqueueEntry(void *entryPtr)
{

  // Acquire the lock.
  pthread_mutex_lock(&queueLock);

  // Store the entry.
  queuePtr[tail] = entryPtr;

  // Reference the next entry.
  tail++;

  if (tail == numberOfEntries)
  {
    // Wrap to the beginning.
    tail = 0;
  } // if

  // Release the lock.
  pthread_mutex_unlock(&queueLock);

  // Indicate that there is data in the queue.
  signal();

  return;

} // enqueueEntry

/*****************************************************************************

  Name: dequeueEntry

  Purpose: The purpose of this function is to remove an entry from the
  queue.

  Calling Sequence: entryPtr = dequeueEntry()

  Inputs:

    None.

 Outputs:

    entryPtr - A pointer to the entry that is to be entered into the queue.

*****************************************************************************/
void * MessageQueue::dequeueEntry(void)
{
  void *entryPtr;

  // Acquire the lock.
  pthread_mutex_lock(&queueLock);

  if (head == tail)
  {
    // The queue is empty.
    entryPtr = NULL;

    // Wait for an entry to arrive.
    wait();

    // Did an entry arrive in the queue?
    if (head != tail)
    {
      // Retrieve the entry.
      entryPtr = queuePtr[head];

      // Reference the next available entry.
      head++;

      if (head == numberOfEntries)
      {
        // Wrap to the beginning.
        head = 0;
      } // if

    } // if
  } // if
  else
  {
    // Retrieve the entry.
    entryPtr = queuePtr[head];

    // Reference the next available entry.
    head++;

    if (head == numberOfEntries)
    {
      // Wrap to the beginning.
      head = 0;
    } // if

  } // else

  // Release the lock.
  pthread_mutex_unlock(&queueLock);

  return (entryPtr);

} // dequeueEntry

