//**************************************************************************
// file name: BasebandDataProcessor.cc
//**************************************************************************

#include <stdio.h>
#include <sys/select.h>
#include <sys/time.h>
#include <unistd.h>
#include <string.h>

#include "BasebandDataProcessor.h"

//_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
// This table is used for computing the starting index of the
// ringOutputIndex when transmission is initially started.
// We want the initial of the output index to lag the input
// index by by a 8 entries to provide a pipeline.
//_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
static int pcmReaderStartIndexTable[PCM_RING_SIZE] =
{8, 9, 10, 11, 12, 13, 14, 15, 0, 1, 2, 3, 4, 5, 6, 7};
//_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/

extern void nprintf(FILE *s,const char *formatPtr, ...);

/**************************************************************************

  Name: BasebandDataProcessor

  Purpose: The purpose of this function is to serve as the constructor
  of a BasebandDataProcessor object.

  Calling Sequence: BasebandDataProcessor()

  Inputs:

    None.

  Outputs:

    None.

**************************************************************************/
BasebandDataProcessor::BasebandDataProcessor(void)
{
  int i;

  // Ensure that we don't have any dangling pointers.
  amModulatorPtr = 0;
  fmModulatorPtr = 0;
  wbFmModulatorPtr = 0;
  ssbModulatorPtr = 0;

  // Indicate that the baseband readeris not requested to stop.
  timeTostopReaderThread = false;

  // Default to no modulation of the signal
  modulatorMode = None;

  // Indicate that the transmit ring reader is not synchronized..
  synchronized = false;

  // Indicate that the system is idle.
  streamState = Idle;

  // Populate the ring.
  for (i = 0; i < PCM_RING_SIZE; i++)
  {
    pcmBufferRing[i] = pcmBuffer[i];
  } // for

  // Make sure the buffer has PCM values of zero amplitude.
  for (i = 0; i < PCM_BLOCK_SIZE; i++)
  {
    zeroPcmBuffer[i] = 0;
  } // for

  // Set the ring index to a sane value.
  pcmWriterIndex = PCM_RING_SIZE - 1;
  pcmReaderIndex = pcmReaderStartIndexTable[pcmWriterIndex];

  // Clear statistics.
  buffersProduced = 0;
  buffersConsumed = 0;
  pcmBlocksDropped = 0;
  pcmBlocksAdded = 0;

  return; 

} // BasebandDataProcessor

/**************************************************************************

  Name: ~BasebandDataProcessor

  Purpose: The purpose of this function is to serve as the destructor 
  of a BasebandDataProcessor object.

  Calling Sequence: ~BasebandDataProcessor()

  Inputs:

    None.

  Outputs:

    None.

**************************************************************************/
BasebandDataProcessor::~BasebandDataProcessor()
{

  // Stop the baseband reader.
  stop();

  return; 

} // ~BasebandDataProcessor

/**************************************************************************

  Name: setAmModulator

  Purpose: The purpose of this function is to associate an instance of
  an AM modulator object with this object.

  Calling Sequence: setAmModulator(modulatorPtr)

  Inputs:

    modulatorPtr - A pointer to an instance of an AmModulator object.

  Outputs:

    None.

**************************************************************************/
void BasebandDataProcessor::setAmModulator(AmModulator *modulatorPtr)
{

  // Save for future use.
  amModulatorPtr = modulatorPtr;

  return;

} // setAmModulator

/**************************************************************************

  Name: setFmModulator

  Purpose: The purpose of this function is to associate an instance of
  an FM modulator object with this object.

  Calling Sequence: setFmModulator(modulatorPtr)

  Inputs:

    modulatorPtr - A pointer to an instance of an FmModulator object.

  Outputs:

    None.

**************************************************************************/
void BasebandDataProcessor::setFmModulator(FmModulator *modulatorPtr)
{

  // Save for future use.
  fmModulatorPtr = modulatorPtr;

  return;

} // setFmModulator

/**************************************************************************

  Name: setWbFmModulator

  Purpose: The purpose of this function is to associate an instance of
  a wideband FM modulator object with this object.

  Calling Sequence: setWbFmModulator(modulatorPtr)

  Inputs:

    modulatorPtr - A pointer to an instance of a WbFmModulator object.

  Outputs:

    None.

**************************************************************************/
void BasebandDataProcessor::setWbFmModulator(WbFmModulator *modulatorPtr)
{

  // Save for future use.
  wbFmModulatorPtr = modulatorPtr;

  return;

} // setWbFmModulator

/**************************************************************************

  Name: setSsbModulator

  Purpose: The purpose of this function is to associate an instance of
  a SSB modulator object with this object.

  Calling Sequence: setSsbModulator(modulatorPtr)

  Inputs:

    modulatorPtr - A pointer to an instance of a SsbModulator object.

  Outputs:

    None.

**************************************************************************/
void BasebandDataProcessor::setSsbModulator(SsbModulator *modulatorPtr)
{

  // Save for future use.
  ssbModulatorPtr = modulatorPtr;

  return;

} // setSsbModulator

/**************************************************************************

  Name: setModulatorMode

  Purpose: The purpose of this function is to set the modulator mode.
  This mode dictates which modulator should be used when processing
  baseband data samples.

  Calling Sequence: setModulatorMode(mode)

  Inputs:

    mode - The modulator mode.  Valid values are None, Am, and Fm,
    WbFm, Lsb, and Usb.

  Outputs:

    None.

**************************************************************************/
void BasebandDataProcessor::setModulatorMode(modulatorType mode)
{

  // Save for future use.
  this->modulatorMode = mode;

  switch (mode)
  {
    case Lsb:
    {
      // Set to demodulate lower sideband signals.
      ssbModulatorPtr->setLsbModulationMode();
      break;
    } // case

    case Usb:
    {
      // Set to demodulate upper sideband signals.
      ssbModulatorPtr->setUsbModulationMode();
      break;
    } // case

  } // switch

  return;

} // setModulatorMode

/*****************************************************************************

  Name: start

  Purpose: The purpose of this function is to enable the baseband reader
  to process data.

  Calling Sequence: start()

  Inputs:

    None.

  Outputs:

    None.

*****************************************************************************/
void BasebandDataProcessor::start(void)
{

  if (streamState == Idle)
  {
    // Create the baseband reader thread.
    pthread_create(&basebandReaderThread,0,
                   (void *(*)(void *))basebandReaderProcedure,this);

    // Indicate that the system is streaming.
    streamState = Running;
  } // if

  return;

} // start

/*****************************************************************************

  Name: stop

  Purpose: The purpose of this function is to disable the baseband reader
  from processing data.

  Calling Sequence: stop()

  Inputs:

    None.

  Outputs:

    None.

*****************************************************************************/
void BasebandDataProcessor::stop(void)
{

  if (streamState == Running)
  {
    // Notify the baseband reader that it is time to exit.
    timeTostopReaderThread = true;

    // Wait for thread to terminate.
    pthread_join(basebandReaderThread,0);

    // Indicate that the system is no longer streaming.
    streamState = Idle;

    // Indicate that the transmit ring reader is not synchronized..
    synchronized = false;
  } // if

  return;

} // stop

/**************************************************************************

  Name: getIqData

  Purpose: The purpose of this function is to provide IQ data to send to
  the radio.

  Calling Sequence: getIqData(bufferPtr,byteCount);

  Inputs:

    bufferPtr - A pointer to IQ data.

    byteCount - The number of bytes contained in the buffer that is
    referenced by bufferPtr.

  Outputs:

    None.

**************************************************************************/
void BasebandDataProcessor::getIqData(int8_t *bufferPtr,int32_t byteCount)
{

  // Fill the buffer with modulated data samples.
  modulateBasebandData(bufferPtr,byteCount);
  
  return;
 
} // getIqData

/*****************************************************************************

  Name: getNextUnfilledBuffer

  Purpose: The purpose of this function is to retrieve the next unfilled
  buffer from the ring and advance the ring index.  This method is used
  by the modulation method.

  Calling Sequence: bufferPtr = getNextUnfilledBuffer()

  Inputs:

    None.

  Outputs:

    bufferPtr - A pointer to a block of data to be filled with data.

*****************************************************************************/
int16_t * BasebandDataProcessor::getNextUnfilledBuffer(void)
{
  int16_t *bufferPtr;

  // Increment in a modulo manner.
  pcmWriterIndex++;
  pcmWriterIndex %= PCM_RING_SIZE;

   // Retrieve the buffer.
  bufferPtr = pcmBufferRing[pcmWriterIndex];

  // One more buffer produced.
  buffersProduced++;

  return (bufferPtr);

} // getNextUnfilledBuffer

/*****************************************************************************

  Name: getNextFilledBuffer

  Purpose: The purpose of this function is to retrieve the next filled
  buffer from the ring and advance the ring index.  Note that if the
  streaming state is not set to Running, a default buffer will be
  referenced, and no other actions will be performed.

  This function also handles the cases such that either the PCM arrival
  rate is higher than the departure rate (dictated by the transmit callback
  rate) or when the PCM arrival rate is lower than the departure rate (also
  dictated by the transmit callback rate).  For the case when the arrival
  rate is higher than the departure rate, PCM blocks are dropped in order
  to compensate for the rate mismatch.  For the case when the arrival rate
  is lower than the departure rate. the previous block of PCM data is resent
  in order to compensate for the rate mismatch.  This prevents PCM data
  overruns or underruns.

  Calling Sequence: bufferPtr = getNextFilledBuffer()

  Inputs:

    None.

  Outputs:

    bufferPtr - A pointer to a block of data.

*****************************************************************************/
int16_t * BasebandDataProcessor::getNextFilledBuffer(void)
{
  uint32_t ringOutputIndex;
  int16_t *bufferPtr;
  int32_t l;
  int32_t u;
  int32_t lag;
  int32_t decrementerIndex;

  // Grab these right away.
  u = (int32_t)pcmWriterIndex;
  l = (int32_t)pcmReaderIndex;

  if (u < l)
  {
    u += PCM_RING_SIZE - 1;
  } // if

  // Compute how far behind the reader is than the writer.
  lag = u - l;

  if (lag > 10)
  {
    //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
    // This block of code performs a modulo addition so
    // that the current PCM block is dropped in order to
    // prevent an overrun of the circular ring.
    //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
    // Increment in a modulo manner to drop buffers.
    pcmReaderIndex++;
    pcmReaderIndex %= PCM_RING_SIZE;

    // Update the statistic.
    pcmBlocksDropped++;
  } // if
  else
  {
    //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
    // This block of code performs a modulo subtraction so
    // that the PCM reader index can be properly adjusted.
    // The case that is handled is when the transmitter
    // is sending information out faster than the arrival
    // rate of the PCM samples.  The strategy is to send
    // the previous PCM block as filler.  This prevents an
    // underrun of data.
    //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
    if (lag < 6)
    {
      // We need a signed value for the next calculations.
      decrementerIndex = (int32_t)pcmReaderIndex - 1;

      if (decrementerIndex < 0)
      {
        // Wrap in a modulo manner.
        decrementerIndex += PCM_RING_SIZE;
      } // if

      // Adjust the index to resend the previous PCM block.
      pcmReaderIndex = (uint32_t)decrementerIndex;

      // Adjust the statistic.
      pcmBlocksAdded++;
    } // if
  } // else

  if (streamState == Running)
  {
    if (!synchronized)
    {
      // Transition since we only want to synchronize once.
      synchronized = true;

      // Synchronize the reader index to the writer index.
      pcmReaderIndex = pcmReaderStartIndexTable[pcmWriterIndex];
    } // if

    // Retrieve the buffer.
    bufferPtr = pcmBufferRing[pcmReaderIndex];

    // Increment in a modulo manner.
    pcmReaderIndex++;
    pcmReaderIndex %= PCM_RING_SIZE;

    // One more buffer consumed.
    buffersConsumed++;
  } // if
  else
  {
    // Set the buffer pointer to something sane.
    bufferPtr = zeroPcmBuffer;
  } // else

  return (bufferPtr);

} // getNextFilledBuffer

/**************************************************************************

  Name: modulateBasebandData

  Purpose: The purpose of this function is to provide modulated data to
  the transmit process.  Input data is retrieved from the ring and
  presented to the appropriate modulator.  It is the modulator that fills
  the output buffer with modulated data.

  Calling Sequence: modulateBasebandData(bufferPtr,bufferLengthPtr);

  Inputs:

    bufferPtr - A pointer to modulated data.

    bufferLength - The number of samples to store in the data buffer
    that is referenced by bufferPtr.

  Outputs:

    None.

**************************************************************************/
void BasebandDataProcessor::modulateBasebandData(int8_t *bufferPtr,
                                                 uint32_t bufferLength)
{
  int16_t *inputBufferPtr;
  uint32_t outputBufferLength;

  // Retrieve next buffer of PCM data.
  inputBufferPtr = getNextFilledBuffer();

  switch (modulatorMode)
  {
    case None:
    {
      // Produce an unmodulated carrier with half of the maximum amplitude.
      memset(bufferPtr,64,bufferLength);
      break;
    } // case

    case Am:
    {
      // Modulate as an AM signal.
      amModulatorPtr->acceptData(inputBufferPtr,
                                 PCM_BLOCK_SIZE,
                                 bufferPtr,
                                 &outputBufferLength);
      break;
    } // case

    case Fm:
    {
      // Modulate as an FM signal.
      fmModulatorPtr->acceptData(inputBufferPtr,
                                 PCM_BLOCK_SIZE,
                                 bufferPtr,
                                 &outputBufferLength);
      break;
    } // case

    case WbFm:
    {
      // Modulate as an FM signal.
      wbFmModulatorPtr->acceptData(inputBufferPtr,
                                   PCM_BLOCK_SIZE,
                                   bufferPtr,
                                   &outputBufferLength);
      break;
    } // case

    case Lsb:
    case Usb:
    {
      // Modulate as a single sideband signal.
      ssbModulatorPtr->acceptData(inputBufferPtr,
                                  PCM_BLOCK_SIZE,
                                  bufferPtr,
                                  &outputBufferLength);
      break;
    } // case

    default:
    {
      break;
    } // case
  } // switch

  return;

} // modulateBasebandData

/**************************************************************************

  Name: displayInternalInformation

  Purpose: The purpose of this function is to display internal information
  in the baseband data processor.

  Calling Sequence: displayInternalInformation()

  Inputs:

    None.

  Outputs:

    None.

**************************************************************************/
void BasebandDataProcessor::displayInternalInformation(void)
{

  nprintf(stderr,"\n--------------------------------------------\n");
  nprintf(stderr,"Baseband Data Processor Internal Information\n");
  nprintf(stderr,"--------------------------------------------\n");

  nprintf(stderr,"Stream State           : ");

  switch (streamState)
  {
    case Idle:
    {
      nprintf(stderr,"Idle\n");
      break;
    } // case

    case Running:
    {
      nprintf(stderr,"Running\n");
      break;
    } // case

    default:
    {
      break;
    } // case
  } // switch

  nprintf(stderr,"Synchronization State  : ");

  if (synchronized)
  {
    nprintf(stderr,"Synchronized\n");
  } // if
  else
  {
    nprintf(stderr,"Unsynchronized\n");
  } // else

  nprintf(stderr,"PCM Writer Index       : %u\n",pcmWriterIndex);
  nprintf(stderr,"PCM Reader Index       : %u\n",pcmReaderIndex);
  nprintf(stderr,"Buffers Produced       : %u\n",buffersProduced);
  nprintf(stderr,"Buffers Consumed       : %u\n",buffersConsumed);
  nprintf(stderr,"PCM Blocks Dropped     : %u\n",pcmBlocksDropped);
  nprintf(stderr,"PCM Blocks Added       : %u\n",pcmBlocksAdded);

  nprintf(stderr,"Modulator Mode         : ");

  switch (modulatorMode)
  {
    case None:
    {
      nprintf(stderr,"None\n");
      break;
    } // case

    case Am:
    {
      nprintf(stderr,"AM\n");
      break;
    } // case

    case Fm:
    {
      nprintf(stderr,"FM\n");
      break;
    } // case

    case WbFm:
    {
      nprintf(stderr,"WBFM\n");
      break;
    } // case

    case Lsb:
    {
      nprintf(stderr,"LSB\n");
      break;
    } // case

    case Usb:
    {
      nprintf(stderr,"USB\n");
      break;
    } // case

    default:
    {
      break;
    } // case

  } // switch

  return;

} // displayInternalInformation

/*****************************************************************************

  Name: basebandReaderProcedure

  Purpose: The purpose of this function is to handle reading stdin for
  baseband data that is to be modulated.

  Calling Sequence: basebandReaderProcedure(arg)

  Inputs:

    arg - A pointer to an argument.cound: 0


  Outputs:

    None.

*****************************************************************************/
void BasebandDataProcessor::basebandReaderProcedure(void *argPtr)
{
  BasebandDataProcessor *mePtr;
  size_t count;
  int16_t *inputBufferPtr;
  fd_set readFds;
  struct timeval timeout;
  int status;

  fprintf(stderr,"Entering Baseband Reader.\n");

  // Save the pointer to the object.
  mePtr = (BasebandDataProcessor *)argPtr;

  // Allow this thread to run.
  mePtr->timeTostopReaderThread  = false;

  while (!mePtr->timeTostopReaderThread)
  {
    // Watch stdin (fd 0) to see when it has input.
    FD_ZERO(&readFds);
    FD_SET(0,&readFds);

    // Wait up to 5 milliseconds.
    timeout.tv_sec = 0;
    timeout.tv_usec = 5000;

    status = select(1,&readFds,NULL,NULL,&timeout);

    if (status > 0)
    {
      // Data is available, so retrieve the next buffer from the ring.
      inputBufferPtr = mePtr->getNextUnfilledBuffer();

      // Read the baseband data into the buffer.
      count = fread(inputBufferPtr,2,PCM_BLOCK_SIZE,stdin);
    } // if
  } // while */

  fprintf(stderr,"Exiting Baseband Reader.\n");

  pthread_exit(0);

} // basebandReaderProcedure
