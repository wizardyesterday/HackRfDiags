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

// The current variable gain setting.
extern uint32_t radio_adjustableReceiveGainInDb;

extern void nprintf(FILE *s,const char *formatPtr, ...);

/**************************************************************************

  Name: IqDataProcessor

  Purpose: The purpose of this function is to serve as the constructor
  of an IqDataProcessor object.

  Calling Sequence: IqDataProcessor(hostIpAddress,hostPort)

  Inputs:

    hostIpAddress - A dotted decimal representation of the IP address
    of the host to support streaming of IQ data.

    hostPort - The UDP port for which the above mentioned host is
    listening.

  Outputs:

    None.

**************************************************************************/
IqDataProcessor::IqDataProcessor(char *hostIpAddress,int hostPort)
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
  signalDetectThreshold = -200;

  // Instantiate a squelch.
  squelchPtr = new Squelch(signalDetectThreshold);

  // Default to no notification of signal state.p
  signalNotificationEnabled = false;

  // Default to no callback registered.
  signalCallbackPtr = NULL;

  // Default to no notification of signal state.p
  signalMagnitudeNotificationEnabled = false;

  // Default to no callback registered.
  signalMagnitudeCallbackPtr = NULL;

  // Instantiate network connection.
  networkInterfacePtr = new UdpClient(hostIpAddress,hostPort);

  // Initially, dumping IQ data over a network connection is disabled.
  iqDumpEnabled = false;

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
  if (stage1IDecimatorPtr != NULL)
  {
    delete stage1IDecimatorPtr;
  } // if

  if (stage1QDecimatorPtr != NULL)
  {
    delete stage1QDecimatorPtr;
  } // ig

  if (stage2IDecimatorPtr != NULL)
  {
    delete stage2IDecimatorPtr;
  } // if

  if (stage2QDecimatorPtr != NULL)
  {
      delete stage2QDecimatorPtr;
  } // if

  if (stage3IDecimatorPtr != NULL)
  {
  delete stage3IDecimatorPtr;
  } // if

  if (stage3QDecimatorPtr != NULL)
  {
    delete stage3QDecimatorPtr;
  } // if

  if (squelchPtr != NULL)
  {
    delete squelchPtr;
  } // if

  if (networkInterfacePtr != NULL)
  {
    delete networkInterfacePtr;
  } // if

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

    threshold - The signal detection threshold in decibels referenced
    to full scale.

  Outputs:

    None.

**************************************************************************/
void IqDataProcessor::setSignalDetectThreshold(int32_t threshold)
{

  // Update for later use.
  this->signalDetectThreshold = threshold;

  // Notify the signal tracker of the new threshold.
  squelchPtr->setThreshold(threshold);

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

  Name: downconvertByFsOver4

  Purpose: The purpose of this function is to downconvert an IQ
  data stream by Fs/4.  The equations for this function originated
  from "Understanding Digital Signal Processing, Third Edition" by
  Richard G. Lyons.  Section 13.1.2 Frequency Translation by -Fs/4,
  explains how this all works.  Equations (13-2) portray downconversion,
  and Equations (13-3) portray upconversion.

  Calling Sequence: downconvertByFsOver4(bufferPtr,byteCount)

  Inputs:

    bufferPtr - A pointer to signed IQ data.

    byteCount - The number of bytes contained in the buffer that is
    in the buffer.

  Outputs:

    None.

**************************************************************************/
void IqDataProcessor::downconvertByFsOver4(
  int8_t *bufferPtr,
  uint32_t byteCount)
{
  uint32_t i;
  int8_t x, y;

  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
  // This block of code translates the spectrum by -Fs/4.
  // Here's how it works.  The IQ samples are multiplied by
  // exp(j*PI/2) = {1,-j1,-1,j1,...}. Note that this sequence
  // repeats modulo 4.  Consider that z(n) = x(n) + jy(n).
  // The first 4 values of the translated spectrum are,
  //
  //    znew(0) =1[x(0) + jy(0)] = x(0) + jy(0).
  //    znew(1) = -j[(x(1) + jy(1)] = y(1) - jx(1).
  //    znew(2) = -1[x(2) + jy(2)] = -x(2) -jy(2).
  //    znew(3) = j[x(3) + jy(3)] = -y(3) + jx(3).
  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
  for (i = 0; i < byteCount; i += 8)
  {
    // znew(0) = x(0) + jy(0).
    // No processing needs to be done.

    // znew(1) = y(1) - jx(1).
    x = bufferPtr[i + 2];
    y = bufferPtr[i + 3];
    bufferPtr[i + 2] = y;
    bufferPtr[i + 3] = -x;  

    // znew(2) = -x(2) - jy(2).
    x = bufferPtr[i + 4];
    y = bufferPtr[i + 5];
    bufferPtr[i + 4] = -x;
    bufferPtr[i + 5] = -y;

    // znew(3) = -y(3) + jx(3).
    x = bufferPtr[i + 6];
    y = bufferPtr[i + 7];
    bufferPtr[i + 6] = -y;
    bufferPtr[i + 7] = x;
  } // for
  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/

} // downconvertByFsOver4

/**************************************************************************

  Name: upconvertByFsOver4

  Purpose: The purpose of this function is to upconvert an IQ
  data stream by Fs/4.  The equations for this function originated
  from "Understanding Digital Signal Processing, Third Edition" by
  Richard G. Lyons.  Section 13.1.2 Frequency Translation by Fs/4,
  explains how this all works.  Equations (13-2) portray downconversion,
  and Equations (13-3) portray upconversion.

  Calling Sequence: upconvertByFsOver4(bufferPtr,byteCount)

  Inputs:

    bufferPtr - A pointer to signed IQ data.

    byteCount - The number of bytes contained in the buffer that is
    in the buffer.

  Outputs:

    None.

**************************************************************************/
void IqDataProcessor::upconvertByFsOver4(
  int8_t *bufferPtr,
  uint32_t byteCount)
{
  uint32_t i;
  int8_t x, y;

  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
  // This block of code translates the spectrum by Fs/4.
  // Here's how it works.  The IQ samples are multiplied by
  // exp(j*PI/2) = {1,j1,-1,-j1,...}. Note that this sequence
  // repeats modulo 4.  Consider that z(n) = x(n) + jy(n).
  // The first 4 values of the translated spectrum are,
  //
  //    znew(0) =1[x(0) + jy(0)] = x(0) + jy(0).
  //    znew(1) = j[(x(1) + jy(1)] = -y(1) + jx(1).
  //    znew(2) = -1[x(2) + jy(2)] = -x(2) -jy(2).
  //    znew(3) = -j[x(3) + jy(3)] = y(3) - jx(3).
  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
  for (i = 0; i < byteCount; i += 8)
  {
    // znew(0) = x(0) + jy(0).
    // No processing needs to be done.

    // znew(1) = -y(1) + jx(1).
    x = bufferPtr[i + 2];
    y = bufferPtr[i + 3];
    bufferPtr[i + 2] = -y;
    bufferPtr[i + 3] = x;  

    // znew(2) = -x(2) - jy(2).
    x = bufferPtr[i + 4];
    y = bufferPtr[i + 5];
    bufferPtr[i + 4] = -x;
    bufferPtr[i + 5] = -y;

    // znew(3) = y(3) - jx(3).
    x = bufferPtr[i + 6];
    y = bufferPtr[i + 7];
    bufferPtr[i + 6] = y;
    bufferPtr[i + 7] = -x;
  } // for
  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/

} // upconvertByFsOver4

/**************************************************************************

  Name: enableIqDump

  Purpose: The purpose of this function is to enable the streaming of
  IQ data over a UDP connection.  This allows a link parter to
  process this data in any required way: for example, demodulation,
  spectrum analysis, etc.

  Calling Sequence: enableIqDump()

  Inputs:

    None.

  Outputs:

    None.

**************************************************************************/
void IqDataProcessor::enableIqDump(void)
{

  // Enable the streaming of IQ data over a UDP connection.
  iqDumpEnabled = true;

  return;

} // enableIqDump

/**************************************************************************

  Name: disableIqDump

  Purpose: The purpose of this function is to disable the streaming of
  IQ data over a UDP connection.

  Calling Sequence: disableIqDump()

  Inputs:

    None.

  Outputs:

    None.

**************************************************************************/
void IqDataProcessor::disableIqDump(void)
{

  // Disable the streaming of IQ data over a UDP connection.
  iqDumpEnabled = false;

  return;

} // disableIqDump

/**************************************************************************

  Name: isIqDumpEnabled

  Purpose: The purpose of this function is to determine whether or not
  the dumping of IQ data, over a UDP connection is enabled or not.

  Calling Sequence: enabled = isIqDumpEnabled()

  Inputs:

    None.

  Outputs:

    enabled - A flag that indicates whether or not the dumping of IQ
    data is enabled.  A value of true indicates that IQ dumping is
    enabled, and a value of false indicates that IQ dumping is disabled.

**************************************************************************/
bool IqDataProcessor::isIqDumpEnabled(void)
{

  return (iqDumpEnabled);

} // isIqDumpEnabled

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
  bool signalAllowed;
  uint32_t signalMagnitude;

  // Decimate to a sample rate that is usable further downstream.
  decimatedByteCount = reduceSampleRate(bufferPtr,byteCount);

  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
  // When the receive frequency is set by the Radio object, the
  // radio is tuned to the frequency with an offset of Fs/4.
  // This allows the 1/f noise to be translated away from the
  // desired frequency, once the signal is mixed back to
  // baseband in the digital domain.  Before frequency
  // translation, the desired signal resides at -Fs/4.  Once
  // upconverted, the signal will reside at baseband.
  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
  upconvertByFsOver4(decimatedData,decimatedByteCount);
  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/

  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
  // Send IQ data independent of squelch threshold.  This way,
  // we maintain a live display.
  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
  if (iqDumpEnabled == true)
  {
    // Send the IQ data to the peer over the network.
    networkInterfacePtr->sendData(decimatedData,decimatedByteCount);
  } // if
  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/

  // Determine if a signal is available.
  signalAllowed = squelchPtr->run(radio_adjustableReceiveGainInDb,
                                 decimatedData,
                                 decimatedByteCount);

  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
  // In this context, signalAllowed is used as a signal presence
  // indicator.
  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
  if ((signalNotificationEnabled) && (signalCallbackPtr != NULL))
  {
    // Notify the client of new signal state information.
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
    signalMagnitude = squelchPtr->getSignalMagnitude();

    // Notify the client of new signal magnitude information.
    signalMagnitudeCallbackPtr(signalMagnitude,
                               signalMagnitudeCallbackContextPtr);
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

  nprintf(stderr,"Signal Detect Threhold   : %d dBFs\n",signalDetectThreshold);

  if (iqDumpEnabled)
  {
    nprintf(stderr,"IQ Dump Enabled          : Yes\n");
  } // if
  else
  {
    nprintf(stderr,"IQ Dump Enabled          : No\n");
  } // else

  return;

} // displayInternalInformation
