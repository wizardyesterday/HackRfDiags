//************************************************************************
// file name: testFirFilter.cc
//************************************************************************
//_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
// This program tests the FirFilter class by applying a unit impulse
// signal and a unit step signal to the filter structure.
//
// To build, type,
//  g++ -o testFirFilter testFirFilter.cc FirFilter.cc -lm
//
// To run, type,
// ./testFirFilter
//_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include "FirFilter.h"

using namespace std;

FirFilter *myFilterPtr;

float coefficients[] = {1,2,3,4,1,1,1,8};
float impulse[] = {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
float step[] = {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1};

int main(int argc,char **argv)
{
  int i;
  int filterLength, inputSampleLength;
  float sample;

  filterLength = sizeof(coefficients) / sizeof(float);
  inputSampleLength = sizeof(impulse)/sizeof(float);

  // Instantiate filter.
  myFilterPtr = new FirFilter(filterLength,coefficients);

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

  // Release resources.
  delete myFilterPtr;

  return (0);

} // main
