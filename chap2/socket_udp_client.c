#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

int main(void)
{
  int client_socket = socket(AF_INET, SOCK_DGRAM, 0);
  struct sockaddr_in client_addr;

  memset(&client_addr, 0, sizeof(client_addr));
  client_addr.sin_family = AF_INET;
  client_addr.sin_addr = htonl(INADDR_ANY);

  return 0;
}