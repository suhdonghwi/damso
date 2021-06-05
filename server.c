#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/select.h>
#include <errno.h>

#include "./common.h"

// Helper functions

struct client accept_client(struct socket serv_sock)
{
  struct sockaddr_in addr;
  socklen_t addr_size = sizeof(addr);
  int descriptor = accept(serv_sock.descriptor, (struct sockaddr *)&addr, &addr_size);

  struct socket sock =
      {
          .descriptor = descriptor,
          .addr = addr,
      };

  char name[BUF_SIZE];
  read(descriptor, name, BUF_SIZE);

  struct client result = {
      .sock = sock,
      .name = name,
  };

  return result;
}

struct socket make_server_sock()
{
  int descriptor = socket(PF_INET, SOCK_STREAM, 0);

  struct sockaddr_in serv_addr;
  memset(&serv_addr, 0, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  serv_addr.sin_port = htons(PORT);

  int on = 1;
  setsockopt(descriptor, SOL_SOCKET, SO_REUSEADDR, (const char *)&on, sizeof(on));

  struct socket result = {.descriptor = descriptor, .addr = serv_addr};
  return result;
}

struct socket make_multicast_sock(char *addr)
{
  int descriptor = socket(PF_INET, SOCK_DGRAM, 0);
  if (descriptor == -1)
  {
    error_handle("socket() error");
  }

  struct sockaddr_in mul_addr; // 패킷을 보낼 주소의 정보를 담는 구조체입니다.
  memset(&mul_addr, 0, sizeof(mul_addr));
  mul_addr.sin_family = AF_INET;              // IPv4 형식으로 지정합니다.
  mul_addr.sin_addr.s_addr = inet_addr(addr); // IP를 지정합니다. htonl(INADDR_ANY)가 현재 시스템의 IP를 반환합니다.
  mul_addr.sin_port = htons(PORT);            // 포트를 지정합니다.

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
    printf("Usage : %s <Group IP> <Own IP>\n", argv[0]);
    exit(1);
  }

  char server_addr[BUF_SIZE];
  strcpy(server_addr, argv[2]);

  struct socket serv_sock = make_server_sock();

  if (bind(serv_sock.descriptor, (struct sockaddr *)&serv_sock.addr, sizeof(serv_sock.addr)) == -1)
  {
    error_handle("bind() error");
  }

  if (listen(serv_sock.descriptor, 5) == -1)
  {
    error_handle("listen() error");
  }

  struct socket multi_sock = make_multicast_sock(argv[1]);

  struct timeval timeout;
  timeout.tv_sec = 1;
  timeout.tv_usec = 0;

  fd_set read_set, read_set_backup;
  FD_ZERO(&read_set);
  FD_ZERO(&read_set_backup);

  FD_SET(serv_sock.descriptor, &read_set_backup);

  struct client_array clnt_arr = {.data = {}, .size = 0};
  int fd_max = serv_sock.descriptor;

  while (1)
  {
    FD_COPY(&read_set_backup, &read_set);

    // Heartbeat
    sendto(multi_sock.descriptor, server_addr, sizeof(server_addr), 0, (struct sockaddr *)&multi_sock.addr, sizeof(multi_sock.addr));

    int fd_num = select(fd_max + 1, &read_set, 0, 0, &timeout);
    if (fd_num == -1)
    {
      printf("%d\n", errno);
      error_handle("select() error");
      break;
    }
    else if (fd_num == 0)
    {
      // printf("Timeout\n");
    }

    if (FD_ISSET(serv_sock.descriptor, &read_set)) // connection request
    {
      struct client clnt = accept_client(serv_sock);

      FD_SET(clnt.sock.descriptor, &read_set_backup);
      if (fd_max < clnt.sock.descriptor)
        fd_max = clnt.sock.descriptor;

      push_client_array(&clnt_arr, clnt);
      printf("Connected client: %d\n", clnt.sock.descriptor);
    }

    for (int clnt_index = 0; clnt_index <= clnt_arr.size; clnt_index++)
    {
      struct client clnt = clnt_arr.data[clnt_index];
      if (FD_ISSET(clnt.sock.descriptor, &read_set)) // read message
      {
        char received[BUF_SIZE] = {};
        int str_len = read(clnt.sock.descriptor, received, BUF_SIZE);

        if (str_len == 0) // close request
        {
          FD_CLR(clnt.sock.descriptor, &read_set_backup);
          close(clnt.sock.descriptor);
          printf("Closed client : %d\n", clnt.sock.descriptor);
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
