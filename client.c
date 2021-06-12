#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>

#include "termbox.h"

#include "./common.h"
#include "./ui.h"

void receive_server_addr(char *dest, char *multi_addr)
{
  int multi_sock = socket(PF_INET, SOCK_DGRAM, 0);

  struct sockaddr_in addr;
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_ANY);
  addr.sin_port = htons(PORT);

  int on = 1;
  setsockopt(multi_sock, SOL_SOCKET, SO_REUSEPORT, &on, sizeof(on));

  if (bind(multi_sock, (struct sockaddr *)&addr, sizeof(addr)) == -1)
  {
    error_handle("bind() error");
  }

  struct ip_mreq join_addr;
  join_addr.imr_multiaddr.s_addr = inet_addr(multi_addr);
  join_addr.imr_interface.s_addr = htonl(INADDR_ANY);

  if (setsockopt(multi_sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, (void *)&join_addr,
                 sizeof(join_addr)) < 0)
  {
    error_handle("setsockopt() join error");
  }

  char server_addr[BUF_SIZE];
  int len = recvfrom(multi_sock, &server_addr, sizeof(server_addr), 0, NULL, 0);
  if (len < 0)
  {
    error_handle("recvfrom() error");
  }

  strcpy(dest, server_addr);
  close(multi_sock);
}

struct socket make_client_sock(char *server_addr_str, char name[])
{
  int descriptor = socket(PF_INET, SOCK_STREAM, 0);
  if (descriptor == -1)
  {
    error_handle("socket() error");
  }

  struct sockaddr_in serv_addr;
  memset(&serv_addr, 0, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = inet_addr(server_addr_str);
  serv_addr.sin_port = htons(PORT);

  if (connect(descriptor, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1)
  {
    error_handle("connect() error");
  }

  write(descriptor, name, BUF_SIZE);

  struct socket result = {.descriptor = descriptor, .addr = serv_addr};
  return result;
}

struct chat_status
{
  char **client_list;
  int client_length;
  struct socket *sock;
};

void *get_code(void *payload)
{
  struct chat_status *status = (struct chat_status *)payload;
  while (1)
  {
    int code;
    read(status->sock->descriptor, &code, sizeof(code));
    printf("RECEIVED CODE : %d\n", code);

    switch (code)
    {
    case 1:
    {
      int length;
      read(status->sock->descriptor, &length, sizeof(length));

      if (status->client_length != 0)
      {
        for (int i = 0; i < status->client_length; i++)
        {
          free(status->client_list[i]);
        }

        free(status->client_list);
      }

      status->client_length = length;
      status->client_list = malloc(sizeof(char *) * length);

      for (int i = 0; i < length; i++)
      {
        char *name = malloc(BUF_SIZE);
        read(status->sock->descriptor, name, BUF_SIZE);

        status->client_list[i] = name;
      }

      break;
    }
    }
  }
}

void scene_name_input()
{
  tb_clear();

  char logo[][34] = {
      "     _                           ",
      "    | |  Version 1.0             ",
      "  __| | __ _ _ __ ___  ___  ___  ",
      " / _` |/ _` | '_ ` _ \\/ __|/ _ \\ ",
      "| (_| | (_| | | | | | \\__ \\ (_) |",
      " \\__,_|\\__,_|_| |_| |_|___/\\___/ ",
  };

  for (int i = 0; i < 6; i++)
  {
    ui_print((tb_width() - strlen(logo[i])) / 2, tb_height() / 2 - 8 + i, logo[i], TB_MAGENTA | TB_BOLD, TB_DEFAULT);
  }

  char question[] = "Q. What is your name?";
  ui_print((tb_width() - strlen(question)) / 2, tb_height() / 2, question, TB_WHITE, TB_DEFAULT);

  char name[BUF_SIZE];
  char answer[BUF_SIZE];

  struct tb_event ev;
  while (1)
  {
    sprintf(answer, "A. My name is [%s]", name);
    int answer_line_no = tb_height() / 2 + 2;
    tb_clear_line(answer_line_no);
    ui_print((tb_width() - strlen(answer)) / 2, answer_line_no, answer, TB_WHITE, TB_DEFAULT);

    tb_present();

    if (tb_poll_event(&ev))
    {
      switch (ev.type)
      {
      case TB_EVENT_KEY:
        if (ev.key == TB_KEY_CTRL_Q)
        {
          tb_shutdown();
          exit(0);
        }
        else if (ev.key == TB_KEY_BACKSPACE2)
        {
          int len = strlen(name);
          if (len > 0)
          {
            name[len - 1] = '\0';
          }
        }
        else
        {
          int len = strlen(name);
          name[len] = ev.key == TB_KEY_SPACE ? ' ' : ev.ch;
          name[len + 1] = '\0';
        }
      }
    }
  }
}

int main(int argc, char *argv[])
{
  if (argc != 3)
  {
    printf("Usage : %s <GroupIP>\n", argv[0]);
    exit(1);
  }

  char server_addr_str[BUF_SIZE];
  receive_server_addr(server_addr_str, argv[1]);

  printf("Received server address : %s\n", server_addr_str);

  tb_init();

  scene_name_input();

  char name[BUF_SIZE];

  printf("What is your name? : ");
  fgets(name, BUF_SIZE, stdin);
  name[strcspn(name, "\n")] = 0;
  system("clear");

  struct socket clnt_sock = make_client_sock(server_addr_str, name);

  pthread_t thread;

  char *client_list[MAX_CLIENT_SIZE] = {};
  struct chat_status chat_status = {.client_list = client_list, .client_length = 0, .sock = &clnt_sock};

  pthread_create(&thread, NULL, get_code, (void *)&chat_status);

  while (1)
  {
    for (int i = 0; i < chat_status.client_length; i++)
    {
      //printf("%d. %s\n", i, chat_status.client_list[i]);
    }
  }

  return 0;
}
