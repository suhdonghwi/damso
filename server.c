#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/select.h>
#include <errno.h>
#include <stdbool.h>

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

  return make_client(sock, name);
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

  struct sockaddr_in mul_addr;
  memset(&mul_addr, 0, sizeof(mul_addr));
  mul_addr.sin_family = AF_INET;
  mul_addr.sin_addr.s_addr = inet_addr(addr);
  mul_addr.sin_port = htons(PORT);

  int on = 1;
  int time_live = 2;

  setsockopt(descriptor, IPPROTO_IP, IP_MULTICAST_LOOP, &on, sizeof(on));
  setsockopt(descriptor, IPPROTO_IP,
             IP_MULTICAST_TTL, (void *)&time_live, sizeof(time_live));

  struct socket result = {.descriptor = descriptor, .addr = mul_addr};
  return result;
}

void send_client_list(struct client_array *arr)
{
  for (int i = 0; i < arr->size; i++)
  {
    struct client clnt = arr->list[i];

    write(clnt.sock.descriptor, &SCODE_CLIENT_LIST, sizeof(int));
    write(clnt.sock.descriptor, &arr->size, sizeof(arr->size));
    for (int j = 0; j < arr->size; j++)
    {
      write(clnt.sock.descriptor, arr->list[j].data.name, BUF_SIZE);
      write(clnt.sock.descriptor, &arr->list[j].data.opponent, sizeof(int));
    }
  }
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
  timeout.tv_sec = 5;
  timeout.tv_usec = 0;

  fd_set read_set, read_set_backup;
  FD_ZERO(&read_set);
  FD_ZERO(&read_set_backup);

  FD_SET(serv_sock.descriptor, &read_set_backup);

  struct client_array clnt_arr = {.list = {}, .size = 0};
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
      printf("%s(%d) connected\n", clnt.data.name, clnt.sock.descriptor);

      //send_client_list(&clnt_arr);
    }

    for (int clnt_index = 0; clnt_index < clnt_arr.size; clnt_index++)
    {
      struct client *clnt = &clnt_arr.list[clnt_index];
      if (FD_ISSET(clnt->sock.descriptor, &read_set)) // read message
      {
        int code = 0;
        int str_len = read(clnt->sock.descriptor, &code, sizeof(code));

        if (str_len == 0) // close request
        {
          FD_CLR(clnt->sock.descriptor, &read_set_backup);
          close(clnt->sock.descriptor);

          printf("%s(%d) closed\n", clnt->data.name, clnt->sock.descriptor);
          remove_client_array(&clnt_arr, clnt_index);
          clnt_index--;
        }
        else
        {
          switch (code)
          {
          case CCODE_PAIRING: // Pairing request
          {
            int opponent_index;
            read(clnt->sock.descriptor, &opponent_index, sizeof(opponent_index));

            struct client *opponent = &clnt_arr.list[opponent_index];

            int response;
            if (opponent_index == clnt_index)
            {
              response = 0; // Wants to chat with yourself? :)
            }
            else if (opponent->data.opponent != -1)
            {
              response = 1; // Opponent is busy
            }
            else
            {
              response = 2; // Ok, now wait for the answer

              write(opponent->sock.descriptor, &SCODE_PAIRING_REQUEST, sizeof(int));
              write(opponent->sock.descriptor, clnt->data.name, BUF_SIZE);

              clnt->data.opponent = opponent->data.uid;
              opponent->data.opponent = clnt->data.uid;

              //send_client_list(&clnt_arr);
            }

            write(clnt->sock.descriptor, &SCODE_PAIRING_RESULT, sizeof(int));
            write(clnt->sock.descriptor, &response, sizeof(int));

            break;
          }
          case CCODE_PAIRING_ANSWER:
          {
            int answer;
            read(clnt->sock.descriptor, &answer, sizeof(answer));

            int opponent_index = find_client_array(&clnt_arr, clnt->data.opponent);
            struct client *opponent = &clnt_arr.list[opponent_index];
            if (answer == 0)
            {
              opponent->data.opponent = -1;
              clnt->data.opponent = -1;
            }
            else
            {
              write(clnt->sock.descriptor, &SCODE_OPPONENT_UID, sizeof(int));
              write(clnt->sock.descriptor, &opponent->data.uid, sizeof(int));
            }

            write(opponent->sock.descriptor, &SCODE_PAIRING_ANSWER, sizeof(int));
            write(opponent->sock.descriptor, answer == 0 ? &answer : &clnt->data.uid, sizeof(int));

            //send_client_list(&clnt_arr);
            break;
          }
          case CCODE_GET_NAME:
          {
            int uid;
            read(clnt->sock.descriptor, &uid, sizeof(uid));

            int index = find_client_array(&clnt_arr, uid);

            write(clnt->sock.descriptor, &SCODE_SEND_NAME, sizeof(int));
            write(clnt->sock.descriptor, clnt_arr.list[index].data.name, BUF_SIZE);
            break;
          }
          case CCODE_CHAT_MESSAGE:
          {
            char message[BUF_SIZE];
            read(clnt->sock.descriptor, message, BUF_SIZE - 1);

            int opponent_index = find_client_array(&clnt_arr, clnt->data.opponent);
            struct client *opponent = &clnt_arr.list[opponent_index];

            if (message[0] == '/')
            {
              char command[BUF_SIZE] = "";
              sprintf(command, "python3 ./command/main.py %d %d %s %s %s",
                      clnt->data.uid, opponent->data.uid, clnt->data.name, opponent->data.name, message + 1);

              FILE *fp = popen(command, "r");

              write(clnt->sock.descriptor, &SCODE_SCREEN, sizeof(int));
              write(opponent->sock.descriptor, &SCODE_SCREEN, sizeof(int));

              char buf[BUF_SIZE];
              while (fgets(buf, sizeof(buf), fp) != NULL)
              {
                write(clnt->sock.descriptor, buf, BUF_SIZE);
                write(opponent->sock.descriptor, buf, BUF_SIZE);
              }
            }
            else
            {
              write(opponent->sock.descriptor, &SCODE_CHAT_MESSAGE, sizeof(int));
              write(opponent->sock.descriptor, message, BUF_SIZE);
            }

            break;
          }
          case CCODE_LEAVE_CHAT:
          {
            int opponent_index = find_client_array(&clnt_arr, clnt->data.opponent);
            struct client *opponent = &clnt_arr.list[opponent_index];
            opponent->data.opponent = -1;
            clnt->data.opponent = -1;

            write(opponent->sock.descriptor, &SCODE_FINISH_CHAT, sizeof(int));
            break;
          }
          }
        }
      }
    }

    send_client_list(&clnt_arr);
  }

  close(serv_sock.descriptor);
  return 0;
}