//************************************************************************
// file name: ssb.cc
//************************************************************************
//_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
// This program reads PCM samples from stdin, and converts the signal to
// a baseband representation of single sideband.
//
// To build, type,
//  sh buildSsb.sh
//
// To run, type,
// ./ssb <s | u>  < inputFile > outputFile
//
//_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <ctype.h>

#include "SsbModulator.h"

using namespace std;

int16_t sampleBuffer[512];
int8_t outputBuffer[262144];

SsbModulator *modulatorPtr;

int main(int argc,char **argv)
{
  bool done;
  size_t count;
  uint32_t outputBufferLength;

  // Instantiate a modulator.
  modulatorPtr = new SsbModulator();

  switch (argc)
  {
    case 1:
    {
      modulatorPtr->setLsbModulationMode();
      break;
    }

    case 2:
    {
      if (argv[1][0] == 'l')
      {
        modulatorPtr->setLsbModulationMode();
      } // if
      else
      {
        modulatorPtr->setUsbModulationMode();
      } // else

      break;
    } // case

    default:
    {
      modulatorPtr->setLsbModulationMode();
      break;
    } //case
  } // switch

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

