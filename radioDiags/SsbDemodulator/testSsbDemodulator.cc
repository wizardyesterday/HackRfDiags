//************************************************************************
// file name: testSsDemodulator.cc
//************************************************************************
//_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
// This program tests the SsbDemodulator class by reading a file that
// contains IQ samples that represent an SSB signal.
// To build, type,
//  g++ -o testSsbDemodulator testSsbDemodulator.cc \
//  FirFilter.cc Decimator.cc -lm
//
// To run, type,
// ./testSsbDemodulator <inputFile >outputFile
//_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>

#include "SsbDemodulator.h"

using namespace std;

SsbDemodulator *myDemodPtr;

uint8_t inputBuffer[32768];
int8_t sampleBuffer[32768];

int main(int argc,char **argv)
{
  uint32_t i, j;
  int count;

  // Instantiate an FM demodulator.
  myDemodPtr = new SsbDemodulator();

  // Set the demodulation mode to upper sideband.
  myDemodPtr->setLsbDemodulationMode();

  // Set the gain.
  myDemodPtr->setDemodulatorGain(300);

  myDemodPtr->displayInternalInformation();

  for (i = 0; i < 156; i++)
  {
    // Read a block of input samples.
    count = fread(inputBuffer,1,32768,stdin);

    // Convert to signed values.
    for (j = 0; j < 32768; j++)
    {
      sampleBuffer[j] = (int8_t)inputBuffer[j] - 128;
    } // for

    // Demodulate the samples.
    myDemodPtr->acceptIqData(sampleBuffer,32768);
  } // for

  // Release resources.
  delete myDemodPtr;

  return (0);

} // main
