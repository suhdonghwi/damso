#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/select.h>
#include <errno.h>

#define BUF_SIZE 1024
#define MAX_CLIENT_SIZE 1024

// Structs

struct socket
{
  int descriptor;
  struct sockaddr_in addr;
};

struct socket accept_client(struct socket serv_sock)
{
  struct sockaddr_in addr;
  socklen_t addr_size = sizeof(addr);
  int descriptor = accept(serv_sock.descriptor, (struct sockaddr *)&addr, &addr_size);

  struct socket result =
      {
          .descriptor = descriptor,
          .addr = addr,
      };

  return result;
}

struct socket_array
{
  struct socket data[MAX_CLIENT_SIZE];
  uint64_t size;
};

void push_client_array(struct socket_array *arr, struct socket clnt)
{
  arr->data[arr->size++] = clnt;
}

// Helper functions

struct socket make_server_sock(int port)
{
  int descriptor = socket(PF_INET, SOCK_STREAM, 0);

  struct sockaddr_in serv_addr;
  memset(&serv_addr, 0, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  serv_addr.sin_port = htons(port);

  int on = 1;

  setsockopt(descriptor, SOL_SOCKET, SO_REUSEADDR, (const char *)&on, sizeof(on));

  struct socket result = {.descriptor = descriptor, .addr = serv_addr};
  return result;
}

struct socket make_multicast_sock(char *addr, int port)
{
  int descriptor = socket(PF_INET, SOCK_DGRAM, 0);

  struct sockaddr_in mul_addr; // 패킷을 보낼 주소의 정보를 담는 구조체입니다.
  memset(&mul_addr, 0, sizeof(mul_addr));
  mul_addr.sin_family = AF_INET;              // IPv4 형식으로 지정합니다.
  mul_addr.sin_addr.s_addr = inet_addr(addr); // IP를 지정합니다. htonl(INADDR_ANY)가 현재 시스템의 IP를 반환합니다.
  mul_addr.sin_port = htons(port);            // 포트를 지정합니다.

  int on = 1;
  int time_live = 2;

  setsockopt(descriptor, IPPROTO_IP, IP_MULTICAST_LOOP, &on, sizeof(on));
  setsockopt(descriptor, IPPROTO_IP,
             IP_MULTICAST_TTL, (void *)&time_live, sizeof(time_live));

  struct socket result = {.descriptor = descriptor, .addr = mul_addr};
  return result;
}

// Main

int main(int argc, char *argv[])
{
  if (argc != 3)
  {
    printf("Usage : %s <Group IP> <Port>\n", argv[0]);
    exit(1);
  }

  struct socket serv_sock = make_server_sock(atoi(argv[2]));

  if (bind(serv_sock.descriptor, (struct sockaddr *)&serv_sock.addr, sizeof(serv_sock.addr)) == -1)
  {
    printf("bind() error");
    exit(0);
  }

  if (listen(serv_sock.descriptor, 5) == -1)
  {
    printf("listen() error");
    exit(0);
  }

  struct socket multi_sock = make_multicast_sock(argv[1], atoi(argv[2]));

  struct timeval timeout;
  timeout.tv_sec = 1;
  timeout.tv_usec = 0;

  fd_set read_set, read_set_backup;
  FD_ZERO(&read_set);
  FD_ZERO(&read_set_backup);

  FD_SET(serv_sock.descriptor, &read_set_backup);

  struct socket_array clnt_arr = {.data = {}, .size = 0};
  int fd_max = serv_sock.descriptor;

  while (1)
  {
    FD_COPY(&read_set_backup, &read_set);

    // Heartbeat
    char to_send[1024] = "Hello";
    int l = sendto(multi_sock.descriptor, to_send, 1024, 0, (struct sockaddr *)&multi_sock.addr, sizeof(multi_sock.addr));

    int fd_num = select(fd_max + 1, &read_set, 0, 0, &timeout);
    if (fd_num == -1)
    {
      break;
    }
    else if (fd_num == 0)
    {
      // printf("Timeout\n");
    }

    if (FD_ISSET(serv_sock.descriptor, &read_set)) // connection request
    {
      struct socket clnt = accept_client(serv_sock);

      FD_SET(clnt.descriptor, &read_set_backup);
      if (fd_max < clnt.descriptor)
        fd_max = clnt.descriptor;

      push_client_array(&clnt_arr, clnt);
      printf("Connected client: %d\n", clnt.descriptor);
    }

    for (int clnt_index = 0; clnt_index <= clnt_arr.size; clnt_index++)
    {
      struct socket clnt = clnt_arr.data[clnt_index];
      if (FD_ISSET(clnt.descriptor, &read_set)) // read message
      {
        char received[BUF_SIZE] = {};
        int str_len = read(clnt.descriptor, received, BUF_SIZE);

        if (str_len == 0) // close request
        {
          FD_CLR(clnt.descriptor, &read_set);
          close(clnt.descriptor);
          printf("Closed client: %d \n", clnt.descriptor);
        }
        else
        {
          printf("> CLIENT : %s", received);
          /*
          printf("> SERVER : ");

          char message[BUF_SIZE] = {};
          fgets(message, sizeof(message), stdin);

          write(clnt.sock, message, sizeof(message)); // echo
          */
        }
      }
    }
  }

  close(serv_sock.descriptor);
  return 0;
}
