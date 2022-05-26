#include <arpa/inet.h>
#include <assert.h>
#include <ctype.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define PORT 25565
#define MAX_THREADS 3

typedef struct __MyArg {
  int sockfd;
  int thread_id;
  uint16_t port;
  char ip[16];
} MyArg;

pthread_t thread[MAX_THREADS];
MyArg thread_args[MAX_THREADS];

/** Free thread queue */
pthread_mutex_t queue_lock;
int free_queue[MAX_THREADS], front = -1, rear = -1;
int queue_size = 0;
int size()
{
  pthread_mutex_lock(&queue_lock);
  int ret = queue_size;
  pthread_mutex_unlock(&queue_lock);
  return ret;
}
int pop()
{
  pthread_mutex_lock(&queue_lock);
  --queue_size;
  front = (front + 1) % MAX_THREADS;
  int ret = free_queue[front];
  pthread_mutex_unlock(&queue_lock);
  return ret;
}
void push(int x)
{
  pthread_mutex_lock(&queue_lock);
  ++queue_size;
  rear = (rear + 1) % MAX_THREADS;
  free_queue[rear] = x;
  pthread_mutex_unlock(&queue_lock);
}

void *connection_routine(void *arg)
{
  MyArg *args = (MyArg *)arg;
  int connection_sock = args->sockfd;

  char recv_msg[1024];

  while(1) {
    if(read(connection_sock, recv_msg, 1024) <= 0 || strcmp(recv_msg, "exit") == 0) {
      break;
    }

    printf("*Received: %s from %s:%d\n\n", recv_msg, args->ip, args->port);

    for (char *p = recv_msg; *p != '\0'; ++p) {
      *p = toupper(*p);
    }

    send(connection_sock, recv_msg, sizeof(recv_msg), 0);
  }

  args->sockfd = -1;
  close(connection_sock);
  push(args->thread_id);

  printf("Disconnected from %s:%d\n\n", args->ip, args->port);

  return NULL;
}

void *start_routine(void *arg)
{
  MyArg *args = (MyArg *)arg;
  int server_sock = args->sockfd;

  int connection_sock;
  struct sockaddr_in client_addr;
  socklen_t client_addrlen = sizeof(client_addr);

  while (1) {
    if(size() == 0) {
      continue;
    }

    if ((connection_sock = accept(server_sock, (struct sockaddr *) &client_addr, (socklen_t *)&client_addrlen)) < 0) {
      perror("Accept error");
      continue;
    }

    int thread_id = pop();
    thread_args[thread_id].sockfd = connection_sock;
    thread_args[thread_id].thread_id = thread_id;
    thread_args[thread_id].port = ntohs(client_addr.sin_port);
    inet_ntop(AF_INET, &client_addr.sin_addr, thread_args[thread_id].ip, sizeof(thread_args[thread_id].ip));

    printf("Connection from %s:%d accepted!\n\n", thread_args[thread_id].ip, thread_args[thread_id].port);

    pthread_create(&thread[thread_id], NULL, connection_routine, &thread_args[thread_id]);
  }

  return NULL;
}

int main(void)
{
  for(int i = 0; i < MAX_THREADS; ++i) {
    push(i);
  }
  assert(pthread_mutex_init(&queue_lock, NULL) == 0);

  int server_sock, option = 1;
  if ((server_sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    perror("Socket creation error");
    return 1;
  }
  setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));

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

  pthread_t server_thread;
  MyArg server_arg;

  server_arg.sockfd = server_sock;
  
  pthread_create(&server_thread, NULL, start_routine, &server_arg);

  printf("Type 'exit' to quit.\n\n");

  char cmd[100];

  while (1) {
    scanf("%s", cmd);
    if(strcmp(cmd, "exit") == 0) {
      break;
    }
  }

  close(server_sock);

  return 0;
}