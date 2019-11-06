/*************************************************************************/
/* file name: console.c                                                  */
/*************************************************************************/

#include <unistd.h>
#include <stdio.h>
#include <sys/select.h>
#include <string.h>

#include "console.h"

#define CONSOLE_SLEEP_TIME_IN_MICROSECONDS (2000)

extern bool diagUi_timeToExit;

/**************************************************************************

  Name: console

  Purpose: The purpose of this function is to initialize the console
  class.  This function serves as the constructor of this class.

  Calling Sequence: console(networkFileDescriptor,connectStatePtr)

  Inputs:

    networkFileDescriptor - The file descriptor used for network I/O.

    connectStatePtr - A pointer to a boolean state variable that indicates
    to the application the link state.  A value of true indicates that
    a network connection exists, and a value of false indicates that a
    network connection does not exist.

  Outputs:

   None.

**************************************************************************/
console::console(int networkFileDescriptor,bool *connectStatePtr)
{

  // retrieve for later use
  this->networkFileDescriptor = networkFileDescriptor;
  this->connectStatePtr = connectStatePtr;

} // console

/**************************************************************************

  Name: ~console

  Purpose: The purpose of this function is to perform the necessary
  cleanup for the console class.  This function serves as the
  destructor of this class.

  Calling Sequence: ~console()

  Inputs:

    None.

  Outputs:

   None.

**************************************************************************/
console::~console(void)
{

} // ~console

/**************************************************************************

  Name: getChar

  Purpose: The purpose of this function is to get a character from the
  console.

  Calling Sequence: ch = getChar()

  Inputs:

    None.

  Outputs:

    ch - The character received from the console.

**************************************************************************/
char console::getChar(void)
{
  int done;
  char ch;
  ssize_t byteCount;

  // set up for loop entry
  done = 0;

  while (!done)
  {
    // sleep for a while so that we don't hog CPU bandwidth
    usleep(CONSOLE_SLEEP_TIME_IN_MICROSECONDS);

    if (kbhit())
    {
      // bail out of loop
      done = 1;
    } // if

    if (diagUi_timeToExit)
    {
      // bail out of loop
      done = 1;
    } // if

  } // while

  if (!diagUi_timeToExit)
  {
    /* get character */
    byteCount = read(networkFileDescriptor,&ch,1);

    if (byteCount == 0)
    {
      // indicate that network connection is dropped
      *connectStatePtr = false;
    } // if
  } // if

  return (ch);

} // getChar

/**************************************************************************

  Name: putChar

  Purpose: The purpose of this function is to display a character to the
  console.

  Calling Sequence: putChar(ch)

  Inputs:

    ch - The character to display.

  Outputs:

    None.

**************************************************************************/
void console::putChar(char ch)
{

  // send the character to the network
  write(networkFileDescriptor,&ch,1);

  return;

} // putChar

/**************************************************************************

  Name: putLine

  Purpose: The purpose of this function is to send a buffer of characters
  to the console.  A line is defined as a NULL terminated string.

  Calling Sequence: putLine(bufferPtr)

  Inputs:

    bufferPtr - A pointer to a buffer to display.

  Outputs:

    None.

**************************************************************************/
void console::putLine(const char *bufferPtr)
{
  int length;

  // number of octets
  length = strlen(bufferPtr);

  // send the data to the network
  write(networkFileDescriptor,bufferPtr,length);

  return;

} /* putLine */

/**************************************************************************

  Name: getLine

  Purpose: The purpose of this function is to get a line of data from
  the console.  A line is defined by a collection of one or more
  characters terminated by a carriage return.

  Calling Sequence: length = getLine(bufferPtr,bufferLength)

  Inputs:

    bufferPtr - A pointer to an buffer to receive data.

    bufferLength - The number of bytes allowed in the buffer.

  Outputs:

    length - The number of characters that were typed.

    bufferPtr - A pointer to the buffer of data.  This will be a
    NULL terminated string.

**************************************************************************/
unsigned short console::getLine(char *bufferPtr,unsigned short bufferLength)
{
  unsigned char ch, done;
  int count;
  unsigned short length;

  putLine(">"); // display prompt

  done = 0;
  count = 0;

  while (done == 0)
  {
    ch = getChar(); // get a byte from the serial interface

    if ((diagUi_timeToExit) || (*connectStatePtr == false))
    {
      // we don't need to process any commands
      count = 0;
      // bail out of loop
      break;
    } // if

    switch (ch)
    {
      case '\r': // yeah we get the carriage return with telnet
        break;

      case '\n': // end of line
        *bufferPtr = '\0'; // terminate our string
        /* putChar('\n'); */  /* send newline */
        /* putChar(ch);   */  /* echo the carriage return */
        done = 1; // we're done!
        break;

      case 0x08: // backspace
      case 0x7f: // rubout
        if (count > 0)
        {
          putChar(0x08); // erase our character from the display
          putChar(' ');
          putChar(0x08);
          count--; // retract our count
          bufferPtr--; // retract our buffer pointer
        } /* if */
        break;

      default: // any other character
        if (count < bufferLength)
        {
          count++; // advance our count
          *bufferPtr = ch; // store our character
          bufferPtr++; // reference our next position in the buffer
          if (ch >= ' ')
          { // display displayable characters
            // putChar(ch); // display the character
          } // if
        } // if
      break;

    } // case

    // time to exit the thread
    if (diagUi_timeToExit)
    {
      // bail out of loop
      done = 1;
    } // if
  } // while

  length = (unsigned short)count; // return number of characters typed
 
  return (length);

} // getLine

/**************************************************************************

  Name: waitForOperatorIntervention

  Purpose: The purpose of this function is to provide a mechanism for
  pausing the system until the operator presses a key.

  Calling Sequence: waitForOperatorIntervention()

  Inputs:

    None.

  Outputs:

    None.

**************************************************************************/
void console::waitForOperatorIntervention(void)
{

  putLine("\npress <enter> key to continue\n");

  // consume typed character
  getChar();

 return;

} // waitForOperatorIntervention

/**************************************************************************

  Name: kbhit

  Purpose: The purpose of this function is to determine if a key has
  been pressed at the console.  Credit goes to
  http://www.fedoraforum.org/forum/archive/index.php/t-172337.html.  The
  poster was ibbo.

  Calling Sequence: status = kbhit()

  Inputs:

    None.

  Outputs:

    status - The returned status.  A value of 0 indicates a key was not
    pressed, and a value of 1 indicates that a key was pressed.

**************************************************************************/
int console::kbhit(void)
{
  struct timeval tv;
  fd_set read_fd;

  // Do not wait at all, not even a microsecond.
  tv.tv_sec=0;
  tv.tv_usec=0;

  // Must be done first to initialize read_fd.
  FD_ZERO(&read_fd);

  // Makes select() ask if input is ready:
  // 0 is the file descriptor for stdin.
  FD_SET(networkFileDescriptor,&read_fd);

  // The first parameter is the number of the
  // largest file descriptor to check + 1.
  if(select(networkFileDescriptor+1,&read_fd,NULL,NULL,&tv) == -1)
  {
    // an error occurred
    return (0);
  } // if

  // read_fd now holds a bit map of files that are
  // readable. We test the entry for the standard
  // input (file 0). */
  if(FD_ISSET(networkFileDescriptor,&read_fd))
  {
    // Character pending on stdin
    return (1);
  } // if

  /* no characters were pending */
  return (0);

} // kbhit
