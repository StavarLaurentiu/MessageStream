#ifndef _HELPERS_H
#define _HELPERS_H 1

#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <poll.h>
#include <vector>
#include <map>
#include <algorithm>

using namespace std;

#include "po_udp.h"
#include "po_tcp.h"

#define BUFLEN 61

// TAKEN FROM LAB 7

/*
 * Macro for handling errors
 * Example:
 * 		int fd = open (file_name , O_RDONLY);
 * 		DIE(fd == -1, "open failed");
 */

#define DIE(assertion, call_description)                                       \
  do {                                                                         \
    if (assertion) {                                                           \
      fprintf(stderr, "(%s, %d): ", __FILE__, __LINE__);                       \
      perror(call_description);                                                \
      exit(EXIT_FAILURE);                                                      \
    }                                                                          \
  } while (0)

#endif
