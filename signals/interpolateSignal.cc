//************************************************************************
// file name: interpolateSignal.cc
//************************************************************************
//_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
// This program reads samples from stdin, increases the sample rate,
// and writes the samples to stdout. The format of the input data is
// 16-bit signed integers that represent an IQ data stream that is sampled
// at 8000S/s.  The data is interleaved with the first sample representing
// the I sample, and the second sample representing the Q sample.  The
// output is a stream of 8-bit signed IQ pairs that are upsampled to a
// rate of 2048000S/s.
//
// To build, type,
//  g++ -o interpolateSignal interpolateSignal.cc Interpolator.cc -lm
//
// To run, type,
// ./interpolateSignal < inputFile > outputFile
//
//_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <ctype.h>

#include "Interpolator_int16.h"

using namespace std;

float interpolator1Coefficients[] =
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
  0.0587608,
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

float interpolator2Coefficients[] =
{
  -0.0440934,
  0,
  0.2913764,
  0.5000000,
  0.2913764,
  0,
  -0.0440934,
  0
};

float interpolator3Coefficients[] =
{
  0.2570951,
  0.5000000,
  0.2570951,
  0
};

float interpolator4Coefficients[] =
{
  -0.0440934,
  0,
  0.2913764,
  0.5000000,
  0.2913764,
  0,
  -0.0440934,
  0
};

float interpolator5Coefficients[] =
{
  -0.0440934,
  0,
  0.2913764,
  0.5000000,
  0.2913764,
  0,
  -0.0440934,
  0
};

float interpolator6Coefficients[] =
{
  0.2570951,
  0.5000000,
  0.2570951,
  0
};

float interpolator7Coefficients[] =
{
  0.2517491,
  0.4999998,
  0.2517491,
  0
};

float interpolator8Coefficients[] =
{
  0.2504357,
  0.5000000,
  0.2504357,
  0
};

int16_t iqSampleBuffer[512];
int8_t outputBuffer[512];

int16_t iStage1Buffer[2];
int16_t iStage2Buffer[4];
int16_t iStage3Buffer[8];
int16_t iStage4Buffer[16];
int16_t iStage5Buffer[32];
int16_t iStage6Buffer[64];
int16_t iStage7Buffer[128];
int16_t iStage8Buffer[256];
int16_t qStage1Buffer[2];
int16_t qStage2Buffer[4];
int16_t qStage3Buffer[8];
int16_t qStage4Buffer[16];
int16_t qStage5Buffer[32];
int16_t qStage6Buffer[64];
int16_t qStage7Buffer[128];
int16_t qStage8Buffer[256];

Interpolator_int16 *iFilter1Ptr;
Interpolator_int16 *iFilter2Ptr;
Interpolator_int16 *iFilter3Ptr;
Interpolator_int16 *iFilter4Ptr;
Interpolator_int16 *iFilter5Ptr;
Interpolator_int16 *iFilter6Ptr;
Interpolator_int16 *iFilter7Ptr;
Interpolator_int16 *iFilter8Ptr;
Interpolator_int16 *qFilter1Ptr;
Interpolator_int16 *qFilter2Ptr;
Interpolator_int16 *qFilter3Ptr;
Interpolator_int16 *qFilter4Ptr;
Interpolator_int16 *qFilter5Ptr;
Interpolator_int16 *qFilter6Ptr;
Interpolator_int16 *qFilter7Ptr;
Interpolator_int16 *qFilter8Ptr;

int main(int argc,char **argv)
{
  bool done;
  size_t count, outCount;
  int i, j;
  int filter1Length, filter2Length, filter3Length, filter4Length;
  int filter5Length, filter6Length, filter7Length, filter8Length;

  // Compute the number of taps in the prototype filter.
  filter1Length = sizeof(interpolator1Coefficients)/sizeof(float);
  filter2Length = sizeof(interpolator2Coefficients)/sizeof(float);
  filter3Length = sizeof(interpolator3Coefficients)/sizeof(float);
  filter4Length = sizeof(interpolator4Coefficients)/sizeof(float);
  filter5Length = sizeof(interpolator5Coefficients)/sizeof(float);
  filter6Length = sizeof(interpolator6Coefficients)/sizeof(float);
  filter7Length = sizeof(interpolator7Coefficients)/sizeof(float);
  filter8Length = sizeof(interpolator8Coefficients)/sizeof(float);

  // Create interpolators.
  iFilter1Ptr = new Interpolator_int16(filter1Length,
                                       interpolator1Coefficients,
                                       2);
  iFilter2Ptr = new Interpolator_int16(filter2Length,
                                       interpolator2Coefficients,
                                       2);
  iFilter3Ptr = new Interpolator_int16(filter3Length,
                                       interpolator3Coefficients,
                                       2);
  iFilter4Ptr = new Interpolator_int16(filter4Length,
                                       interpolator4Coefficients,
                                       2);
  iFilter5Ptr = new Interpolator_int16(filter5Length,
                                       interpolator5Coefficients,
                                       2);
  iFilter6Ptr = new Interpolator_int16(filter6Length,
                                       interpolator6Coefficients,
                                       2);
  iFilter7Ptr = new Interpolator_int16(filter7Length,
                                       interpolator7Coefficients,
                                       2);
  iFilter8Ptr = new Interpolator_int16(filter8Length,
                                       interpolator8Coefficients,
                                       2);
  qFilter1Ptr = new Interpolator_int16(filter1Length,
                                       interpolator1Coefficients,
                                       2);
  qFilter2Ptr = new Interpolator_int16(filter2Length,
                                       interpolator2Coefficients,
                                       2);
  qFilter3Ptr = new Interpolator_int16(filter3Length,
                                       interpolator3Coefficients,
                                       2);
  qFilter4Ptr = new Interpolator_int16(filter4Length,
                                       interpolator4Coefficients,
                                       2);
  qFilter5Ptr = new Interpolator_int16(filter5Length,
                                       interpolator5Coefficients,
                                       2);
  qFilter6Ptr = new Interpolator_int16(filter6Length,
                                       interpolator6Coefficients,
                                       2);
  qFilter7Ptr = new Interpolator_int16(filter7Length,
                                       interpolator7Coefficients,
                                       2);
  qFilter8Ptr = new Interpolator_int16(filter8Length,
                                       interpolator8Coefficients,
                                       2);

  // Set up for loop entry.
  done = false;

  while (!done)
  {
    // Read 16-bit IQ input sample pair from stdin.
    count = fread(iqSampleBuffer,2,512,stdin);

    if (count == 0)
    {
      // We're done.
      done = true;
    } // if
    else
    {
      for (j = 0; j < count; j += 2)
      {
        //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
        // Interpolate the in-phase samples.
        //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
        // Perform the first stage of interpolation (1:2).
        iFilter1Ptr->interpolate(iqSampleBuffer[j],iStage1Buffer);

        // Perform the second stage of interpolation (cascade: 1:4).
        for (i = 0; i < 2; i++)
        {
          iFilter2Ptr->interpolate(iStage1Buffer[i],&iStage2Buffer[i * 2]);
        } // for

        // Perform the third stage of interpolation (cascade: 1:8).
        for (i = 0; i < 4; i++)
        {
          iFilter3Ptr->interpolate(iStage2Buffer[i],&iStage3Buffer[i * 2]);
        } // for

        // Perform the fourth stage of interpolation (cascade: 1:16).
        for (i = 0; i < 8; i++)
        {
          iFilter4Ptr->interpolate(iStage3Buffer[i],&iStage4Buffer[i * 2]);
        } // for

        // Perform the fifth stage of interpolation (cascade: 1:32).
        for (i = 0; i < 16; i++)
        {
          iFilter5Ptr->interpolate(iStage4Buffer[i],&iStage5Buffer[i * 2]);
        } // for

        // Perform the sixth stage of interpolation (cascade: 1:64).
        for (i = 0; i < 32; i++)
        {
          iFilter6Ptr->interpolate(iStage5Buffer[i],&iStage6Buffer[i * 2]);
        } // for

        // Perform the seventh stage of interpolation (cascade: 1:128).
        for (i = 0; i < 64; i++)
        {
          iFilter7Ptr->interpolate(iStage6Buffer[i],&iStage7Buffer[i * 2]);
        } // for

        // Perform the eighth stage of interpolation (cascade: 1:256).
        for (i = 0; i < 128; i++)
        {
          iFilter8Ptr->interpolate(iStage7Buffer[i],&iStage8Buffer[i * 2]);
        } // for
        //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/

        //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
        // Interpolate the quadrature samples.
        //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
        // Perform the first stage of interpolation (now at 1:2).
        qFilter1Ptr->interpolate(iqSampleBuffer[j+1],qStage1Buffer);

        // Perform the second stage of interpolation (cascade: 1:4).
        for (i = 0; i < 2; i++)
        {
          qFilter2Ptr->interpolate(qStage1Buffer[i],&qStage2Buffer[i * 2]);
        } // for

        // Perform the third stage of interpolation (cascade: 1:8).
        for (i = 0; i < 4; i++)
        {
          qFilter3Ptr->interpolate(qStage2Buffer[i],&qStage3Buffer[i * 2]);
        } // for

        // Perform the fourth stage of interpolation (cascade: 1:16).
        for (i = 0; i < 8; i++)
        {
          qFilter4Ptr->interpolate(qStage3Buffer[i],&qStage4Buffer[i * 2]);
        } // for

        // Perform the fifth stage of interpolation (cascade: 1:32).
        for (i = 0; i < 16; i++)
        {
          qFilter5Ptr->interpolate(qStage4Buffer[i],&qStage5Buffer[i * 2]);
        } // for

        // Perform the sixth stage of interpolation (cascade: 1:64).
        for (i = 0; i < 32; i++)
        {
          qFilter6Ptr->interpolate(qStage5Buffer[i],&qStage6Buffer[i * 2]);
        } // for

        // Perform the seventh stage of interpolation (cascade: 1:128).
        for (i = 0; i < 64; i++)
        {
          qFilter7Ptr->interpolate(qStage6Buffer[i],&qStage7Buffer[i * 2]);
        } // for

        // Perform the eighth stage of interpolation (cascade: 1:256).
        for (i = 0; i < 128; i++)
        {
          qFilter8Ptr->interpolate(qStage7Buffer[i],&qStage8Buffer[i * 2]);
        } // for
        //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/

        // Construct the output buffer.
        for (i = 0; i < 512; i += 2)
        {
          // Use these variables to make things easier.
          outputBuffer[i]  = (int8_t)iStage8Buffer[i >> 1];
          outputBuffer[i+1] = (int8_t)qStage8Buffer[i >> 1];
        } // for

        // Write the interleaved baseband output to stdout.
        outCount = fwrite(outputBuffer,1,512,stdout);
      } // for
    } // else
  } // while

  // Release resources.
  delete iFilter1Ptr;
  delete iFilter2Ptr;
  delete iFilter3Ptr;
  delete iFilter4Ptr;
  delete iFilter5Ptr;
  delete iFilter6Ptr;
  delete iFilter7Ptr;
  delete iFilter8Ptr;
  delete qFilter1Ptr;
  delete qFilter2Ptr;
  delete qFilter3Ptr;
  delete qFilter4Ptr;
  delete qFilter5Ptr;
  delete qFilter6Ptr;
  delete qFilter7Ptr;
  delete qFilter8Ptr;

  return (0);

} // main

