#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT 8080

int main(void)
{
  int client_socket = socket(AF_INET, SOCK_STREAM, 0);

  if(client_socket == -1) {
    printf("Socket creation error.\n");
    return 1;
  }

  struct sockaddr_in server_addr;

  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(PORT);
  
  if(inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr) != 1) {
    printf("Address conversion error.\n");
    return 1;
  }

  if(connect(client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) != 0) {
    perror("");
    printf("Connection error.\n");
    return 1;
  }

  char *send_msg = "Hi from client.", buf[1024] = {0};

  if(send(client_socket, send_msg, strlen(send_msg), 0) == -1) {
    printf("Send error.\n");
    return 1;
  }

  printf("Message sent to server.\n");
  read(client_socket, buf, 1024);
  printf("%s\n", buf);

  return 0;
}