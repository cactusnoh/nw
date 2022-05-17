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

  if ((server_sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
    perror("Socket creation error");
    return 1;
  }

  struct sockaddr_in server_addr, client_addr;

  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(PORT);
  
  if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) != 0) {
    perror("Bind error");
    return 1;
  }

  char recv_msg[1024];
  socklen_t addr_len = sizeof(client_addr);

  while (1) {
    recvfrom(server_sock, recv_msg, 1024, 0, (struct sockaddr *)&client_addr, &addr_len);

    printf("Received: %s\n", recv_msg);

    if (strcmp(recv_msg, "exit") == 0) {
      break;
    }

    for (char *p = recv_msg; *p != '\0'; ++p) {
      *p = toupper(*p);
    }

    sendto(server_sock, recv_msg, sizeof(recv_msg), 0, (struct sockaddr *)&client_addr, (socklen_t)sizeof(client_addr));
  }

  return 0;
}