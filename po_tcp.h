// PO_TDP -- Protocol over TCP -- Header file
#ifndef _PO_TCP_H
#define _PO_TCP_H 1

#pragma pack(1)

#include "po_udp.h"

#define MAX_ID_LEN 10

#define SUBSCRIBE 0
#define SUBSCRIBE_ACK 1
#define UNSUBSCRIBE 2
#define UNSUBSCRIBE_ACK 3
#define POST 4
#define CONNECT 5
#define CONNECT_ACK 6
#define DISCONNECT 7

struct tcp_client
{
    // Client ID
    char id[MAX_ID_LEN];

    // Connection status
    bool connected;

    // TCP client IP
    char ip[INET_ADDRSTRLEN];

    // TCP client port
    uint16_t port;

    // Socket file descriptor
    int sockfd;

    // Topics subscribed by the client
    vector<string> topics_subscribed;
};

struct tcp_message
{
    // Operation code
    uint8_t op_code;

    // Informations about the UDP client -- used only for POST
    char udp_client_ip[INET_ADDRSTRLEN];
    uint16_t udp_client_port;

    // Actual message -- used only for POST
    struct udp_message message;
    
    // Topic -- used only for SUBSCRIBE, UNSUBSCRIBE
    char topic[MAX_TOPIC_LEN];

    // Client ID -- used only for CONNECT, DISCONNECT
    char id[MAX_ID_LEN];
};

#endif