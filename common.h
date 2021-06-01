#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define BUF_SIZE 1024
#define MAX_CLIENT_SIZE 1024

struct socket
{
  int descriptor;
  struct sockaddr_in addr;
};

struct socket_array
{
  struct socket data[MAX_CLIENT_SIZE];
  uint64_t size;
};

void push_socket_array(struct socket_array *arr, struct socket clnt)
{
  arr->data[arr->size++] = clnt;
}

void error_handle(char *msg)
{
  fputs(msg, stderr);
  exit(1);
}
