//**************************************************************************
// file name: console.h
//**************************************************************************

#ifndef __CONSOLE__
#define __CONSOLE__

class console
{
  //***************************** operations **************************

  public:

  console(int networkFileDescriptor,bool *connectStatePtr);
  ~console(void);
  void putLine(const char *bufferPtr);
  unsigned short getLine(char *bufferPtr,unsigned short bufferLength);
  void waitForOperatorIntervention(void);

  private:

  char getChar(void);
  void putChar(char ch);
  int kbhit(void);

  //***************************** operations **************************
  private:

  int networkFileDescriptor;
  bool *connectStatePtr;

};

#endif /* __CONSOLE__ */
