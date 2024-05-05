// PO_UDP -- Protocol over UDP -- Header file
#ifndef _PO_UDP_H
#define _PO_UDP_H 1

#define MAX_CONTENT_LEN 1500
#define MAX_TOPIC_LEN 50

#pragma pack(1)

#define TYPE_INT 0
#define TYPE_SHORT_REAL 1
#define TYPE_FLOAT 2
#define TYPE_STRING 3

struct udp_message
{
  char topic[MAX_TOPIC_LEN];
  uint8_t data_type;
  char content[MAX_CONTENT_LEN];
};

#endif