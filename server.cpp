#include "headers.h"
#include "utils.h"

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
  while (1) {
    tmp_fds = fds;

    // Select the active sockets
    rc = select(fdmax + 1, &tmp_fds, NULL, NULL, NULL);
    DIE(rc < 0, "Select ERROR");

    // Check if the STDIN is active
    if (FD_ISSET(STDIN_FILENO, &tmp_fds)) {
      char buffer[BUFLEN];
      memset(buffer, 0, BUFLEN);

      // Read the command from STDIN
      rc = read(STDIN_FILENO, buffer, BUFLEN);
      DIE(rc < 0, "Read from STDIN ERROR");

      // Check if the command is 'exit'
      if (strncmp(buffer, "exit", 4) == 0) {
        // Close all the sockets
        for (auto client : clients) {
          close(client.sockfd);
        }

        break;
      } else {
        DIE(true, "Invalid command");
      }
    }

    // Check if any other socket is active
    for (int i = 2; i <= fdmax; i++) {
      // If the socket is not active, continue
      if (!FD_ISSET(i, &tmp_fds)) 
        continue;

      // Check the type of the socket
      if (i == tcp_sockfd) {
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
        if (message->op_code != CONNECT) {
          close(newsockfd);
          continue;
        }

        // Check if the client ID is already in use
        bool found = false;
        for (auto client : clients) {
          if (strcmp(client.id, message->id) == 0) {
            found = true;
            break;
          }
        }

        // If the client ID is already in use, print "Client <ID> already in use"
        if (found) {
          fprintf(stdin, "Client %s already connected.\n", message->id);

          // Send a message to the client that the ID is already in use
          struct tcp_message response;
          response.op_code = DISCONNECT;
          rc = send_all(newsockfd, &response, sizeof(struct tcp_message));
          DIE(rc < 0, "Send DISCONNECT message ERROR");

          close(newsockfd);
          continue;
        }

        // Print "New client <ID> connected from <IP>:<PORT>."
        fprintf(stdin, "New client %s connected from %s:%hu.\n", message->id, tcp_client_ip, tcp_client_port);

        // Send a CONNECT_ACK message to the client
        struct tcp_message response;
        response.op_code = CONNECT_ACK;
        rc = send_all(newsockfd, &response, sizeof(struct tcp_message));
        DIE(rc < 0, "Send CONNECT_ACK message ERROR");

        // Create a new client
        struct tcp_client new_client;
        strcpy(new_client.id, message->id);
        new_client.connected = true;
        strcpy(new_client.ip, tcp_client_ip);
        new_client.port = tcp_client_port;
        new_client.sockfd = newsockfd;

        // Add the new client to the list of clients
        clients.push_back(new_client);

        // Set NO_DELAY option
        int flag = 1;
        rc = setsockopt(newsockfd, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(int));

        // Add the new socket to the set
        FD_SET(newsockfd, &fds);
        fdmax = max(fdmax, newsockfd);
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