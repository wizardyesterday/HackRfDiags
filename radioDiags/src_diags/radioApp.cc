/************************************************************************/
/* file name: main.cc                                                   */
/************************************************************************/
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <pthread.h>
#include <math.h>
#include <errno.h>
#include <strings.h>
#include <unistd.h>

#include "Radio.h"
#include "FrequencyScanner.h"
#include "FrequencySweeper.h"
#include "diagUi.h"

#define ENGINEERING_CONSOLE_PORT (20300)
extern bool diagUi_timeToExit;

//_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
// All of our object pointers are defined here to make things simple.
//_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
Radio *diagUi_radioPtr;
FrequencyScanner *diagUi_frequencyScannerPtr;
FrequencySweeper *diagUi_frequencySweeperPtr;
//_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/

//*************************************************************************
// This program serves as a template for writing radio software that
// utilizes the radio services provided by the underlying system.
// Additionally, this program portrays the used of the diagnostic user
// interface system.  A call to select() is used so that the application
// is nice to the system.  The variable diagUi_timeToExit
// has a value of true when the program is allowed to run.  If, for some
// reason, it is time to exit the program, the variable is set to a value
// of false.
//
// It should be noted that after invoking diagUi_start(), a network
// listener thread will be launched by the user interface subsystem.
// When a user established a connection to the diag system, an additional
// thread will be launched so that the command line interpretor can run.
// Thus, a total of two threads will be running when a user is connected
// to the diag interface.  If a user is not connected to the diag
// interface, only one additional thread will be running -- the network
// listener thread.
//
// When diagUi_start() is invoked, a TCP listener thread will be launched
// that waits for a network connection.  When the user connects to the
// user interface subsystem they would connect to port 20240 for this test
// app, although a different port can be used by changing the define,
// ENGINEERING_CONSOLE_PORT, to a diffent value.
// The user interface subsystem is netcentric so that the user can connect
// to the user interface from any node that is on the same network that
// this application is running on.
//
// The user interface system has been written so that it can run on any
// Linux system, and it is hoped that other users will incorporate it into
// embedded systems of their choice.  What one gains from having this
// system is a means to interact with a device on a network connection
// that is different from any command and control or data links that are
// active on the device.
//
// To run this program, type,
//
// ./radioApp,
//
//
// Anyway, enjoy! Chris G.06/06/2017
//*************************************************************************
/*****************************************************************************

  Name: processPcmData

  Purpose: The purpose of this function is to serve as the callback
  function to accept PCM data from a demodulator.

  Calling Sequence: processPcmData(bufferPtr,bufferLength).

  Inputs:

    bufferPtr - A pointer to a buffer of PCM samples.

    bufferLength - The number of PCM samples in the buffer.

  Outputs:

    None.

*****************************************************************************/
static void processPcmData(int16_t *bufferPtr,uint32_t bufferLength)
{

  // Send the PCM samples to stdout for now.
  fwrite(bufferPtr,2,bufferLength,stdout);

  return;

} // processPcmData

//***********************************************************************
// Mainline code.
//***********************************************************************
int main(int argc,char **argv)
{
  bool success;
  uint64_t transceiveFrequency;
  uint32_t sampleRate;
  struct timeval timeout;

  transceiveFrequency = 162550000;
  sampleRate = 2048000;

  // Instantiate a radio.
  diagUi_radioPtr = new Radio(0,sampleRate,processPcmData);

  // Set the desired frequency.
  diagUi_radioPtr->setTransmitFrequency(transceiveFrequency);
  diagUi_radioPtr->setReceiveFrequency(transceiveFrequency);

  // Indicate that no frequency scanner has been allocated.
  diagUi_frequencyScannerPtr = 0;

  // Indicate that no frequency sweeper has been allocated.
  diagUi_frequencySweeperPtr = 0;

  // Start the user interface subsystem.
  diagUi_start(ENGINEERING_CONSOLE_PORT);

  // Execute this loop until the user decides to exit the system.
  while (!diagUi_timeToExit)
  {
    // set up timeout value to 5000 microseconds
    timeout.tv_sec = 0;
    timeout.tv_usec = 5000;

    // wait and be nice to the system
    select(0,0,0,0,&timeout);
  } // while

  // Stop the user interface subsystem.
  diagUi_stop();

  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
  // Release resources.
  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
  if (diagUi_frequencyScannerPtr != 0)
  {
    delete diagUi_frequencyScannerPtr;
  } // if

  if (diagUi_frequencySweeperPtr != 0)
  {
    delete diagUi_frequencySweeperPtr;
  } // if

  if (diagUi_radioPtr != 0)
  {
    delete diagUi_radioPtr;
  } // if
  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/

  return (0);

} // main

