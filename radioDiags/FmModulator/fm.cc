//************************************************************************
// file name: fm.cc
//************************************************************************
//_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
// This program reads PCM samples from stdin, and converts the signal to
// a baseband representation of FM. 
//
// To build, type,
//  sh buildFm.sh
//
// To run, type,
// ./fm < inputFile > outputFile
//
//_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <ctype.h>
#include <math.h>

#include "FmModulator.h"

using namespace std;

int16_t sampleBuffer[512];
int8_t outputBuffer[262144];

FmModulator *modulatorPtr;

int main(int argc,char **argv)
{
  bool done;
  size_t count;
  uint32_t outputBufferLength;

  // Instantiate a modulator.
  modulatorPtr = new FmModulator();

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

