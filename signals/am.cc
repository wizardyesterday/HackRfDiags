//************************************************************************
// file name: am.cc
//************************************************************************
//_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
// This program reads PCM samples from stdin, and converts the signal to
// a baseband representation of large carrier AM. 
//
// To build, type,
//  g++ -o am am.cc
//
// To run, type,
// ./am < inputFile > outputFile
//
//_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <ctype.h>

using namespace std;

int main(int argc,char **argv)
{
  bool done;
  size_t count;
  float scaledSample;
  int16_t inputSample;
  int16_t iOutputSample, qOutputSample;

  // Set up for loop entry.
  done = false;

  while (!done)
  {
    // Read PCM input sample from stdin.
    count = fread(&inputSample,2,1,stdin);

    if (count == 0)
    {
      // We're done.
      done = true;
    } // if
    else
    {
      // Use these variables to make things easier.
      scaledSample = (float)inputSample;

      // Let's go with 80% modulation.
      scaledSample *= 0.8;

      // Insert the dc term to create a carrier.
      scaledSample += 65536;

      // Scale this to avoid overflow.
      scaledSample /= 4;

      iOutputSample = (int16_t)scaledSample;
      qOutputSample = (int16_t)scaledSample;

      count = fwrite(&iOutputSample,2,1,stdout);
      count = fwrite(&qOutputSample,2,1,stdout);
    } // else
  } // while

  return (0);

} // main

