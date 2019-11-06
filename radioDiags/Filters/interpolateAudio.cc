//************************************************************************
// file name: interpolateAudio.cc
//************************************************************************
//_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
// This program reads an input file, called orignal8000.raw, increases the
// sample rate, and writes the samples to a file called
// interpolated16000.raw.  The format of the files is single-channel
// 16-bit little endian format audio samples.  The file, yo8000.raw
// contains 10 seconds of samples at a sample rate of 8000S/s.  The file,
// interpolated16000.raw contais 10 seconds of samples at a sample rate
// of 16000S.s.  This program uses the Interpolator class with an
// interpolation factor of 2.
//
// To build, type,
//  g++ -o interpolateAudio interpolateAudio.cc Interpolator.cc -lm
//
// To run, type,
// ./interpolateAudio
//
// Keep in mind, that the inputfile, original8000.raw, has to be available in
// the current working directory.
//_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <ctype.h>

#include "Interpolator.h"

using namespace std;

// For Fs = 16000S/s, the response is 0 <= F <= 3400Hz.
float h16000[] =
{
  -0.0011405,
   0.0183372,
   0.0030542,
  -0.0100052,
  -0.0059350,
   0.0115377,
   0.0109293,
  -0.0120883,
  -0.0175779,
   0.0110390,
   0.0262645,
  -0.0074772,
  -0.0377408,
  -0.0003152,
   0.0541009,
   0.0165897,
  -0.0829085,
  -0.0587608,
   0.1736804,
   0.4222137,
   0.4222137,
   0.1736804,
  -0.0587608,
  -0.0829085,
   0.0165897,
   0.0541009,
  -0.0003152,
  -0.0377408,
  -0.0074772,
   0.0262645,
   0.0110390,
  -0.0175779,
  -0.0120883,
   0.0109293,
   0.0115377,
  -0.0059350,
  -0.0100052,
   0.0030542,
   0.0183372,
  -0.0011405  
};

// Storage of the audio samples.
int16_t sampleBuffer[80000];

// Storage for our interpolated samples.
float outputBuffer[160000];

// File output buffer.
int16_t fileOutputBuffer[160000];

Interpolator *myFilterPtr;

int main(int argc,char **argv)
{
  FILE *inputFileStream, *outputFileStream;
  size_t count;
  int i, j;
  int filterLength, inputSampleLength, outputSampleLength;

  // Ten seconds at 8000S/s.
  inputSampleLength = 10 * 8000;

  // Ten seconds at 16000S/s.
  outputSampleLength = 10 * 16000;

  // Compute the number of taps in the prototype filter.
  filterLength = sizeof(h16000)/sizeof(float);

  // Create interpolator.
  myFilterPtr = new Interpolator(filterLength,h16000,2);

  // Open the input file for reading the audio samples.
  inputFileStream = fopen("original8000.raw","r");

  // Read 16-bit input samples from the input audio file.
  count = fread(sampleBuffer,2,inputSampleLength,inputFileStream);

  // We're done with this file.
  fclose(inputFileStream);

  for (i = 0; i < inputSampleLength; i++)
  {
    myFilterPtr->interpolate((float)sampleBuffer[i],&outputBuffer[i * 2]);
  } // for

  // Convert to integers.
  for (i = 0; i < outputSampleLength; i++)
  {
    fileOutputBuffer[i] = (int16_t)outputBuffer[i];
  } // for

  // Open the output file for writing the interpolated the audio samples.
  outputFileStream = fopen("interpolated16000.raw","w");

  // Write output samples to the output audio file.
  count = fwrite(fileOutputBuffer,2,outputSampleLength,outputFileStream);

  // We're done with this file.
  fclose(outputFileStream);

  // Release resources.
  delete myFilterPtr;

  return (0);

} // main

