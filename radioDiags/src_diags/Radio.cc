//**********************************************************************
// file name: Radio.cc
//**********************************************************************

#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "Radio.h"

// The callback methods need access to this.
Radio * Radio::mePtr;

// The current variable gain setting.
int32_t radio_adjustableReceiveGainInDb;

extern void nprintf(FILE *s,const char *formatPtr, ...);

/*****************************************************************************

  Name: nullPcmDataHandler

  Purpose: The purpose of this function is to serve as the callback
  function to accept PCM data from a demodulator in the event that a
  callback is not passed to the constructor to an instance of a Radio.
  This prevents the demodulators from invocation a NULL address in this
  case.  This handler will merely discard any PCM data that is passed
  to it.

  Calling Sequence: nullPcmDataHandler(bufferPtr,bufferLength).

  Inputs:

    bufferPtr - A pointer to a buffer of PCM samples.

    bufferLength - The number of PCM samples in the buffer.

  Outputs:

    None.

*****************************************************************************/
static void nullPcmDataHandler(int16_t *bufferPtr,uint32_t bufferLength)
{

  // Discard the data.

  return;

} // nullPcmDataHandler

/**************************************************************************

  Name: Radio

  Purpose: The purpose of this function is to serve as the constructor
  of a Radio object.

  Calling Sequence: RadiotxSampleRate,rxSampleRate,pcmCallbackPtr)

  Inputs:

    txSampleRate - The sample rate of the baseband portion of the
    transmitter in samples per second.

    rxSampleRate - The sample rate of the baseband portion of the
    receiver in samples per second.

    pcmCallbackPtr - A pointer to a callback function that is to
    process demodulated data.

  Outputs:

    None.

**************************************************************************/
Radio::Radio(uint32_t txSampleRate,uint32_t rxSampleRate,
    void (*pcmCallbackPtr)(int16_t *bufferPtr,uint32_t bufferLength))

{ 
  int i;
  int status;
  bool success;
  pthread_mutexattr_t mutexAttribute;

  if (pcmCallbackPtr == NULL)
  {
    // Set the pointer to something sane.
    pcmCallbackPtr = nullPcmDataHandler;
  } // if

  // Save for later use.
  this->transmitSampleRate = txSampleRate;
  this->receiveSampleRate = rxSampleRate;

  // Reference the instance of this object for use by static methods.
  mePtr = this;

  // Indicate that there is no association with a device.
  devicePtr = 0;

  // Ensure that we don't have any dangling pointers.
  receiveDataProcessorPtr = 0;
  transmitBasebandDataProcessorPtr = 0;
  dataConsumerPtr = 0;
  dataProviderPtr = 0;

  // Start with a clean slate.
  receiveTimeStamp = 0;
  receiveBlockCount = 0;
  transmitTimeStamp = 0;
  transmitBlockCount = 0;

  status = hackrf_init();
  status |= hackrf_open_by_serial(NULL,(hackrf_device **)&devicePtr);

  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
  // Initialize the I/O subsystem lock.  This occurs
  // since some functions that lock the mutex invoke
  // other functions that lock the same mutex.
  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
  // Set up the attribute.
  pthread_mutexattr_init(&mutexAttribute);
  pthread_mutexattr_settype(&mutexAttribute,PTHREAD_MUTEX_RECURSIVE_NP);

  // Initialize the mutex using the attribute.
  pthread_mutex_init(&ioSubsystemLock,&mutexAttribute);

  // This is no longer needed.
  pthread_mutexattr_destroy(&mutexAttribute);
  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/

  if (status == HACKRF_SUCCESS)
  {
    // Set up the receiver with a default configuration.
    success = setupReceiver();

    if (success)
    {
      // Set up the transmitter with a default configuration.
      success = setupTransmitter();

      if (!success)
      {
        fprintf(stderr,"Could not initialize transmitter.\n");
      } // if
    } // if
    else
    {
      fprintf(stderr,"Could not initialize receiver.\n");
    } // else

    if (success)
    {
      // Instantiate the data provider object. The transmit method will
      // need this.
      dataProviderPtr = new DataProvider(); 

      // Instantiate the baseband processor object.  The transmit callback
      // will need this.
      transmitBasebandDataProcessorPtr = new BasebandDataProcessor();

      // Instantiate the IQ data processor object.  The data consumer object
      // will need this.
      receiveDataProcessorPtr = new IqDataProcessor();

      // Instantiate the data consumer object.  eventConsumerProcedure method
      // will need this.
      dataConsumerPtr = new DataConsumer(receiveDataProcessorPtr);

      // Instantiate the AM demodulator.
      amDemodulatorPtr = new AmDemodulator(pcmCallbackPtr);

      // Associate the AM demodulator with the Iq data processor.
      receiveDataProcessorPtr->setAmDemodulator(amDemodulatorPtr);

      // Instantiate the FM demodulator.
      fmDemodulatorPtr = new FmDemodulator(pcmCallbackPtr);

      // Associate the FM demodulator with the Iq data processor.
      receiveDataProcessorPtr->setFmDemodulator(fmDemodulatorPtr);

      // Instantiate the wideband FM demodulator.
      wbFmDemodulatorPtr = new WbFmDemodulator(pcmCallbackPtr);

      // Associate the FM demodulator with the Iq data processor.
      receiveDataProcessorPtr->setWbFmDemodulator(wbFmDemodulatorPtr);

      // Instantiate the SSB demodulator.
      ssbDemodulatorPtr = new SsbDemodulator(pcmCallbackPtr);

      // Associate the SSB demodulator with the Iq data processor.
      receiveDataProcessorPtr->setSsbDemodulator(ssbDemodulatorPtr);

      // Set the demodulation mode.
      receiveDataProcessorPtr->setDemodulatorMode(IqDataProcessor::Fm);

      // Instantiate the AM modulator.
      amModulatorPtr = new AmModulator();

      // Associate the AM modulator with the baseband data processor.
      transmitBasebandDataProcessorPtr->setAmModulator(amModulatorPtr);

      // Instantiate the FM modulator.
      fmModulatorPtr = new FmModulator();

      // Associate the FM modulator with the baseband data processor.
      transmitBasebandDataProcessorPtr->setFmModulator(fmModulatorPtr);

      // Instantiate the wideband FM modulator.
      wbFmModulatorPtr = new WbFmModulator();

      // Associate the wideband FM modulator with the baseband data processor.
      transmitBasebandDataProcessorPtr->setWbFmModulator(wbFmModulatorPtr);

      // Instantiate the SSB modulator.
      ssbModulatorPtr = new SsbModulator();

      // Associate the SSB modulator with the baseband data processor.
      transmitBasebandDataProcessorPtr->setSsbModulator(ssbModulatorPtr);

      // Set the modulation mode.
      transmitBasebandDataProcessorPtr->setModulatorMode(
        BasebandDataProcessor::None);

      // Instantiate an AGC with an operating point of -6dBFs.
      agcPtr = new AutomaticGainControl(this,-6);

      // Default the information source to a file.
      informationSource = File;
    } // if
  } // if
  else
  {
    fprintf(stderr,"Could not initialize hackrf.\n");
  } // else
 
  return;
 
} // Radio

/**************************************************************************

  Name: ~Radio

  Purpose: The purpose of this function is to serve as the destructor
  of a Radio object.

  Calling Sequence: Radio()

  Inputs:

    None.

  Outputs:

    None.


**************************************************************************/
Radio::~Radio(void)
{
  int status;

  // Notify the callback functions that it is time to exit.
  timeToExit = true;

  // We're done with the receiver.
  tearDownReceiver();

  // We're done with the transmitter.
  tearDownTransmitter();

  //-------------------------------------
  // Release resources in this order.
  //-------------------------------------
  if (dataConsumerPtr != 0)
  {
    delete dataConsumerPtr;
  } // if

  if (receiveDataProcessorPtr != 0)
  {
    delete receiveDataProcessorPtr;
  } // if

  if (dataProviderPtr != 0)
  {
    delete dataProviderPtr;
  } // if

  if (transmitBasebandDataProcessorPtr != 0)
  {
    delete transmitBasebandDataProcessorPtr;
  } // if

  if (amDemodulatorPtr != 0)
  {
    delete amDemodulatorPtr;
  } // if

  if (fmDemodulatorPtr != 0)
  {
    delete fmDemodulatorPtr;
  } // if

  if (wbFmDemodulatorPtr != 0)
  {
    delete wbFmDemodulatorPtr;
  } // if

  if (ssbDemodulatorPtr != 0)
  {
    delete ssbDemodulatorPtr;
  } // if

  if (amModulatorPtr != 0)
  {
    delete amModulatorPtr;
  } // if

  if (fmModulatorPtr != 0)
  {
    delete fmModulatorPtr;
  } // if

  if (wbFmModulatorPtr != 0)
  {
    delete wbFmModulatorPtr;
  } // if

  if (ssbModulatorPtr != 0)
  {
    delete ssbModulatorPtr;
  } // if

  if (agcPtr != 0)
  {
    delete agcPtr;
  } // if
  //-------------------------------------

  // Release HackRf resources.
  status = hackrf_close((hackrf_device *)devicePtr);
  status |= hackrf_exit();

  // Destroy the I/O subsystem lock.
  pthread_mutex_destroy(&ioSubsystemLock);

  return;
 
} // ~Radio

/**************************************************************************

  Name: setupReceiver

  Purpose: The purpose of this function is to set up the receiver with
  a default configuration.  Note that when invoking the methods that
  return a bool value, the complement of the value is used.  This way we
  can detect if a failure has occurred.

  Calling Sequence: success = setupReceiver()

  Inputs:

    None.

  Outputs:

    success - A boolean that indicates the outcome of the operation.  A
    value of true indicates success, and a value of false indicates
    failure.

**************************************************************************/
bool Radio::setupReceiver(void)
{
  bool success;
  int error;

  // Indicate that the receive process is disabled.
  receiveEnabled = false;

  // Indicate that the front end amplifier is not enabled.
  receiveFrontEndAmplifierEnabled = false;

  // Default to 2.048MS/s.
  success = !setSampleRate(2048000);

  // Default to 100MHz.
  success = success || !setFrequency(100000000);

  // Use a receive filter bandwidth of 1.75MHz
  success = success || !setBandwidth(1750000);

  // Disable the front end amplifier.
  success = success || !disableReceiveFrontEndAmplifier();

  // Set receive IF gain to 16dB.
  success = success || !setReceiveIfGainInDb(16);

  // Set receive baseband gain to 16dB.
  success = success || !setReceiveBasebandGainInDb(16);

  // Other subsystems need to know this.
  radio_adjustableReceiveGainInDb = 16;  

  // Default to no frequency error.
  receiveWarpInPartsPerMillion = 0;

  // Complement our cumulative result.
  success = !success;

  return (success);
 
} // setupReceiver

/**************************************************************************

  Name: setupTransmitter

  Purpose: The purpose of this function is to set up the transmitter with
  a default configuration.

  Calling Sequence: success = setupTransmitter()

  Inputs:

    None.

  Outputs:

    success - A boolean that indicates the outcome of the operation.  A
    value of true indicates success, and a value of false indicates
    failure.

**************************************************************************/
bool Radio::setupTransmitter(void)
{
  bool success;
  int error;

  // Default to success.
  success = true;

  // Indicate that the transmit process is disabled.
  transmitEnabled = false;

  // Indicate that the front end amplifier is not enabled.
  transmitFrontEndAmplifierEnabled = false;

   // Minimize the transmit gain, and set it to 2dB.
  success = setTransmitIfGainInDb(2);

  // set the transmit frequency.
  // Set the transmit sample rate.
  // set the transmi bandwidth.
  // set the transmit gain.

  return (success);
 
} // setupTransmitter

/**************************************************************************

  Name: tearDownReceiver

  Purpose: The purpose of this function is to tear down the receiver.

  Calling Sequence: tearDownReceiver()

  Inputs:

    None.

  Outputs:

    None.

**************************************************************************/
void Radio::tearDownReceiver(void)
{

  // Stop all data flow.
  stopReceiver();

  return;
  
} // tearDownReceiver

/**************************************************************************

  Name: tearDownTransmitter

  Purpose: The purpose of this function is to tear down the transmitter.

  Calling Sequence: tearDownTransmitter()

  Inputs:

    None.

  Outputs:

    None.

**************************************************************************/
void Radio::tearDownTransmitter(void)
{

  // Stop all data flow.
  stopTransmitter();

  return;
  
} // tearDownTransmitter

/**************************************************************************

  Name: startReceiver

  Purpose: The purpose of this function is to enable the receiver.  The
  underlying system software will perform the necessary processing to
  accomplish this function.

  Calling Sequence: success = startReceiver()

  Inputs:

    None.

  Outputs:

    success - A boolean that indicates the outcome of the operation.  A
    value of true indicates success, and a value of false indicates
    failure.

**************************************************************************/
bool Radio::startReceiver(void)
{
  bool success;
  int status;

  // Acquire the I/O subsystem lock.
  pthread_mutex_lock(&ioSubsystemLock);

  // Default to success
  success = true;

  if (!receiveEnabled)
  {
    if (devicePtr != 0)
    {
      // Ensure that the data consumer will accept data.
      dataConsumerPtr->start();

      //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
      // We'll keep track of amplifier enable state here,
      // since the firmware in the HackRf disables the amp
      // when the radio is stopped.
      //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
      if (receiveFrontEndAmplifierEnabled)
      {
        success = enableReceiveFrontEndAmplifier();
      } // if
      else
      {
        success = disableReceiveFrontEndAmplifier();
      } // else
      //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/

      // Start the receive process.
      status = hackrf_start_rx((hackrf_device *)devicePtr,
                               receiveCallbackProcedure,
                               NULL);

      if (status == HACKRF_SUCCESS)
      {
        // Ensure that the system will accept any arriving data.
        receiveEnabled = true;
      } // if
      else
      {
        // We're busted, so we don't need the data consumer.
        dataConsumerPtr->stop();

        // Indicate failure.
        success = false;
      } // else
    } // if
    else
    {
      // Indicate failure.
      success = false;
    } // else
  } // if

  // Release the I/O subsystem lock.
  pthread_mutex_unlock(&ioSubsystemLock);

  return (success);
  
} // startReceiver

/**************************************************************************

  Name: stopReceiver

  Purpose: The purpose of this function is to disable the receiver.  The
  underlying system software will take care of all of the necessary
  details.

  Calling Sequence: success = stopReceiver()

  Inputs:

    None.

  Outputs:

    success - A boolean that indicates the outcome of the operation.  A
    value of true indicates success, and a value of false indicates
    failure.

**************************************************************************/
bool Radio::stopReceiver(void)
{
  bool success;
  int status;

  // Acquire the I/O subsystem lock.
  pthread_mutex_lock(&ioSubsystemLock);

  // Default to success
  success = true;

  if (receiveEnabled)
  {
    if (devicePtr != 0)
    {
      // Stop the receiver.
      status = hackrf_stop_rx((hackrf_device *)devicePtr);

      if (status != HACKRF_SUCCESS)
      {
        // Indicate failure.
        success = false;
      } // if

      // Ensure that the system will discard any arriving data.
      receiveEnabled = false;

      // Ensure that the data consumer will not accept data.
      dataConsumerPtr->stop();
    } // if
  } // if

  // Release the I/O subsystem lock.
  pthread_mutex_unlock(&ioSubsystemLock);

  return (success);
  
} // stopReceiver

/**************************************************************************

  Name: startTransmitter

  Purpose: The purpose of this function is to enable the transmitter.  The
  underlying system software will perform the necessary processing to
  accomplish this function.

  Calling Sequence: success = startTransmitter()

  Inputs:

    None.

  Outputs:

    success - A boolean that indicates the outcome of the operation.  A
    value of true indicates success, and a value of false indicates
    failure.

**************************************************************************/
bool Radio::startTransmitter(void)
{
  bool success;
  int status;

  // Acquire the I/O subsystem lock.
  pthread_mutex_lock(&ioSubsystemLock);

  // Default to success
  success = true;

  if (!transmitEnabled)
  {
    if (devicePtr != 0)
    {
      //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
      // We'll keep track of amplifier enable state here,
      // since the firmware in the HackRf disables the amp
      // when the radio is stopped.
      //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
      if (transmitFrontEndAmplifierEnabled)
      {
        success = enableTransmitFrontEndAmplifier();
      } // if
      else
      {
        success = disableTransmitFrontEndAmplifier();
      } // else
      //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/

      // Start the transmit process.
      status = hackrf_start_tx((hackrf_device *)devicePtr,
                               transmitCallbackProcedure,
                               NULL);

      if (status == HACKRF_SUCCESS)
      {
        // Ensure that the system will allow transmission of data.
        transmitEnabled = true;;
      } // if
      else
      {
        // Indicate failure.
        success = false;
      } // else
   } // if
  } // if

  // Release the I/O subsystem lock.
  pthread_mutex_unlock(&ioSubsystemLock);

  return (success);
  
} // startTransmitter

/**************************************************************************

  Name: stopTransmitter

  Purpose: The purpose of this function is to disable the transmitte.
  The underlying system software will take care of all of the necessary
  details.

  Calling Sequence: success = stopTransmitter()

  Inputs:

    None.

  Outputs:

    success - A boolean that indicates the outcome of the operation.  A
    value of true indicates success, and a value of false indicates
    failure.

**************************************************************************/
bool Radio::stopTransmitter(void)
{
  bool success;
  int status;

  // Acquire the I/O subsystem lock.
  pthread_mutex_lock(&ioSubsystemLock);

  // Default to success
  success = true;

  if (transmitEnabled)
  {
    if (devicePtr != 0)
    {
      // Stop the transmitter.
      status = hackrf_stop_tx((hackrf_device *)devicePtr);

      if (status != HACKRF_SUCCESS)
      {
        // Indicate failure.
        success = false;
      } // if

      // Ensure that the system will discard requests for more data.
      transmitEnabled = false;
    } // if
  } // if

  // Release the I/O subsystem lock.
  pthread_mutex_unlock(&ioSubsystemLock);

  return (success);
  
} // stopTransmitter

/**************************************************************************

  Name: selectFileSource

  Purpose: The purpose of this function is to select the transmit
  information source to be that of a file that has been loaded into
  memory.

  Calling Sequence: selectFileSource()

  Inputs:

    None.

  Outputs:

    None.

**************************************************************************/
void Radio::selectFileSource(void)
{

  // Transmit information will be retrieved from a file that has been
  // loaded into memory.
  informationSource = File;

  return;

} // selectFileSource

/**************************************************************************

  Name: selectLiveSource

  Purpose: The purpose of this function is to select the transmit
  information source to be from the transmit baseband data processor.
  The information will be a realtime stream of modulated data.

  Calling Sequence: selectLiveSource()

  Inputs:

    None.

  Outputs:

    None.

**************************************************************************/
void Radio::selectLiveSource(void)
{

  // Transmit information will be retrieved from the transmit baseband
  // data processor.
  informationSource = Live;

  return;

} // selectLiveSource

/**************************************************************************

  Name: startLiveStream

  Purpose: The purpose of this function is to start modulation of the
  realtime live information stream.

  Calling Sequence: startLiveStream()

  Inputs:

    None.

  Outputs:

    None.

**************************************************************************/
void Radio::startLiveStream(void)
{

  // Start modulation of the realtime data stream.
  transmitBasebandDataProcessorPtr->start();

  return;

} // startLiveStream

/**************************************************************************

  Name: stopLiveStream

  Purpose: The purpose of this function is to stop modulation of the
  realtime live information stream.

  Calling Sequence: stopLiveStream()

  Inputs:

    None.

  Outputs:

    None.

**************************************************************************/
void Radio::stopLiveStream(void)
{

  // Stop modulation of the realtime data stream.
  transmitBasebandDataProcessorPtr->stop();

  return;

} // stopLiveStream

/**************************************************************************

  Name: setFrequency

  Purpose: The purpose of this function is to set the operating frequency
  of the system.

  Calling Sequence: success = setFrequency(frequency)

  Inputs:

    frequency - The receiver frequency in Hertz.

  Outputs:

    success - A boolean that indicates the outcome of the operation.  A
    value of true indicates success, and a value of false indicates
    failure.

**************************************************************************/
bool Radio::setFrequency(uint64_t frequency)
{
  bool success;
  int error;
  int64_t correctedFrequency;

  // Acquire the I/O subsystem lock.
  pthread_mutex_lock(&ioSubsystemLock);

  // Default to failure.
  success = false;

  if (devicePtr != 0)
  {
    // Correct for warp.
    correctedFrequency = frequency *
                         (1000000 - receiveWarpInPartsPerMillion) /1000000; 

    // Set the system to the new frequency.
    error = hackrf_set_freq((hackrf_device *)devicePtr,correctedFrequency);

    if (error == HACKRF_SUCCESS)
    {
      // Update attributes.
      receiveFrequency = frequency;
      transmitFrequency = frequency;

      // indicate success.
      success = true;
    } // if
  } // if

  // Release the I/O subsystem lock.
  pthread_mutex_unlock(&ioSubsystemLock);

  return (success);
  
} // setFrequency

/**************************************************************************

  Name: setBandwidth

  Purpose: The purpose of this function is to set the baseband filter
  bandwidth of the system.

  Calling Sequence: success = setBandwidth(bandwidth)

  Inputs:

    bandwidth - The baseband filter bandwidth in Hertz.

  Outputs:

    success - A boolean that indicates the outcome of the operation.  A
    value of true indicates success, and a value of false indicates
    failure.

**************************************************************************/
bool Radio::setBandwidth(uint32_t bandwidth)
{
  bool success;
  int error;

  // Acquire the I/O subsystem lock.
  pthread_mutex_lock(&ioSubsystemLock);

  // Default to failure.
  success = false;

  if (devicePtr != 0)
  {
    // Set the system to the new bandwidth.
    error = hackrf_set_baseband_filter_bandwidth((hackrf_device *)devicePtr,
                                                  bandwidth);

    if (error == HACKRF_SUCCESS)
    {
      // Update attributes.
      receiveBandwidth = bandwidth;
      transmitBandwidth = bandwidth;

      // indicate success.
      success = true;
    } // if
  } // if

  // Release the I/O subsystem lock.
  pthread_mutex_unlock(&ioSubsystemLock);

  return (success);
  
} // setBandwidth

/**************************************************************************

  Name: setSampleRate

  Purpose: The purpose of this function is to set the sample rate
  of the system.

  Calling Sequence: success = setSampleRate(sampleRate)

  Inputs:

    sampleRate - The sample rate in samples per second.

  Outputs:

    success - A boolean that indicates the outcome of the operation.  A
    value of true indicates success, and a value of false indicates
    failure.

**************************************************************************/
bool Radio::setSampleRate(uint32_t sampleRate)
{
  uint32_t correctedSampleRate;
  bool success;
  int error;

  // Acquire the I/O subsystem lock.
  pthread_mutex_lock(&ioSubsystemLock);

  // Default to failure.
  success = false;

  if (devicePtr != 0)
  {
    // Correct for warp.
    correctedSampleRate = (uint32_t)((double)sampleRate *
                          (1000000 - receiveWarpInPartsPerMillion)/1000000+0.5);

    // Set the system to the new sample rate.
    error = hackrf_set_sample_rate((hackrf_device *)devicePtr,
                                   correctedSampleRate);

    // Set the bandwidth to override that dictated by the sample rate.
    error = success || !setBandwidth(receiveBandwidth);

    if (error == HACKRF_SUCCESS)
    {
      // Update attributes.
      receiveSampleRate = sampleRate;
      transmitSampleRate = sampleRate;

      // indicate success.
      success = true;
    } // if
  } // if

  // Release the I/O subsystem lock.
  pthread_mutex_unlock(&ioSubsystemLock);

  return (success);
  
} // setSampleRate

/**************************************************************************

  Name: setWarpInPartsPerMillion

  Purpose: The purpose of this function is to set the frequency warp
  of the system.  Note that the frequency and sample rate are then
  updated in the hardware to account for the new warp setting.

  Calling Sequence: success = setWarpInPartsPerMillion(warp)

  Inputs:

    The warp value in parts per million.

  Outputs:

    success - A boolean that indicates the outcome of the operation.  A
    value of true indicates success, and a value of false indicates
    failure.

**************************************************************************/
bool Radio::setWarpInPartsPerMillion(int warp)
{
  bool success;

  // Acquire the I/O subsystem lock.
  pthread_mutex_lock(&ioSubsystemLock);

  // Update attribute.
  receiveWarpInPartsPerMillion = warp;

  // Update the sample rate and frequency to take the warp value into account.
  success = !setSampleRate(receiveSampleRate);
  success = success || !setFrequency(receiveFrequency);
 
  // Complement our cumulative result.
  success = !success;

  // Release the I/O subsystem lock.
  pthread_mutex_unlock(&ioSubsystemLock);

  return (success);
  
} // setWarpInPartsPerMillion

/**************************************************************************

  Name: setReceiveFrequency

  Purpose: The purpose of this function is to set the operating frequency
  of the receiver.

  Calling Sequence: success = setReceiveFrequency(frequency)

  Inputs:

    frequency - The receiver frequency in Hertz.

  Outputs:

    success - A boolean that indicates the outcome of the operation.  A
    value of true indicates success, and a value of false indicates
    failure.

**************************************************************************/
bool Radio::setReceiveFrequency(uint64_t frequency)
{
  bool success;

  // Invoke the core function.
  success = setFrequency(frequency);

  return (success);
  
} // setReceiveFrequency

/**************************************************************************

  Name: setReceiveBandwidth

  Purpose: The purpose of this function is to set the baseband filter
  bandwidth of the receiver.

  Calling Sequence: success = setReceiveBandwidth(bandwidth)

  Inputs:

    bandwidth - The receive baseband filter bandwidth in Hertz.

  Outputs:

    success - A boolean that indicates the outcome of the operation.  A
    value of true indicates success, and a value of false indicates
    failure.

**************************************************************************/
bool Radio::setReceiveBandwidth(uint32_t bandwidth)
{
  bool success;

  // Invoke core function.
  success = setBandwidth(bandwidth);

  return (success);
  
} // setReceiveBandwidth

/**************************************************************************

  Name: enableReceiveFrontEndAmplifier

  Purpose: The purpose of this function is to enable the receive front
  end amplifier in the system.  This provides an additional 14dB of system
  gain.  Enabled implies that the amplifier is in the signal chain.

  Calling Sequence: success = enableReceiveFrontEndAmplifier()

  Inputs:

    None.

  Outputs:

    success - A boolean that indicates the outcome of the operation.  A
    value of true indicates success, and a value of false indicates
    failure.

**************************************************************************/
bool Radio::enableReceiveFrontEndAmplifier(void)

{
  bool success;
  int error;

  // Acquire the I/O subsystem lock.
  pthread_mutex_lock(&ioSubsystemLock);

  // Default to failure.
  success = false;

  if (devicePtr != 0)
  {
    if (isReceiving())
    {
      // Enable the front end amplifier
      error = hackrf_set_amp_enable((hackrf_device *)devicePtr,1);
    } // if
    else
    {
      // This always succeeds.
      error = HACKRF_SUCCESS;
    } // else

    if (error == HACKRF_SUCCESS)
    {
      // Update attribute.
      receiveFrontEndAmplifierEnabled = true;

      // indicate success.
      success = true;
    } // if
  } // if

  // Release the I/O subsystem lock.
  pthread_mutex_unlock(&ioSubsystemLock);

  return (success);
  
} // enableReceiveFrontEndAmplifier

/**************************************************************************

  Name: disableReceiveFrontEndAmplifier

  Purpose: The purpose of this function is to disable the receive front
  end amplifier in the system.  This removes the additional 14dB of system
  gain.  Disabled implies that the amplifier is in the signal chain.

  Calling Sequence: success = disableReceiveFrontEndAmplifier()

  Inputs:

    None.

  Outputs:

    success - A boolean that indicates the outcome of the operation.  A
    value of true indicates success, and a value of false indicates
    failure.

**************************************************************************/
bool Radio::disableReceiveFrontEndAmplifier(void)

{
  bool success;
  int error;

  // Acquire the I/O subsystem lock.
  pthread_mutex_lock(&ioSubsystemLock);

  // Default to failure.
  success = false;

  if (devicePtr != 0)
  {
    if (isReceiving())
    {
      // Disable the front end amplifier
      error = hackrf_set_amp_enable((hackrf_device *)devicePtr,0);
    } // if
    else
    {
      // This always succeeds.
      error = HACKRF_SUCCESS;
    } // else

    if (error == HACKRF_SUCCESS)
    {
      // Update attribute.
      receiveFrontEndAmplifierEnabled = false;

      // indicate success.
      success = true;
    } // if
  } // if

  // Release the I/O subsystem lock.
  pthread_mutex_unlock(&ioSubsystemLock);

  return (success);
  
} // disableReceiveFrontEndAmplifier

/**************************************************************************

  Name: enableTransmitFrontEndAmplifier

  Purpose: The purpose of this function is to enable the transmit front
  end amplifier in the system.  This provides an additional 14dB of system
  gain.  Enabled implies that the amplifier is in the signal chain.

  Calling Sequence: success = enableTransmitFrontEndAmplifier()

  Inputs:

    None.

  Outputs:

    success - A boolean that indicates the outcome of the operation.  A
    value of true indicates success, and a value of false indicates
    failure.

**************************************************************************/
bool Radio::enableTransmitFrontEndAmplifier(void)

{
  bool success;
  int error;

  // Acquire the I/O subsystem lock.
  pthread_mutex_lock(&ioSubsystemLock);

  // Default to failure.
  success = false;

  if (devicePtr != 0)
  {
    if (isTransmitting())
    {
      // Enable the front end amplifier
      error = hackrf_set_amp_enable((hackrf_device *)devicePtr,1);
    } // if
    else
    {
      // This always succeeds.
      error = HACKRF_SUCCESS;
    } // else

    if (error == HACKRF_SUCCESS)
    {
      // Update attribute.
      transmitFrontEndAmplifierEnabled = true;

      // indicate success.
      success = true;
    } // if
  } // if

  // Release the I/O subsystem lock.
  pthread_mutex_unlock(&ioSubsystemLock);

  return (success);
  
} // enableTransmitFrontEndAmplifier

/**************************************************************************

  Name: disableTransmitFrontEndAmplifier

  Purpose: The purpose of this function is to disable the transmit front
  end amplifier in the system.  This removes the additional 14dB of system
  gain.  Disabled implies that the amplifier is in the signal chain.

  Calling Sequence: success = disableTransmitFrontEndAmplifier()

  Inputs:

    None.

  Outputs:

    success - A boolean that indicates the outcome of the operation.  A
    value of true indicates success, and a value of false indicates
    failure.

**************************************************************************/
bool Radio::disableTransmitFrontEndAmplifier(void)

{
  bool success;
  int error;

  // Acquire the I/O subsystem lock.
  pthread_mutex_lock(&ioSubsystemLock);

  // Default to failure.
  success = false;

  if (devicePtr != 0)
  {
    if (isTransmitting())
    {
      // Disable the front end amplifier
      error = hackrf_set_amp_enable((hackrf_device *)devicePtr,0);
    } // if
    else
    {
      // This always succeeds.
      error = HACKRF_SUCCESS;
    } // else

    if (error == HACKRF_SUCCESS)
    {
      // Update attribute.
      transmitFrontEndAmplifierEnabled = false;

      // indicate success.
      success = true;
    } // if
  } // if

  // Release the I/O subsystem lock.
  pthread_mutex_unlock(&ioSubsystemLock);

  return (success);
  
} // disableTransmitFrontEndAmplifier

/**************************************************************************

  Name: setReceiveIfGainInDb

  Purpose: The purpose of this function is to set the gain of the IF
  amplifier in the receiver.

  Calling Sequence: success = setReceiveIfGainInDb(gain)

  Inputs:

    gain - The gain in decibels

  Outputs:

    success - A boolean that indicates the outcome of the operation.  A
    value of true indicates success, and a value of false indicates
    failure.

**************************************************************************/
bool Radio::setReceiveIfGainInDb(uint32_t gain)

{
  bool success;
  int error;

  // Acquire the I/O subsystem lock.
  pthread_mutex_lock(&ioSubsystemLock);

  // Default to failure.
  success = false;

  if (devicePtr != 0)
  {
    // Set the gain.
    error = hackrf_set_lna_gain((hackrf_device *)devicePtr,gain);

    if (error == HACKRF_SUCCESS)
    {
      // Update attribute.
      receiveIfGainInDb = gain;

      // indicate success.
      success = true;
    } // if
  } // if

  // Release the I/O subsystem lock.
  pthread_mutex_unlock(&ioSubsystemLock);

  return (success);
  
} // setReceiveIfGainInDb

/**************************************************************************

  Name: setReceiveBasebandGainInDb

  Purpose: The purpose of this function is to set the gain of the
  baseband amplifier in the receiver.

  Calling Sequence: success = setReceiveBasebandGainInDb(gain)

  Inputs:

    gain - The gain in decibels

  Outputs:

    success - A boolean that indicates the outcome of the operation.  A
    value of true indicates success, and a value of false indicates
    failure.

**************************************************************************/
bool Radio::setReceiveBasebandGainInDb(uint32_t gain)

{
  bool success;
  int error;

  // Acquire the I/O subsystem lock.
  pthread_mutex_lock(&ioSubsystemLock);

  // Default to failure.
  success = false;

  if (devicePtr != 0)
  {
    // Set the gain.
    error = hackrf_set_vga_gain((hackrf_device *)devicePtr,gain);

    if (error == HACKRF_SUCCESS)
    {
      // Update attribute.
      receiveBasebandGainInDb = gain;

      // This variable is used by other subsystems.
      radio_adjustableReceiveGainInDb = receiveBasebandGainInDb;

      // indicate success.
      success = true;
    } // if
  } // if

  // Release the I/O subsystem lock.
  pthread_mutex_unlock(&ioSubsystemLock);

  return (success);
  
} // setReceiveBasebandGainInDb

/**************************************************************************

  Name: setReceiveSampleRate

  Purpose: The purpose of this function is to set the sample rate
  of the receiver.

  Calling Sequence: success = setReceiveSampleRate(sampleRate)

  Inputs:

    sampleRate - The receiver sample rate in samples per second.

  Outputs:

    success - A boolean that indicates the outcome of the operation.  A
    value of true indicates success, and a value of false indicates
    failure.

**************************************************************************/
bool Radio::setReceiveSampleRate(uint32_t sampleRate)
{
  bool success;

  // Invoke the core function.
  success = setSampleRate(sampleRate);

  return (success);
  
} // setReceiveSampleRate

/**************************************************************************

  Name: setReceiveWarpInPartsPerMillion

  Purpose: The purpose of this function is to set the frequency warp
  of the receiver.  Note that the receive frequency and receive sample
  rate are then updated in the hardware to account for the new warp
  setting.

  Calling Sequence: success = setReceiveWarpInPartsPerMillion(warp)

  Inputs:

    The warp value in parts per million.

  Outputs:

    success - A boolean that indicates the outcome of the operation.  A
    value of true indicates success, and a value of false indicates
    failure.

**************************************************************************/
bool Radio::setReceiveWarpInPartsPerMillion(int warp)
{
  bool success;

  // Invoke core function.
  success = setWarpInPartsPerMillion(warp);

  return (success);
  
} // setReceiveWarpInPartsPerMillion

/**************************************************************************

  Name: setTransmitFrequency

  Purpose: The purpose of this function is to set the transmit frequency
  of the transmitter.

  Calling Sequence: success = setTransmitFrequency(frequency)

  Inputs:

    frequency - The transmitter frequency in Hertz.

  Outputs:

    success - A boolean that indicates the outcome of the operation.  A
    value of true indicates success, and a value of false indicates
    failure.

**************************************************************************/
bool Radio::setTransmitFrequency(uint64_t frequency)
{
  bool success;

  // Invoke the core function.
  success = setFrequency(frequency);

  return (success);
  
} // setTransmitFrequency

/**************************************************************************

  Name: setTransmitBandwidth

  Purpose: The purpose of this function is to set the baseband filter
  bandwidth of the transmitter.

  Calling Sequence: success = setTransmitBandwidth(bandwidth)

  Inputs:

    bandwidth - The bandwidth in Hertz.

  Outputs:

    success - A boolean that indicates the outcome of the operation.  A
    value of true indicates success, and a value of false indicates
    failure.

**************************************************************************/
bool Radio::setTransmitBandwidth(uint32_t bandwidth)
{
  bool success;

  // Invoke core function.
  success = setBandwidth(bandwidth);

  return (success);
  
} // setTransmitBandwidth

/**************************************************************************

  Name: setTransmitIfGainInDb

  Purpose: The purpose of this function is to set the gain of the IF
  amplifier in the transmitter.

  Calling Sequence: success = setTransmitIfGainInDb(gain)

  Inputs:

    gain - The gain in decibels

  Outputs:

    success - A boolean that indicates the outcome of the operation.  A
    value of true indicates success, and a value of false indicates
    failure.

**************************************************************************/
bool Radio::setTransmitIfGainInDb(uint32_t gain)

{
  bool success;
  int error;

  // Acquire the I/O subsystem lock.
  pthread_mutex_lock(&ioSubsystemLock);

  // Default to failure.
  success = false;

  if (devicePtr != 0)
  {
    // Set the gain.
    error = hackrf_set_txvga_gain((hackrf_device *)devicePtr,gain);

    if (error == HACKRF_SUCCESS)
    {
      // Update attribute.
      transmitIfGainInDb = gain;

      // indicate success.
      success = true;
    } // if
  } // if

  // Release the I/O subsystem lock.
  pthread_mutex_unlock(&ioSubsystemLock);

  return (success);
  
} // setTransmitIfGainInDb

/**************************************************************************

  Name: setTransmitSampleRate

  Purpose: The purpose of this function is to set the sample rate
  of the transmitter.

  Calling Sequence: success = setTransmitSampleRate(sampleRate)

  Inputs:

    sampleRate - The transmit sample rate in samples per second.

  Outputs:

    success - A boolean that indicates the outcome of the operation.  A
    value of true indicates success, and a value of false indicates
    failure.

**************************************************************************/
bool Radio::setTransmitSampleRate(uint32_t sampleRate)
{
  bool success;

  // Invoke the core function.
  success = setSampleRate(sampleRate);

  return (success);
  
} // setTransmitSampleRate

/**************************************************************************

  Name: setSignalDetectThreshold

  Purpose: The purpose of this function is to set the signal detect
  threshold.  A signal is considered as detected if a signal magnitude
  matches or exceeds the threshold value.

  Calling Sequence: success = setSignalDetectThreshold(threshold)

  Inputs:

    threshold - The signal detection threshold in decibels referenced
    to full scale.  Valid values are,
    -400 <= threshold <= 10.

  Outputs:

    success - A boolean that indicates the outcome of the operation.  A
    value of true indicates success, and a value of false indicates
    failure.

**************************************************************************/
bool Radio::setSignalDetectThreshold(int32_t threshold)
{
  bool success;

  // Default to failure.
  success = false;

  if ((threshold >= -400) && (threshold <= 10))
  {
    // Inform data processor of new threshold.
    receiveDataProcessorPtr->setSignalDetectThreshold(threshold);

    // Indicate success.
    success = true;
  } // if

  return (success);

} // setSignalDetectThreshold

/**************************************************************************

  Name: receiveFrontEndAmplifierIsEnabled

  Purpose: The purpose of this function is to indicate whether or not
  the receive front end amplifier is enabled or bypassed in the
  signal chain.

  Calling Sequence: enabled = receiveFrontEndAmplifierIsEnabled()

  Inputs:

    None.

  Outputs:

    enabled - A boolean that indicates whether or not the front end
    amplifier is enabled.  A value of true indicates that the amplifier
    is enabled, and a value of false indicates that the amplifier is
    disabled.

**************************************************************************/
bool Radio::receiveFrontEndAmplifierIsEnabled(void)

{

  return (receiveFrontEndAmplifierEnabled);
  
} // receiveFrontEndAmplifierIsEnabled

/**************************************************************************

  Name: transmitFrontEndAmplifierIsEnabled

  Purpose: The purpose of this function is to indicate whether or not
  the transmit front end amplifier is enabled or bypassed in the
  signal chain.

  Calling Sequence: enabled = transmitFrontEndAmplifierIsEnabled()

  Inputs:

    None.

  Outputs:

    enabled - A boolean that indicates whether or not the front end
    amplifier is enabled.  A value of true indicates that the amplifier
    is enabled, and a value of false indicates that the amplifier is
    disabled.

**************************************************************************/
bool Radio::transmitFrontEndAmplifierIsEnabled(void)

{

  return (transmitFrontEndAmplifierEnabled);
  
} // transmitFrontEndAmplifierIsEnabled

/**************************************************************************

  Name: getReceiveFrequency

  Purpose: The purpose of this function is to get the operating frequency
  of the receiver.

  Calling Sequence: frequency = getReceiveFrequency()

  Inputs:

    None.

  Outputs:

    frequency - The receiver frequency in Hertz.

**************************************************************************/
uint64_t Radio::getReceiveFrequency(void)
{

  return (receiveFrequency);
  
} // getReceiveFrequency

/**************************************************************************

  Name: getReceiveBandwidth

  Purpose: The purpose of this function is to get the baseband filter
  bandwidth of the receiver.

  Calling Sequence: bandwidth = getReceiveBandwidth()

  Inputs:

    None.

  Outputs:

    bandwidth - The receive baseband filter bandwidth in Hertz.

**************************************************************************/
uint32_t Radio::getReceiveBandwidth(void)
{
  return (receiveBandwidth);
  
} // getReceiveBandwidth

/**************************************************************************

  Name: getReceiveIfGainInDb

  Purpose: The purpose of this function is to get the IF gain of the
  receiver.

  Calling Sequence: gain = getReceiveIfGainInDb()

  Inputs:

    None.

  Outputs:

    gain - The gain in decibels.

**************************************************************************/
uint32_t Radio::getReceiveIfGainInDb(void)

{

  return (receiveIfGainInDb);
  
} // getReceiveIfGainInDb

/**************************************************************************

  Name: getReceiveBasebandGainInDb

  Purpose: The purpose of this function is to get the baseband gain of the
  receiver.

  Calling Sequence: gain = getReceiveBasebandGainInDb()

  Inputs:

    None.

  Outputs:

    gain - The gain in decibels.

**************************************************************************/
uint32_t Radio::getReceiveBasebandGainInDb(void)

{

  return (receiveBasebandGainInDb);
  
} // getReceiveBasebandGainInDb

/**************************************************************************

  Name: getReceiveSampleRate

  Purpose: The purpose of this function is to get the sample rate
  of the receiver.

  Calling Sequence: sampleRate = getReceiveSampleRate()

  Inputs:

    None.

  Outputs:

    sampleRate - The receiver sample rate in samples per second.

**************************************************************************/
uint32_t Radio::getReceiveSampleRate(void)
{

  return (receiveSampleRate);
  
} // getReceiveSampleRate

/**************************************************************************

  Name: getReceiveWarpInPartsPerMillion

  Purpose: The purpose of this function is to get the frequency warp value
  of the receiver.

  Calling Sequence: warp = getReceiveWarpInPartsPerMillion()

  Inputs:

    None.

  Outputs:

    warp - The frequency warp value in parts per million.

**************************************************************************/
int Radio::getReceiveWarpInPartsPerMillion(void)
{

  return (receiveWarpInPartsPerMillion);
  
} // getReceiveWarpInPartsPerMillion

/**************************************************************************

  Name: getTransmitFrequency

  Purpose: The purpose of this function is to get the operating frequency
  of the transmitter.

  Calling Sequence: frequency = getTransmitFrequency()

  Inputs:

    None.

  Outputs:

    frequency - The transmitter frequency in Hertz.

**************************************************************************/
uint64_t Radio::getTransmitFrequency(void)
{

  // The transmit and receive frequencies are the same.
  return (receiveFrequency);
  
} // getTransmitFrequency

/**************************************************************************

  Name: getTransmitBandwidth

  Purpose: The purpose of this function is to get the baseband filter
  bandwidth of the receiver.

  Calling Sequence: bandwidth = getTransmitBandwidth()

  Inputs:

    None.

  Outputs:

    bandwidth - The bandwidth in Hertz.

**************************************************************************/
uint32_t Radio::getTransmitBandwidth(void)
{

  // The transmit and receive bandwidths are the same.
  return (receiveBandwidth);
  
} // getTransmitBandwidth

/**************************************************************************

  Name: getTransmitIfGainInDb

  Purpose: The purpose of this function is to get the IF gain of the
  transmitter.

  Calling Sequence: gain = getTransmitIfGainInDb()

  Inputs:

    None.

  Outputs:

    gain - The gain in decibels.

**************************************************************************/
uint32_t Radio::getTransmitIfGainInDb(void)

{

  return (transmitIfGainInDb);
  
} // getTransmitIfGainInDb

/**************************************************************************

  Name: getTransmitSampleRate

  Purpose: The purpose of this function is to get the sample rate
  of the transmitter.

  Calling Sequence: sampleRate = getTransmitSampleRate()

  Inputs:

    None.

  Outputs:

    sampleRate - The transmit sample rate in samples per second.

**************************************************************************/
uint32_t Radio::getTransmitSampleRate(void)
{

  // Yhe transmit and receive sample rates are the same.
  return (receiveSampleRate);
  
} // getTransmitSampleRate

/**************************************************************************

  Name: getDataProvider

  Purpose: The purpose of this function is to get the pointer to the
  transmit data provider.

  Calling Sequence: providerPtr = getDataProvider()

  Inputs:

    None.

  Outputs:

    providerPtr - A pointer to the transmit data provider object.

**************************************************************************/
DataProvider * Radio::getDataProvider(void)
{

  return (dataProviderPtr);

} // getDataProvider

/**************************************************************************

  Name: isReceiving

  Purpose: The purpose of this function is to indicate whether or not the
  receiver is enabled.

  Calling Sequence: enabled = isReceiving()

  Inputs:

    None.

  Outputs:

    Enabled - A boolean that indicates whether or not the receiver is
    enabled.  A value of true indicates that the receiver is enabled,
    and a value of false indicates that the receiveer is disabled.

**************************************************************************/
bool  Radio::isReceiving(void)
{

  return (receiveEnabled);
  
} // isReceiving

/**************************************************************************

  Name: isTransmitting

  Purpose: The purpose of this function is to indicate whether or not the
  transmitter is enabled for the RF path of interest.

  Calling Sequence: enabled = isTransmitting()

  Inputs:

    None.

  Outputs:

    Enabled - A boolean that indicates whether or not the transmitter is
    enabled.  A value of true indicates that the transmitter is enabled,
    and a value of false indicates that the transmitter is disabled.

**************************************************************************/
bool  Radio::isTransmitting(void)
{

  return (transmitEnabled);
  
} // isTransmitting

/**************************************************************************

  Name: setDemodulatorMode

  Purpose: The purpose of this function is to set the demodulator mode.
  This mode dictates which demodulator should be used when processing IQ
  data samples.

  Calling Sequence: setDemodulatorMode(mode)

  Inputs:

    mode - The demodulator mode.  Valid values are None, Am, and Fm.

  Outputs:

    None.

**************************************************************************/
void Radio::setDemodulatorMode(uint8_t mode)
{
  IqDataProcessor::demodulatorType kludge;

  // Let's make things happy.
  kludge = static_cast<IqDataProcessor::demodulatorType>(mode);

  // Notify the IQ data processor to use this mode.
  receiveDataProcessorPtr->setDemodulatorMode(kludge);

  return;

} // setDemodulaterMode

/**************************************************************************

  Name: setModulatorMode

  Purpose: The purpose of this function is to set the modulator mode.
  This mode dictates which modulator should be used when processing
  data samples from a live stream.

  Calling Sequence: setModulatorMode(mode)

  Inputs:

    mode - The modulator mode.  Valid values are None, Am, Fm, Lsb, and Usb.

  Outputs:

    None.

**************************************************************************/
void Radio::setModulatorMode(uint8_t mode)
{
  BasebandDataProcessor::modulatorType kludge;

  // Let's make things happy.
  kludge = static_cast<BasebandDataProcessor::modulatorType>(mode);

  // Notify the IQ data processor to use this mode.
  transmitBasebandDataProcessorPtr->setModulatorMode(kludge);

  return;

} // setModulaterMode

/*****************************************************************************

  Name: setAmDemodulatorGain

  Purpose: The purpose of this function is to set the gain of the
  AM demodulator.

  Calling Sequence: setAmDemodulatorGain(gain)

  Inputs:

    gain - The demodulator gain.

  Outputs:

    None.

*****************************************************************************/
void Radio::setAmDemodulatorGain(float gain)
{

  // Notifier the AM demodulator to set its gain.
  amDemodulatorPtr->setDemodulatorGain(gain);

  return;

} // setAmDemodulatorGain

/*****************************************************************************

  Name: setFmDemodulatorGain

  Purpose: The purpose of this function is to set the gain of the
  FM demodulator.

  Calling Sequence: setFmDemodulatorGain(gain)

  Inputs:

    gain - The demodulator gain.

  Outputs:

    None.

*****************************************************************************/
void Radio::setFmDemodulatorGain(float gain)
{

  // Notifier the FM demodulator to set its gain.
  fmDemodulatorPtr->setDemodulatorGain(gain);

  return;

} // setFmDemodulatorGain

/*****************************************************************************

  Name: setWbFmDemodulatorGain

  Purpose: The purpose of this function is to set the gain of the wideband
  FM demodulator.

  Calling Sequence: setWbFmDemodulatorGain(gain)

  Inputs:

    gain - The demodulator gain.

  Outputs:

    None.

*****************************************************************************/
void Radio::setWbFmDemodulatorGain(float gain)
{

  // Notifier the FM demodulator to set its gain.
  wbFmDemodulatorPtr->setDemodulatorGain(gain);

  return;

} // setWbFmDemodulatorGain

/*****************************************************************************

  Name: setSsbDemodulatorGain

  Purpose: The purpose of this function is to set the gain of the SSB
  demodulator.

  Calling Sequence: setSsbDemodulatorGain(gain)

  Inputs:

    gain - The demodulator gain.

  Outputs:

    None.

*****************************************************************************/
void Radio::setSsbDemodulatorGain(float gain)
{

  // Notifier the SSB demodulator to set its gain.
  ssbDemodulatorPtr->setDemodulatorGain(gain);

  return;

} // setSsbDemodulatorGain

/*****************************************************************************

  Name: setAmModulationIndex

  Purpose: The purpose of this function is to set the modulaton index of the
  AM modulator.

  Calling Sequence: setAmModulation(modulationIndex);

  Inputs:

    modulationIndex - The modulation index.

  Outputs:

    None.

*****************************************************************************/
void Radio::setAmModulationIndex(float modulationIndex)
{

  // Notifier the AM demodulator to set its gain.
  amModulatorPtr->setModulationIndex(modulationIndex);

  return;

} // setAmModulationIndex

/*****************************************************************************

  Name: setFmDeviation

  Purpose: The purpose of this function is to set the frequency deviation
  of the FM modulator.

  Calling Sequence: setFmDeviation(deviation)

  Inputs:

    deviation - The frequency deviation.

  Outputs:

    None.

*****************************************************************************/
void Radio::setFmDeviation(float deviation)
{

  // Notifier the FM demodulator to set its gain.
  fmModulatorPtr->setFrequencyDeviation(deviation);

  return;

} // setFmDeviation

/*****************************************************************************

  Name: setWbFmDeviation

  Purpose: The purpose of this function is to set the frequency deviation
  of the wideband FM modulator.

  Calling Sequence: setWbFmDeviation(deviation)

  Inputs:

    deviation - The frequency deviation.

  Outputs:

    None.

*****************************************************************************/
void Radio::setWbFmDeviation(float deviation)
{

  // Notifier the FM demodulator to set its gain.
  wbFmModulatorPtr->setFrequencyDeviation(deviation);

  return;

} // setWbFmDeviation

/*****************************************************************************

  Name: getIqProcessor

  Purpose: The purpose of this function is to retrieve the address of the
  IqDataProcessor object.

  Calling Sequence: dataProcessorPtr = getIqProcessor()

  Inputs:

    None.

  Outputs:

    dataProcessorPtr - A pointer to an instance of an IqDataProcessor
    that is used by the Radio.

*****************************************************************************/
void * Radio::getIqProcessor(void)
{

  return (receiveDataProcessorPtr);

} // getIqProcessorPtr

/**************************************************************************

  Name: setAgcType

  Purpose: The purpose of this function is to set the type of AGC
  algorithm to be used.

  Calling Sequence: success = setAgcType(type)

  Inputs:

    type - The type of AGC.  A value of 0 indicates that the lowpass AGC
    is to be used, and a value of 1 indicates that the Harris AGC is to
    be used.

  Outputs:

    success - A flag that indicates whether the the AGC type was set.
    A value of true indicates that the AGC type was successfully set,
    and a value of false indicates that the AGC type was not set due
    to an invalid value for the AGC type was specified.

**************************************************************************/
bool Radio::setAgcType(uint32_t type)
{
  bool success;

  // Update the AGC type.
  success = agcPtr->setType(type);

  return (success);

} // setAgcType

/**************************************************************************

  Name: setAgcDeadband

  Purpose: The purpose of this function is to set the deadband of the
  AGC.  This presents gain setting oscillations

  Calling Sequence: success = uint32_t deadband(deadbandInDb)

  Inputs:

    deadbandInDb - The deadband, in decibels, used to prevent unwanted
    gain setting oscillations.  A window is created such that a gain
    adjustment will not occur if the current gain is within the window
    new gain <= current gain += deadband.

  Outputs:

    success - A flag that indicates whether or not the deadband parameter
    was updated.  A value of true indicates that the deadband was
    updated, and a value of false indicates that the parameter was not
    updated due to an invalid specified deadband value.

**************************************************************************/
bool  Radio::setAgcDeadband(uint32_t deadbandInDb)
{
  bool success;

  // Update the AGC deadband.
  success = agcPtr->setDeadband(deadbandInDb);

  return (success);

} // setAgcDeadband

/**************************************************************************

  Name: setBlankingLimit

  Purpose: The purpose of this function is to set the blanking limit of
  the AGC.  This presents gain setting oscillations.  What happens if
  transients exist after an adjustment is made is that the AGC becomes
  blind to the actual signal that is being received since the transient
  can swamp the system.

  Calling Sequence: success = setBlankingLimit(blankingLimit)

  Inputs:

    blankingLimit - The number of measurements to ignore before making
    the next gain adjustment.

  Outputs:

    success - A flag that indicates whether or not the blanking limit
    parameter was updated.  A value of true indicates that the blanking
    limit was updated, and a value of false indicates that the parameter
    was not updated due to an invalid specified blanking limit value.

**************************************************************************/
bool  Radio::setAgcBlankingLimit(uint32_t blankingLimit)
{
  bool success;

  // Set the blank counter limit to avoid transients.
  success = agcPtr->setBlankingLimit(blankingLimit);

  return (success);

} // setAgcBlankingLimit

/**************************************************************************

  Name: setAgcOperatingPoint

  Purpose: The purpose of this function is to set the operating point
  of the AGC.

  Calling Sequence: setAgcOperatingPoint(operatingPointInDbFs)
  Inputs:

    operatingPointInDbFs - The operating point in decibels referenced to
    the full scale value.

  Outputs:

    None.

**************************************************************************/
void Radio::setAgcOperatingPoint(int32_t operatingPointInDbFs)
{

  // Update operating point.
  agcPtr->setOperatingPoint(operatingPointInDbFs);

  return;

} // setAgcOperatingPoint

/**************************************************************************

  Name: setAgcFilterCoefficient

  Purpose: The purpose of this function is to set the coefficient of
  the first order lowpass filter that filters the baseband gain value.
  In effect, the time constant of the filter is set.

  Calling Sequence: success = setAgcFilterCoefficient(coefficient)

  Inputs:

    coefficient - The filter coefficient for the lowpass filter that
    // filters the baseband gain value.

  Outputs:

    success - A flag that indicates whether the filter coefficient was
    updated.  A value of true indicates that the coefficient was
    updated, and a value of false indicates that the coefficient was
    not updated due to an invalid coefficient value.

**************************************************************************/
bool  Radio::setAgcFilterCoefficient(float coefficient)
{
  bool success;

  // Update the lowpass filter coefficient.
  success = agcPtr->setAgcFilterCoefficient(coefficient);

  return (success);

} // setAgcFilterCoefficient

/**************************************************************************

  Name: enableAgc

  Purpose: The purpose of this function is to enable the AGC.

  Calling Sequence: success = enableAgc();

  Inputs:

    None.

  Outputs:

    success - A flag that indicates whether or not the operation was
    successful.  A value of true indicates that the operation was
    successful, and a value of false indicates that the AGC was already
    enabled.

**************************************************************************/
bool Radio::enableAgc(void)
{
  bool success;

  // Enable the AGC.
  success = agcPtr->enable();

  return (success);

} // enableAgc

/**************************************************************************

  Name: disableAgc

  Purpose: The purpose of this function is to disable the AGC.

  Calling Sequence: success = disableAgc();

  Inputs:

    None.

  Outputs:

    success - A flag that indicates whether or not the operation was
    successful.  A value of true indicates that the operation was
    successful, and a value of false indicates that the AGC was already
    disabled.

**************************************************************************/
bool Radio::disableAgc(void)
{
  bool success;

  // Disable the AGC.
  success = agcPtr->disable();

  return (success);

} // disableAgc

/**************************************************************************

  Name: isAgcEnabled

  Purpose: The purpose of this function is to determine whether or not
  the AGC is enabled.

  Calling Sequence: status = isAgcEnabled()

  Inputs:

    None.

  Outputs:

    success - A flag that indicates whether or not the AGC is enabled.
    A value of true indicates that the AGC is enabled, and a value of
    false indicates that the AGC is disabled.

**************************************************************************/
bool Radio::isAgcEnabled(void)
{

  return (agcPtr->isEnabled());

} // isAgcEnabled

/**************************************************************************

  Name: displayAgcInternalInformation

  Purpose: The purpose of this function is to display information in the
  AGC.

  Calling Sequence: displayAgcInternalInformation()

  Inputs:

    None.

  Outputs:

    None.

**************************************************************************/
void Radio::displayAgcInternalInformation(void)
{

  agcPtr->displayInternalInformation();

  return;

} // displayAgcInternalInformation

/**************************************************************************

  Name: displayInternalInformation

  Purpose: The purpose of this function is to display internal
  in the radio.

  Calling Sequence: displayInternalInformation()

  Inputs:

    None.

  Outputs:

    None.

**************************************************************************/
void Radio::displayInternalInformation(void)
{

  // Display top level radio information.
  nprintf(stderr,"\n------------------------------------------------------\n");
  nprintf(stderr,"Radio Internal Information\n");
  nprintf(stderr,"------------------------------------------------------\n");

  nprintf(stderr,"Information Source                  : ");  

  switch (informationSource)
  {
    case File:
    {
      nprintf(stderr,"File\n");
      break;
    } // case

    case Live:
    {
      nprintf(stderr,"Live\n");
      break;
    } // case

    default:
    {
      break;
    } // case
  } // switch

  nprintf(stderr,"Receive Enabled                     : ");
  if (receiveEnabled)
  {
    nprintf(stderr,"Yes\n");
  } // if
  else
  {
     nprintf(stderr,"No\n");
  } // else

  nprintf(stderr,"Receive Front End Amp Enabled       : ");
  if (receiveFrontEndAmplifierEnabled)
  {
    nprintf(stderr,"Yes\n");
  } // if
  else
  {
     nprintf(stderr,"No\n");
  } // else

  nprintf(stderr,"Receive IF Gain:                    : %u dB\n",
          receiveIfGainInDb);
  nprintf(stderr,"Receive Baseband Gain:              : %u dB\n",
          receiveBasebandGainInDb);
  nprintf(stderr,"Receive Frequency                   : %llu Hz\n",
          receiveFrequency);
  nprintf(stderr,"Receive Bandwidth                   : %u Hz\n",
          receiveBandwidth);
  nprintf(stderr,"Receive Sample Rate:                : %u S/s\n",
          receiveSampleRate);
  nprintf(stderr,"Receive Frequency Warp:             : %d ppm\n",
          receiveWarpInPartsPerMillion);
  nprintf(stderr,"Receive Timestamp                   : %u\n",
          receiveTimeStamp);
  nprintf(stderr,"Receive Block Count                 : %u\n",
          receiveBlockCount);

  nprintf(stderr,"Transmit Enabled                    : ");
  if (transmitEnabled)
  {
    nprintf(stderr,"Yes\n");
  } // if
  else
  {
     nprintf(stderr,"No\n");
   } // else

  nprintf(stderr,"Transmit Front End Amp Enabled      : ");
  if (transmitFrontEndAmplifierEnabled)
  {
    nprintf(stderr,"Yes\n");
  } // if
  else
  {
     nprintf(stderr,"No\n");
  } // else

  nprintf(stderr,"Transmit IF Gain:                   : %d dB\n",
          (unsigned int)transmitIfGainInDb);
  nprintf(stderr,"Transmit Frequency                  : %llu Hz\n",
          receiveFrequency);
  nprintf(stderr,"Transmit Bandwidth                  : %u Hz\n",
          receiveBandwidth);
  nprintf(stderr,"Transmit Sample Rate                : %u S/s\n",
          receiveSampleRate);
  nprintf(stderr,"Transmit Frequency Warp:            : %d ppm\n",
          receiveWarpInPartsPerMillion);
 
  nprintf(stderr,"Transmit Block Count                : %u\n",
          transmitBlockCount);

  // Display data consumer information.
  dataConsumerPtr->displayInternalInformation();

  // Display data provider information.
  dataProviderPtr->displayInternalInformation();

  // Display IQ data processor information.
  receiveDataProcessorPtr->displayInternalInformation();

  // Display baseband data processor information.
  transmitBasebandDataProcessorPtr->displayInternalInformation();

  // Display AM demodulator information to the user.
  amDemodulatorPtr->displayInternalInformation();

  // Display FM demodulator information to the user.
  fmDemodulatorPtr->displayInternalInformation();

  // Display wideband FM demodulator information to the user.
  wbFmDemodulatorPtr->displayInternalInformation();

  // Display wideband SSB demodulator information to the user.
  ssbDemodulatorPtr->displayInternalInformation();

  // Display AM modulator information to the user.
  amModulatorPtr->displayInternalInformation();

  // Display FM modulator information to the user.
  fmModulatorPtr->displayInternalInformation();

  // Display wideband FM modulator information to the user.
  wbFmModulatorPtr->displayInternalInformation();

  // Display SSB modulator information to the user.
  ssbModulatorPtr->displayInternalInformation();

  return;

} // displayInternalInformation

/*****************************************************************************

  Name: receiveCallbackProcedure

  Purpose: The purpose of this function is to handle receiver data that is
  supplied by the underlying system software.

  Calling Sequence: status - receiveCallbackProcedure(transferPtr)

  Inputs:

    transferPtr - A pointer to a hackrf_transfer structure.  The fields of
    interest are:
      1. buffer - A pointer to the received data.
      2. valid_length - The number of bytes available in the buffer.

  Outputs:

    status - The processing outcome.  A value of 0 indicates success, and a
    value of -1 indicates failure.

*****************************************************************************/
int Radio::receiveCallbackProcedure(hackrf_transfer *transferPtr)
{
  int status;

  // Default to failure.
  status = -1;

  if (mePtr->receiveEnabled)
  {
   // Indicate that more samples have been received.
    mePtr->receiveTimeStamp += (uint32_t )(transferPtr->valid_length >> 1);

    // Send the IQ data to the data consumer system.
    mePtr->dataConsumerPtr->acceptData(mePtr->receiveTimeStamp,
                                       transferPtr->buffer,
                                       transferPtr->valid_length);

    // Indicate that one more block of data has been received.
    mePtr->receiveBlockCount++;

    // Indicate success.
    status = 0;
  } // if

  return (status);

} // receiveCallbackProcedure

/*****************************************************************************

  Name: transmitCallbackProcedure

  Purpose: The purpose of this function is to handle receiver data that is
  supplied by the underlying system software.  This function chooses the
  information stream to provide to the underlying software.  If the source
  is of type File, the information will be provided by a file that was loaded
  into memory.  If the source is Live, the information will be provided by
  the transmit baseband data processor.

  Calling Sequence: status - transmitCallbackProcedure(transferPtr)

  Inputs:

    transferPtr - A pointer to a hackrf_transfer structure.  The fields of
    interest are:
      1. buffer - A pointer to the received data.
      2. valid_length - The number of bytes desired to be transferred to
      the buffer.

  Outputs:

    status - The processing outcome.  A value of 0 indicates success, and a
    value of -1 indicates failure.

*****************************************************************************/
int Radio::transmitCallbackProcedure(hackrf_transfer *transferPtr)
{
  int status;
  int8_t *bufferPtr;

  // Default to failure.
  status = -1;

  // Reference the buffer in the appropriate context.
  bufferPtr = (int8_t *)transferPtr->buffer;

  if (mePtr->transmitEnabled)
  {
    // Indicate that more samples have been received.
    mePtr->transmitTimeStamp += (uint32_t )(transferPtr->valid_length >> 1);

    switch (mePtr->informationSource)
    {
      case File: // Information source originated from a file.
      {
        // Retrieve IQ data from the data provider system.
        mePtr->dataProviderPtr->getIqData(bufferPtr,transferPtr->valid_length);
        break;
      } // case

      case Live: // Information source is a live data stream.
      {
        // Retrieve IQ data address from the baseband data processor system.
          mePtr->transmitBasebandDataProcessorPtr->getIqData(
            bufferPtr,
            transferPtr->valid_length);          
        break;
      } // case

      default: // This should never occur.
      {
        // Generate a signal with zero amplitude.
        memset(transferPtr->buffer,0,transferPtr->valid_length);
        break;
      } // case
    } // switch

    // Indicate that one more block of data has been received.
    mePtr->transmitBlockCount++;

    // Indicate success.
    status = 0;
  } // if

  return (status);

} // transmitCallbackProcedure

