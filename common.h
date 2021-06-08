#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdbool.h>

#define BUF_SIZE 1024
#define MAX_CLIENT_SIZE 1024
#define PORT 5000

struct socket
{
  int descriptor;
  struct sockaddr_in addr;
};

struct client
{
  struct socket sock;
  char *name;
  bool alive;
};

struct client make_client(struct socket sock, char name[])
{
  struct client result = {
      .sock = sock,
      .name = malloc(BUF_SIZE),
      .alive = true,
  };

  strcpy(result.name, name);
  return result;
}

void free_client(struct client *clnt)
{
  clnt->alive = false;
  free(clnt->name);
}

struct client_array
{
  struct client data[MAX_CLIENT_SIZE];
  int size;
};

void push_client_array(struct client_array *arr, struct client clnt)
{
  arr->data[arr->size++] = clnt;
}

void error_handle(char *msg)
{
  fputs(msg, stderr);
  exit(1);
}
