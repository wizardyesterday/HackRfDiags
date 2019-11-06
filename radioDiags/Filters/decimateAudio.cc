//************************************************************************
// file name: interpolateAudio.cc
//************************************************************************
//_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
// This program reads an input file, called original32000.raw, decreases
// the sample rate, and writes the samples to a file called hi8000.raw.
// The format of the files is single-channel 16-bit little endian format
// audio samples.  The file, decimated8000.raw contains 10 seconds of
// samples at a sample rate of 8000S/s.  The file, original32000.raw
// contais 10 seconds of samples at a sample rate of 32000S.s.
// This program uses the Decimator class with an decimation factor
// of 4.
//
// To build, type,
//  g++ -o decimateAudio decimateAudio.cc Decimator.cc -lm
//
// To run, type,
// ./decimateAudio
//
// Keep in mind, that the inputfile, original32000.raw, has to be
// available in the current working directory.
//_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <ctype.h>

#include "Decimator.h"

using namespace std;

// For Fs = 32000S/s, the response is 0 <= F <= 3400Hz.
float h32000[] =
{
  -0.0084477,
   0.0084043,
   0.0075154,
   0.0064417,
   0.0039647,
   0.0002320,
  -0.0034617,
  -0.0054143,
  -0.0044711,
  -0.0008117,
   0.0038463,
   0.0071423,
   0.0070406,
   0.0031175,
  -0.0030506,
  -0.0084378,
  -0.0100055,
  -0.0063619,
   0.0012652,
   0.0093359,
   0.0135270,
   0.0109874,
   0.0020484,
  -0.0094525,
  -0.0176862,
  -0.0176222,
  -0.0078167,
   0.0082021,
   0.0228999,
   0.0279489,
   0.0183743,
  -0.0041729,
  -0.0307927,
  -0.0479859,
  -0.0428189,
  -0.0086291,
   0.0512069,
   0.1232103,
   0.1877819,
   0.2258935,
   0.2258935,
   0.1877819,
   0.1232103,
   0.0512069,
  -0.0086291,
  -0.0428189,
  -0.0479859,
  -0.0307927,
  -0.0041729,
   0.0183743,
   0.0279489,
   0.0228999,
   0.0082021,
  -0.0078167,
  -0.0176222,
   0.0176862,
  -0.0094525,
   0.0020484,
   0.0109874,
   0.0135270,
   0.0093359,
   0.0012652,
  -0.0063619,
  -0.0100055,
  -0.0084378,
  -0.0030506,
   0.0031175,
   0.0070406,
   0.0071423,
   0.0038463,
  -0.0008117,
  -0.0044711,
  -0.0054143,
  -0.0034617,
   0.0002320,
   0.0039647,
   0.0064417,
   0.0075154,
   0.0084043,
  -0.0084477  
};

// Storage of the audio samples.
int16_t sampleBuffer[320000];

// Storage for our interpolated samples.
float outputBuffer[80000];

// File output buffer.
int16_t fileOutputBuffer[80000];

Decimator *myFilterPtr;

int main(int argc,char **argv)
{
  FILE *inputFileStream, *outputFileStream;
  size_t count;
  int i, j;
  uint32_t outputBufferIndex;
  int filterLength, inputSampleLength, outputSampleLength;
  bool sampleAvailable;
  float sample;

  // Ten seconds at 8000S/s.
  inputSampleLength = 10 * 32000;

  // Ten seconds at 16000S/s.
  outputSampleLength = 10 * 8000;

  // Compute the number of taps in the prototype filter.
  filterLength = sizeof(h32000)/sizeof(float);

  // Create interpolator.
  myFilterPtr = new Decimator(filterLength,h32000,4);

  // Open the input file for reading the audio samples.
  inputFileStream = fopen("original32000.raw","r");

  // Read 16-bit input samples from the input audio file.
  count = fread(sampleBuffer,2,inputSampleLength,inputFileStream);

  // We're done with this file.
  fclose(inputFileStream);

  // Reference the beginning of the buffer.
  outputBufferIndex = 0;

  for (i = 0; i < inputSampleLength; i++)
  {
    sampleAvailable = myFilterPtr->decimate((float)sampleBuffer[i],&sample);

    if (sampleAvailable)
    {
      // Save the decimated sample.
      outputBuffer[outputBufferIndex] = sample;

      // Reference next storage locaton.
      outputBufferIndex++;
    } // if
  } // for

  // Convert to integers.
  for (i = 0; i < outputSampleLength; i++)
  {
    fileOutputBuffer[i] = (int16_t)outputBuffer[i];
  } // for

  // Open the output file for writing the decimated the audio samples.
  outputFileStream = fopen("decimated8000.raw","w");

  // Write output samples to the output audio file.
  count = fwrite(fileOutputBuffer,2,outputSampleLength,outputFileStream);

  // We're done with this file.
  fclose(outputFileStream);

  // Release resources.
  delete myFilterPtr;

  return (0);

} // main

