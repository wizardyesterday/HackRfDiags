//**********************************************************************
// file name: Radio.h
//**********************************************************************

#ifndef _RADIO_H_
#define _RADIO_H_

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <math.h>

#include "IqDataProcessor.h"
#include "DataConsumer.h"
#include "DataProvider.h"
#include "AmDemodulator.h"
#include "FmDemodulator.h"
#include "WbFmDemodulator.h"
#include "SsbDemodulator.h"

#include "AmModulator.h"
#include "FmModulator.h"
#include "WbFmModulator.h"
#include "SsbModulator.h"
#include "BasebandDataProcessor.h"
#include "AutomaticGainControl.h"

#include <hackrf.h>

class Radio
{
  public:

  enum informationSourceType {File, Live};

  Radio(uint32_t txSampleRate,uint32_t rxSampleRate,
        char *hostIpAddress,int hostPort,
        void (*pcmCallbackPtr)(int16_t *bufferPtr,uint32_t bufferLength));

  ~Radio(void);

  // Setters.
  void selectFileSource(void);
  void selectLiveSource(void);
  bool setFrequency(uint64_t frequency);
  bool setReceiveBandwidth(uint32_t bandwidth);
  bool setReceiveSampleRate(uint32_t sampleRate);
  bool setReceiveWarpInPartsPerMillion(int warp);
  bool setTransmitBandwidth(uint32_t bandwidth);
  bool setTransmitSampleRate(uint32_t sampleRate);
  bool setSignalDetectThreshold(int32_t threshold);

  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
  // New HackRF methods.
  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
  bool enableReceiveFrontEndAmplifier(void);
  bool disableReceiveFrontEndAmplifier(void);
  bool enableTransmitFrontEndAmplifier(void);
  bool disableTransmitFrontEndAmplifier(void);
  bool setReceiveIfGainInDb(uint32_t gain);
  bool setReceiveBasebandGainInDb(uint32_t gain);
  bool setTransmitIfGainInDb(uint32_t gain);
  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/

  // Getters.
  uint64_t getReceiveFrequency(void);
  uint32_t getReceiveBandwidth(void);
  uint32_t getReceiveSampleRate(void);
  int getReceiveWarpInPartsPerMillion(void);
  uint64_t getTransmitFrequency(void);
  uint32_t getTransmitBandwidth(void);
  uint32_t getTransmitSampleRate(void);

  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
  // New HackRF methods.
  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
  bool receiveFrontEndAmplifierIsEnabled(void); // New HackRf stuff.
  bool transmitFrontEndAmplifierIsEnabled(void); // New HackRf stuff.
  uint32_t getReceiveIfGainInDb(void); // New HackRf stuff.
  uint32_t getReceiveBasebandGainInDb(void); // New HackRf stuff.
  uint32_t getTransmitIfGainInDb(void); // New HackRf stuff.
  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/

  DataProvider *getDataProvider();

  bool isReceiving(void);
  bool isTransmitting(void);
  bool startReceiver(void);
  bool stopReceiver(void);
  bool startTransmitter(void);
  bool stopTransmitter(void);
  void startLiveStream(void);
  void stopLiveStream(void);

  // Demodulator support.
  void setDemodulatorMode(uint8_t mode);
  void setAmDemodulatorGain(float gain);
  void setFmDemodulatorGain(float gain);
  void setWbFmDemodulatorGain(float gain);
  void setSsbDemodulatorGain(float gain);

  // Modulator support.
  void setModulatorMode(uint8_t mode);
  void setAmModulationIndex(float modulationIndex);
  void setFmDeviation(float deviation);
  void setWbFmDeviation(float deviation);

  void *getIqProcessor(void);
  void enableIqDump(void);
  void disableIqDump(void);
  bool isIqDumpEnabled(void);

  // AGC support.
  bool setAgcType(uint32_t type);
  bool setAgcDeadband(uint32_t deadbandInDb);
  bool setAgcBlankingLimit(uint32_t blankingLimit);
  void setAgcOperatingPoint(int32_t operatingPointInDbFs);
  bool setAgcFilterCoefficient(float coefficient);
  bool enableAgc(void);
  bool disableAgc(void);
  bool isAgcEnabled(void);
  void displayAgcInternalInformation(void);

  void displayInternalInformation(void);

  private:

  // Utility functions.
  bool setupReceiver(void);
  bool setupTransmitter(void);
  void tearDownReceiver(void);
  void tearDownTransmitter(void);

  // Core functions used by the interface functions.
  bool setReceiveFrequency(uint64_t frequency);
  bool setTransmitFrequency(uint64_t frequency);

  bool setBandwidth(uint32_t bandwidth);
  bool setSampleRate(uint32_t sampleRate);
  bool setWarpInPartsPerMillion(int warp);

  static int receiveCallbackProcedure(hackrf_transfer *transferPtr);
  static int transmitCallbackProcedure(hackrf_transfer *transferPtr);

  // Attributes.

  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
  // Receiver configuration.  Note that, although parameters
  // for frequency, sample rate, and bandwidth are shown
  // separately for receive and transmit cased, the HackRf
  // applies these parameters for both, receive and transmit,
  // mode of operation. That is, there exists only one signal
  // processing lineup for both receive and transmit.  One
  // might ask, why didn't I change the attributes to frequency,
  // sample rate, bandwidth, and warp in parts per million?
  // I wanted to minimize the changes to the implementation of
  // the Radio class, and additionally, porting to a different
  // platform that *does* have separate configurable parameters
  // for the transmitter and receiver, will be much easier (it's
  // easier to change already existing code than to add code).
  // Chris G. 01/04/2018.
  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
  uint64_t receiveFrequency;
  uint32_t receiveSampleRate;
  uint32_t receiveBandwidth;
  int receiveWarpInPartsPerMillion;

  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
  // Transmitter configuration.
  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
  uint32_t transmitBandwidth;
  uint64_t transmitFrequency;
  uint32_t transmitSampleRate;

  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
  // New HackRF attributes.
  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
  // Common Radio Configuration.
  bool receiveFrontEndAmplifierEnabled;
  bool transmitFrontEndAmplifierEnabled;

  // Receiver Configuration.
  uint32_t receiveIfGainInDb;
  uint32_t receiveBasebandGainInDb;

  // Transmitter Confguration.
  uint32_t transmitIfGainInDb;
  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/

  // HackRf device support.
  void *devicePtr;

  // Information source support.
  informationSourceType informationSource;

  // Data handler support.
  IqDataProcessor *receiveDataProcessorPtr;
  BasebandDataProcessor *transmitBasebandDataProcessorPtr;
  DataConsumer *dataConsumerPtr;
  DataProvider *dataProviderPtr;

  // Demodulator support.
  AmDemodulator *amDemodulatorPtr;
  FmDemodulator *fmDemodulatorPtr;
  WbFmDemodulator *wbFmDemodulatorPtr;
  SsbDemodulator *ssbDemodulatorPtr;

  // Modulator support.
  AmModulator *amModulatorPtr;
  FmModulator *fmModulatorPtr;
  WbFmModulator *wbFmModulatorPtr;
  SsbModulator *ssbModulatorPtr;

  // Automatic gain control support.
  AutomaticGainControl *agcPtr;

  // Control information.
  bool receiveEnabled;
  bool transmitEnabled;
  bool timeToExit;

  // Radio callback support.
  static Radio *mePtr;

  // Multi-thread support.
  pthread_mutex_t ioSubsystemLock;

  // Statistics.
  uint32_t receiveTimeStamp;
  uint32_t receiveBlockCount;
  uint32_t transmitTimeStamp;
  uint32_t transmitBlockCount;
};

#endif // _RADIO_H_
