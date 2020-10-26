//************************************************************************
// file name: am.cc
//************************************************************************
//_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
// This program reads PCM samples from stdin, and converts the signal to
// a baseband representation of large carrier AM. 
//
// To build, type,
//  sh buildAm.sh
//
// To run, type,
// ./am < inputFile > outputFile
//
//_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <ctype.h>

#include "AmModulator.h"

using namespace std;

int16_t sampleBuffer[512];
int8_t outputBuffer[262144];

AmModulator *modulatorPtr;

int main(int argc,char **argv)
{
  bool done;
  size_t count;
  uint32_t outputBufferLength;

  // Instantiate a modulator.
  modulatorPtr = new AmModulator();

  // Set up for loop entry.
  done = false;

  while (!done)
  {
    // Read block of PCM input samples from stdin.
    count = fread(sampleBuffer,2,512,stdin);

    if (count == 0)
    {
      // We're done.
      done = true;
    } // if
    else
    {
      // Modulate the data.
      modulatorPtr->acceptData(sampleBuffer,
                               count,
                               outputBuffer,
                               &outputBufferLength);

      // Output the modulated data.
      count = fwrite(outputBuffer,1,outputBufferLength,stdout);
    } // else
  } // while

  return (0);

} // main

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

  // set up for argument retrieval  
  va_start(args,formatPtr);

  // store the formated data to a string
  vsprintf(buffer,formatPtr,args);

  // we're done with the args
  va_end(args);

  // display the information to the user
  fprintf(stderr,buffer);

} // nprintf

