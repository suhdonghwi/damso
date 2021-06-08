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
};

struct client make_client(struct socket sock, char name[])
{
  struct client result = {
      .sock = sock,
      .name = malloc(BUF_SIZE),
  };

  strcpy(result.name, name);
  return result;
}

void free_client(struct client *clnt)
{
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

void remove_client_array(struct client_array *arr, int index)
{
  char *temp = arr->data[index].name;

  for (int i = index + 1; i < arr->size; i++)
  {
    arr->data[i - 1].name = arr->data[i].name;
    arr->data[i - 1].sock = arr->data[i].sock;
  }

  free(temp);
  arr->size--;
}

void error_handle(char *msg)
{
  fputs(msg, stderr);
  exit(1);
}
