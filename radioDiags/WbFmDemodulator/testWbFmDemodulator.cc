//************************************************************************
// file name: testWbFmDemodulator.cc
//************************************************************************
//_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
// This program tests the WbFmDemodulator class by reading a file that
// contains IQ samples that represent a narrowband FM signal.
// To build, type,
//  g++ -o testWbFmDemodulator testWbFmDemodulator.cc Decimator.cc -lm
//
// To run, type,
// ./testWbFmDemodulator <inputFile >outputFile
//_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>

#include "WbFmDemodulator.h"

using namespace std;

WbFmDemodulator *myDemodPtr;

uint8_t inputBuffer[32768];
int8_t sampleBuffer[32768];

int main(int argc,char **argv)
{
  uint32_t i, j;
  int count;

  // Instantiate a wideband FM demodulator.
  myDemodPtr = new WbFmDemodulator();

  // Set the gain.
  myDemodPtr->setDemodulatorGain(64000/(2 * M_PI));

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
