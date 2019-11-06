//**********************************************************************
// file name: DataConsumer.cc
//**********************************************************************

#include <stdio.h>
#include "IqDataProcessor.h"
#include "DataConsumer.h"

extern void nprintf(FILE *s,const char *formatPtr, ...);

/**************************************************************************

  Name: DataConsumer

  Purpose: The purpose of this function is to serve as the constructor
  of a DataConsumer object.

  Calling Sequence: DataConsumer(dataProcessorPtr)

  Inputs:

    dataProcessorPtr - A pointer to an IQ data processor object.

  Outputs:

    None.

**************************************************************************/
DataConsumer::DataConsumer(IqDataProcessor *dataProcessorPtr)
{

  // The data consumer thread will need this.
  this->dataProcessorPtr = dataProcessorPtr;

  // Ensure that the system will discard any arriving data.
  enabled = false;

  // Start at the beginning of the message pool.
  messageIndex = 0;

  // These will be set when the first data block  is received.
  lastTimeStamp = 0;

  // Indicate that all blocks received were the proper length.
  shortBlockCount = 0;

  // Create the data consumer thread.
  pthread_create(&dataConsumerThread,0,
                 (void *(*)(void *))dataConsumerProcedure,this);
 
  return;
 
} // DataConsumer

/**************************************************************************

  Name: ~DataConsumer

  Purpose: The purpose of this function is to serve as the destructor
  of a DataConsumer object.

  Calling Sequence: ~DataConsumer()

  Inputs:

    None.

  Outputs:

    None.

**************************************************************************/
DataConsumer::~DataConsumer(void)
{

  // Ensure that the system will discard any arriving data.
  enabled = false;

 // Notify the data consumer thread that it is time to exit.
  timeToExit = true;

  // Wait for thread to terminate.
  pthread_join(dataConsumerThread,0);

  return;
 
} // ~DataConsumer

/**************************************************************************

  Name: start

  Purpose: The purpose of this function is to enable transmission of
  data.

  Calling Sequence: start()

  Inputs:

    None.

  Outputs:

    None.

**************************************************************************/
void DataConsumer::start(void)
{

  // These will be set when the first data block  is received.
  lastTimeStamp = 0;

  // Enable transmission of data.
  enabled = true;

  return;
 
} // start

/**************************************************************************

  Name: stop

  Purpose: The purpose of this function is to disable transmission of
  data.

  Calling Sequence: stop()

  Inputs:

    None.

  Outputs:

    None.

**************************************************************************/
void DataConsumer::stop(void)
{

  // Disable transmission of data.
  enabled = false;

  return;
 
} // stop

/**************************************************************************

  Name: reset

  Purpose: The purpose of this function is to reset most of the system
  attributes.

  Calling Sequence: reset()

  Inputs:

    None.

  Outputs:

    None.

**************************************************************************/
void DataConsumer::reset(void)
{
  int i, count;

  // Ensure that the system will discard any arriving data.
  enabled = false;

  // Start at the beginning of the message pool.
  messageIndex = 0;

  // These will be set when the first data block  is received.
  lastTimeStamp = 0;

  // Indicate that all blocks received were the proper length.
  shortBlockCount = 0;

  // See how many entries are in the message queue.
  count = iqMessageQueue.get_count();

  // Clear the message queue.
  for (i = 0; i < count; i++)
  {
    // Dequeue without waiting.
    iqMessageQueue.dequeue(0);
  } // for  

  return;
 
} // reset

/**************************************************************************

  Name: acceptData

  Purpose: The purpose of this function is to queue data to be transmitted
  over the network.  The data consumer thread will dequeue the message
  and perform the forwarding of the data.

  Calling Sequence: acceptData(timeStamp,bufferPtr,bufferLength);

  Inputs:

    timeStamp - A 32-bit timestamp associated with the message.

    bufferPtr - A pointer to IQ data.

    bufferLength - The number of bytes contained in the buffer that is
    in the buffer.

  Outputs:

    None.

**************************************************************************/
void DataConsumer::acceptData(uint32_t timeStamp,
                              void *bufferPtr,
                              uint32_t bufferLength)
{
  bool status;

  if (enabled)
  {
    // Save for future use.
    lastTimeStamp = timeStamp;

    if (bufferLength > DATA_CONSUMER_BUFFER_SIZE)
    {
      // Clip it.
      bufferLength = DATA_CONSUMER_BUFFER_SIZE;
    } // if
    else
    {
      if (bufferLength < DATA_CONSUMER_BUFFER_SIZE)
      {
        // Indicate that we received less bytes than expected.
        shortBlockCount++;
      } // if
    } // else

    // Construct the message.
    message[messageIndex].timeStamp = timeStamp;
    message[messageIndex].byteCount = bufferLength;
    memcpy(message[messageIndex].buffer,bufferPtr,bufferLength);

    // Queue the message to the data consumer thread.
    iqMessageQueue.enqueue(&message[messageIndex]);

    // Reference the next message.
    messageIndex++;

    // Wrap back to the beginning of the ring if necessary.
    messageIndex = messageIndex % DATA_CONSUMER_NUMBER_OF_MESSAGES;
  } // if

  return;
 
} // acceptData

/**************************************************************************

  Name: displayInternalInformation

  Purpose: The purpose of this function is to display internal
  in the data consumer.

  Calling Sequence: displayInternalInformation()

  Inputs:

    None.

  Outputs:

    None.

**************************************************************************/
void DataConsumer::displayInternalInformation(void)
{

  nprintf(stderr,"\n--------------------------------------------\n");
  nprintf(stderr,"Data Consumer Internal Information\n");
  nprintf(stderr,"--------------------------------------------\n");

  nprintf(stderr,"Last Timestamp           : %u\n",lastTimeStamp);
  nprintf(stderr,"Short Block Count        : %u\n",shortBlockCount);

  return;

} // displayInternalInformation

/*****************************************************************************

  Name: dataConsumerProcedure

  Purpose: The purpose of this function is to provide data consumption
  functionality of the system.  The function blocks on the message queue
  until a message arrives.  When a message is available, the contents are
  marshalled as arguments to the data processing object via an invocation
  of its data handling method.
  A timeout is used when blocking for messages so that a request for
  termination can be interrogated.  This function is the entry point to
  the data consumer thread.

  Calling Sequence: dataConsumerProcedure(arg)

  Inputs:

    arg - A pointer to an argument.

  Outputs:

    None.

*****************************************************************************/
void DataConsumer::dataConsumerProcedure(void *arg)
{
  DataConsumer *mePtr;
  iqMessage *messagePtr;

  fprintf(stderr,"Entering Data Consumer.\n");

  // Save the pointer to the object.
  mePtr = (DataConsumer *)arg;

  // Allow this thread to run.
  mePtr->timeToExit = false;

  while (!mePtr->timeToExit)
  {
    // Retrieve the queued message.
    messagePtr = mePtr->iqMessageQueue.dequeue(100000);

    // Is a message available?
    if (messagePtr != 0)
    {
      // Send the information to the data processing object.
      mePtr->dataProcessorPtr->acceptIqData(messagePtr->timeStamp,
                                            messagePtr->buffer,
                                            messagePtr->byteCount);
    } // if
  } // while */

  fprintf(stderr,"Exiting Data Consumer.\n");

  pthread_exit(0);

} // dataConsumerProcedure
