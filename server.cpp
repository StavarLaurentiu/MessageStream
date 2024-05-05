// Description: This file contains the implementation of the UDP/TCP server
#include "headers.h"
#include "utils.h"

// Function that checks if two topics are matching
// Inclunding regexes such as "+" or "*"
bool topics_are_matching(const char *topic1, const char *topic2)
{
  // If the topics are the same, return true
  if (strcmp(topic1, topic2) == 0)
    return true;

  // Create two copies of the topics
  char *topic1_copy = strdup(topic1);
  char *topic2_copy = strdup(topic2);

  // If the topics are different, take tokens from them
  vector<string> tokens1;
  vector<string> tokens2;

  // Tokenize the first topic
  char *token = strtok(topic1_copy, "/");
  while (token != NULL)
  {
    tokens1.push_back(token);
    token = strtok(NULL, "/");
  }

  // Tokenize the second topic
  token = strtok(topic2_copy, "/");
  while (token != NULL)
  {
    tokens2.push_back(token);
    token = strtok(NULL, "/");
  }

  return false;
}

void run_app_multi_server(int tcp_sockfd, int udp_sockfd)
{
  // Initialize the vector of clients
  vector<struct tcp_client> clients;
  int rc;

  // Initialize the set of active sockets
  fd_set fds, tmp_fds;

  // Initialize the sets of file descriptors
  FD_ZERO(&fds);
  FD_ZERO(&tmp_fds);

  // Add the STDIN, TCP and UDP sockets to the set
  FD_SET(STDIN_FILENO, &fds);
  FD_SET(tcp_sockfd, &fds);
  FD_SET(udp_sockfd, &fds);

  // Initialize the maximum file descriptor
  int fdmax = max(tcp_sockfd, udp_sockfd);

  // Run the application
  while (1)
  {
    tmp_fds = fds;

    // Select the active sockets
    rc = select(fdmax + 1, &tmp_fds, NULL, NULL, NULL);
    DIE(rc < 0, "Select ERROR");

    // Check if the STDIN is active and an exit command was given
    if (FD_ISSET(STDIN_FILENO, &tmp_fds))
    {
      char buffer[BUFLEN];
      memset(buffer, 0, BUFLEN);

      // Read the command from STDIN
      rc = read(STDIN_FILENO, buffer, BUFLEN);
      DIE(rc < 0, "Read from STDIN ERROR");

      // Check if the command is 'exit'
      if (strncmp(buffer, "exit", 4) == 0)
      {
        // Close all the sockets and send a DISCONNECT message to the clients
        for (auto client : clients)
        {
          // If the client is not connected, continue
          if (!client.connected)
            continue;

          struct tcp_message message;
          message.op_code = DISCONNECT;
          rc = send_all(client.sockfd, &message, sizeof(struct tcp_message));
          DIE(rc < 0, "Send DISCONNECT message ERROR");

          close(client.sockfd);
        }

        break;
      }
      else
      {
        fprintf(stderr, "Invalid command.\n");
      }
    }

    // Check if any other socket is active
    for (int i = 2; i <= fdmax; i++)
    {
      // If the socket is not active, continue
      if (!FD_ISSET(i, &tmp_fds))
        continue;

      // Check the type of the socket
      if (i == tcp_sockfd)
      {
        // Accept the new TCP connection
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int newsockfd = accept(tcp_sockfd, (struct sockaddr *)&client_addr, &client_len);
        DIE(newsockfd < 0, "Accept TCP connection ERROR");

        // Get the client IP and port
        char tcp_client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, tcp_client_ip, INET_ADDRSTRLEN);
        uint16_t tcp_client_port = ntohs(client_addr.sin_port);

        // Get the client ID by reading from the socket an message
        struct tcp_message *message = (struct tcp_message *)malloc(sizeof(struct tcp_message));
        rc = recv_all(newsockfd, message, sizeof(struct tcp_message));
        DIE(rc < 0, "Receive client ID ERROR");

        // Check that the message is a CONNECT message
        if (message->op_code != CONNECT)
        {
          close(newsockfd);
          continue;
        }

        // Check if the client ID is already in use
        bool found = false;
        struct tcp_client *client_found = NULL;
        for (auto client : clients)
        {
          if (strcmp(client.id, message->id) == 0)
          {
            found = true;
            client_found = &client;
            break;
          }
        }

        // If the client ID is already in use, print "Client <ID> already in use"
        if (found && client_found->connected)
        {
          fprintf(stdout, "Client %s already connected.\n", message->id);

          // Send a message to the client that the ID is already in use
          struct tcp_message response;
          response.op_code = DISCONNECT;
          rc = send_all(newsockfd, &response, sizeof(struct tcp_message));
          DIE(rc < 0, "Send DISCONNECT message ERROR");

          close(newsockfd);
          continue;
        }
        else if (found)
        {
          // RECONNECT THE CLIENT

          // Mark the client as connected again
          for (auto &client : clients)
          {
            if (strcmp(client.id, message->id) == 0)
            {
              client.connected = true;
              break;
            }
          }

          // Update the client's IP and port
          strcpy(client_found->ip, tcp_client_ip);
          client_found->port = tcp_client_port;

          // Update the client's socket
          client_found->sockfd = newsockfd;
        }
        else
        {
          // CREATE A NEW CLIENT

          // Create a new client
          struct tcp_client new_client;
          strcpy(new_client.id, message->id);
          new_client.connected = true;
          strcpy(new_client.ip, tcp_client_ip);
          new_client.port = tcp_client_port;
          new_client.sockfd = newsockfd;

          // Add the new client to the list of clients
          clients.push_back(new_client);
        }

        // Set NO_DELAY option
        int flag = 1;
        rc = setsockopt(newsockfd, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(int));

        // Add the new socket to the set
        FD_SET(newsockfd, &fds);
        fdmax = max(fdmax, newsockfd);

        // Send a CONNECT_ACK message to the client
        struct tcp_message response;
        response.op_code = CONNECT_ACK;
        rc = send_all(newsockfd, &response, sizeof(struct tcp_message));
        DIE(rc < 0, "Send CONNECT_ACK message ERROR");

        // Print "New client <ID> connected from <IP>:<PORT>."
        fprintf(stdout, "New client %s connected from %s:%hu.\n", message->id, tcp_client_ip, tcp_client_port);
      }
      else if (i == udp_sockfd)
      {
        // Declare a udp_message to receive the message from the UDP client
        struct udp_message *message = (struct udp_message *)malloc(sizeof(struct udp_message));
        memset(message, 0, sizeof(struct udp_message));

        // Declare a sockaddr_in to receive the address of the UDP client
        struct sockaddr_in udp_client_addr;
        socklen_t udp_client_len = sizeof(udp_client_addr);

        // Receive the message from the UDP client
        rc = recvfrom(udp_sockfd, message, sizeof(struct udp_message), 0, (struct sockaddr *)&udp_client_addr, &udp_client_len);
        DIE(rc < 0, "Receive message from UDP client ERROR");

        // Get the UDP client IP and port
        char udp_client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &udp_client_addr.sin_addr, udp_client_ip, INET_ADDRSTRLEN);
        uint16_t udp_client_port = ntohs(udp_client_addr.sin_port);

        // Find the clients that are subscribed to a matching topic
        for (auto client : clients)
        {
          if (!client.connected)
            continue;

          for (auto topic : client.topics_subscribed)
          {
            if (topics_are_matching(topic.c_str(), message->topic))
            {
              // Send the message to the TCP client
              struct tcp_message response;
              response.op_code = POST;
              strcpy(response.udp_client_ip, udp_client_ip);
              response.udp_client_port = udp_client_port;
              memcpy(&response.message, message, sizeof(struct udp_message));
              rc = send_all(client.sockfd, &response, sizeof(struct tcp_message));
              DIE(rc < 0, "Send POST message ERROR");
            }
          }
        }
        
      }
      else
      {
        // Receive a message from an TCP client
        // Could be a DISCONNECT/SUBSCRIBE/UNSUBSCRIBE message

        // Find the client that sent the message
        struct tcp_client *client_sender = NULL;
        for (auto client : clients)
        {
          if (client.sockfd == i)
          {
            client_sender = &client;
            break;
          }
        }

        // Receive the message from the client
        struct tcp_message *message = (struct tcp_message *)malloc(sizeof(struct tcp_message));
        rc = recv_all(i, message, sizeof(struct tcp_message));
        DIE(rc < 0, "Receive message from client ERROR");

        // Check the operation code
        if (message->op_code == DISCONNECT)
        {
          // Mark the client as disconnected
          for (auto &client : clients)
          {
            if (strcmp(client.id, message->id) == 0)
            {
              client.connected = false;
              break;
            }
          }

          // Remove the client's socket from the set
          FD_CLR(i, &fds);

          // Close the socket
          close(i);

          // Print "Client <ID> disconnected."
          fprintf(stdout, "Client %s disconnected.\n", client_sender->id);
        }
        else if (message->op_code == SUBSCRIBE)
        {
          // SUBSCRIBE

          // Add the topic to the list of topics of the client
          for (auto &client : clients)
          {
            if (strcmp(client.id, message->id) == 0)
            {
              string topic(message->topic, MAX_TOPIC_LEN);
              client.topics_subscribed.push_back(topic);
              break;
            }
          }

          // Send a message to the client that it subscribed to the topic
          struct tcp_message response;
          response.op_code = SUBSCRIBE_ACK;
          strncpy(response.topic, message->topic, MAX_TOPIC_LEN);
          rc = send_all(i, &response, sizeof(struct tcp_message));
          DIE(rc < 0, "Send SUBSCRIBE_ACK message ERROR");
        }
        else if (message->op_code == UNSUBSCRIBE)
        {
          // UNSUBSCRIBE

          // Remove the topic from the list of topics of the client
          for (auto &client : clients)
          {
            if (strcmp(client.id, message->id) == 0)
            {
              client.topics_subscribed.erase(remove(client.topics_subscribed.begin(), client.topics_subscribed.end(), message->topic), client.topics_subscribed.end());
              break;
            }
          }

          // Send a message to the client that it unsubscribed from the topic
          struct tcp_message response;
          response.op_code = UNSUBSCRIBE_ACK;
          strncpy(response.topic, message->topic, MAX_TOPIC_LEN);
          rc = send_all(i, &response, sizeof(struct tcp_message));
          DIE(rc < 0, "Send UNSUBSCRIBE_ACK message ERROR");
        }
        else
        {
          // Invalid operation code
          fprintf(stderr, "Invalid operation code.\n");
        }
      }
    }
  }

  return;
}

int main(int argc, char *argv[])
{
  // Disable buffering for stdout
  setvbuf(stdout, NULL, _IONBF, BUFSIZ);

  // Check if the number of arguments is valid
  if (argc != 2)
  {
    printf("\n Usage: ./server <port>\n");
    return 1;
  }

  // Parse the port
  uint16_t port;
  int rc = sscanf(argv[1], "%hu", &port);
  DIE(rc != 1 || port < 1024, "Given port is invalid");

  // Initialize server address
  struct sockaddr_in server_addr;
  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(port);
  server_addr.sin_addr.s_addr = INADDR_ANY;

  // Create udp datagrams socket
  int udp_sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  DIE(udp_sockfd < 0, "UDP socket ERROR");

  // Create tcp connections socket
  int tcp_sockfd = socket(AF_INET, SOCK_STREAM, 0);
  DIE(tcp_sockfd < 0, "TCP socket ERROR");

  // Disable Nagle's algorithm
  int flag = 1;
  rc = setsockopt(tcp_sockfd, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(int));
  DIE(rc < 0, "Setsockopt -- TCP_NODELAY ERROR");

  // Enable the socket to reuse the address
  const int enable = 1;
  rc = setsockopt(tcp_sockfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));
  DIE(rc < 0, "setsockopt -- TCP_REUSEADDR ERROR");

  rc = setsockopt(udp_sockfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));
  DIE(rc < 0, "setsockopt -- UDP_REUSEADDR ERROR");

  // Bind the socket with the server address
  rc = bind(tcp_sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr));
  DIE(rc < 0, "TCP bind ERROR");

  rc = bind(udp_sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr));
  DIE(rc < 0, "UDP bind ERROR");

  // Listen for incoming TCP connections
  rc = listen(tcp_sockfd, SOMAXCONN);
  DIE(rc < 0, "TCP listen ERROR");

  // Run the application
  run_app_multi_server(tcp_sockfd, udp_sockfd);

  // Close the sockets
  close(tcp_sockfd);
  close(udp_sockfd);

  return 0;
}
