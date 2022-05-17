#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT 25565

int main(void)
{
  int server_sock;

  if ((server_sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    perror("Socket creation error");
    return 1;
  }

  struct sockaddr_in server_addr;

  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(PORT);
  
  if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) != 0) {
    perror("Bind error");
    return 1;
  }

  if (listen(server_sock, 3) != 0) {
    perror("Listen error");
    return 1;
  }

  int connection_sock;
  socklen_t addrlen = sizeof(server_addr);

  if ((connection_sock = accept(server_sock, (struct sockaddr *) &server_addr, (socklen_t *)&addrlen)) < 0) {
    perror("Accept error");
    return 1;
  }

  char recv_msg[1024];

  while (1) {
    read(connection_sock, recv_msg, 1024);

    printf("Received: %s\n", recv_msg);

    if (strcmp(recv_msg, "exit") == 0) {
      break;
    }

    for (char *p = recv_msg; *p != '\0'; ++p) {
      *p = toupper(*p);
    }

    send(connection_sock, recv_msg, sizeof(recv_msg), 0);
  }

  return 0;
}