//**************************************************************************
// file name: IqDataProcessor.cc
//**************************************************************************

#include <stdio.h>
#include "IqDataProcessor.h"

static float decimator1Coefficients[] =
{
  0.2504357,
  0.5000000,
  0.2504357
};

static float decimator2Coefficients[] =
{
  0.2517491,
  0.4999998,
  0.2517491
};

static float decimator3Coefficients[] =
{
  0.2570951,
  0.5000000,
  0.2570951
};

extern void nprintf(FILE *s,const char *formatPtr, ...);

/**************************************************************************

  Name: IqDataProcessor

  Purpose: The purpose of this function is to serve as the constructor
  of an IqDataProcessor object.

  Calling Sequence: IqDataProcessor()

  Inputs:

    None.

  Outputs:

    None.

**************************************************************************/
IqDataProcessor::IqDataProcessor(void)
{
  int numberOfDecimator1Taps;
  int numberOfDecimator2Taps;
  int numberOfDecimator3Taps;

  // Default to no demodulation of the signal
  demodulatorMode = None;

  numberOfDecimator1Taps =
      sizeof(decimator1Coefficients) / sizeof(float);

  numberOfDecimator2Taps =
      sizeof(decimator2Coefficients) / sizeof(float);

  numberOfDecimator3Taps =
      sizeof(decimator3Coefficients) / sizeof(float);

  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
  // This pair of decimators reduce the sample rate from 2048000S/s
  // to 1024000S/s.
  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
  // Allocate the decimator for the in-phase component.
  stage1IDecimatorPtr = new Decimator_int16(numberOfDecimator1Taps,
                                      decimator1Coefficients,
                                      2);

  // Allocate the decimator for the quadrature component.
  stage1QDecimatorPtr = new Decimator_int16(numberOfDecimator1Taps,
                                      decimator1Coefficients,
                                      2);
  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/

  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
  // This pair of decimators reduce the sample rate from 1024000S/s
  // to 512000S/s.
  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
  // Allocate the decimator for the in-phase component.
  stage2IDecimatorPtr = new Decimator_int16(numberOfDecimator2Taps,
                                      decimator2Coefficients,
                                      2);

  // Allocate the decimator for the quadrature component.
  stage2QDecimatorPtr = new Decimator_int16(numberOfDecimator2Taps,
                                      decimator2Coefficients,
                                      2);
  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/

  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
  // This pair of decimators reduce the sample rate from 512000S/s
  // to 256000S/s.
  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
  // Allocate the decimator for the in-phase component.
  stage3IDecimatorPtr = new Decimator_int16(numberOfDecimator3Taps,
                                      decimator3Coefficients,
                                      2);

  // Allocate the decimator for the quadrature component.
  stage3QDecimatorPtr = new Decimator_int16(numberOfDecimator3Taps,
                                      decimator3Coefficients,
                                      2);
  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/


  // Let all signal exceed threshold.
  signalDetectThreshold = 0;

  // Instantiate a signal tracker.
  trackerPtr = new SignalTracker(signalDetectThreshold);

  // Default to no notification of signal state.p
  signalNotificationEnabled = false;

  // Default to no callback registered.
  signalCallbackPtr = NULL;

  // Default to no notification of signal state.p
  signalMagnitudeNotificationEnabled = false;

  // Default to no callback registered.
  signalMagnitudeCallbackPtr = NULL;

  return; 

} // IqDataProcessor

/**************************************************************************

  Name: ~IqDataProcessor

  Purpose: The purpose of this function is to serve as the destructor 
  of an IqDataProcessor object.

  Calling Sequence: ~IqDataProcessor()

  Inputs:

    None.

  Outputs:

    None.

**************************************************************************/
IqDataProcessor::~IqDataProcessor()
{

  // Release resources.
  delete stage1IDecimatorPtr;
  delete stage1QDecimatorPtr;
  delete stage2IDecimatorPtr;
  delete stage2QDecimatorPtr;
  delete stage3IDecimatorPtr;
  delete stage3QDecimatorPtr;

  delete trackerPtr;

  return; 

} // ~IqDataProcessor

/**************************************************************************

  Name: setAmDemodulator

  Purpose: The purpose of this function is to associate an instance of
  an AM demodulator object with this object.

  Calling Sequence: setAmDemodulator(demodulatorPtr)

  Inputs:

    demodulatorPtr - A pointer to an instance of an FmDemodulator object.

  Outputs:

    None.

**************************************************************************/
void IqDataProcessor::setAmDemodulator(AmDemodulator *demodulatorPtr)
{

  // Save for future use.
  amDemodulatorPtr = demodulatorPtr;

  return;

} // setAmDemodulator

/**************************************************************************

  Name: setFmDemodulator

  Purpose: The purpose of this function is to associate an instance of
  an FM demodulator object with this object.

  Calling Sequence: setFmDemodulator(demodulatorPtr)

  Inputs:

    demodulatorPtr - A pointer to an instance of an FmDemodulator object.

  Outputs:

    None.

**************************************************************************/
void IqDataProcessor::setFmDemodulator(FmDemodulator *demodulatorPtr)
{

  // Save for future use.
  fmDemodulatorPtr = demodulatorPtr;

  return;

} // setFmDemodulator

/**************************************************************************

  Name: setWbFmDemodulator

  Purpose: The purpose of this function is to associate an instance of
  a wideband FM demodulator object with this object.

  Calling Sequence: setWbFmDemodulator(demodulatorPtr)

  Inputs:

    demodulatorPtr - A pointer to an instance of a WbFmDemodulator object.

  Outputs:

    None.

**************************************************************************/
void IqDataProcessor::setWbFmDemodulator(WbFmDemodulator *demodulatorPtr)
{

  // Save for future use.
  wbFmDemodulatorPtr = demodulatorPtr;

  return;

} // setWbFmDemodulator

/**************************************************************************

  Name: setSsbDemodulator

  Purpose: The purpose of this function is to associate an instance of
  a SSB demodulator object with this object.

  Calling Sequence: setSsbDemodulator(demodulatorPtr)

  Inputs:

    demodulatorPtr - A pointer to an instance of a SsbDemodulator object.

  Outputs:

    None.

**************************************************************************/
void IqDataProcessor::setSsbDemodulator(SsbDemodulator *demodulatorPtr)
{

  // Save for future use.
  ssbDemodulatorPtr = demodulatorPtr;

  return;

} // setSsbDemodulator

/**************************************************************************

  Name: setDemodulatorMode

  Purpose: The purpose of this function is to set the demodulator mode.
  This mode dictates which demodulator should be used when processing IQ
  data samples.

  Calling Sequence: setDemodulatorMode(mode)

  Inputs:

    mode - The demodulator mode.  Valid values are None, Am, and Fm,
    WbFm, Lsb, and Usb.

  Outputs:

    None.

**************************************************************************/
void IqDataProcessor::setDemodulatorMode(demodulatorType mode)
{

  // Save for future use.
  this->demodulatorMode = mode;

  switch (mode)
  {
    case Lsb:
    {
      // Set to demodulate lower sideband signals.
      ssbDemodulatorPtr->setLsbDemodulationMode();
      break;
    } // case

    case Usb:
    {
      // Set to demodulate upper sideband signals.
      ssbDemodulatorPtr->setUsbDemodulationMode();
      break;
    } // case

  } // switch

  return;

} // setDemodulatorMode

/**************************************************************************

  Name: setSignalDetectThreshold

  Purpose: The purpose of this function is to set the signal detect
  threshold.  A signal is considered as detected if a signal magnitude
  matches or exceeds the threshold value.

  Calling Sequence: success = setSignalDetectThreshold(threshold)

  Inputs:

    threshold - The signal detection threshold.

  Outputs:

    None.

**************************************************************************/
void IqDataProcessor::setSignalDetectThreshold(uint32_t threshold)
{

  // Update for later use.
  this->signalDetectThreshold = threshold;

  // Notify the signal tracker of the new threshold.
  trackerPtr->setThreshold(threshold);

  return;

} // setSignalDetectThreshold

/*****************************************************************************

  Name: reduceSampleRate

  Purpose: The purpose of this function is to accept interleaved IQ data
  and decimate the data by a factor of 8.  The decimated data will be stored
  in the output array, decimatedData[], which will also contain interleaved
  IQ data..

  Calling Sequence: byteCount = reduceSampleRate(bufferPtr,bufferLength)

  Inputs:

    bufferPtr - A pointer to IQ data.

    bufferLength - The number of bytes contained in the buffer.

  Outputs:

    byteCount - The number of bytes stored in the decimatedData[] array.

*****************************************************************************/
uint32_t IqDataProcessor::reduceSampleRate(
    int8_t *bufferPtr,
    uint32_t bufferLength)
{
  uint32_t byteCount;
  uint32_t i;
  uint32_t outputBufferIndex;
  bool sampleAvailable;
  int16_t sample;

  // Set to reference the beginning of the in-phase output buffer.
  outputBufferIndex = 0;

  // Decimate the in-phase samples.
  for (i = 0; i < bufferLength; i += 2)
  {
    sampleAvailable = stage1IDecimatorPtr->decimate((int16_t)bufferPtr[i],
                                                    &sample);
    if (sampleAvailable)
    {
      sampleAvailable = stage2IDecimatorPtr->decimate(sample,&sample);

      if (sampleAvailable)
      {
        sampleAvailable = stage3IDecimatorPtr->decimate(sample,&sample);

        if (sampleAvailable)
        {
          // Store the decimated sample.
          decimatedData[outputBufferIndex] = (int8_t)sample;

          // Reference the next storage location.
          outputBufferIndex += 2;
        } // if
      } // if
    } // if
  } // for

  // Save the value to return to the caller.
  byteCount = outputBufferIndex;

  // Set to reference the beginning of the quadrature output buffer.
  outputBufferIndex = 1;

  // Decimate the quadrature samples.
  for (i = 1; i < (bufferLength + 1); i += 2)
  {
    sampleAvailable = stage1QDecimatorPtr->decimate((int16_t)bufferPtr[i],
                                                    &sample);
    if (sampleAvailable)
    {
      sampleAvailable = stage2QDecimatorPtr->decimate(sample,&sample);

      if (sampleAvailable)
      {
        sampleAvailable = stage3QDecimatorPtr->decimate(sample,&sample);

        if (sampleAvailable)
        {
          // Store the decimated sample.
          decimatedData[outputBufferIndex] = (int8_t)sample;

          // Reference the next storage location.
          outputBufferIndex += 2;
        } // if
      } // if
    } // if
  } // for

  return (byteCount);

} // reduceSampleRate

/**************************************************************************

  Name: enableSignalNotification

  Purpose: The purpose of this function is to allow notification when
  a new block of IQ data arrives.

  Calling Sequence: enableSignalNotification()

  Inputs:

    None.

  Outputs:

    None.

**************************************************************************/
void IqDataProcessor::enableSignalNotification(void)
{

  signalNotificationEnabled = true;

  return;

} // enableSignalNotification

/**************************************************************************

  Name: disableSignalNotification

  Purpose: The purpose of this function is to disallow notification when
  a new block of IQ data arrives.

  Calling Sequence: disableSignalNotification()

  Inputs:

    None.

  Outputs:

    None.

**************************************************************************/
void IqDataProcessor::disableSignalNotification(void)
{

  signalNotificationEnabled = false;

  return;

} // disableSignalNotification

/**************************************************************************

  Name: registerSignalStateCallback

  Purpose: The purpose of this function is to register a callback function
  that will be invoked whenever a block of IQ data is received.

  Calling Sequence: registerSignalStateCallback(callbackPtr,contextPtr)

  Inputs:

    callbackPtr - A pointer to a callback function

    contextPtr - A pointer to private data that will be passed to the
    callback function upon invocation.

  Outputs:

    None.

**************************************************************************/
void IqDataProcessor::registerSignalStateCallback(
    void (*signalCallbackPtr)(bool signalPresent,void *contextPtr),
    void *contextPtr)
{

  // Save for later use.
  signalCallbackContextPtr = contextPtr;
  this->signalCallbackPtr = signalCallbackPtr;

} // registerSignalStateCallback

/**************************************************************************

  Name: enableSignalMagnitudeNotification

  Purpose: The purpose of this function is to allow notification of
  the average signal magnitude when a new block of IQ data arrives.

  Calling Sequence: enableSignalMagnitudeNotification()

  Inputs:

    None.

  Outputs:

    None.

**************************************************************************/
void IqDataProcessor::enableSignalMagnitudeNotification(void)
{

  signalMagnitudeNotificationEnabled = true;

  return;

} // enableSignalMagnitudeNotification

/**************************************************************************

  Name: disableSignalMagnitudeNotification

  Purpose: The purpose of this function is to disallow notification of
  signal magnitude data when a new block of IQ data arrives.

  Calling Sequence: disableSignalMagnitudeNotification()

  Inputs:

    None.

  Outputs:

    None.

**************************************************************************/
void IqDataProcessor::disableSignalMagnitudeNotification(void)
{

  signalMagnitudeNotificationEnabled = false;

  return;

} // disableSignalMagnitudeNotification

/**************************************************************************

  Name: registerSignalMagnitudeCallback

  Purpose: The purpose of this function is to register a callback function
  that will be invoked with signal magnitude information whenever a block
  of IQ data is received.

  Calling Sequence: registerSignalMagnitudeCallback(callbackPtr,contextPtr)

  Inputs:

    callbackPtr - A pointer to a callback function

    contextPtr - A pointer to private data that will be passed to the
    callback function upon invocation.

  Outputs:

    None.

**************************************************************************/
void IqDataProcessor::registerSignalMagnitudeCallback(
    void (*callbackPtr)(uint32_t signalMagnitude,void *contextPtr),
    void *contextPtr)
{

  // Save for later use.
  signalMagnitudeCallbackContextPtr = contextPtr;
  this->signalMagnitudeCallbackPtr = callbackPtr;

} // registerSignalStateCallback

/**************************************************************************

  Name: acceptIqData

  Purpose: The purpose of this function is to queue data to be transmitted
  over the network.  The data consumer thread will dequeue the message
  and perform the forwarding of the data.

  Calling Sequence: acceptIqData(timeStamp,bufferPtr,byteCount);

  Inputs:

    timeStamp - A 32-bit timestamp associated with the message.

    bufferPtr - A pointer to IQ data.

    byteCount - The number of bytes contained in the buffer that is
    in the buffer.

  Outputs:

    None.

**************************************************************************/
void IqDataProcessor::acceptIqData(unsigned long timeStamp,
                                   int8_t *bufferPtr,
                                   unsigned long byteCount)
{
  uint32_t decimatedByteCount;
  uint16_t signalPresenceIndicator;
  bool signalAllowed;
  uint32_t signalMagnitude;

  // Decimate to a sample rate that is usable further downstream.
  decimatedByteCount = reduceSampleRate(bufferPtr,byteCount);

  // Determine if a signal is available.
  signalPresenceIndicator = trackerPtr->run(bufferPtr,decimatedByteCount);

  switch (signalPresenceIndicator)
  {
    case SIGNALTRACKER_NOISE:
    {
      // We have no signal.
      signalAllowed = false;
      break;
    } // case

    case SIGNALTRACKER_STARTOFSIGNAL:
    case SIGNALTRACKER_SIGNALPRESENT:
    {
      // We have a signal.
      signalAllowed = true;
      break;
    } // case

    case SIGNALTRACKER_ENDOFSIGNAL:
    {
      // We like a squelch tail.
      signalAllowed = true;
      break;
    } // case

    default:
    {
      signalAllowed = false;
      break;
    } // case
  } // switch

  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
  // In this context, signalAllowed is used as a signal presence
  // indicator.
  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
  if ((signalNotificationEnabled) && (signalCallbackPtr != NULL))
  {
    // Notify the client client of new signal state information.
    signalCallbackPtr(signalAllowed,signalCallbackContextPtr);
  } // if
  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/

  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
  // Notify the client of signal magnitude information.
  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
  if ((signalMagnitudeNotificationEnabled) &&
      (signalMagnitudeCallbackPtr != NULL))
  {
    // Retrieve the average magnitude of the last received IQ block.
    signalMagnitude = trackerPtr->getSignalMagnitude();

    // Notify the client client of new signal magnitude information.
    signalMagnitudeCallbackPtr(signalMagnitude,signalCallbackContextPtr);
  } // if
  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/

  if (signalAllowed)
  {
    switch (demodulatorMode)
    {
      case None:
      {
        break;
      } // case

      case Am:
      {
        // Demodulate as an AM signal.
        amDemodulatorPtr->acceptIqData(decimatedData,decimatedByteCount);
        break;
      } // case

      case Fm:
      {
        // Demodulate as an FM signal.
        fmDemodulatorPtr->acceptIqData(decimatedData,decimatedByteCount);
        break;
      } // case

      case WbFm:
      {
        // Demodulate as a wideband FM signal.
        wbFmDemodulatorPtr->acceptIqData(decimatedData,decimatedByteCount);
        break;
      } // case

      case Lsb:
      case Usb:
      {
        // Demodulate as a single sideband signal.
        ssbDemodulatorPtr->acceptIqData(decimatedData,decimatedByteCount);
        break;
      } // case

      default:
      {
        break;
      } // case
    } // switch
  } // if

  return;

} // acceptIqData

/**************************************************************************

  Name: displayInternalInformation

  Purpose: The purpose of this function is to display internal information
  in the IQ data processor.

  Calling Sequence: displayInternalInformation()

  Inputs:

    None.

  Outputs:

    None.

**************************************************************************/
void IqDataProcessor::displayInternalInformation(void)
{

  nprintf(stderr,"\n--------------------------------------------\n");
  nprintf(stderr,"IQ Data Processor Internal Information\n");
  nprintf(stderr,"--------------------------------------------\n");

  nprintf(stderr,"Demodulator Mode         : ");

  switch (demodulatorMode)
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

  nprintf(stderr,"Signal Detect Threhold   : %u\n",signalDetectThreshold);

  return;

} // displayInternalInformation

