//**********************************************************************
// file name: DataProvider.cc
//**********************************************************************

#include <stdio.h>
#include "DataProvider.h"

extern void nprintf(FILE *s,const char *formatPtr, ...);

/**************************************************************************

  Name: DataProvider

  Purpose: The purpose of this function is to serve as the constructor
  of a DataProvider object.

  Calling Sequence: DataProvider()

  Inputs:

   None.

  Outputs:

    None.

**************************************************************************/
DataProvider::DataProvider(void)
{

  // Ensure that we have no file name.
  iqFileName[0] = 0;

  // Indicate that no data has been loaded from an IQ file.
  iqSampleBufferPtr = 0;

  // Indicate that no data is in the buffer.
  iqSampleBufferLength = 0;

  // Start at the beginning of the buffer.
  iqSampleBufferIndex = 0;
 
  return;
 
} // DataProvider

/**************************************************************************

  Name: initializeStreamingParameters

  Purpose: The purpose of this function is to initialize all parameters
  that are associated with the streaming of IQ data. 

  Calling Sequence: setInitialTimeStamp(timeStamp)

  Inputs:

    timeStamp - The initial timestamp value to use for the transmission
    of data blocks over the radio.

  Outputs:

    None.

**************************************************************************/
void DataProvider::initializeStreamingParameters(void)
{

  // Ensure that we have no file name.
  iqFileName[0] = 0;

  // Indicate that no data has been loaded from an IQ file.
  iqSampleBufferPtr = 0;

  // Indicate that no data is in the buffer.
  iqSampleBufferLength = 0;

  // Start at the beginning of the buffer.
  iqSampleBufferIndex = 0;

  // Free the buffer if one has been previously allocated.
  if (iqSampleBufferPtr != 0)
  {
    // Release the resource.
    free(iqSampleBufferPtr);

    // We don't like dangling pointers.
    iqSampleBufferPtr = 0;
  } // if

  return;

} // initializeStreamingParameters
/**************************************************************************

  Name: ~DataProvider

  Purpose: The purpose of this function is to serve as the destructor
  of a DataProvider object.

  Calling Sequence: ~DataProvider()

  Inputs:

    None.

  Outputs:

    None.

**************************************************************************/
DataProvider::~DataProvider(void)
{

  return;
 
} // ~DataProvider

/**************************************************************************

  Name: getIqData

  Purpose: The purpose of this function is to provide IQ data to send to
  the radio.

  Calling Sequence: getIqData(bufferPtr,byteCount);

  Inputs:

    bufferPtr - A pointer to IQ data.

    byteCount - The number of bytes contained in the buffer that is
    referenced by bufferPtr.

  Outputs:

    None.

**************************************************************************/
void DataProvider::getIqData(int8_t *bufferPtr,
                             uint32_t byteCount)
{

  // Retrieve the IQ data samples.
  retrieveIqDataFromBuffer(bufferPtr,byteCount);
  
  return;
 
} // getIqData

/**************************************************************************

  Name: retrieveIqDataFromBuffer

  Purpose: The purpose of this function is to copy IQ data from a ring
  buffer to a buffer of data that is to be transmitted via the radio.
  If IQ data is available in the ring buffer (a file was opened and the
  buffer was filled with the contents of the file), the data will be
  copied, otherwise, no action is taken.

  Calling Sequence: retrieveIqDataFromBuffer(bufferPtr,byteCount)

  Inputs:

    bufferPtr - A pointer to where the IQ data is to be stored.

    byteCount - The number of bytes to copy.

  Outputs:

    None.

**************************************************************************/
void DataProvider::retrieveIqDataFromBuffer(
    int8_t *bufferPtr,
    uint32_t byteCount)
{
  uint32_t partitionedByteCount;

  if (iqSampleBufferPtr != 0)
  {
    while (byteCount > 0)
    {
      if (byteCount < (iqSampleBufferLength - iqSampleBufferIndex))
      {
        partitionedByteCount = byteCount; 
      } // if
      else
      {
        partitionedByteCount = iqSampleBufferLength - iqSampleBufferIndex;
      } // else

      // Copy those samples!
      memcpy(bufferPtr,
             (iqSampleBufferPtr + iqSampleBufferIndex),
             partitionedByteCount);

      // Less bytes to copy.
      byteCount -= partitionedByteCount;

      // Advance the destination buffer pointer.
      bufferPtr += partitionedByteCount;

      // Advance the index module the length.
      iqSampleBufferIndex += partitionedByteCount;
      iqSampleBufferIndex %= iqSampleBufferLength;
    } // while

  } // if

  return;

} // retrieveIqDataFromBuffer

/**************************************************************************

  Name: loadIqFile

  Purpose: The purpose of this function is to load IQ data samples from
  a file into the IQ data ring buffer.

  Calling Sequence: success = loadIqFile(fileNamePtr)

  Inputs:

    fileNamePtr - The name of the file that contains the IQ data samples.

  Outputs:

    success - A flag that indicates whether or not data was loaded.  A
    value of true indicates that data was loaded, and a value of false
    indicates that data was not loaded.

**************************************************************************/
bool DataProvider::loadIqFile(char *fileNamePtr)
{
  FILE *iqFileStreamPtr;
  bool success;

  // Default to a failure.
  success = false;

  // Set all parameters to an initial state for future use.
  initializeStreamingParameters();

  // Open the file containing the IQ data samples.
  iqFileStreamPtr = fopen(fileNamePtr,"r");

  if (iqFileStreamPtr != NULL)
  {
    // Seek to the end of the file so that the number of bytes can be found.
    fseek(iqFileStreamPtr,0L,SEEK_END);

    // Retrieve the number of bytes in the file.
    iqSampleBufferLength = ftell(iqFileStreamPtr);

    // Allocate the space to contain the IQ data samples.
    iqSampleBufferPtr = (int8_t *)malloc(iqSampleBufferLength);

    // We have a buffer.
    if (iqSampleBufferPtr != 0)
    {
      // Seek to the beginning of the file.
      fseek(iqFileStreamPtr,0L,SEEK_SET);

      // Retrieve the data.
      fread(iqSampleBufferPtr,1,iqSampleBufferLength,iqFileStreamPtr);

      // Save the file name for display, and zero-terminate the string.
      strncpy(iqFileName,fileNamePtr,strlen(fileNamePtr));
      iqFileName[strlen(fileNamePtr)] = '\0';

      // Indicate success.
      success = true;
    } // if

    // We're done with the file.
    fclose(iqFileStreamPtr);
  } // if

  return (success);

} // loadIqFile

/**************************************************************************

  Name: displayInternalInformation

  Purpose: The purpose of this function is to display internal
  in the data provider.

  Calling Sequence: displayInternalInformation()

  Inputs:

    None.

  Outputs:

    None.

**************************************************************************/
void DataProvider::displayInternalInformation(void)
{

  nprintf(stderr,"\n--------------------------------------------\n");
  nprintf(stderr,"Data Provider Internal Information\n");
  nprintf(stderr,"--------------------------------------------\n");

  nprintf(stderr,"IQ File Name            : %s\n",iqFileName);
  nprintf(stderr,"IQ Sample Buffer Length : %u\n",iqSampleBufferLength);
  nprintf(stderr,"IQ Sample Buffer Index  : %u\n",iqSampleBufferIndex);

  return;

} // displayInternalInformation

