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

