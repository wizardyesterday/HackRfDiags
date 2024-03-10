//************************************************************************
// file name: Nco.cc
//************************************************************************
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>

#include "Nco.h"

using namespace std;

/*****************************************************************************

  Name: Nco

  Purpose: The purpose of this function is to serve as the constructor for
  an instance of a Nco.

  Calling Sequence: Nco(sampleRate,frequency);

  Inputs:

    sampleRate - The sample rate in S/s.

    frequency - The frequency in Hz.

  Outputs:

    None.

*****************************************************************************/
Nco::Nco(float sampleRate,float frequency)
{
  int i;
  float phaseAngle;
  float phaseIncrement;

  // Save for frequency updates.
  this->sampleRate = sampleRate;

  // Save for display purposes.
  this->frequency = frequency;

  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
  // Construct the sine and cosine tables.  The representation of the
  // phase is in a signed fractional format with 1 sign bit, two mantissa
  // bits and 13 fractional bits.
  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
  phaseIncrement = 2 * M_PI / 16384;
  phaseAngle = -M_PI;

  for (i = 0; i < 16384; i++)
  {
    // Construct sine and cosine tables.
    Sin[i] = sin(phaseAngle);
    Cos[i] = cos(phaseAngle);

    // Advance phase.
    phaseAngle += phaseIncrement;
  } // for
  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/

  // Create an instance of a phase accumulator.
  phaseAccumulatorPtr = new PhaseAccumulator(sampleRate,frequency);

  // Set system to an initial state.
  reset();

  return;

} // Nco

/*****************************************************************************

  Name: ~Nco

  Purpose: The purpose of this function is to serve as the destructor for
  an instance of a Nco.

  Calling Sequence: ~Nco()

  Inputs:

    None.

  Outputs:

    None.

*****************************************************************************/
Nco::~Nco(void)
{

  // Release resources.
  if (phaseAccumulatorPtr != NULL)
  {
    delete phaseAccumulatorPtr;
  } // if

  return;

} // ~Nco

/*****************************************************************************

  Name: setFrquency

  Purpose: The purpose of this function is to reset all runtime values to
  initial values.

  Calling Sequence: setFrequency(frequency)

  Inputs:

    frequency - The operating frequency in Hz.

  Outputs:

    None.

*****************************************************************************/
void Nco::setFrequency(float frequency)
{
  uint32_t i;

  // Save for later use.
  this->frequency = frequency;

  // Update the phase accumulator.
  phaseAccumulatorPtr->setFrequency(frequency);

  return;

} // setFrequency

/*****************************************************************************

  Name: reset

  Purpose: The purpose of this function is to reset all runtime values to
  initial values.

  Calling Sequence: reset()

  Inputs:

    None.

  Outputs:

    None.

*****************************************************************************/
void Nco::reset(void)
{
  uint32_t i;

  // Reset the phase accumulator to the starting point.
  phaseAccumulatorPtr->reset();

  return;

} // reset

/*****************************************************************************

  Name: run

  Purpose: The purpose of this function is to generate one sample of a
  complex exponentional function.

  Calling Sequence: run(iValuePtr,qValuePtr)

  Inputs:

    iValuePtr - A pointer to storage for the in-phase component.

    qValuePtr - A pointer to storage for the quadrature component.

  Outputs:

    None.

*****************************************************************************/
void Nco::run(float *iValuePtr,float *qValuePtr)
{
  float phase;

  // Run and get the next phase value.
  phase = phaseAccumulatorPtr->run();

  // Generate the next complex sinusoid sample.
  *iValuePtr = cos(phase);
  *qValuePtr = sin(phase);

  return;

} // run

/*****************************************************************************

  Name: runFast

  Purpose: The purpose of this function is to generate one sample of a
  complex exponentional function.  This function uses table lookup for
  speed.

  Calling Sequence: runFast(iValuePtr,qValuePtr)

  Inputs:

    iValuePtr - A pointer to storage for the in-phase component.

    qValuePtr - A pointer to storage for the quadrature component.

  Outputs:

    None.

*****************************************************************************/
void Nco::runFast(float *iValuePtr,float *qValuePtr)
{
  float phase;
  int phaseTableIndex;

  // Run and get the next phase value.
  phase = phaseAccumulatorPtr->run();

  // Map the phase to a table index.
  phaseTableIndex = (int16_t)(phase * 16384 / (2 * M_PI));
  phaseTableIndex += 8192;

  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
  // Ensure that roundoff error doesn't take
  // the index out of bounds.
  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
  if (phaseTableIndex < 0)
  {
    phaseTableIndex = 0;
  } // if
  else
  {
    if (phaseTableIndex > 16383)
    {
      phaseTableIndex = 16383;
    } // if
  } // else
  //_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/

  // Generate the next complex sinusoid sample.
  *iValuePtr = Cos[phaseTableIndex];
  *qValuePtr = Sin[phaseTableIndex];

  return;

} // runFast


