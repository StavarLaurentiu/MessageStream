// TAKEN FROM LAB 7
#include "utils.h"

#include <sys/socket.h>
#include <unistd.h>
#include <stdio.h>

// Receives len bytes from the socket sockfd and stores them in the buffer
// Blocks until all bytes are received
int recv_all(int sockfd, void *buffer, size_t len)
{
  size_t bytes_received = 0;
  size_t bytes_remaining = len;
  char *buff = (char *)buffer;

  while (bytes_remaining)
  {
    int bytes = recv(sockfd, buff, bytes_remaining, 0);
    if (bytes < 0)
    {
      perror("recv in recv_all failed");
      return -1;
    }

    bytes_received += bytes;
    bytes_remaining -= bytes;
    buff += bytes;
  }

  return bytes_received;
}

// Sends len bytes from the buffer to the socket sockfd
// Blocks until all bytes are sent
int send_all(int sockfd, void *buffer, size_t len)
{
  size_t bytes_sent = 0;
  size_t bytes_remaining = len;
  char *buff = (char *)buffer;

  while (bytes_remaining)
  {
    int bytes = send(sockfd, buff, bytes_remaining, 0);

    if (bytes < 0)
    {
      perror("send in send_all failed");
      return -1;
    }

    bytes_sent += bytes;
    bytes_remaining -= bytes;
    buff += bytes;
  }

  return bytes_sent;
}