//**********************************************************************
// file name: IqDataProcessor.h
//**********************************************************************

#ifndef _IQDATAPROCESSOR_H_
#define _IQDATAPROCESSOR_H_

#include <stdint.h>
#include "Decimator_int16.h"
#include "AmDemodulator.h"
#include "FmDemodulator.h"
#include "WbFmDemodulator.h"
#include "SsbDemodulator.h"
#include "SignalTracker.h"

class IqDataProcessor
{
  public:

  enum demodulatorType {None=0, Am=1, Fm=2, WbFm = 3, Lsb = 4, Usb = 5};

  IqDataProcessor(void);
  ~IqDataProcessor(void);

  void setDemodulatorMode(demodulatorType mode);
  void setAmDemodulator(AmDemodulator *demodulatorPtr);
  void setFmDemodulator(FmDemodulator *demodulatorPtr);
  void setWbFmDemodulator(WbFmDemodulator *demodulatorPtr);
  void setSsbDemodulator(SsbDemodulator *demodulatorPtr);
  void setSignalDetectThreshold(uint32_t threshold);
  uint32_t reduceSampleRate(int8_t *bufferPtr,uint32_t bufferLength);

  void acceptIqData(unsigned long timeStamp,
                    int8_t *bufferPtr,
                    unsigned long byteCount);

  void enableSignalNotification(void);
  void disableSignalNotification(void);

  void registerSignalStateCallback(
      void (*signalCallbackPtr)(bool signalPresent,
                                void *contextPtr),
      void *contextPtr);

  void enableSignalMagnitudeNotification(void);
  void disableSignalMagnitudeNotification(void);

  void registerSignalMagnitudeCallback(
      void (*callbackPtr)(uint32_t signalMagnitude,void *contextPtr),
      void *contextPtr);

  void displayInternalInformation(void);

private:

  // Attributes.

  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
  // This set of decimators is used  to provide a decimation by 8.  Each
  // decimator will perform a decimate by 2, and the coefficients will
  // realize a half band filter.  The first decimator reduces the sample
  // rate from 2048000S/s to 1024000S/s.  The second decimator reduces the
  // sample rate from 1024000S/s to 512000S/s.  The third decimator reduces
  // the sample rate from 512000S/s to the final sample rate of 256000S/s.
  // Note that what really have are pairs of decimators so that the in-phase
  // and quadrature components of the signal can be decimated.
  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
  Decimator_int16 *stage1IDecimatorPtr;
  Decimator_int16 *stage1QDecimatorPtr;
  Decimator_int16 *stage2IDecimatorPtr;
  Decimator_int16 *stage2QDecimatorPtr;
  Decimator_int16 *stage3IDecimatorPtr;
  Decimator_int16 *stage3QDecimatorPtr;

  // This buffer contains interleaved I and Q samples.
  int8_t decimatedData[32768];

  // The demodulator operating mode.
  demodulatorType demodulatorMode;

  // This is used for the squelch system.
  uint32_t signalDetectThreshold;

  // Demodulator support.
  AmDemodulator *amDemodulatorPtr;
  FmDemodulator *fmDemodulatorPtr;
  WbFmDemodulator *wbFmDemodulatorPtr;
  SsbDemodulator *ssbDemodulatorPtr;

  // Squelch support.
  SignalTracker *trackerPtr;

  // Signal notification support.
  bool signalNotificationEnabled;
  void *signalCallbackContextPtr;
  void (*signalCallbackPtr)(bool signalPresent,void *contextPtr);

  // Signal magnitude notification support.
  bool signalMagnitudeNotificationEnabled;
  void *signalMagnitudeCallbackContextPtr;
  void (*signalMagnitudeCallbackPtr)(uint32_t signalMagnitude,
                                     void *contextPtr);
};

#endif // _IQDATAPROCESSOR_H_
