//************************************************************************
// file name: pm.cc
//************************************************************************
//_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
// This program reads PCM samples from stdin, and converts the signal to
// a baseband representation of PM. 
//
// To build, type,
//  g++ -o pm pm.cc
//
// To run, type,
// ./pm < inputFile > outputFile
//
//_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <ctype.h>
#include <math.h>

using namespace std;

int main(int argc,char **argv)
{
  bool done;
  size_t count;
  float scaledSample;
  float iSample, qSample;
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

      // Map this to a phase angle.
      scaledSample /= 60000;
      scaledSample *= M_PI;

      // Create I and Q signals.


      iSample = cos(scaledSample) * 16000;
      qSample = sin(scaledSample) * 16000;

      iOutputSample = (int16_t)iSample;
      qOutputSample = (int16_t)qSample;;

      count = fwrite(&iOutputSample,2,1,stdout);
      count = fwrite(&qOutputSample,2,1,stdout);
    } // else
  } // while

  return (0);

} // main

