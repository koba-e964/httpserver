#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define SIZE 100
#define DEBUG 0
#define PNUM 4 /* the number of processes */

char recvbuf[SIZE];

void create_server(int sock, int cpid) {
  struct sockaddr_in cliaddr;
  socklen_t clilen;
  (void) cpid;
  while (1) {
    int clisock = accept(sock, (struct sockaddr *)&cliaddr, &clilen);
#if DEBUG
    char addrstr[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &cliaddr, addrstr, INET_ADDRSTRLEN);
    printf("accepted (sock=%d, cpid=%d, cliaddr=%s)\n", clisock, cpid, addrstr);
#endif
    char buf[SIZE];
    while (1) {
      ssize_t size = recv(clisock, buf, SIZE, 0);
      if (size == 0) { /* connection was connectly shut down. */
        break;
      }
      if (size < 0) {
        perror("recv");
        exit(3);
      }
      send(clisock, buf, size, 0);
#if DEBUG
      int i;
      for(i=0; i<size; ++i) {
        putchar(buf[i]);
      }
#endif
    }
    close(clisock);
#if DEBUG
    printf("closed (sock=%d, cpid=%d, cliaddr=%s)\n", clisock, cpid, addrstr);
#endif
  }
}

int serve(uint16_t port, const char *docroot) {
  int sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_ANY);
  addr.sin_port = htons(port);
  if (bind(sock, (struct sockaddr *)&addr, sizeof addr) <0) {
    perror("bind");
    return 2;
  }
#if DEBUG
  puts("listening...");
#endif
  listen(sock, 10);
  int i;
  for (i = 0; i < PNUM; ++i) {
    if (fork() == 0) {
      create_server(sock, i);
      exit(0);
    }
  }
  while (1) {
    int status;
    if (waitpid(-1, &status, 0) == -1) {
      if (errno == ECHILD) { /* No children exist. */
        break;
      }
    }
  }
  close(sock);
  return 0;
}
