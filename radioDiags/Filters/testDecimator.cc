//************************************************************************
// file name: testDecimator.cc
//************************************************************************
//_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
// This program tests the Decimator class by applying a unit impulse
// signal and a unit step signal to the filter structure.  A decimation
// factor of 2 is used.

// To build, type,
//  g++ -o testDecimator testDecimator.cc Decimator.cc -lm
//
// To run, type,
// ./testDecimator
//_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include "Decimator.h"

using namespace std;

Decimator *myFilterPtr;

float coefficients[] = {1,2,3,4,1,1,1,8};
float impulse[] = {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
float step[] = {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1};

float outputBuffer[100];

int main(int argc,char **argv)
{
  int i;
  int filterLength, inputSampleLength;
  float sample;
  bool outputSampleAvailable;

  filterLength = sizeof(coefficients) / sizeof(float);
  inputSampleLength = sizeof(impulse)/sizeof(float);

  // Instantiate filter.
  myFilterPtr = new Decimator(filterLength,coefficients,1);

  // Clear the output buffer.
  for (i = 0; i < 100; i++)
  {
    outputBuffer[i] = 0;
  } // for

  // Reset the filter state.
  myFilterPtr->resetFilterState();

  printf("Testing filter with decimation factor of 1 with impulse.\n");
  for (i = 0; i < inputSampleLength; i++)
  {
    outputSampleAvailable = myFilterPtr->decimate(impulse[i],&sample);

    if (outputSampleAvailable)
    {
      printf("sample[%d] = %f\n",i,sample);
    } // if
  } // for

  // Send newline.
  printf("\n");

  // Release resources.
  delete myFilterPtr;

  // Instantiate filter.
  myFilterPtr = new Decimator(filterLength,coefficients,2);

  printf("Testing filter with decimation factor of 2 with step.\n");
  for (i = 0; i < inputSampleLength; i++)
  {
    outputSampleAvailable = myFilterPtr->decimate(step[i],&sample);

    if (outputSampleAvailable)
    {
      printf("sample[%d] = %f\n",i/2,sample);
    } // if
  } // for

  // Release resources.
  delete myFilterPtr;

  return (0);

} // main
