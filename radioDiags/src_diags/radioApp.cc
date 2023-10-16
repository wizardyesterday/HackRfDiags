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

#define DEFAULT_HOST_IP_ADDRESS "192.93.16.87"
#define DEFAULT_HOST_PORT (8001)

// This structure is used to consolidate user parameters.
struct MyParameters
{
  char *hostIpAddressPtr;
  int *hostPortPtr;
};

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
// ./radioApp -a hostIpAddress -p hostPort
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

/*****************************************************************************

  Name: getUserArguments

  Purpose: The purpose of this function is to retrieve the user arguments
  that were passed to the program.  Any arguments that are specified are
  set to reasonable default values.

  Calling Sequence: exitProgram = getUserArguments(parameters)

  Inputs:

    parameters - A structure that contains pointers to the user parameters.

  Outputs:

    exitProgram - A flag that indicates whether or not the program should
    be exited.  A value of true indicates to exit the program, and a value
    of false indicates that the program should not be exited..

*****************************************************************************/
bool getUserArguments(int argc,char **argv,struct MyParameters parameters)
{
  bool exitProgram;
  bool done;
  int i;
  int opt;

  // Default not to exit program.
  exitProgram = false;

  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
  // Default parameters.
  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
  // The host IP address.
  strcpy(parameters.hostIpAddressPtr,DEFAULT_HOST_IP_ADDRESS);
 
  // The host listener port.
  *parameters.hostPortPtr = DEFAULT_HOST_PORT;
  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/

  // Set up for loop entry.
  done = false;

  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
  // Retrieve the command line arguments.
  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
  while (!done)
  {
    // Retrieve the next option.
    opt = getopt(argc,argv,"a:p:h");

    switch (opt)
    {
      case 'a':
      {
        // Retrieve the IP address.        
        strcpy(parameters.hostIpAddressPtr,optarg);
        break;
      } // case

      case 'p':
      {
        // Retrieve the host listener port.
        *parameters.hostPortPtr = atoi(optarg);
        break;
      } // case

      case 'h':
      {
        // Display usage.
        fprintf(stderr,"./radioDiags -a <hostIpAddress: x.x.x.x> "
                "-p <hostPort>\n");

        // Indicate that program must be exited.
        exitProgram = true;
        break;
      } // case

      case -1:
      {
        // All options consumed, so bail out.
        done = true;
      } // case

    } // switch

  } // while
  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/

  return (exitProgram);

} // getUserArguments

//***********************************************************************
// Mainline code.
//***********************************************************************
int main(int argc,char **argv)
{
  bool success;
  bool exitProgram;
  uint64_t transceiveFrequency;
  uint32_t sampleRate;
  struct timeval timeout;
  int hostPort;
  char hostIpAddress[32];
  struct MyParameters parameters;

  // Set up for parameter transmission.
  parameters.hostPortPtr = &hostPort;
  parameters.hostIpAddressPtr = hostIpAddress;

  // Retrieve the system parameters.
  exitProgram = getUserArguments(argc,argv,parameters);

  if (exitProgram)
  {
    // Bail out.
    return (0);
  } // if

  transceiveFrequency = 162550000;
  sampleRate = 2048000;

  // Instantiate a radio.
  diagUi_radioPtr = new Radio(0,
                              sampleRate,
                              hostIpAddress,
                              hostPort,
                              processPcmData);

  // Set the desired frequency.
  diagUi_radioPtr->setTransmitFrequency(transceiveFrequency);
  diagUi_radioPtr->setReceiveFrequency(transceiveFrequency);

  // Instantiate a frequency scanner.
  diagUi_frequencyScannerPtr = new FrequencyScanner(diagUi_radioPtr);

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

