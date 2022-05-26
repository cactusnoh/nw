#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT 25565

int main(void)
{
  int client_sock;

  if ((client_sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    perror("Socket creation error");
    return 1;
  }

  struct sockaddr_in server_addr;

  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(PORT);
  
  if (inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr) != 1) {
    perror("Address conversion error");
    return 1;
  }

  if (connect(client_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) != 0) {
    perror("Connection error");
    return 1;
  }

  char send_msg[1024], recv_msg[1024];

  while (1) {
    printf("Type in text: ");
    scanf("%s", send_msg);

    if (send(client_sock, send_msg, sizeof(send_msg), 0) == -1) {
      perror("Send error");
      return 1;
    }

    if (strcmp(send_msg, "exit") == 0 || read(client_sock, recv_msg, 1024) <= 0) {
      printf("Disconnected.\n");
      break;
    }

    printf("Uppercase: %s\n", recv_msg);
  }

  close(client_sock);

  return 0;
}