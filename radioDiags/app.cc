#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#include <hackrf.h>

bool timeToExit;
size_t bytesReceived;

int receiveCallback(hackrf_transfer *transferPtr)
{
  int status;
  size_t bytesToWrite;
  size_t bytesWritten;

  // Default to success.
  status = 0;

  // Retrieve the number of bytes in the buffer.
  bytesToWrite = transferPtr->valid_length;

  // Update our accumulated value.
  bytesReceived += transferPtr->valid_length;

  if (bytesReceived >= 2000000)
  {
    // Indicate that we're done.
//    timeToExit = true;

    // Let the library know that we're done.
//    status = -1;
  } // if
  else
  {
    // Dump the contents to the iq file.
    bytesWritten = fwrite(transferPtr->buffer,1,bytesToWrite,stdout);
 
    if (bytesWritten != bytesToWrite)
    {
      // Notify the user of a problem.
      fprintf(stderr,"We're hosed.\n");

      // Indicate a failure.
      status = -1;
    } // if
  } // if

  return (status);
 
} // receiveCallback

int main(int argc,char **argv)
{
  int status;
  hackrf_device *devicePtr;
  unsigned int lnaGain;
  unsigned int vgaGain;
  unsigned int txVgaGain;
  int64_t frequency;
  int64_t sampleRate;
  int64_t loFrequency;

  // Indicate that it is not time to exit.
  timeToExit = false;

  // Indicate that no bytes have been received.
  bytesReceived = 0;

  // Initially we want NULL.
  devicePtr = (hackrf_device *)NULL;

  // Let's go with a low sample rate.
  sampleRate = 2560000LL;

  // Set default gains.
  lnaGain = 8;
  vgaGain = 20;
  txVgaGain = 0;

  // Set default frequencies.
  frequency = 162550000LL;
  loFrequency = 1000000000LL;

  status = hackrf_init();
  fprintf(stderr,"status: %d\n",status);

  status = hackrf_open_by_serial(NULL,&devicePtr);
  fprintf(stderr,"status: %d\n",status);

  status = hackrf_set_sample_rate(devicePtr,sampleRate);
  fprintf(stderr,"status: %d\n",status);

  status = hackrf_set_hw_sync_mode(devicePtr,0);
  fprintf(stderr,"status: %d\n",status);

  status = hackrf_set_vga_gain(devicePtr,vgaGain);
  fprintf(stderr,"status: %d\n",status);

  status = hackrf_set_lna_gain(devicePtr,lnaGain);
  fprintf(stderr,"status: %d\n",status);

  // Start the receiver.
  status = hackrf_start_rx(devicePtr,receiveCallback,NULL);
  fprintf(stderr,"status: %d\n",status);

  // Set the frequency.
  status = hackrf_set_freq(devicePtr,frequency);
  fprintf(stderr,"status: %d\n",status);

  status = hackrf_set_amp_enable(devicePtr,1);
  fprintf(stderr,"status: %d\n",status);

  status = hackrf_set_antenna_enable(devicePtr,0);
  fprintf(stderr,"status: %d\n",status);

  while ((hackrf_is_streaming(devicePtr) == HACKRF_TRUE) &&
         (timeToExit == false))
  {
  } // while

  status = hackrf_close(devicePtr);
  fprintf(stderr,"status: %d\n",status);

  status = hackrf_exit();
  fprintf(stderr,"status: %d\n",status);


} // main
