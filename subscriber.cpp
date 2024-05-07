// Description: This file contains the implementation of the TCP subscriber
#include "headers.h"
#include "utils.h"

// Function that gets the integer value from the content
int get_INT_value(char *content)
{
    // Get the sign of the integer
    int8_t sign = content[0];

    // Get the integer value
    uint32_t value = 0;
    memcpy(&value, content + sizeof(int8_t), sizeof(uint32_t));
    value = ntohl(value);

    // Return the integer value based on the sign
    return sign == 0 ? value : -value;
}

// Function that gets the short real value from the content
float get_SHORT_REAL_value(char *content)
{
    // Get the short real value
    uint16_t value = 0;
    memcpy(&value, content, sizeof(uint16_t));
    value = ntohs(value);

    // Return the float value
    return (float)value / 100;
}

// Function that gets the float value from the content
float get_FLOAT_value(char *content)
{
    // Get the sign of the float
    int8_t sign = content[0];

    // Get the float value
    uint32_t value = 0;
    memcpy(&value, content + sizeof(int8_t), sizeof(uint32_t));
    value = ntohl(value);

    // Get the power of 10
    int8_t power = content[sizeof(int8_t) + sizeof(uint32_t)];

    // Return the float value based on the sign and power
    return sign == 0 ? (float)value / pow(10, power) : -(float)value / pow(10, power);
}

// Function that parses the response from the server
void parse_response(struct tcp_message response)
{
    // Print the message received from the server
    // FORMAT: "<TOPIC> - <TIP_DATE> - <VALOARE_MESAJ>"
    switch (response.message.data_type)
    {
    case TYPE_INT:
        printf("%s - INT - %d\n", response.message.topic, get_INT_value(response.message.content));
        break;
    case TYPE_SHORT_REAL:
        printf("%s - SHORT_REAL - %.2f\n", response.message.topic, get_SHORT_REAL_value(response.message.content));
        break;
    case TYPE_FLOAT:
        printf("%s - FLOAT - %.4f\n", response.message.topic, get_FLOAT_value(response.message.content));
        break;
    case TYPE_STRING:
        printf("%s - STRING - %s\n", response.message.topic, response.message.content);
        break;
    default:
        fprintf(stderr, "Invalid message type.\n");
        break;
    }
}

void run_client(int tcp_sockfd, char *client_id)
{
    // Declare the variables used in the client
    struct pollfd fds[2];
    struct tcp_message message;
    struct tcp_message response;
    int rc;

    // Add the STDIN to the poll
    fds[0].fd = STDIN_FILENO;
    fds[0].events = POLLIN;

    // Add the tcp socket to the poll
    fds[1].fd = tcp_sockfd;
    fds[1].events = POLLIN;

    while (1)
    {
        // Poll the sockets
        rc = poll(fds, 2, -1);
        DIE(rc < 0, "poll ERROR");

        // Check if one socket is active
        if (fds[0].revents & POLLIN)
        {
            // Read the input from STDIN
            // Input format is subscribe <topic>, unsubscribe <topic>, exit
            char buffer[BUFLEN];
            memset(buffer, 0, BUFLEN);

            // Read the input
            rc = read(STDIN_FILENO, buffer, BUFLEN);
            DIE(rc < 0, "read command ERROR");

            // Parse the input
            char *token = strtok(buffer, " ");
            if (token == NULL)
            {
                fprintf(stderr, "Invalid command.\n");
                continue;
            }

            // Check the command
            if (strcmp(token, "subscribe") == 0)
            {
                // Send a message to the server to subscribe to a topic
                token = strtok(NULL, " ");
                if (token == NULL)
                {
                    fprintf(stderr, "Invalid command.\n");
                    continue;
                }

                // Erase the '\n' from the token
                token[strlen(token) - 1] = '\0';

                // Send the message to the server
                message.op_code = SUBSCRIBE;
                strncpy(message.topic, token, MAX_TOPIC_LEN);
                strncpy(message.id, client_id, MAX_ID_LEN);
                rc = send_all(tcp_sockfd, &message, sizeof(struct tcp_message));
                DIE(rc < 0, "send_all");
            }
            else if (strcmp(token, "unsubscribe") == 0)
            {
                // Send a message to the server to unsubscribe from a topic
                token = strtok(NULL, " ");
                if (token == NULL)
                {
                    fprintf(stderr, "Invalid command.\n");
                    continue;
                }

                // Erase the '\n' from the token
                token[strlen(token) - 1] = '\0';

                // Send the message to the server
                message.op_code = UNSUBSCRIBE;
                strncpy(message.topic, token, MAX_TOPIC_LEN);
                strncpy(message.id, client_id, MAX_ID_LEN);
                rc = send_all(tcp_sockfd, &message, sizeof(struct tcp_message));
                DIE(rc < 0, "send_all");
            }
            else if (strncmp(token, "exit", 4) == 0)
            {
                // Send a message to the server to disconnect
                message.op_code = DISCONNECT;
                strcpy(message.id, client_id);
                rc = send_all(tcp_sockfd, &message, sizeof(struct tcp_message));
                DIE(rc < 0, "send_all");
                return;
            }
            else
            {
                fprintf(stderr, "Invalid command.\n");
            }
        }
        else if (fds[1].revents & POLLIN)
        {
            // Receive the message/response from the server
            rc = recv_all(tcp_sockfd, &response, sizeof(struct tcp_message));
            DIE(rc < 0, "recv_all");

            // Check the response code
            switch (response.op_code)
            {
            case SUBSCRIBE_ACK:
                printf("Subscribed to topic %s\n", response.topic);
                break;
            case UNSUBSCRIBE_ACK:
                printf("Unsubscribed from topic %s\n", response.topic);
                break;
            case POST:
                parse_response(response);
                break;
            case DISCONNECT:
                fprintf(stderr, "Disconnected from server.\n");
                return;
            default:
                fprintf(stderr, "Invalid response code.\n");
                break;
            }
        }
    }
}

int main(int argc, char *argv[])
{
    // Disable buffering for stdout
    setvbuf(stdout, NULL, _IONBF, BUFSIZ);
    int rc;

    // Check if the number of arguments is valid
    if (argc != 4)
    {
        printf("\n Usage: ./subscriber <CLIENT_ID> <SERVER_IP> <SERVER_PORT>\n");
        return 1;
    }

    // Parse the arguments
    char *client_id = argv[1];
    char *server_ip = argv[2];
    uint16_t server_port = atoi(argv[3]);

    // Initialize server address
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);
    rc = inet_aton(server_ip, &server_addr.sin_addr);
    DIE(rc == 0, "inet_aton ERROR");

    // Create a socket to connect to the server
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    DIE(sockfd < 0, "socket");

    // Connect to the server
    rc = connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    DIE(rc < 0, "connect");

    // Send a message containing the client_id to the server
    struct tcp_message message;
    message.op_code = CONNECT;
    strcpy(message.id, client_id);
    rc = send_all(sockfd, &message, sizeof(struct tcp_message));

    // Receive the response from the server
    struct tcp_message response;
    rc = recv_all(sockfd, &response, sizeof(struct tcp_message));

    // Check the response code is CONNECT_ACK
    if (response.op_code != CONNECT_ACK)
    {
        fprintf(stderr, "Connection to server failed or client ID already in use.\n");
        close(sockfd);
        return 1;
    }

    // Run the client
    run_client(sockfd, client_id);

    // Close the socket
    close(sockfd);

    return 0;
}
