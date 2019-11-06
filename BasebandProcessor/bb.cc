//************************************************************************
// file name: bb.cc
//************************************************************************
//_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
// This program reads PCM samples from stdin, and converts the signal to
// a baseband representation of AM, FM, or SSB. 
//
// To build, type,
//  sh buildbb.sh
//
// To run, type,
// ./bb {a|f|l|u < inputFile > outputFile
//
//_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
#include <stdio.h>
#include <unistd.h>

#include "AmModulator.h"
#include "FmModulator.h"
#include "SsbModulator.h"
#include "BasebandDataProcessor.h"

AmModulator *myAmPtr;
FmModulator *myFmPtr;
SsbModulator *mySsbPtr;
BasebandDataProcessor *myBbPtr;

int main(int argc,char **argv)
{
  int i;
  BasebandDataProcessor::modulatorType kludge;

  i = 0;
  myAmPtr = new AmModulator();
  myFmPtr = new FmModulator();
  mySsbPtr = new SsbModulator();
  myBbPtr = new BasebandDataProcessor();

  myBbPtr->setAmModulator(myAmPtr);
  myBbPtr->setFmModulator(myFmPtr);
  myBbPtr->setSsbModulator(mySsbPtr);

  // Default to no modulation.
  kludge = static_cast<BasebandDataProcessor::modulatorType>(0);

  if (argc == 2)
  {
    switch (argv[1][0])
    {
      case 'a': // AM
      {
        kludge = static_cast<BasebandDataProcessor::modulatorType>(1);
        break;
      } // case

      case 'f': // FM
      {
        kludge = static_cast<BasebandDataProcessor::modulatorType>(2);
        break;
      } // case

      case 'l': // LSB
      {
        kludge = static_cast<BasebandDataProcessor::modulatorType>(4);
        break;
      } // case

      case 'u': // USB
      {
        kludge = static_cast<BasebandDataProcessor::modulatorType>(5);
        break;
      } // case

      default: // None
      {
        kludge = static_cast<BasebandDataProcessor::modulatorType>(0);
      } // case

    } // switch
  } // if

  // Set the modulation mode of choice.
  myBbPtr->setModulatorMode(kludge);

  // Start the data stream.
  myBbPtr->start();

  while (1)
  {
    // Do something.
    i++;

    // Be nice to the system.
    usleep(5000);
  } // while

  myBbPtr->stop();

  delete myAmPtr;
  delete myFmPtr;
  delete mySsbPtr;
  delete myBbPtr;

  return (0);

} // main
