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

  puts("Waiting for server address...");

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

  while (connect(descriptor, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1)
  {
    if (errno == EINTR) {
      continue;
    } else {
      error_handle("connect() error");
    }
  }

  write(descriptor, name, BUF_SIZE);

  struct socket result = {.descriptor = descriptor, .addr = serv_addr};
  return result;
}

struct chat_status
{
  struct client_data *client_list;
  int client_length;
  char *name;
  struct socket *sock;

  char **chat_logs;
  int chat_length;

  char *pair_request;
  void *response;

  char **screen;
};

#define WAIT_RESPONSE(S, T) \
  *(T *)wait_response(S);   \
  free_response(S)

void *wait_response(struct chat_status *status)
{
  while (status->response == NULL)
  {
    usleep(1000);
  }

  return status->response;
}

void free_response(struct chat_status *status)
{
  free(status->response);
  status->response = NULL;
}

void add_chat(struct chat_status *status, char *message)
{
  status->chat_logs[status->chat_length] = malloc(BUF_SIZE);
  strcpy(status->chat_logs[status->chat_length], message);
  status->chat_length++;
}

void *get_code(void *payload)
{
  struct chat_status *status = (struct chat_status *)payload;
  while (1)
  {
    int code;
    read(status->sock->descriptor, &code, sizeof(code));

    switch (code)
    {
    case SCODE_CLIENT_LIST:
    {
      int length;
      read(status->sock->descriptor, &length, sizeof(length));

      if (status->client_length != 0)
      {
        for (int i = 0; i < status->client_length; i++)
        {
          free(status->client_list[i].name);
        }

        free(status->client_list);
      }

      status->client_length = length;
      status->client_list = malloc(sizeof(struct client_data) * length);

      for (int i = 0; i < length; i++)
      {
        char *name = malloc(BUF_SIZE);
        read(status->sock->descriptor, name, BUF_SIZE);

        int opponent;
        read(status->sock->descriptor, &opponent, sizeof(int));

        struct client_data data = {.name = name, .opponent = opponent};
        status->client_list[i] = data;
      }

      break;
    }
    case SCODE_PAIRING_REQUEST:
    {
      char *name = malloc(BUF_SIZE);
      read(status->sock->descriptor, name, BUF_SIZE);

      status->pair_request = name;
      break;
    }
    case SCODE_PAIRING_RESULT:
    case SCODE_PAIRING_ANSWER:
    case SCODE_OPPONENT_UID:
    {
      int *response = malloc(sizeof(int));
      read(status->sock->descriptor, response, sizeof(int));

      status->response = response;
      break;
    }
    case SCODE_SEND_NAME:
    {
      char *name = malloc(BUF_SIZE);
      read(status->sock->descriptor, name, BUF_SIZE);

      status->response = name;
      break;
    }
    case SCODE_CHAT_MESSAGE:
    {
      char message[BUF_SIZE];
      read(status->sock->descriptor, message + 1, BUF_SIZE);
      message[0] = '1';

      add_chat(status, message);
      break;
    }

    case SCODE_FINISH_CHAT:
    {
      for (int i = 0; i < status->chat_length; i++)
      {
        free(status->chat_logs[i]);
      }

      free(status->chat_logs);
      status->chat_logs = NULL;
      status->chat_length = 0;
      break;
    }
    case SCODE_SCREEN:
    {
      if (status->screen != NULL)
      {
        for (int i = 0; i < 14; i++)
        {
          free(status->screen[i]);
        }
        free(status->screen);
      }

      char **screen = malloc(sizeof(char *) * 14);

      for (int i = 0; i < 14; i++)
      {
        char buf[BUF_SIZE];
        read(status->sock->descriptor, buf, BUF_SIZE);

        screen[i] = malloc(BUF_SIZE);
        strcpy(screen[i], buf);
        //screen[i][28] = '\0';
      }

      status->screen = screen;
      break;
    }
    }
  }
}

void scene_name_input(char *output)
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
    ui_print_center(tb_height() / 2 - 8 + i, logo[i], 0x05 | TB_BOLD, TB_DEFAULT);
  }

  char question[] = "Q. What is your name?";
  ui_print_center(tb_height() / 2, question, 0x07, TB_DEFAULT);

  char name[BUF_SIZE];
  memset(name, '\0', BUF_SIZE);

  char answer[BUF_SIZE] = "";
  char error_message[BUF_SIZE] = "";

  struct tb_event ev;
  while (1)
  {
    sprintf(answer, "A. My name is %s", name);

    int answer_line_no = tb_height() / 2 + 2;
    tb_clear_line(answer_line_no);
    int start = ui_print_center(answer_line_no, answer, 0x07, TB_DEFAULT);

    tb_change_cell_style(start + 14,
                         strlen(name),
                         answer_line_no,
                         0x02 | TB_UNDERLINE | TB_BOLD, TB_DEFAULT);

    tb_clear_line(answer_line_no + 2);
    ui_print_center(answer_line_no + 2, error_message, 0x01, TB_DEFAULT);
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
        else if (ev.key == TB_KEY_ENTER)
        {
          if (strlen(name) == 0)
          {
            strcpy(error_message, "Name should not be empty");
          }
          else if (name[0] == ' ')
          {
            strcpy(error_message, "Name should not start with space");
          }
          else if (name[strlen(name) - 1] == ' ')
          {
            strcpy(error_message, "Name should not end with space");
          }
          else
          {
            strcpy(output, name);
            return;
          }
        }
        else
        {
          int len = strlen(name);
          name[len] = ev.key == TB_KEY_SPACE ? ' ' : ev.ch;
        }
      }
    }
  }
}

void scene_chat_list(struct chat_status *status, int *result)
{
  int rect_width = 35, rect_height = 17;
  int rect_top = (tb_height() - rect_height) / 2, rect_left = (tb_width() - rect_width) / 2;

  char message[BUF_SIZE] = "";
  sprintf(message, "Hello, %s. Select a person to chat with!", status->name);
  char error_message[BUF_SIZE] = "";

  int selection = 0;

  struct tb_event ev;
  while (1)
  {
    tb_clear();
    ui_rect(rect_top,
            rect_top + rect_height,
            rect_left,
            rect_left + rect_width,
            0x07);

    if (status->client_length > 0 && selection >= status->client_length)
    {
      selection = status->client_length - 1;
    }

    for (int i = 0; i < status->client_length; i++)
    {
      char item[BUF_SIZE] = "";

      struct client_data data = status->client_list[i];
      if (strlen(data.name) > 29)
      {
        sprintf(item, "%d. %.26s...", i, data.name);
      }
      else
      {
        sprintf(item, "%d. %s", i, data.name);
      }

      uint16_t fg = i == selection ? 0xe8 : (data.opponent == -1 ? 0x07 : 0xf1);
      uint16_t bg = i == selection ? 0x02 : TB_DEFAULT;
      ui_print(rect_left + 2,
               rect_top + i + 1, item,
               fg,
               bg);
    }

    ui_print_center(rect_top - 2, message, 0x07, TB_DEFAULT);

    int error_message_line = rect_top + rect_height + 2;
    tb_clear_line(error_message_line);
    ui_print_center(error_message_line, error_message, 0x01, TB_DEFAULT);

    tb_present();

    if (status->pair_request != NULL)
    {
      char message[BUF_SIZE] = "";
      sprintf(message, "'%s' wants to chat with you. Do you want to accept?", status->pair_request);
      int answer = !ui_dialog(50, 10, message, "Yes", "Nah");

      free(status->pair_request);
      status->pair_request = NULL;

      write_code(status->sock->descriptor, CCODE_PAIRING_ANSWER);
      write(status->sock->descriptor, &answer, sizeof(int));

      if (answer == 1)
      {
        int opponent_uid = WAIT_RESPONSE(status, int);
        *result = opponent_uid;
        return;
      }
    }

    if (tb_peek_event(&ev, 10))
    {
      switch (ev.type)
      {
      case TB_EVENT_KEY:
        if (ev.key == TB_KEY_CTRL_Q)
        {
          tb_shutdown();
          exit(0);
        }
        else if (ev.key == TB_KEY_ENTER)
        {
          write_code(status->sock->descriptor, CCODE_PAIRING);
          write(status->sock->descriptor, &selection, sizeof(selection));

          int response = WAIT_RESPONSE(status, int);

          switch (response)
          {
          case 0:
            strcpy(error_message, "You can't chat with yourself");
            break;
          case 1:
            strcpy(error_message, "Opponent is busy");
            break;
          case 2:
            ui_modal(45, 6, "Waiting for opponent's answer...", 3);

            int answer = WAIT_RESPONSE(status, int);

            if (answer == 0)
            {
              strcpy(error_message, "You are rejected! :(");
            }
            else // answer == opponent's uid
            {
              *result = answer;
              return;
            }

            break;
          }
        }
        else if (ev.key == TB_KEY_ARROW_DOWN)
        {
          if (selection < status->client_length - 1)
          {
            selection++;
          }
        }
        else if (ev.key == TB_KEY_ARROW_UP)
        {
          if (selection > 0)
          {
            selection--;
          }
        }
      }
    }
  }
}

void scene_chatting(struct chat_status *status, char *opponent_name)
{
  char message[BUF_SIZE];
  memset(message, '\0', BUF_SIZE);
  message[0] = '0';

  while (1)
  {
    tb_clear();

    if (status->chat_logs == NULL)
    {
      return;
    }

    char title[BUF_SIZE] = "";
    sprintf(title, "Chatting with %s (CTRL-Q to leave)", opponent_name);
    ui_print(4, 3, title, 0x07, TB_DEFAULT);

    ui_rect(4, tb_height() - 7, 3, tb_width() / 2 - 1, 0x07);
    ui_rect(tb_height() - 6, tb_height() - 3, 3, tb_width() / 2 - 1, 0x07);

    int length = 15;
    int screen_top = (tb_height() - length) / 2, screen_left = tb_width() / 2 + (tb_width() / 2 - length * 2) / 2;
    ui_rect(screen_top,
            screen_top + length,
            screen_left,
            screen_left + length * 2 - 1,
            0x07);
    ui_print(screen_left + 1, screen_top - 1, "Server screen", 0x07, TB_DEFAULT);
    if (status->screen != NULL)
    {
      for (int i = 0; i < 14; i++)
      {
        ui_print(screen_left + 1, screen_top + 1 + i, status->screen[i], 0x07, TB_DEFAULT);
      }
    }

    int text_width = tb_width() / 2 - 7;
    ui_print_width(5, tb_height() - 5, text_width, message + 1, 0x07, TB_DEFAULT);

    int line = tb_height() - 10;
    for (int i = status->chat_length - 1; i >= 0; i--)
    {
      int chat_line_count = (strlen(status->chat_logs[i] + 1) - 1) / text_width;

      line -= chat_line_count - 1;
      if (line <= 4)
        break;

      ui_print_width(5, line, tb_width() / 2 - 9, status->chat_logs[i] + 1, 0x07, TB_DEFAULT);

      int is_mine = status->chat_logs[i][0] == '0';
      line--;
      if (line <= 4)
        break;

      ui_print(5, line, " ", 0xe8, is_mine ? 0x02 : 0x01);
      ui_print(6, line, is_mine ? status->name : opponent_name, 0xe8, is_mine ? 0x02 : 0x01);
      ui_print(6 + strlen(is_mine ? status->name : opponent_name), line, " ", 0xe8, is_mine ? 0x02 : 0x01);

      line -= 3;
      if (line <= 4)
        break;
    }
    tb_present();

    struct tb_event ev;
    if (tb_peek_event(&ev, 10))
    {
      switch (ev.type)
      {
      case TB_EVENT_KEY:
        if (ev.key == TB_KEY_CTRL_Q)
        {
          write_code(status->sock->descriptor, CCODE_LEAVE_CHAT);
          return;
        }
        else if (ev.key == TB_KEY_ENTER)
        {
          if (strlen(message) > 1)
          {
            write_code(status->sock->descriptor, CCODE_CHAT_MESSAGE);
            write(status->sock->descriptor, message + 1, BUF_SIZE - 1);

            add_chat(status, message);

            memset(message, '\0', BUF_SIZE);
            message[0] = '0';
          }
        }
        else if (ev.key == TB_KEY_BACKSPACE2)
        {
          int len = strlen(message);
          if (len > 1)
          {
            message[len - 1] = '\0';
          }
        }
        else
        {
          int len = strlen(message);
          message[len] = ev.key == TB_KEY_SPACE ? ' ' : ev.ch;
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
  tb_select_output_mode(TB_OUTPUT_256);

  char name[BUF_SIZE] = "";
  scene_name_input(name);

  struct socket clnt_sock = make_client_sock(server_addr_str, name);

  pthread_t thread;
  struct client_data *client_list = NULL;
  struct chat_status chat_status = {
      .client_list = client_list,
      .client_length = 0,
      .name = name,
      .sock = &clnt_sock,

      .chat_logs = NULL,
      .chat_length = 0,
      .screen = NULL,

      .pair_request = NULL,
      .response = NULL,
  };

  pthread_create(&thread, NULL, get_code, (void *)&chat_status);

  while (1)
  {
    int opponent_uid = 0;
    scene_chat_list(&chat_status, &opponent_uid);

    write_code(clnt_sock.descriptor, CCODE_GET_NAME);
    write(clnt_sock.descriptor, &opponent_uid, sizeof(int));

    char *opponent_name = malloc(BUF_SIZE);
    strcpy(opponent_name, (char *)wait_response(&chat_status));
    free_response(&chat_status);

    chat_status.chat_logs = malloc(sizeof(char *) * BUF_SIZE);
    chat_status.chat_length = 0;
    chat_status.screen = NULL;
    scene_chatting(&chat_status, opponent_name);
  }

  tb_shutdown();
  return 0;
}
