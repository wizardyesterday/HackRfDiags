/************************************************************************/
/* file name: diagUi.cc                                                 */
/************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdint.h>
#include <unistd.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ctype.h>
#include <execinfo.h>
#include <iostream>

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <errno.h>

#include <sys/select.h>
#include <sys/time.h>

#include "Radio.h"
#include "FrequencyScanner.h"
#include "FrequencySweeper.h"
#include "AutomaticGainControl.h"
#include "diagUi.h"
#include "console.h"

using namespace std;

extern Radio *diagUi_radioPtr;
extern FrequencyScanner *diagUi_frequencyScannerPtr;
extern FrequencySweeper *diagUi_frequencySweeperPtr;

/************************************************************************/
/* general defines                                                      */
/************************************************************************/

/************************************************************************/
/* structure definitions                                                */
/************************************************************************/
/* structure of entry in the user interface command table */
typedef struct _commandEntry
{
  const char *firstTokenPtr;            /* pointer to the command */
  const char *secondTokenPtr;           /* pointer to the subcommand */
  void (*handlerPtr)(char *bufferPtr); /* pointer to handler */
} commandEntry;

/************************************************************************/
/* global variables                                                     */
/************************************************************************/
static char commandBuffer[200];
static char lastCommandBuffer[200];
bool diagUi_timeToExit;
static pthread_t cliServerThread;

static console *consolePtr;
static bool consoleConnected;
static int connectFileDescriptor;
static int diagUiNetworkPort;

/**************************************************************************/
/* function prototypes                                                    */
/**************************************************************************/
/* command interpreter functions */
static void decodeCommand(char *commandBufferPtr);
static int newstrcmp(const char *string1Ptr,const char *string2Ptr);

static void getCommandTokens(char *bufferPtr,
                             char *firstTokenPtr,
                             char *secondTokenPtr,
                             char **parameterPtr);

/* command interpreter handlers */
static void cmdSelectFileSource(char *bufferPtr);
static void cmdSelectLiveSource(char *bufferPtr);
static void cmdSetDemodMode(char *bufferPtr);
static void cmdSetModMode(char *bufferPtr);
static void cmdSetAmDemodGain(char *bufferPtr);
static void cmdSetFmDemodGain(char *bufferPtr);
static void cmdSetWbFmDemodGain(char *bufferPtr);
static void cmdSetSsbDemodGain(char *bufferPtr);
static void cmdSetAmModIndex(char *bufferPtr);
static void cmdSetFmModDeviation(char *bufferPtr);
static void cmdSetWbFmModDeviation(char *bufferPtr);
static void cmdEnableRxFrontendAmp(char *bufferPtr);
static void cmdDisableRxFrontendAmp(char *bufferPtr);
static void cmdEnableAgc(char *bufferPtr);
static void cmdDisableAgc(char *bufferPtr);
static void cmdSetAgcType(char *bufferPtr);
static void cmdSetAgcDeadband(char *bufferPtr);
static void cmdSetAgcBlank(char *bufferPtr);
static void cmdSetAgcAlpha(char *bufferPtr);
static void cmdSetAgcLevel(char *bufferPtr);
static void cmdGetAgcInfo(char *buffeerPtr);
static void cmdEnableTxFrontendAmp(char *bufferPtr);
static void cmdDisableTxFrontendAmp(char *bufferPtr);
static void cmdSetTxIfGain(char *bufferPtr);
static void cmdSetRxIfGain(char *bufferPtr);
static void cmdSetRxBasebandGain(char *bufferPtr);
static void cmdSetFrequency(char *bufferPtr);
static void cmdSetRxBandwidth(char *bufferPtr);
static void cmdSetRxSampleRate(char *bufferPtr);
static void cmdSetRxWarp(char *bufferPtr);
static void cmdEnableIqDump(char *bufferPtr);
static void cmdDisableIqDump(char *bufferPtr);
static void cmdSetSquelch(char *bufferPtr);
static void cmdStartTransmitter(char *bufferPtr);
static void cmdStopTransmitter(char *bufferPtr);
static void cmdStartReceiver(char *bufferPtr);
static void cmdStopReceiver(char *bufferPtr);
static void cmdStartLiveStream(char *bufferPtr);
static void cmdStopLiveStream(char *bufferPtr);
static void cmdSetFscanValues(char *bufferPtr);
static void cmdStartFscan(char *bufferPtr);
static void cmdStopFscan(char *bufferPtr);
static void cmdStartFrequencySweep(char *bufferPtr);
static void cmdStopFrequencySweep(char *bufferPtr);
static void cmdLoadIqFile(char *bufferPtr);
static void cmdGetRadioInfo(char *bufferPtr);
static void cmdGetFscanInfo(char *bufferPtr);
static void cmdGetSweeperInfo(char *bufferPtr);
static void cmdExitSystem(char *bufferPtr);
static void cmdHelp(void);

/* network functions */
static void startCliServer(void *arg);
static void waitForCliConnection(int socketDescriptor);
static void serviceCliConnection(void);

// user interface thread entry point
static void userInterfaceProcedure(void *arg);

/************************************************************************/
/* configurable parameters                                              */
/************************************************************************/

/*************************************************************************/
/* The following table is the command table. The structure of each entry */
/* is a tuple as follows: {command, subcommand, handler}.  The last      */
/* entry in the table has zeros so that the decoder knows when the end   */
/* of the table has been reached.                                        */
/*************************************************************************/
static const commandEntry commandTable[] =
{ // command descriptors                             syntax
  {"select","filesource",cmdSelectFileSource}, // select filesource
  {"select","livesource",cmdSelectLiveSource}, // select livesource
  {"set","demodmode",cmdSetDemodMode},         // set demodmode mode
  {"set","modmode",cmdSetModMode},             // set modmode mode
  {"set","amdemodgain",cmdSetAmDemodGain},     // set amdemodgain gain
  {"set","fmdemodgain",cmdSetFmDemodGain},     // set fmdemodgain gain
  {"set","wbfmdemodgain",cmdSetWbFmDemodGain}, // set wbfmdemodgain gain
  {"set","ssbdemodgain",cmdSetSsbDemodGain},   // set ssbdemodgain gain
  {"set","ammodindex",cmdSetAmModIndex},     // set ammodindex modulationindex 
  {"set","fmmoddeviation",cmdSetFmModDeviation},
                                             // set fmmoddeviation deviation 
  {"set","wbfmmoddeviation",cmdSetWbFmModDeviation},
                                             // set wbfmmoddeviation deviation 
  {"enable","rxfrontendamp",cmdEnableRxFrontendAmp}, // enable rxfrontendamp
  {"disable","rxfrontendamp",cmdDisableRxFrontendAmp}, // disable rxfrontendamp
  {"enable","agc",cmdEnableAgc},             // enable agc
  {"disable","agc",cmdDisableAgc},           // disable agc
  {"set","agctype",cmdSetAgcType},           // set agctype type
  {"set","agcdeadband",cmdSetAgcDeadband},   // set agcdeadband deadband
  {"set","agcblank",cmdSetAgcBlank}, // setagcblank blankinglimit
  {"set","agcalpha",cmdSetAgcAlpha},         // set agcalpha alpha
  {"set","agclevel",cmdSetAgcLevel},         // set agclevel level
  {"get","agcinfo",cmdGetAgcInfo},           // get agcinfo
  {"enable","txfrontendamp",cmdEnableTxFrontendAmp}, // enable txfrontendamp
  {"disable","txfrontendamp",cmdDisableTxFrontendAmp}, // disable txfrontendamp
  {"set","txifgain",cmdSetTxIfGain},           // set txifgain gain
  {"set","rxifgain",cmdSetRxIfGain},           // set rxifgain gain
  {"set","rxbasebandgain",cmdSetRxBasebandGain},// set rxbasebandgain gain
  {"set","frequency",cmdSetFrequency},   // set frequency frequency
  {"set","bandwidth",cmdSetRxBandwidth},   // set rxbandwidth bandwidth 
  {"set","samplerate",cmdSetRxSampleRate}, // set rxsamplerate samplerate 
  {"set","warp",cmdSetRxWarp},             // set rxwarp warp 
  {"set","squelch",cmdSetSquelch},           // set squelch threshold
  {"enable","iqdump",cmdEnableIqDump},       // enable iqdump
  {"disable","iqdump",cmdDisableIqDump},       // disable iqdump
  {"start","transmitter",cmdStartTransmitter}, // start transmitter
  {"stop","transmitter",cmdStopTransmitter}, // stop transmitter
  {"start","receiver",cmdStartReceiver}, // start receiver
  {"stop","receiver",cmdStopReceiver}, // stop receiver
  {"start","livestream",cmdStartLiveStream}, // start livestream
  {"stop","livestream",cmdStopLiveStream},  // stop livestream
  {"set","fscanvalues",cmdSetFscanValues},
    // set fscanvalues startfrequency stopfrequency stepsize
  {"start","fscan",cmdStartFscan},          // start fscan
  {"stop","fscan",cmdStopFscan},            // stop fscan
  {"start","frequencysweep",cmdStartFrequencySweep}, 
    // start frequencysweep startfrequency stepsize count dwelltime
  {"stop","frequencysweep",cmdStopFrequencySweep}, // stop frequencysweep
  {"load","iqfile",cmdLoadIqFile},       // load iqfile filename
  {"get","radioinfo",cmdGetRadioInfo},   // get radioinfo
  {"get","fscaninfo",cmdGetFscanInfo}, // get fscaninfo
  {"get","sweeperinfo",cmdGetSweeperInfo}, // get sweeperinfo
  {"exit","system",cmdExitSystem},       // exit system
  {"\0","\0",0}                          // last entry in command table
};

/*****************************************************************************

  Name: diagUi_start

  Purpose: The purpose of this function is to initialize the user interface
  subsystem and start it up.

  Calling Sequence: diagUi_start(networkPort)

  Inputs:

    networkPort - The TCP port for which a user is to connect.

  Outputs:

    None.

*****************************************************************************/
void diagUi_start(int networkPort)
{

  // this stops nprintf() from sending data on a nonexistant connection  
  consolePtr = 0;

  // save for later use
  diagUiNetworkPort = networkPort;

  // allow things to run
  diagUi_timeToExit = false;

  // create the user interface thread
  pthread_create(&cliServerThread,0,
                 (void *(*)(void *))startCliServer,0);

  return;

} // diagUi_start

/*****************************************************************************

  Name: diagUi_stop

  Purpose: The purpose of this function is to stop the user interface
  subsystem.

  Calling Sequence: diagUi_stop()

  Inputs:

    None.

  Outputs:

    None.

*****************************************************************************/
void diagUi_stop()
{

  // notify the user interface thread to terminate
  diagUi_timeToExit = true;

  // we're done .. wait for the user interface thread to terminate
  pthread_join(cliServerThread,0);

  return;

} // diagUi_stop

/*****************************************************************************

  Name: decodeCommand

  Purpose: The purpose of this function is to decode a user command and
  execute the appropriate function. The structure of a command is as
  follows:

    command subcommand parameter1 parameter2 ... parameterN

  An example of a command would be: "set dac 3 225".
  The command is "set", the subcommand is "dac", and the parameters are
  3 and 225 corresponding to the channel number and DAC value, respectively.

  Calling Sequence: decodeCommand(commandBufferPtr)

  Inputs:

    commandBufferPtr - A pointer to a command buffer.

  Outputs:

    None.

*****************************************************************************/
static void decodeCommand(char *commandBufferPtr)
{
  int status, i;
  char firstToken[80], secondToken[80], *parameterPtr;
  unsigned char found, done;

  /* get the first two tokens associated with the command */
  getCommandTokens(commandBufferPtr,firstToken,secondToken,&parameterPtr);

  status = newstrcmp("help",firstToken);
  
  if (status == 0)
  { /* help command */
    cmdHelp();
    return;
  } /* if */

  i = -1; /* reference one element BEFORE the command table */
  done = 0; /* initialize loop flag */
  found = 0; /* set to a 'not found' condition */

  while (done == 0)
  { /* scan the command table for a command match */
    i++; /* reference next entry in the command table */
    if (commandTable[i].firstTokenPtr[0] == '\0')
    { /* we're at the end of the table */
      done = 1; /* bail out of loop */
    } /* if */
    else
    { /* we have a command to compare */
      status = newstrcmp(commandTable[i].firstTokenPtr,firstToken);
      if (status == 0)
      { /* we have a match on the first token */
        status = newstrcmp(commandTable[i].secondTokenPtr,secondToken);
        if (status == 0)
        { /* we have a match on both tokens */
          found = 1; /* indicate our match! */
          done = 1; /* bail out of loop */
        } /* if */
      } /* if */
    } /* else */
  } /* while */

  if (found == 1)
  { /* we have a valid command */
    commandTable[i].handlerPtr(parameterPtr);
  } /* if */
  else
  { /* we have an invalid command */
    nprintf(stderr,
            "\n\nINVALID COMMAND - - type help for a list of commands\n\n\r");
  } /* else */

  return;

} /* decodeCommand */

/*****************************************************************************

  Name: getCommandTokens

  Purpose: The purpose of this function is to get the first tokens from
  the command buffer and store them as NULL terminated strings.  In
  addition, a pointer to the buffer parameters is returned.

    command subcommand parameter1 parameter2 ... parameterN

  An example of a command would be: "set dac 3 225".
  The first token is "set", the second token is "dac", and the parameters
  are 3 and 225 corresponding to the channel number and DAC value,
  respectively. So, firstTokenPtr would reference "set", secondTokenPtr
  would reference "dac", and parameterPtr would reference the space
  character preceeding the number 3.

  Calling Sequence: getCommandTokens(bufferPtr,
                                     firstTokenPtr,
                                     secondTokenPtr,
                                     &parameterPtr)

  Inputs:

    bufferPtr - A pointer to the command bufer.

    firstTokenPtr - A pointer to memory which accepts the first token.

    secondTokenPtr - A pointer to memory which accepts the second token.

  Outputs:

    firstTokenPtr - A pointer to memory which accepts the first token.

    secondTokenPtr - A pointer to memory which accepts the second token.

    parameterPtr - A pointer to the parameters in the command buffer.

*****************************************************************************/
static void getCommandTokens(char *bufferPtr,
                             char *firstTokenPtr,
                             char *secondTokenPtr,
                             char **parameterPtr)
{

  *firstTokenPtr = '\0'; /* protect ourselves */
  *secondTokenPtr = '\0'; /* protect ourselves */

  while ((*bufferPtr == ' ') && (*bufferPtr != '\0'))
  {
    bufferPtr++; /* move past any leading white space */
  } /* while */

  /* construct first token */
  while ((*bufferPtr != ' ') && (*bufferPtr != '\0'))
  {
    *firstTokenPtr = *bufferPtr++; /* get first token */
    *firstTokenPtr |= 0x60; /* convert to lower case */
    firstTokenPtr++; /* reference next storage location */
  } /* while */

  while ((*bufferPtr == ' ') && (*bufferPtr != '\0'))
  {
    bufferPtr++; /* move past white space */
  } /* while */

  while ((*bufferPtr != ' ') && (*bufferPtr != '\0'))
  {
    *secondTokenPtr = *bufferPtr++; /* get second token */
    *secondTokenPtr |= 0x60; /* convert to lower case */
    secondTokenPtr++; /* reference next storage location */
  } /* while */

  *firstTokenPtr = '\0'; /* terminate our string for the first token */
  *secondTokenPtr = '\0'; /* terminate our string for the second token */
  *parameterPtr = bufferPtr; /* reference parameters in buffer */

  return;

} /* getCommandTokens */

/**************************************************************************

  Name: newstrcmp

  Purpose: The purpose of this function is to compare two strings.

  Calling Sequence: status = newstrcmp(s1,s2)

  Inputs:

    s1 - A pointer to the first string to be compared.
    s2 - A pointer to the second string to be compared.

  Outputs:

    status - The result of the comparison.

**************************************************************************/
static int newstrcmp(const char *s1,const char *s2)
{
  while( (*s1 == *s2) && (*s1) ) ++s1, ++s2;

  return ((int)(unsigned char)*s1) - ((int)(unsigned char)*s2);
}

/*****************************************************************************

  Name: cmdSelectFileSource

  Purpose: The purpose of this function is to select a loaded file as the
  information source for data transmission.

  The syntax for the corresponding command is the following:

    "select filesource"

  Calling Sequence: cmdSelectFileSource(bufferPtr)

  Inputs:

    bufferPtr - A pointer to the command parameters.

  Outputs:

    None.

*****************************************************************************/
static void cmdSelectFileSource(char *bufferPtr)
{
  
  // The transmit stream originates from a file that was loaded.
  diagUi_radioPtr->selectFileSource();

  nprintf(stderr,"File stream selected.\n");

  return;

} // cmdSelectFileSource

/*****************************************************************************

  Name: cmdSelectLiveSource

  Purpose: The purpose of this function is to select a realtime data
  stream as the information source for data transmission.

  The syntax for the corresponding command is the following:

    "select filesource"

  Calling Sequence: cmdSelectLiveSource(bufferPtr)

  Inputs:

    bufferPtr - A pointer to the command parameters.

  Outputs:

    None.

*****************************************************************************/
static void cmdSelectLiveSource(char *bufferPtr)
{
  
  // The transmit stream originates from a realtime data stream.
  diagUi_radioPtr->selectLiveSource();

  nprintf(stderr,"Live stream selected.\n");

  return;

} // cmdSelectLiveSource

/*****************************************************************************

  Name: cmdSetDemodMode

  Purpose: The purpose of this function is to set the demodulation mode
  in the system.

  The syntax for the corresponding command is the following:

    "set demodmode mode"

  Calling Sequence: cmdSetDemodMode(bufferPtr)

  Inputs:

    bufferPtr - A pointer to the command parameters.

  Outputs:

    None.

*****************************************************************************/
static void cmdSetDemodMode(char *bufferPtr)
{
  int mode;
  char *displayValue[6] = {"None","AM","FM","WBFM","LSB","USB"};

  // Retrieve value
  sscanf(bufferPtr,"%d",&mode);

  if ((mode >= 0) && (mode <=5))
  {
    // Set the demodulator mode.
    diagUi_radioPtr->setDemodulatorMode(mode);

    nprintf(stderr,"Demodulator mode set to %s.\n",displayValue[mode]);
  } // if
  else
  {
    nprintf(stderr,"Error: 0 <= mode <= 5.\n");
  } // else

  return;

} // cmdSetDemodMode

/*****************************************************************************

  Name: cmdSetModMode

  Purpose: The purpose of this function is to set the modulation mode
  in the system.

  The syntax for the corresponding command is the following:

    "set modmode mode"

  Calling Sequence: cmdSetModMode(bufferPtr)

  Inputs:

    bufferPtr - A pointer to the command parameters.

  Outputs:

    None.

*****************************************************************************/
static void cmdSetModMode(char *bufferPtr)
{
  int mode;
  char *displayValue[6] = {"None","AM","FM","WBFM","LSB","USB"};

  // Retrieve value
  sscanf(bufferPtr,"%d",&mode);

  switch (mode)
  {
    case 0:
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    {
      // Set the modulator mode.
      diagUi_radioPtr->setModulatorMode(mode);

      nprintf(stderr,"Modulator mode set to %s.\n",displayValue[mode]);
      break;
    } // case

    default:
    {
      nprintf(stderr,"Error: [0 | 1 | 2 | 3 | 4 | 5]\n");
      break;
    } // case
  } // switch

  return;

} // cmdSetModMode

/*****************************************************************************

  Name: cmdSetAmDemodGain

  Purpose: The purpose of this function is to set the gain of the AM
  demodulator in the system.

  The syntax for the corresponding command is the following:

    "set amdemodgain gain"

  Calling Sequence: cmdSetAmDemodGain(bufferPtr)

  Inputs:

    bufferPtr - A pointer to the command parameters.

  Outputs:

    None.

*****************************************************************************/
static void cmdSetAmDemodGain(char *bufferPtr)
{
  float gain;

  // Retrieve value
  sscanf(bufferPtr,"%f",&gain);

  if (gain >= 0)
  {
    // Set the demodulator gain.
    diagUi_radioPtr->setAmDemodulatorGain(gain);

    nprintf(stderr,"AM Demodulator gain set to %f.\n",gain);
  } // if
  else
  {
    nprintf(stderr,"Error: gain >= 0.\n");
  } // else

  return;

} // cmdSetAmDemodGain

/*****************************************************************************

  Name: cmdSetFmDemodGain

  Purpose: The purpose of this function is to set the gain of the FM
  demodulator in the system.

  The syntax for the corresponding command is the following:

    "set fmdemodgain gain"

  Calling Sequence: cmdSetFmDemodGain(bufferPtr)

  Inputs:

    bufferPtr - A pointer to the command parameters.

  Outputs:

    None.

*****************************************************************************/
static void cmdSetFmDemodGain(char *bufferPtr)
{
  float gain;

  // Retrieve value
  sscanf(bufferPtr,"%f",&gain);

  if (gain >= 0)
  {
    // Set the demodulator gain.
    diagUi_radioPtr->setFmDemodulatorGain(gain);

    nprintf(stderr,"FM Demodulator gain set to %f.\n",gain);
  } // if
  else
  {
    nprintf(stderr,"Error: gain >= 0.\n");
  } // else

  return;

} // cmdSetFmDemodGain

/*****************************************************************************

  Name: cmdSetWbFmDemodGain

  Purpose: The purpose of this function is to set the gain of the wideband
  FM demodulator in the system.

  The syntax for the corresponding command is the following:

    "set wbfmdemodgain gain"

  Calling Sequence: cmdSetFmDemodGain(bufferPtr)

  Inputs:

    bufferPtr - A pointer to the command parameters.

  Outputs:

    None.

*****************************************************************************/
static void cmdSetWbFmDemodGain(char *bufferPtr)
{
  float gain;

  // Retrieve value
  sscanf(bufferPtr,"%f",&gain);

  if (gain >= 0)
  {
    // Set the demodulator gain.
    diagUi_radioPtr->setWbFmDemodulatorGain(gain);

    nprintf(stderr,"Wideband FM Demodulator gain set to %f.\n",gain);
  } // if
  else
  {
    nprintf(stderr,"Error: gain >= 0.\n");
  } // else

  return;

} // cmdSetWbFmDemodGain

/*****************************************************************************

  Name: cmdSetSsbDemodGain

  Purpose: The purpose of this function is to set the gain of the single
  sideband demodulator in the system.

  The syntax for the corresponding command is the following:

    "set ssbdemodgain gain"

  Calling Sequence: cmdSetSsbDemodGain(bufferPtr)

  Inputs:

    bufferPtr - A pointer to the command parameters.

  Outputs:

    None.

*****************************************************************************/
static void cmdSetSsbDemodGain(char *bufferPtr)
{
  float gain;

  // Retrieve value
  sscanf(bufferPtr,"%f",&gain);

  if (gain >= 0)
  {
    // Set the demodulator gain.
    diagUi_radioPtr->setSsbDemodulatorGain(gain);

    nprintf(stderr,"SSB Demodulator gain set to %f.\n",gain);
  } // if
  else
  {
    nprintf(stderr,"Error: gain >= 0.\n");
  } // else

  return;

} // cmdSetSsbDemodGain


/*****************************************************************************

  Name: cmdSetAmModIndex

  Purpose: The purpose of this function is to set the modulation index of
  the AM modulator in the system.

  The syntax for the corresponding command is the following:

    "set ammodindex modulationindex"

  Calling Sequence: cmdSetAmModIndex(bufferPtr)

  Inputs:

    bufferPtr - A pointer to the command parameters.

  Outputs:

    None.

*****************************************************************************/
static void cmdSetAmModIndex(char *bufferPtr)
{
  float modulationIndex;

  // Retrieve value
  sscanf(bufferPtr,"%f",&modulationIndex);

  if (modulationIndex >= 0)
  {
    // Set the modulation index.
    diagUi_radioPtr->setAmModulationIndex(modulationIndex);

    nprintf(stderr,"AM Modulation index set to %f.\n",modulationIndex);
  } // if
  else
  {
    nprintf(stderr,"Error: modulaton index >= 0.\n");
  } // else

  return;

} // cmdSetAmModIndex

/*****************************************************************************

  Name: cmdSetFmModDeviation

  Purpose: The purpose of this function is to set the frequency deviation
  of the FM modulator in the system.

  The syntax for the corresponding command is the following:

    "set fmmoddeviaton deviation"

  Calling Sequence: cmdSetFmModDeviation(bufferPtr)

  Inputs:

    bufferPtr - A pointer to the command parameters.

  Outputs:

    None.

*****************************************************************************/
static void cmdSetFmModDeviation(char *bufferPtr)
{
  float deviation;

  // Retrieve value
  sscanf(bufferPtr,"%f",&deviation);

  if ((deviation >= 0) && (deviation <= 3500))
  {
    // Set the modulator deviation.
    diagUi_radioPtr->setFmDeviation(deviation);

    nprintf(stderr,"FM Modulator deviation set to %fHz.\n",deviation);
  } // if
  else
  {
    nprintf(stderr,"Error: 0 <= deviation <= 3500.\n");
  } // else

  return;

} // cmdSetFmModDeviation

/*****************************************************************************

  Name: cmdSetWbFmModDeviation

  Purpose: The purpose of this function is to set the frequency deviation
  of the wideband FM modulator in the system.

  The syntax for the corresponding command is the following:

    "set wbfmmoddeviaton deviation"

  Calling Sequence: cmdSetWbFmModDeviation(bufferPtr)

  Inputs:

    bufferPtr - A pointer to the command parameters.

  Outputs:

    None.

*****************************************************************************/
static void cmdSetWbFmModDeviation(char *bufferPtr)
{
  float deviation;

  // Retrieve value
  sscanf(bufferPtr,"%f",&deviation);

  if ((deviation >= 0) && (deviation <= 112000))
  {
    // Set the modulator deviation.
    diagUi_radioPtr->setWbFmDeviation(deviation);

    nprintf(stderr,"Wideband FM Modulator deviation set to %fHz.\n",deviation);
  } // if
  else
  {
    nprintf(stderr,"Error: 0 <= deviation <= 112000.\n");
  } // else

  return;

} // cmdSetWbFmModDeviation

/*****************************************************************************

  Name: cmdEnableRxFrontendAmp

  Purpose: The purpose of this function is to enable the receive frontend
  in the system.  This amplifier provides an additional 14dB of system gain.

  The syntax for the corresponding command is the following:

    "enable rxfrontendamp"

  Calling Sequence: cmdEnableRxFrontendAmp(bufferPtr)

  Inputs:

    bufferPtr - A pointer to the command parameters.

  Outputs:

    None.

*****************************************************************************/
static void cmdEnableRxFrontendAmp(char *bufferPtr)
{
  bool enabled;
  bool success;

  enabled = diagUi_radioPtr->receiveFrontEndAmplifierIsEnabled();

  if (!enabled)
  {
    // Enable the frontend amplifier.
    success = diagUi_radioPtr->enableReceiveFrontEndAmplifier();

    if (success)
    {
      nprintf(stderr,"Receive frontend amplifier is enabled.\n");
    } // if
    else
    {
      nprintf(stderr,"Error: Could not enable receive frontend amplifier.\n");
    } // else
  } // if
  else
  {
    nprintf(stderr,"Error: Receive frontend amplifier is already enabled.\n");
  } // else

  return;

} // cmdEnableRxFrontendAmp

/*****************************************************************************

  Name: cmdDisableRxFrontendAmp

  Purpose: The purpose of this function is to disable the receive frontend
  amplifier in the system.  This amplifier provides an additional 14dB of
  system gain.

  The syntax for the corresponding command is the following:

    "disable frontendamp"

  Calling Sequence: cmdDisableRxFrontendAmp(bufferPtr)

  Inputs:

    bufferPtr - A pointer to the command parameters.

  Outputs:

    None.

*****************************************************************************/
static void cmdDisableRxFrontendAmp(char *bufferPtr)
{
  bool enabled;
  bool success;

  enabled = diagUi_radioPtr->receiveFrontEndAmplifierIsEnabled();

  if (enabled)
  {
    // Disable the frontend amplifier.
    success = diagUi_radioPtr->disableReceiveFrontEndAmplifier();

    if (success)
    {
      nprintf(stderr,"Receive frontend amplifier is disabled.\n");
    } // if
    else
    {
      nprintf(stderr,"Error: Could not disable receive frontend amplifier.\n");
    } // else
  } // if
  else
  {
    nprintf(stderr,"Error: Receive frontend amplifier is already disabled.\n");
  } // else

  return;

} // cmdDisableRxFrontendAmp

/*****************************************************************************

  Name: cmdEnableAgc

  Purpose: The purpose of this function is to enable the automatic gain
  control in the receiver.

  The syntax for the corresponding command is the following:

    "enable agc"

  Calling Sequence: cmdEnableAgc(bufferPtr)

  Inputs:

    bufferPtr - A pointer to the command parameters.

  Outputs:

    None.

*****************************************************************************/
static void cmdEnableAgc(char *bufferPtr)
{
  bool success;

  // Enable the AGC.
  success = diagUi_radioPtr->enableAgc();

  if (success)
  {
    nprintf(stderr,"AGC enabled.\n");  
  } // if
  else
  {
    nprintf(stderr,"Error: AGC is already enabled or receiver is disabled.\n");  
  } // else

  return;

} // cmdEnableAgc

/*****************************************************************************

  Name: cmdDisableAgc

  Purpose: The purpose of this function is to disable the automatic gain
  control in the receiver.

  The syntax for the corresponding command is the following:

    "disable agc"

  Calling Sequence: cmdDisableAgc(bufferPtr)

  Inputs:

    bufferPtr - A pointer to the command parameters.

  Outputs:

    None.

*****************************************************************************/
static void cmdDisableAgc(char *bufferPtr)
{
  bool success;

  // Disable the AGC.
  success = diagUi_radioPtr->disableAgc();

  if (success)
  {
    nprintf(stderr,"AGC disabled.\n");  
  } // if
  else
  {
    nprintf(stderr,"Error: AGC is already disabled.\n");  
  } // else

  return;

} // cmdDisableAgc

/*****************************************************************************

  Name: cmdSetAgcDeadband

  Purpose: The purpose of this function is to set the automatic gain
  control deadband.

  The syntax for the corresponding command is the following:

    "set agcdeadband deadband"

  Calling Sequence: cmdSetAgcDeadband(bufferPtr)

  Inputs:

    bufferPtr - A pointer to the command parameters.

  Outputs:

    None.

*****************************************************************************/
static void cmdSetAgcDeadband(char *bufferPtr)
{
  uint32_t deadbandInDb;
  bool success;

  // Retrieve parameter.
  sscanf(bufferPtr,"%u",&deadbandInDb);

  // Update the AGC type.
  success = diagUi_radioPtr->setAgcDeadband(deadbandInDb);

  if (success)
  {
    nprintf(stderr,"AGC deadband set to set to: %u.\n",deadbandInDb);
  } // if
  else
  {
    nprintf(stderr,"Error: Invalid AGC deadband.\n");
  } // else
  
  return;

} // cmdSetAgcDeadband

/*****************************************************************************

  Name: cmdSetAgcBlank

  Purpose: The purpose of this function is to set the automatic gain
  control blanking interval.

  The syntax for the corresponding command is the following:

    "set agcblank blanklimit"

  Calling Sequence: cmdSetAgcBlank(bufferPtr)

  Inputs:

    bufferPtr - A pointer to the command parameters.

  Outputs:

    None.

*****************************************************************************/
static void cmdSetAgcBlank(char *bufferPtr)
{
  uint32_t blankingLimit;
  bool success;

  // Retrieve parameter.
  sscanf(bufferPtr,"%u",&blankingLimit);

  // Update the AGC blanking limit.
  success = diagUi_radioPtr->setAgcBlankingLimit(blankingLimit);

  if (success)
  {
    nprintf(stderr,"AGC blanking limit set to set to: %u.\n",blankingLimit);
  } // if
  else
  {
    nprintf(stderr,"Error: Invalid AGC blanking limit.\n");
  } // else
  
  return;

} // cmdSetAgcBlank

/*****************************************************************************

  Name: cmdSetAgcType

  Purpose: The purpose of this function is to set the automatic gain
  control type.

  The syntax for the corresponding command is the following:

    "set agctype type"

  Calling Sequence: cmdSetAgcType(bufferPtr)

  Inputs:

    bufferPtr - A pointer to the command parameters.

  Outputs:

    None.

*****************************************************************************/
static void cmdSetAgcType(char *bufferPtr)
{
  uint32_t type;
  bool success;

  // Retrieve parameter.
  sscanf(bufferPtr,"%u",&type);

  // Update the AGC type.
  success = diagUi_radioPtr->setAgcType(type);

  if (success)
  {
    nprintf(stderr,"AGC type set to set to: %u.\n",type);
  } // if
  else
  {
    nprintf(stderr,"Error: Invalid AGC type.\n");
  } // else
  
  return;

} // cmdSetAgcType

/*****************************************************************************

  Name: cmdSetAgcAlpha

  Purpose: The purpose of this function is to set the automatic gain
  control filter coefficient.  This coefficient determines the time
  constant of the AGC.

  The syntax for the corresponding command is the following:

    "set agcalpha alpha"

  Calling Sequence: cmdSetAgcAlpha(bufferPtr)

  Inputs:

    bufferPtr - A pointer to the command parameters.

  Outputs:

    None.

*****************************************************************************/
static void cmdSetAgcAlpha(char *bufferPtr)
{
  float alpha;
  bool success;

  // Retrieve parameter.
  sscanf(bufferPtr,"%f",&alpha);

  // Set the AGC filter coefficient.
  success = diagUi_radioPtr->setAgcFilterCoefficient(alpha);

  if (success)
  {
    nprintf(stderr,"AGC filter coefficient set to: %f.\n",alpha);
  } // if
  else
  {
    nprintf(stderr,"Error: 0.001 < alpha < 0.999.\n");
  } // else

  return;

} // cmdSetAgcAlpha

/*****************************************************************************

  Name: cmdSetAgcLevel

  Purpose: The purpose of this function is to set the automatic gain
  control operating point in the receiver.  The units are decibels
  referenced to full scale.

  The syntax for the corresponding command is the following:

    "set agclevel level"

  Calling Sequence: cmdSetAgcLevel(bufferPtr)

  Inputs:

    bufferPtr - A pointer to the command parameters.

  Outputs:

    None.

*****************************************************************************/
static void cmdSetAgcLevel(char *bufferPtr)
{
  int32_t operatingPointInDbFs;

  // Retrieve parameter.
  sscanf(bufferPtr,"%d",&operatingPointInDbFs);

  // Set the operating point.
  diagUi_radioPtr->setAgcOperatingPoint(operatingPointInDbFs);

  nprintf(stderr,"AGC level set to: %d.\n",operatingPointInDbFs);

  return;

} // cmdSetAgcLevel

/*****************************************************************************

  Name: cmdGetAgcInfo

  Purpose: The purpose of this function is to display the automatic gain
  information in the receiver.

  The syntax for the corresponding command is the following:

    "get agcinfo"

  Calling Sequence: cmdGetAgcInfo(bufferPtr)

  Inputs:

    bufferPtr - A pointer to the command parameters.

  Outputs:

    None.

*****************************************************************************/
static void cmdGetAgcInfo(char *buffeerPtr)
{

  diagUi_radioPtr->displayAgcInternalInformation();

  return;

} // cmdGetAgcInfo

/*****************************************************************************

  Name: cmdEnableTxFrontendAmp

  Purpose: The purpose of this function is to enable the transmit frontend
  in the system.  This amplifier provides an additional 14dB of system gain.

  The syntax for the corresponding command is the following:

    "enable txfrontendamp"

  Calling Sequence: cmdEnableTxFrontendAmp(bufferPtr)

  Inputs:

    bufferPtr - A pointer to the command parameters.

  Outputs:

    None.

*****************************************************************************/
static void cmdEnableTxFrontendAmp(char *bufferPtr)
{
  bool enabled;
  bool success;

  enabled = diagUi_radioPtr->transmitFrontEndAmplifierIsEnabled();

  if (!enabled)
  {
    // Enable the frontend amplifier.
    success = diagUi_radioPtr->enableTransmitFrontEndAmplifier();

    if (success)
    {
      nprintf(stderr,"Transmit frontend amplifier is enabled,\n");
    } // if
    else
    {
      nprintf(stderr,"Error: Could not enable transmit frontend amplifier,\n");
    } // else
  } // if
  else
  {
    nprintf(stderr,"Error: Transmit frontend amplifier is already enabled,\n");
  } // else

  return;

} // cmdEnableTxFrontendAmp

/*****************************************************************************

  Name: cmdDisableTxFrontendAmp

  Purpose: The purpose of this function is to disable the transmit frontend
  amplifier in the system.  This amplifier provides an additional 14dB of
  system gain.

  The syntax for the corresponding command is the following:

    "disable txfrontendamp"

  Calling Sequence: cmdDisableTxFrontendAmp(bufferPtr)

  Inputs:

    bufferPtr - A pointer to the command parameters.

  Outputs:

    None.

*****************************************************************************/
static void cmdDisableTxFrontendAmp(char *bufferPtr)
{
  bool enabled;
  bool success;

  enabled = diagUi_radioPtr->transmitFrontEndAmplifierIsEnabled();

  if (enabled)
  {
    // Disable the frontend amplifier.
    success = diagUi_radioPtr->disableTransmitFrontEndAmplifier();

    if (success)
    {
      nprintf(stderr,"Transmit frontend amplifier is disabled,\n");
    } // if
    else
    {
      nprintf(stderr,"Error: Could not disable transmit frontend amplifier,\n");
    } // else
  } // if
  else
  {
    nprintf(stderr,"Error: Transmit frontend amplifier is already disabled,\n");
  } // else

  return;

} // cmdDisableTxFrontendAmp

/*****************************************************************************

  Name: cmdSetTxIfGain

  Purpose: The purpose of this function is to set the transmit IF gain of
  the system.  The range of values is 0 <= gain <= 47dB in 1dB steps.

  The syntax for the corresponding command is the following:

    "set txifgain gain"

  Calling Sequence: cmdSetTxIfGain(bufferPtr)

  Inputs:

    bufferPtr - A pointer to the command parameters.

  Outputs:

    None.

*****************************************************************************/
static void cmdSetTxIfGain(char *bufferPtr)
{
  bool success;
  unsigned int gain;

  // Retrieve value
  sscanf(bufferPtr,"%u",&gain);

  if (gain > 47)
  {
    // Clip it.
    gain = 47;
  } // if

  // Set the transmitter gain.
  success = diagUi_radioPtr->setTransmitIfGainInDb(gain);

  if (success)
  {
  nprintf(stderr,"Transmit IF gain set to %udB.\n",gain);
  } // if
  else
  {
    nprintf(stderr,"Error: Could not set the transmit IF gain.\n");
  } // else

  return;

} // cmdSetTxIfGain

/*****************************************************************************

  Name: cmdSetRxIfGain

  Purpose: The purpose of this function is to set the receive IF gain of
  the system.  The range of values is 0 <= gain <= 40dB in 8dB steps.

  The syntax for the corresponding command is the following:

    "set rxifgain gain"

  Calling Sequence: cmdSetRxIfGain(bufferPtr)

  Inputs:

    bufferPtr - A pointer to the command parameters.

  Outputs:

    None.

*****************************************************************************/
static void cmdSetRxIfGain(char *bufferPtr)
{
  bool success;
  uint32_t gain;

  // Retrieve value
  sscanf(bufferPtr,"%u",&gain);

  if (gain > 40)
  {
    // Clip it.
    gain = 40;
  } // if

  // Ensure that the gain is quantized to 8dB steps.
  gain = (gain / 8) * 8;

  // Set the receiver attenuation.
  success = diagUi_radioPtr->setReceiveIfGainInDb(gain);

  if (success)
  {
    nprintf(stderr,"Receive IF gain set to %udB.\n",gain);
  } // if
  else
  {
    nprintf(stderr,"Error: Could not set the receive IF gain.\n");
  } // else

  return;

} // cmdSetRxIfGain

/*****************************************************************************

  Name: cmdSetRxBasebandGain

  Purpose: The purpose of this function is to set the receive baseband gain
  of the system.  The range of values is 0 <= gain <= 62dB in 2dB steps.

  The syntax for the corresponding command is the following:

    "set rxbasebandgain gain"

  Calling Sequence: cmdSetRxBasebandGain(bufferPtr)

  Inputs:

    bufferPtr - A pointer to the command parameters.

  Outputs:

    None.

*****************************************************************************/
static void cmdSetRxBasebandGain(char *bufferPtr)
{
  bool success;
  uint32_t gain;

  // Retrieve value
  sscanf(bufferPtr,"%u",&gain);

  if (gain > 62)
  {
    // Clip it.
    gain = 62;
  } // if

  // Ensure that the gain is quantized to 2dB steps.
  gain = (gain / 2) * 2;

  // Set the receiver attenuation.
  success = diagUi_radioPtr->setReceiveBasebandGainInDb(gain);

  if (success)
  {
    nprintf(stderr,"Receive baseband gain set to %udB.\n",gain);
  } // if
  else
  {
    nprintf(stderr,"Error: Could not set the receive baseband gain.\n");
  } // else

  return;

} // cmdSetRxBasebandGain

/*****************************************************************************

  Name: cmdSetFrequency

  Purpose: The purpose of this function is to set the receive frequency of
  the system.

  The syntax for the corresponding command is the following:

    "set frequency frequency"

  Calling Sequence: cmdSetFrequency(bufferPtr)

  Inputs:

    bufferPtr - A pointer to the command parameters.

  Outputs:

    None.

*****************************************************************************/
static void cmdSetFrequency(char *bufferPtr)
{
  bool success;
  uint64_t frequency;

  success = true;

  // Retrieve value
  sscanf(bufferPtr,"%llu",&frequency);

  if ((frequency >= 1000000) && (frequency <= 6000000000))
  {
    // Set the receiver frequency.
    success = diagUi_radioPtr->setFrequency(frequency);

    if (success)
    {
      nprintf(stderr,"Frequency set to %llu Hz.\n",frequency);
    } // if
    else
    {
      nprintf(stderr,"Error: Could not set the frequency.\n");
    } // else
  } // if
  else
  {
    nprintf(stderr,"Error: 1000000 <= frequency 6000000000 Hz.\n");
  } // else

  return;

} // cmdSetFrequency

/*****************************************************************************

  Name: cmdSetRxBandwidth

  Purpose: The purpose of this function is to set the receive bandwidth of
  the system.

  The syntax for the corresponding command is the following:

    "set rxbandwidth bandwidth"

  Calling Sequence: cmdSetRxBandwidth(bufferPtr)

  Inputs:

    bufferPtr - A pointer to the command parameters.

  Outputs:

    None.

*****************************************************************************/
static void cmdSetRxBandwidth(char *bufferPtr)
{
  bool success;
  unsigned long bandwidth;

  success = true;

  // Retrieve value
  sscanf(bufferPtr,"%lu",&bandwidth);

  // Set the receiver bandwidth.
  success = diagUi_radioPtr->setReceiveBandwidth((uint32_t)bandwidth);

  if (success)
  {
    nprintf(stderr,"Bandwidth set to %lu Hz.\n",bandwidth);
  } // if
  else
  {
    nprintf(stderr,"Error: Could not set the bandwidth.\n");
  } // else

  return;

} // cmdSetRxBandwidth

/*****************************************************************************

  Name: cmdSetRxSampleRate

  Purpose: The purpose of this function is to set the receive sample rate of
  the system.

  The syntax for the corresponding command is the following:

    "set rxsamplerate samplerate"

  Calling Sequence: cmdSetRxSampleRate(bufferPtr)

  Inputs:

    bufferPtr - A pointer to the command parameters.

  Outputs:

    None.

*****************************************************************************/
static void cmdSetRxSampleRate(char *bufferPtr)
{
  bool success;
  unsigned long sampleRate;

  success = true;

  // Retrieve value
  sscanf(bufferPtr,"%lu",&sampleRate);

  if ((sampleRate >= 2000000) && (sampleRate <= 20000000))
  {
    // Set the receiver sample rate.
    success = diagUi_radioPtr->setReceiveSampleRate((uint32_t)sampleRate);

    if (success)
    {
      nprintf(stderr,"Sample rate set to %lu S/s.\n",sampleRate);
    } // if
    else
    {
      nprintf(stderr,"Error: Could not set the sample rate.\n");
    } // else
  } // if
  else
  {
    nprintf(stderr,"Error: 2000000< = samplerate <= 20000000S/s,\n");
  } // else

  return;

} // cmdSetRxSampleRate

/*****************************************************************************

  Name: cmdSetRxWarp

  Purpose: The purpose of this function is to set the receive frequency warp
  of the system.

  The syntax for the corresponding command is the following:

    "set rxwarp warp"

  Calling Sequence: cmdSetRxWarp(bufferPtr)

  Inputs:

    bufferPtr - A pointer to the command parameters.

  Outputs:

    None.

*****************************************************************************/
static void cmdSetRxWarp(char *bufferPtr)
{
  bool success;
  int warp;

  success = true;

  // Retrieve value
  sscanf(bufferPtr,"%d",&warp);

  success = diagUi_radioPtr->setReceiveWarpInPartsPerMillion(warp);

  if (success)
  {
    nprintf(stderr,"Warp set to %d ppm.\n",warp);
  } // if
  else
  {
    nprintf(stderr,"Error: Could not set the warp.\n");
  } // else

  return;

} // cmdSetRxWarp

/*****************************************************************************

  Name: cmdEnableIqDump

  Purpose: The purpose of this function is to enable the streaming of
  IQ data over a UDP connection.  This allows a link parter to
  process this data in any required way: for example, demodulation,
  spectrum analysis, etc.

  The syntax for the corresponding command is the following:

    "enable iqdump"

  Calling Sequence: cmdEnableIqDump(bufferPtr)

  Inputs:

    bufferPtr - A pointer to the command parameters.

  Outputs:

    None.

*****************************************************************************/
static void cmdEnableIqDump(char *bufferPtr)
{
  bool enabled;

  enabled = diagUi_radioPtr->isIqDumpEnabled();

  if (!enabled)
  {
    // Enable IQ data streaming over a UDP connection.
    diagUi_radioPtr->enableIqDump();

    nprintf(stderr,"IQ data streaming enabled.\n");
  } // if
  else
  {
    nprintf(stderr,"Error: IQ data streaming is already enabled.\n");
  } // else

  return;

} // cmdEnableIqDump

/*****************************************************************************

  Name: cmdDisableIqDump

  Purpose: The purpose of this function is to disable the streaming of
  IQ data over a UDP connection.

  The syntax for the corresponding command is the following:

    "disable iqdump"

  Calling Sequence: cmdDisableIqDump(bufferPtr)

  Inputs:

    bufferPtr - A pointer to the command parameters.

  Outputs:

    None.

*****************************************************************************/
static void cmdDisableIqDump(char *bufferPtr)
{
  bool enabled;

  enabled = diagUi_radioPtr->isIqDumpEnabled();

  if (enabled)
  {
  // Disable IQ data streaming over a UDP connection.
  diagUi_radioPtr->disableIqDump();

  nprintf(stderr,"IQ data streaming disabled.\n");
  } // if
  else
  {
    nprintf(stderr,"Error: IQ data streaming is already disabled.\n");
  } // else

  return;

} // cmdEnableIqDump

/*****************************************************************************

  Name: cmdSetSquelch

  Purpose: The purpose of this function is to set the squelch threshold of
  the system.

  The syntax for the corresponding command is the following:

    "set squelch threshold"

  Calling Sequence: cmdSetSquelch(bufferPtr)

  Inputs:

    bufferPtr - A pointer to the command parameters.

  Outputs:

    None.

*****************************************************************************/
static void cmdSetSquelch(char *bufferPtr)
{
  bool success;
  int32_t threshold;

  success = true;

  // Retrieve value
  sscanf(bufferPtr,"%d",&threshold);

  success = diagUi_radioPtr->setSignalDetectThreshold(threshold);

  if (success)
  {
    nprintf(stderr,"Squelch threshold set to %d dBFs.\n",threshold);
  } // if
  else
  {
    nprintf(stderr,"Error: Could not set the squelch threshold.\n");
  } // else

  return;

} // cmdSetSquelch

/*****************************************************************************

  Name: cmdStartTransmitter

  Purpose: The purpose of this function is to start the transmitter of
  the system.  There is more to this than merely keying up the transmitter.
  A baseband data stream is presented to the transmit chain, and this
  allows any type of baseband signal to be used for various experiments.

  The syntax for the corresponding command is the following:

    "start transmitter"

  Calling Sequence: cmdStartTransmitter(bufferPtr)

  Inputs:

    bufferPtr - A pointer to the command parameters.

  Outputs:

    None.

*****************************************************************************/
static void cmdStartTransmitter(char *bufferPtr)
{
  bool enabled;
  bool success;

  // Retrieve transmitter state.
  enabled = diagUi_radioPtr->isTransmitting();

  if (!enabled)
  {
    // Check if we're in receive mode.
    enabled = diagUi_radioPtr->isReceiving();

    if (enabled)
    {
      // Stop the receiver since the system is half duplex.
      success = diagUi_radioPtr->stopReceiver();
    } // if

    // Start the transmitter.
    success = diagUi_radioPtr->startTransmitter();

    if (success)
    {
      nprintf(stderr,"Transmitter started.\n");
    } // if
    else
    {
      nprintf(stderr,"Error: Transmitter not started.\n");
    } // else
  } // if
  else
  {
    nprintf(stderr,"Error: Transmitter is already started.\n");
  } // else
 
  return;

} // cmdStartTransmitter

/*****************************************************************************

  Name: cmdStopTransmitter

  Purpose: The purpose of this function is to stop the transmitter of
  the system.  This will turn off both the transmitter and the baseband
  data stream that is presented to the transmit chain.

  The syntax for the corresponding command is the following:

    "stop transmitter"

  Calling Sequence: cmdStopTransmitter(bufferPtr)

  Inputs:

    bufferPtr - A pointer to the command parameters.

  Outputs:

    None.

*****************************************************************************/
static void cmdStopTransmitter(char *bufferPtr)
{
  bool enabled;
  bool success;

  // Retrieve transmitter state.
  enabled = diagUi_radioPtr->isTransmitting();

  if (enabled)
  {
    // Stop the transmitter.
    success = diagUi_radioPtr->stopTransmitter();

    if (success)
    {
      nprintf(stderr,"Transmitter stopped.\n");
    } // if
    else
    {
      nprintf(stderr,"Error: Transmitter not stopped.\n");
    } // else
  } // if
  else
  {
    nprintf(stderr,"Error: Transmitter is already stopped.\n");
  } // else
 
  return;

} // cmdStopTransmitter

/*****************************************************************************

  Name: cmdStartReceiver

  Purpose: The purpose of this function is to start the receiver of
  the system.  When the receiver is started, received IQ samples are
  presented to whatever receive handler is registered to the Radio
  system.  This allows capture of received IQ data if so desired.

  The syntax for the corresponding command is the following:

    "start receiver"

  Calling Sequence: cmdStartReceiver(bufferPtr)

  Inputs:

    bufferPtr - A pointer to the command parameters.

  Outputs:

    None.

*****************************************************************************/
static void cmdStartReceiver(char *bufferPtr)
{
  bool enabled;
  bool success;

  // Retrieve receiver state.
  enabled = diagUi_radioPtr->isReceiving();

  if (!enabled)
  {
    // Check to see if we're in transmit mode.
    enabled = diagUi_radioPtr->isTransmitting();

    if (enabled)
    {
      // Take the system out of transmit mode.
      success = diagUi_radioPtr->stopTransmitter();
    } // if

    // Start the receiver.
    success = diagUi_radioPtr->startReceiver();

    if (success)
    {
      nprintf(stderr,"Receiver started.\n");
    } // if
    else
    {
      nprintf(stderr,"Error: Receiver not started.\n");
     } // else
  } // if
  else
  {
    nprintf(stderr,"Error: Receiver is already started.\n");
  } // else

  return;

} // cmdStartReceiver

/*****************************************************************************

  Name: cmdStopReceiver

  Purpose: The purpose of this function is to stop the receiver of
  the system.  This will stop the receiver and any reception of IQ
  samples.

  The syntax for the corresponding command is the following:

    "stop receiver"

  Calling Sequence: cmdStopReceiver(bufferPtr)

  Inputs:

    bufferPtr - A pointer to the command parameters.

  Outputs:

    None.

*****************************************************************************/
static void cmdStopReceiver(char *bufferPtr)
{
  bool enabled;
  bool success;

  // Retrieve receiver state.
  enabled = diagUi_radioPtr->isReceiving();

  if (enabled)
  {
    // Stop the receiverr.
    success = diagUi_radioPtr->stopReceiver();

    if (success)
    {
      nprintf(stderr,"Receiver stopped.\n");
    } // if
    else
    {
      nprintf(stderr,"Error: Receiver not stopped.\n");
    } // else
  } // if
  else
  {
    nprintf(stderr,"Error: Receiver is already stopped.\n");
  } // else
 
  return;

} // cmdStopReceiver

/*****************************************************************************

  Name: cmdStartLiveStream

  Purpose: The purpose of this function is to start the realtime data
  stream that provides a modulated information source for transmission.

  The syntax for the corresponding command is the following:

    "start livestream"

  Calling Sequence: cmdStartLiveStream(bufferPtr)

  Inputs:

    bufferPtr - A pointer to the command parameters.

  Outputs:

    None.

*****************************************************************************/
static void cmdStartLiveStream(char *bufferPtr)
{

  // Start the information stream.
  diagUi_radioPtr->startLiveStream();

  nprintf(stderr,"Live stream started.\n");

  return;

} // cmdStartLiveSteam

/*****************************************************************************

  Name: cmdStopLiveStream

  Purpose: The purpose of this function is to stop the realtime data
  stream that provides a modulated information source for transmission.

  The syntax for the corresponding command is the following:

    "stop livestream"

  Calling Sequence: cmdStopLiveStream(bufferPtr)

  Inputs:

    bufferPtr - A pointer to the command parameters.

  Outputs:

    None.

*****************************************************************************/
static void cmdStopLiveStream(char *bufferPtr)
{

  // Stop the information stream.
  diagUi_radioPtr->stopLiveStream();

  nprintf(stderr,"Live stream stopped.\n");

  return;

} // cmdStopLiveSteam

/*****************************************************************************

  Name: cmdSetFscanValues

  Purpose: The purpose of this function is to set the frequency
  scanner sweep parameters.

  The syntax for the corresponding command is the following:

    "set fscanvalues startfrequency endfrequency stepsize"

  Calling Sequence: cmdSetFscanValues(bufferPtr)

  Inputs:

    bufferPtr - A pointer to the command parameters.

  Outputs:

    None.

*****************************************************************************/
static void cmdSetFscanValues(char *bufferPtr)
{
  bool success;
  uint64_t startFrequency;
  uint64_t endFrequency;
  int64_t stepSize;

  // Default to failure.
  success = false;

  // Retrieve parameters
  sscanf(bufferPtr,"%llu %llu %lld",&startFrequency,&endFrequency,&stepSize);

  // Enforce nonnegative values.
  stepSize = abs(stepSize);

  if ((startFrequency >=1000000LL) && (startFrequency < 6000000000LL)
      && (endFrequency <= 6000000000LL) && (startFrequency < endFrequency))
  {
    // Configure the frequency scanner.
    success =
      diagUi_frequencyScannerPtr->setScanParameters(startFrequency,
                                                    endFrequency,
                                                    (uint64_t)stepSize);
  } // if
  else
  {
    nprintf(stderr,"Error: 1000000 <= frequency 6000000000 Hz.\n");
  } // else

  if (success)
  {
    nprintf(stderr,"Frequency scanner parameters set.\n");
  } // if
  else
  {
    nprintf(stderr,"Error: Frequency scanner cannot be scanning.\n");
  } // else

  return;

} // cmdSetFscanValues

/*****************************************************************************

  Name: cmdStartFscan

  Purpose: The purpose of this function is to start the frequency
  scanner.

  The syntax for the corresponding command is the following:

    "start fscan startfrequency endfrequency stepsize"

  Calling Sequence: cmdStartFscan(bufferPtr)

  Inputs:

    bufferPtr - A pointer to the command parameters.

  Outputs:

    None.

*****************************************************************************/
static void cmdStartFscan(char *bufferPtr)
{
  bool success;

  // Start the scanner.
  success = diagUi_frequencyScannerPtr->start();

  if (success)
  {
    nprintf(stderr,"Frequency scanning started.\n");
  } // if
  else
  {
    nprintf(stderr,"Error: Frequency scanning is already in progress.\n");
  } // else

  return;

} // cmdStartFscan

/*****************************************************************************

  Name: cmdStopFscan

  Purpose: The purpose of this function is to stop a frequency scan.

  The syntax for the corresponding command is the following:

    "stop fscan"

  Calling Sequence: cmdStopFscan(bufferPtr)

  Inputs:

    bufferPtr - A pointer to the command parameters.

  Outputs:

    None.

*****************************************************************************/
static void cmdStopFscan(char *bufferPtr)
{
  bool success;

  // Start the scanner.
  success = diagUi_frequencyScannerPtr->stop();

  if (success)
  {
    nprintf(stderr,"Frequency scan stopped.\n");
  } // if
  else
  {
    nprintf(stderr,"Error: Frequency scanning is already stopped.\n");
  } // else

  return;

} // cmdStopFscan

/*****************************************************************************

  Name: cmdStartFrequencySweep

  Purpose: The purpose of this function is to start sweeping the operational
  frequency.

  The syntax for the corresponding command is the following:

    "start frequencysweep startfrequency stepsize count dwelltime"

  Calling Sequence: cmdStartFrequencySweep(bufferPtr)

  Inputs:

    bufferPtr - A pointer to the command parameters.

  Outputs:

    None.

*****************************************************************************/
static void cmdStartFrequencySweep(char *bufferPtr)
{
  uint64_t frequency;
  int64_t stepSize;
  uint64_t count;
  uint64_t upperFrequency;
  unsigned long dwellTime;


  if (diagUi_frequencySweeperPtr == 0)
  {
    // Retrieve parameters
    sscanf(bufferPtr,"%llu %lld %llu %lu",
           &frequency,
           &stepSize,
           &count,
           &dwellTime);

    // Enforce nonnegative values.
    stepSize = abs(stepSize);

    if ((frequency >= 1000000) && (frequency <= 6000000000LL))
    {
      // Computer upper frequency limit.
      upperFrequency = frequency + (stepSize * count);

      if (upperFrequency <= 6000000000LL)
      {
        // Start the frequency sweeper.
        diagUi_frequencySweeperPtr = new FrequencySweeper(diagUi_radioPtr,
                                                          frequency,
                                                          (uint64_t)stepSize,
                                                          count,
                                                          (uint32_t)dwellTime);

        nprintf(stderr,"Frequency sweeping started.\n");
      } // if
      else
      {
        nprintf(stderr,"Error: Upper frequency: %llu > 6000000000 Hz.\n",
                upperFrequency);

      } // else
    } // if
    else
    {
      nprintf(stderr,"Error: 1000000 <= frequency 6000000000 Hz.\n");
    } // else

  } // if
  else
  {
    nprintf(stderr,"Error: Frequency sweeping is already in progress.\n");
  } // else

  return;

} // cmdStartFrequencySweep

/*****************************************************************************

  Name: cmdStopFrequencySweep

  Purpose: The purpose of this function is to stop a frequency sweep.

  The syntax for the corresponding command is the following:

    "stop frequencysweep"

  Calling Sequence: cmdStopFrequencySweep(bufferPtr)

  Inputs:

    bufferPtr - A pointer to the command parameters.

  Outputs:

    None.

*****************************************************************************/
static void cmdStopFrequencySweep(char *bufferPtr)
{

  if (diagUi_frequencySweeperPtr != 0)
  {
    // Stop the frequency sweeper.
    delete diagUi_frequencySweeperPtr;

    // Indicate that frequency sweep has been stopped.
    diagUi_frequencySweeperPtr = 0;

    nprintf(stderr,"Frequency sweep stopped.\n");
  } // if


  return;

} // cmdStopFrequencySweep

/*****************************************************************************

  Name: cmdLoadIqFile

  Purpose: The purpose of this function is to load a file, that contains
  IQ samples into the system.  The format of the IQ samples is 8-bit binary
  offset.  The data is organized as IQ pairs, so that we have:

    I1,Q1; I2,Q2...

  The syntax for the corresponding command is the following:

    "load iqfile filename"

  Calling Sequence: cmdLoadIqFile(bufferPtr)

  Inputs:

    bufferPtr - A pointer to the command parameters.

  Outputs:

    None.

*****************************************************************************/
void cmdLoadIqFile(char *bufferPtr)
{
  DataProvider *providerPtr;
  char fileName[256];
  bool success;

  // Retrieve the file name.
  sscanf(bufferPtr,"%s",fileName);

  // Retrieve a pointer to the data provider.
  providerPtr = diagUi_radioPtr->getDataProvider();

  // Load the data.
  success = providerPtr->loadIqFile(fileName);

  if (success)
  {
    nprintf(stderr,"Loaded %s\n",fileName);
  } // if
  else
  {
    nprintf(stderr,"Error: Could not load %s\n",fileName);
  } // else

  return;

} // cmdLoadIqFile

/*****************************************************************************

  Name: cmdGetRadioInfo

  Purpose: The purpose of this function is to display radio information
  to the user.

  The syntax for the corresponding command is the following:

    "get radioinfo"

  Calling Sequence: cmdGetRadioInfo(bufferPtr)

  Inputs:

    bufferPtr - A pointer to the command parameters.

  Outputs:

    None.

*****************************************************************************/
static void cmdGetRadioInfo(char *bufferPtr)
{

  // Display radio information to the user.
  diagUi_radioPtr->displayInternalInformation();

  return;

} // cmdGetRadioInfo

/*****************************************************************************

  Name: cmdGetFscanInfo

  Purpose: The purpose of this function is to display frequency scanner
  information to the user.

  The syntax for the corresponding command is the following:

    "get scannerinfo"

  Calling Sequence: cmdGetFscanInfo(bufferPtr)

  Inputs:

    bufferPtr - A pointer to the command parameters.

  Outputs:

    None.

*****************************************************************************/
static void cmdGetFscanInfo(char *bufferPtr)
{

  if (diagUi_frequencyScannerPtr != 0)
  {
    // Display frequency scanner information to the user.
    diagUi_frequencyScannerPtr->displayInternalInformation();
  } // if

  return;

} // cmdGetFscanInfoo

/*****************************************************************************

  Name: cmdGetSweeperInfo

  Purpose: The purpose of this function is to display frequency sweeper
  information to the user.

  The syntax for the corresponding command is the following:

    "get sweeperinfo"

  Calling Sequence: cmdGetSweeperInfo(bufferPtr)

  Inputs:

    bufferPtr - A pointer to the command parameters.

  Outputs:

    None.

*****************************************************************************/
static void cmdGetSweeperInfo(char *bufferPtr)
{

  if (diagUi_frequencySweeperPtr != 0)
  {
    // Display frequency sweeper information to the user.
    diagUi_frequencySweeperPtr->displayInternalInformation();
  } // if

  return;

} // cmdGetSweeperInfo

/*****************************************************************************

  Name: cmdExitSystem

  Purpose: The purpose of this function is to exit the system.

  The syntax for the command is as follows:

    "exit system"

  Calling Sequence: cmdExitSystem()

  Inputs:

    bufferPtr - A pointer to the command parameters.

  Outputs:

    None.

*****************************************************************************/
static void cmdExitSystem(char *bufferPtr)
{

  // indicate that it's time to exit the system
  diagUi_timeToExit = true;

  return;

} /* cmdExitSystem */

/*****************************************************************************

  Name: cmdHelp

  Purpose: The purpose of this function is to display the help information
  to the user.

  The syntax for the command is as follows:

    "help"

  Calling Sequence: cmdHelp()

  Inputs:

    None.

  Outputs:

    None.

*****************************************************************************/
static void cmdHelp(void)
{

  nprintf(stderr,"\nselect filesource\n");
  nprintf(stderr,"select livesource\n");

  nprintf(stderr,"set demodmode <mode: [0 (None) | 1 (AM) | 2 (FM)\n"
                   "                      | 3 (WBFM)] | 4 (LSB) | 5 (USB)>]\n");

  nprintf(stderr,"set modmode <mode: [0 (None) | 1 (AM) | 2 (FM)\n"
                   "                      | 3 (WBFM) | 4 (LSB) | 5 (USB)>]\n");

  nprintf(stderr,"set amdemodgain <gain>\n");
  nprintf(stderr,"set fmdemodgain <gain>\n");
  nprintf(stderr,"set wbfmdemodgain <gain>\n");
  nprintf(stderr,"set ssbdemodgain <gain>\n");
  nprintf(stderr,"set ammodindex <modulation index>\n");
  nprintf(stderr,"set fmmoddeviation <deviation in Hz>\n");
  nprintf(stderr,"set wbfmmoddeviation <deviation in Hz>\n");
  nprintf(stderr,"enable rxfrontendamp\n");
  nprintf(stderr,"disable rxfrontendamp\n");
  nprintf(stderr,"enable agc\n");
  nprintf(stderr,"disable agc\n");
  nprintf(stderr,"set agctype <type: [0 (Lowpass) | 1 (Harris)]>\n");
  nprintf(stderr,"set agcdeadband <deadband in dB: (0 < deadband < 10)>\n");

  nprintf(stderr,
    "set agcblank <blankinglimit in ticks: (0 =< blankinglimit <= 10)>\n");

  nprintf(stderr,"set agcalpha <alpha: (0.001 <= alpha < 0.999)>\n");
  nprintf(stderr,"set agclevel <level in dBFs>\n");
  nprintf(stderr,"enable txfrontendamp\n");
  nprintf(stderr,"disable txfrontendamp\n");
  nprintf(stderr,"set txifgain <gain in dB>\n");
  nprintf(stderr,"set rxifgain <gain in dB>\n");
  nprintf(stderr,"set rxbasebandgain <gain>\n");
  nprintf(stderr,"set frequency <frequency in Hertz>\n");
  nprintf(stderr,"set bandwidth <bandwidth in Hertz>\n");
  nprintf(stderr,"set samplerate <samplerate in S/s>\n");
  nprintf(stderr,"set warp <warp in ppm>\n");
  nprintf(stderr,"set squelch <threshold in dBFs>\n");
  nprintf(stderr,"enable iqdump\n");
  nprintf(stderr,"disable iqdump\n");
  nprintf(stderr,"start transmitter\n");
  nprintf(stderr,"stop transmitter\n");
  nprintf(stderr,"start receiver\n");
  nprintf(stderr,"stop receiver\n");
  nprintf(stderr,"start livestream\n");
  nprintf(stderr,"stop livestream\n");

  nprintf(stderr,
      "set fscanvalues <startfrequency> <endfrequency> <stepsize>\n");

  nprintf(stderr,"start fscan\n");
  nprintf(stderr,"stop fscan\n");

  nprintf(stderr,  
      "start frequencysweep <startfrequency> <stepsize> <count> <dwelltime>\n");

  nprintf(stderr,"stop frequencysweep\n");
  nprintf(stderr,"load iqfile <filename>\n");
  nprintf(stderr,"get radioinfo\n");
  nprintf(stderr,"get fscaninfo\n");
  nprintf(stderr,"get sweeperinfo\n");
  nprintf(stderr,"get agcinfo\n");
  nprintf(stderr,"exit system\n");
  nprintf(stderr,"help\n");
  nprintf(stderr,"Type <^B><enter> key sequence to repeat last command\n");

  return;

} /* cmdHelp */

/*****************************************************************************

  Name: nprintf

  Purpose: The purpose of this function is to start the command line
  interpreter server.

  Calling Sequence: nprint(s,formatPtr,arg1, arg2..)

  Inputs:

    s - A file pointer normally used by nprintf().  This is a dummy
    argument.

    formatPtr - A pointer to a printf()-style format string.

    arg1,arg2,... - The arguments that are to be printed.

  Outputs:

    None.

*****************************************************************************/
void nprintf(FILE *s,const char *formatPtr, ...)
{
  char buffer[2048];
  va_list args;


  if (consolePtr != 0)
  {
    // set up for argument retrieval  
    va_start(args,formatPtr);

    // store the formated data to a string
    vsprintf(buffer,formatPtr,args);

    // we're done with the args
    va_end(args);

    // display the information to the user
    consolePtr->putLine(buffer);
  } // if

} // nprintf

/*****************************************************************************

  Name: startCliServer

  Purpose: The purpose of this function is to start the command line
  interpreter server.  This is the entry point to the server thread.
  This function listens on a TCP socket and waits for a connection
  from a user.  When a connection is established, the user interface
  thread is launched, and this function will block until the user
  interface thread is terminated.  Once the user interface thread is
  terminated, this function will close its half of the client connection
  and will wait for a new connection.

  Calling Sequence: startCliServer(arg)

  Inputs:

    arg - A pointer to an argument.

  Outputs:

    None.

*****************************************************************************/
static void startCliServer(void *arg)
{
  int status, option;
  socklen_t clientAddressLength;
  struct sockaddr_in addr;
  int fileDescriptor;

  // This is used for setting socket options.
  option = 1;

  // Create socket.
  fileDescriptor = socket(PF_INET,SOCK_STREAM,0);

  //*******************************************************************
  // If the file descriptor is valid, a socket has been created.  Now
  // we set up the parameters that are needed for the bind() call.
  //*******************************************************************
  if (fileDescriptor != -1)
  {
    // Set up to reuse the port number.
    status = setsockopt(fileDescriptor,SOL_SOCKET,SO_REUSEADDR,
        (char *)&option,sizeof(option));

    if (status != -1)
    {
      // Set up the address structure.
      bzero((char *)&addr,sizeof(addr));
      addr.sin_family = AF_INET;
      addr.sin_addr.s_addr = inet_addr("0.0.0.0");
      addr.sin_port = htons(diagUiNetworkPort);

      // Associate the address with the socket. 
      status = bind(fileDescriptor,(struct sockaddr *)&addr,sizeof(addr));
    } // if

    if (status != -1)
    {
      // Accept only one connection.
      status = listen(fileDescriptor,1);
    } // if

   /* This is needed needed by the accept() call. */
    clientAddressLength = sizeof(addr);

    //**********************************************************************
    // This is the main server loop.  The server will block on the
    // accept() call until a connection is made by a client.  Once a
    // connection is established, the connection will be serviced, and no
    // other processing will occur.  Note that this loop is traversed only
    // once per connect/disconnect sequence, since this is an interative
    // server that services only one client connection at a time.
    //**********************************************************************
    while (!diagUi_timeToExit)
    {
      // Indicate that there is no network connection.
      consoleConnected = false;

      // Wait until a client is attempting to connect.
      waitForCliConnection(fileDescriptor);

      if (!diagUi_timeToExit)
      {
        //************************************************************
        // This block of code will accept the incoming connection.
        // If the system call is interrupted for some reason, another
        // attempt will be made to accept connections.
        //************************************************************
        do
        {
          // Accept the incoming connection.
          connectFileDescriptor = 
             accept(fileDescriptor,(sockaddr *)&addr,&clientAddressLength);

          if (connectFileDescriptor == -1)
          {
            if (errno == EINTR)
            {
              continue;
            } /* if */
          } // if
        } // do
        while (connectFileDescriptor == -1);

        // Do the appropriate processing for the new connection.
        serviceCliConnection();

        // We're done with this connection.
        close (connectFileDescriptor);

      } // if

    } // while

  } // if
  else
  {
    // clean up the mess
    close(fileDescriptor);
  } // else

  if (fileDescriptor != -1)
  {
    close(fileDescriptor);
  } // if

  pthread_exit(0);

} // startCliServer

/*****************************************************************************

  Name: waitForCliConnection

  Purpose: The purpose of this function is to wait for a client
  connection.

  Calling Sequence: waitForCliConnection(socketDescriptor)

  Inputs:

    socketDescriptor - A descriptor for the listener socket..

  Outputs:

    None.

*****************************************************************************/
static void waitForCliConnection(int socketDescriptor)
{
  bool done;
  int status;
  fd_set connectSet;
  struct timeval timeout;

  // Set up for loop entry.
  done = false;

  while ((!diagUi_timeToExit) && (!done))
  {
    // Ensure that we're clean.
    FD_ZERO(&connectSet);

    // Indicate the descriptor of interest.
    FD_SET(socketDescriptor,&connectSet);

    // Set timeout to 1 second.
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;

    // See which descriptors are available for reading.
    status = select(socketDescriptor+1,&connectSet,0,0,&timeout);

    if (status != -1)
    {
      if (FD_ISSET(socketDescriptor,&connectSet))
      {
        // Bail out.
        done = true;
      } // if
    } // if
  } // while

  return;

} // waitForCliConnection

/*****************************************************************************

  Name: serviceCliConnection

  Purpose: The purpose of this function is to service a command line
  interface user session.  The user interface thread is launched, and the
  function will wait either on an indication of time to exit the system or
  a network connection terminated by the link partner.

  Calling Sequence: cliServer()

  Inputs:

    None.

  Outputs:

    None.

*****************************************************************************/
static void serviceCliConnection(void)
{
  struct timeval timeout;
  pthread_t userInterfaceThread;

  // Indicate that the console is connected.
  consoleConnected = true;

  // Create the user interface thread.
  pthread_create(&userInterfaceThread,0,
                 (void *(*)(void *))userInterfaceProcedure,0);

  // Wait until either it is time to exit or the connection is dropped by
  // the link partner.
  while ((consoleConnected) &&
         (!diagUi_timeToExit))
  {
    // Set up to block for 100ms.
    timeout.tv_sec = 0;
    timeout.tv_usec = 100000;

    // Wait for the designated time interval.
    select(0,0,0,0,&timeout);
  } // while

  // Ok we're done .. wait for the user interface thread to terminate.
  pthread_join(userInterfaceThread,0);

  return;

} // serviceCliConnection

/*****************************************************************************

  Name: userInterfaceProcedure

  Purpose: The purpose of this function is to provide user interface
  functionality of the system.  This function is the entry point to the
  user interface thread.

  Calling Sequence: userInterfaceProcedure(arg)

  Inputs:

    arg - A pointer to an argument.

  Outputs:

    None.

*****************************************************************************/
static void userInterfaceProcedure(void *arg)
{
  unsigned short length;
  unsigned short i;

  // create a console
  consolePtr = new console(connectFileDescriptor,
                                &consoleConnected);

  // display introduction message
  nprintf(stderr,"\nWelcome to System Diagnostics\n");

  while (!diagUi_timeToExit && consoleConnected)
  {
    length = consolePtr->getLine(commandBuffer,80);

    if (length != 0)
    { // we have a command
      if (commandBuffer[0] != 2)
      { // new command

        for (i = 0; i <= length; i++)
        {
          lastCommandBuffer[i] = commandBuffer[i];
        }// for 
        /* execute the new command */
        decodeCommand(commandBuffer);
      } /* if */
      else
      { // repeat last command
        decodeCommand(lastCommandBuffer);
      } // else

    } // if
  } // while */

  // we don't need this anymore
  delete consolePtr;
  consolePtr = 0;

  pthread_exit(0);

} // userInterfaceProcedure

