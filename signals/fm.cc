//************************************************************************
// file name: fm.cc
//************************************************************************
//_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
// This program reads PCM samples from stdin, and converts the signal to
// a baseband representation of FM. 
//
// To build, type,
//  g++ -o fm fm.cc
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

using namespace std;

int main(int argc,char **argv)
{
  bool done;
  size_t count;
  int16_t inputSample;
  float currentSample, previousSample;
  float thetaNew, theta;
  float kF;
  float iSample, qSample;
  int16_t iOutputSample, qOutputSample;

  // Set modulator gain.
  kF = 3.5;

  // Initialize phase accumulator.
  theta = 0;

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
      thetaNew = (float)inputSample;
      thetaNew = thetaNew / 65536;

      // Scale the value by the modulator gain.
      thetaNew *= kF;

      // Update the phase accumulator.
      theta = theta + thetaNew;

      while (theta > (2 * M_PI))
      {
        // Wrap the accumulator.
        theta -= (2 * M_PI);
      } // while

      while (theta < (-(2 * M_PI)))
      {
        // Wrap the accumulator.
        theta += (2 * M_PI);
      } // while

      // Create I and Q signals.
      iSample = cos(theta) * 16000;
      qSample = sin(theta) * 16000;

      iOutputSample = (int16_t)iSample;
      qOutputSample = (int16_t)qSample;

      count = fwrite(&iOutputSample,2,1,stdout);
      count = fwrite(&qOutputSample,2,1,stdout);
    } // else
  } // while

  return (0);

} // main

