//************************************************************************
// file name: testFirFilter.cc
//************************************************************************
//_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
// This program tests the IirFilter class by applying a unit impulse
// signal and a unit step signal to the filter structure.
//
// To build, type,
//  g++ -o testFirFilter testFirFilter.cc FirFilter.cc -lm
//
// To run, type,
// ./testIirFilter
//_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include "IirFilter.h"

using namespace std;

IirFilter *myFilterPtr;
IirFilter *myDcRemovalFilterPtr;

float numeratorCoefficents[] = {1};
float denominatorCoefficients[] = {0.5};

float nDc[] = {1, -1};
float dDc[] = {-0.95};

float impulse[] = {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
float step[] = {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1};

int main(int argc,char **argv)
{
  int i;
  int numeratorFilterLength, denominatorFilterLength, inputSampleLength;
  float sample;

  numeratorFilterLength = sizeof(numeratorCoefficents) / sizeof(float);
  denominatorFilterLength = sizeof(denominatorCoefficients) / sizeof(float);

  inputSampleLength = sizeof(impulse)/sizeof(float);

  // Instantiate filter.
  myFilterPtr = new IirFilter(numeratorFilterLength,
                              numeratorCoefficents,
                              denominatorFilterLength,
                              denominatorCoefficients);

  // Instantiate dc removal filter.
  myDcRemovalFilterPtr = new IirFilter(2,nDc,1,dDc);

  printf("Testing filter with impulse.\n");
  for (i = 0; i < inputSampleLength; i++)
  {
    sample = myFilterPtr->filterData(impulse[i]);
    printf("sample[%d] = %f\n",i,sample);
  } // for

  // Send newline.
  printf("\n");

  // Reset the filter state.
  myFilterPtr->resetFilterState();

  // Send newline.
  printf("\n");

  printf("Testing filter with step.\n");
  for (i = 0; i < inputSampleLength; i++)
  {
    sample = myFilterPtr->filterData(step[i]);
    printf("sample[%d] = %f\n",i,sample);
  } // for

  // Send newline.
  printf("\n");

  printf("Testing dc removal filter with impulse.\n");
  for (i = 0; i < inputSampleLength; i++)
  {
    sample = myDcRemovalFilterPtr->filterData(impulse[i]);
    printf("sample[%d] = %f\n",i,sample);
  } // for

  // Send newline.
  printf("\n");

  // Reset the filter state.
  myDcRemovalFilterPtr->resetFilterState();

  // Send newline.
  printf("\n");

  printf("Testing dc removal filter with step.\n");
  for (i = 0; i < inputSampleLength; i++)
  {
    sample = myDcRemovalFilterPtr->filterData(step[i]);
    printf("sample[%d] = %f\n",i,sample);
  } // for

  // Release resources.
  delete myFilterPtr;
  delete myDcRemovalFilterPtr;

  return (0);

} // main
