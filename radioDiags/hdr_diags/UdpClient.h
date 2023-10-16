//**********************************************************************
// file name: UdpClient.h
//**********************************************************************

#ifndef _UDPCLIENT_H_
#define _UDPCLIENT_H_

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>

class UdpClient
{
  public:

  UdpClient(char *ipAddressPtr,int port);
  ~UdpClient(void);

  bool connectionIsEstablished(void);
  bool sendData(void *bufferPtr,int bufferLength);

  private:

  // Attributes
  size_t maxPayloadLength;
  int socketDescriptor;
  struct sockaddr_in peerAddress;
};

#endif // _UDPCLIENT_H_
