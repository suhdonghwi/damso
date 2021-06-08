#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>

#include "./common.h"

void receive_server_addr(char *dest, char *multi_addr)
{
  /* 
    socket(DOMAIN, TYPE, PROTOCOL): 소켓을 생성하고 소켓 디스크립터를 반환합니다.
    - DOMAIN : 어떤 영역에서 통신할 것인지를 결정합니다. PF_INET은 IPv4를 의미합니다.
    - TYPE : 어떤 서비스 타입의 소켓을 생성할 것인지를 결정합니다. SOCK_DGRAM은 UDP, SOCK_STREAM은 TCP입니다.
    - PROTOCOL : UDP, TCP를 정하는 부분인데 0을 넘기면 그냥 TYPE에서 정한 대로 따라갑니다.
  */
  int multi_sock = socket(PF_INET, SOCK_DGRAM, 0);

  struct sockaddr_in addr; // 소켓 주소에 대한 정보를 담는 구조체입니다.
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;                // IPv4 형식으로 지정합니다.
  addr.sin_addr.s_addr = htonl(INADDR_ANY); // IP를 지정합니다. htonl(INADDR_ANY)가 현재 시스템의 IP를 반환합니다.
  addr.sin_port = htons(PORT);

  /*
    setsockopt(...) : 소켓의 옵션을 설정합니다.
    - SO_REUSEADDR : 지역 주소(IP 주소, 포트번호)의 재사용을 허용합니다.
  */
  int on = 1;
  setsockopt(multi_sock, SOL_SOCKET, SO_REUSEPORT, &on, sizeof(on));

  /* 
    bind(SOCKFD, ADDR, ADDRLEN) : IP 주소와 소켓 디스크립터를 묶어서 외부에서 IP로 통신을 시도하는 컴퓨터가 소켓 디스크립터를 찾을 수 있도록 해줍니다. 실패하면 -1을 반환합니다.
    - SOCKFD : 묶으려는 소켓입니다.
    - ADDR : 서버 주소 정보입니다.
    - ADDRLEN : 서버 주소 정보의 크기입니다.
  */
  if (bind(multi_sock, (struct sockaddr *)&addr, sizeof(addr)) == -1)
  {
    error_handle("bind() error");
  }

  struct ip_mreq join_addr;                               // 멀티캐스트 통신을 위한 정보를 담는 구조체입니다.
  join_addr.imr_multiaddr.s_addr = inet_addr(multi_addr); // 멀티캐스트 그룹의 IP 주소를 지정합니다.
  join_addr.imr_interface.s_addr = htonl(INADDR_ANY);     // 멀티캐스트 패킷을 받을 네트워크 인터페이스를 지정합니다.

  // setsockopt로 IP_ADD_MEMBERSHIP을 지정하면 소켓이 join_addr이 나타내는 멀티캐스트 그룹에 가입됩니다.
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

int get_client_list(struct socket sock, char **list)
{
  int code = 1;
  write(sock.descriptor, &code, sizeof(code));

  int length;
  read(sock.descriptor, &length, sizeof(length));

  printf("%d\n", length);

  for (int i = 0; i < length; i++)
  {
    char name[BUF_SIZE];
    read(sock.descriptor, name, sizeof(name));

    bool alive;
    read(sock.descriptor, &alive, sizeof(alive));

    if (alive)
    {
      list[i] = malloc(BUF_SIZE);
      strcpy(list[i], name);
    }
  }

  return length;
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

  char name[BUF_SIZE];
  printf("What is your name? : ");
  fgets(name, BUF_SIZE, stdin);
  name[strcspn(name, "\n")] = 0;

  struct socket clnt_sock = make_client_sock(server_addr_str, name);

  char *client_list[MAX_CLIENT_SIZE] = {};
  int length = get_client_list(clnt_sock, client_list);

  for (int i = 0; i < length; i++)
  {
    printf("%d : %s\n", i, client_list[i]);
  }

  while (1)
  {
  }
  return 0;
}
