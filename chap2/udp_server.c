#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define PORT 8080

int main(void)
{
  int server_socket = socket(AF_INET, SOCK_STREAM, 0), opt = 1;

  if(server_socket == -1) {
    printf("Socket creation error.\n");
    return 1;
  }

  if (setsockopt(server_socket, SOL_SOCKET,
                   SO_REUSEADDR, &opt,
                   sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
  }

  struct sockaddr_in server_addr;

  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(PORT);
  
  if(bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) != 0) {
    printf("Bind error.\n");
    return 1;
  }

  printf("listening\n");
  if(listen(server_socket, 3) != 0) {
    printf("Listen error.\n");
    return 1;
  }

  int connection_sock;
  socklen_t addrlen = sizeof(server_addr);

  printf("accepting\n");
  if((connection_sock = accept(server_socket, (struct sockaddr *) &server_addr, (socklen_t *)&addrlen)) < 0) {
    printf("Accept error.\n");
    return 1;
  }

  printf("reading\n");
  char send_msg[] = "Hi from server.", buf[1024] = {0};
  read(connection_sock, buf, 1024);
  printf("%s\n", buf);
  send(connection_sock, send_msg, strlen(send_msg), 0);

  return 0;
}