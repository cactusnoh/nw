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

  if ((client_sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
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

  char send_msg[1024], recv_msg[1024];
  socklen_t addr_len = sizeof(server_addr);

  while (1) {
    printf("Type in text: ");
    scanf("%s", send_msg);

    if (sendto(client_sock, send_msg, sizeof(send_msg), 0, 
              (struct sockaddr *)&server_addr, (socklen_t)sizeof(server_addr)) == -1) {
      perror("Send error");
      return 1;
    }

    if (strcmp(send_msg, "exit") == 0) {
      break;
    }

    recvfrom(client_sock, recv_msg, 1024, 0, 
            (struct sockaddr *)&server_addr, &addr_len);
    printf("Uppercase: %s\n", recv_msg);
  }

  close(client_sock);

  return 0;
}