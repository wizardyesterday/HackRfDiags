//************************************************************************
// file name: PhaseAccumulator.cc
//************************************************************************
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>

#include "PhaseAccumulator.h"

using namespace std;

/*****************************************************************************

  Name: PhaseAccumulator

  Purpose: The purpose of this function is to serve as the constructor for
  an instance of a PhaseAccumulator.

  Calling Sequence: PhaseAccumulator(sampleRate,frequency);

  Inputs:

    sampleRate - The sample rate in S/s.

    frequency - The frequency in Hz.

  Outputs:

    None.

*****************************************************************************/
PhaseAccumulator::PhaseAccumulator(float sampleRate,float frequency)
{

  // Save for frequency updates.
  this->sampleRate = sampleRate;

  // Save for display purposes.
  this->frequency = frequency;

  // The phase accumulator will need this.
  this->phaseStepSize = (2 * M_PI * frequency) / sampleRate;

  // Set system to an initial state.
  reset();

  return;

} // PhaseAccumulator

/*****************************************************************************

  Name: ~PhaseAccumulator

  Purpose: The purpose of this function is to serve as the destructor for
  an instance of a PhaseAccumulator.

  Calling Sequence: ~PhaseAccumulator()

  Inputs:

    None.

  Outputs:

    None.

*****************************************************************************/
PhaseAccumulator::~PhaseAccumulator(void)
{

  return;

} // ~PhaseAccumulator

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
void PhaseAccumulator::setFrequency(float frequency)
{
  uint32_t i;

  // Save for later use.
  this->frequency = frequency;

  // The phase accumulator will need this.
  this->phaseStepSize = (2 * M_PI * frequency) / sampleRate;

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
void PhaseAccumulator::reset(void)
{
  uint32_t i;

  // Reset the phase to the starting point.
  phaseAccumulator = 0;

  return;

} // reset

/*****************************************************************************

  Name: run

  Purpose: The purpose of this function is to generate one sample of the
  next phase value of the phase accumulator bounded by the relation,
  -PI <= phase < PI.

  Calling Sequence: phase = run()

  Inputs:

    None.

  Outputs:

    None.

*****************************************************************************/
float PhaseAccumulator::run(void)
{
  float phase;

  // Retrieve the current phase accumulator value.
  phase = phaseAccumulator;

  // Update the phase accumulator.
  phaseAccumulator += phaseStepSize;

  while (phaseAccumulator > M_PI)
  {
    // Wrap the accumulator.
    phaseAccumulator-= (2 * M_PI);
  } // if

  while (phaseAccumulator < (-M_PI))
  {
    // Wrap the accumulator.
    phaseAccumulator += (2 * M_PI);
  } // while

  return (phase);

} // run

