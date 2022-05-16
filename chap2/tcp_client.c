#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT 25565

int main(void)
{
  int client_socket = socket(AF_INET, SOCK_STREAM, 0);

  if(client_socket == -1) {
    perror("Socket creation error");
    return 1;
  }

  struct sockaddr_in server_addr;

  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(PORT);
  
  if(inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr) != 1) {
    perror("Address conversion error");
    return 1;
  }

  if(connect(client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) != 0) {
    perror("Connection error");
    return 1;
  }

  char send_msg[1024], recv_msg[1024];

  printf("Type in text: ");
  scanf("%s", send_msg);

  if(send(client_socket, send_msg, sizeof(send_msg), 0) == -1) {
    perror("Send error");
    return 1;
  }

  read(client_socket, recv_msg, 1024);
  printf("%s\n", recv_msg);

  return 0;
}