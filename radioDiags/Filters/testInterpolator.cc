//************************************************************************
// file name: testInterpolator.cc
//************************************************************************
//_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
// This program tests the Interpolator class by applying a unit impulse
// signal and a unit step signal to the filter structure.  An
// interpolation factor of 2 is used.
//
// To build, type,
//  g++ -o testInterpolator testINterpolator.cc Interpolator.cc -lm
//
// To run, type,
// ./testInterpolator
//_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include "Interpolator.h"

using namespace std;

float prototypeCoefficients[] = {1,2,3,4,5,6,7,8};
float impulse[] = {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
float step[] = {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1};
float outputBuffer[100];

Interpolator *myFilterPtr;

int main(int argc,char **argv)
{
  int i, j;
  int filterLength, inputSampleLength;
  int  L;

  filterLength = sizeof(prototypeCoefficients)/sizeof(float);
  inputSampleLength = sizeof(step)/sizeof(float);
  L = 2;

  // Create interpolator.
  myFilterPtr = new Interpolator(filterLength,prototypeCoefficients,L);

  printf("Testing filter with impulse with an interpolation factor of %d.\n",
         L);

  for (i = 0; i < inputSampleLength; i++)
  {
    myFilterPtr->interpolate(impulse[i],&outputBuffer[i * L]);
  } // for

  // Send newline.
  printf("\n");

  for (i = 0; i < (inputSampleLength * L); i++)
  {
    printf("Output[%d]: %f\n",i,outputBuffer[i]);
  } // for

  // Send newline.
  printf("\n");

  printf("Testing filter with step with an interpolation factor of %d.\n",
         L);

  for (i = 0; i < inputSampleLength; i++)
  {
    myFilterPtr->interpolate(step[i],&outputBuffer[i * L]);
  } // for

  // Send newline.
  printf("\n");

  for (i = 0; i < (inputSampleLength * L); i++)
  {
    printf("Output[%d]: %f\n",i,outputBuffer[i]);
  } // for

  // Send newline.
  printf("\n");

  // Release resources.
  delete myFilterPtr;

  return (0);

} // main

