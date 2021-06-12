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

struct client_data
{
  int uid;
  char *name;
  int opponent;
};

struct client
{
  struct socket sock;
  struct client_data data;
};

struct client make_client(struct socket sock, char name[])
{
  static int uid_count = 0;
  struct client result = {
      .sock = sock,
      .data = {
          .name = malloc(BUF_SIZE),
          .opponent = -1,
          .uid = uid_count++,
      }};

  strcpy(result.data.name, name);
  return result;
}

void free_client(struct client *clnt)
{
  free(clnt->data.name);
}

struct client_array
{
  struct client list[MAX_CLIENT_SIZE];
  int size;
};

void push_client_array(struct client_array *arr, struct client clnt)
{
  arr->list[arr->size++] = clnt;
}

void remove_client_array(struct client_array *arr, int index)
{
  char *temp = arr->list[index].data.name;

  for (int i = index + 1; i < arr->size; i++)
  {
    arr->list[i - 1] = arr->list[i];
  }

  free(temp);
  arr->size--;
}

void error_handle(char *msg)
{
  fputs(msg, stderr);
  exit(1);
}
