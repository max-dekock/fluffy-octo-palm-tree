/* Simple TCP server. Listens on port specified by user argument.
   Upon connection, prints first 255 bytes received from client.
   Then transmits a goodbye message and closes the connection.

   To compile: gcc server.c -o server

   To connect, use netcat or PuTTY on raw TCP mode. Or write your own client. */

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <strings.h>
#include <time.h>
#include <errno.h>

void error(char *msg) {
  perror(msg);
  exit(1);
}

void safe_close(int sockfd) {
        /* Shutdown socket, and keep reading until buffer is empty,
         * or until timeout of 5 seconds elapses. */
  char buffer[256];
  int res;

  struct timeval timeout;
  timeout.tv_sec = 5;
  timeout.tv_usec = 0;
  setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO,
                  (const char*)&timeout, sizeof(timeout));

  shutdown(sockfd, SHUT_WR);

  for( ; ; ) {
    res = read(sockfd, buffer, 256);
    if (res == 0) {
      break;
    } else if (res < 0) {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        break;
      } else {
        error("ERROR reading from socket");
      }
    }
  }

  close(sockfd);
}

int main(int argc, char *argv[]) {
  int listensock, clientsock, port, cliaddrlen;
  char buffer[256];
  struct sockaddr_in serv_addr, cli_addr;
  int n;

  if (argc < 2) {
    fprintf(stdout, "Usage: %s port\n", argv[0]);
    exit(0);
  }

  printf("Opening socket... ");
  listensock = socket(AF_INET, SOCK_STREAM, 0);
  if (listensock < 0) {
    error("ERROR opening socket");
  }
  printf("success.\n");

  bzero((char *) &serv_addr, sizeof(serv_addr));
  port = atoi(argv[1]);
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = INADDR_ANY;
  serv_addr.sin_port = htons(port);

  printf("Binding socket... ");
  if (bind(listensock, (struct sockaddr *) &serv_addr,
      sizeof(serv_addr)) < 0) {
    error("ERROR on binding");
  }
  printf("success.\n");

  printf("\nListening...\n");
  listen(listensock, 5);
  cliaddrlen = sizeof(cli_addr);
  clientsock = accept(listensock, (struct sockaddr *) &cli_addr, &cliaddrlen);
  if (clientsock < 0) {
    error("ERROR on accept");
  }

  char addr_str[INET_ADDRSTRLEN];
  int cliport = ntohs(cli_addr.sin_port);
  inet_ntop(AF_INET, &(cli_addr.sin_addr), addr_str, INET_ADDRSTRLEN);
  printf("Connection established: %s:%d\n", addr_str, cliport);

  printf("Reading...\n");
  bzero(buffer, 256);
  n = read(clientsock, buffer, 255);
  if (n < 0) error("ERROR reading from socket");
  printf("Message received: %s\n", buffer);

  char reply_msg[] = "Message received. Goodbye :-)";
  n = write(clientsock, reply_msg, sizeof(reply_msg));
  if (n < 0) error("ERROR writing to socket");

  safe_close(clientsock);
  printf("Connection closed.\n\n");

  close(listensock);
  return 0;
}