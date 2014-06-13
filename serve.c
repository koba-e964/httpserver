#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define SIZE 10000
#define DEBUG 1
#define PNUM 4 /* the number of processes */

char recvbuf[SIZE];

const char ok_header[] =
  "HTTP/1.1 200 OK\r\n"
  "Content-Type: text/html; charset=UTF-8\r\n"
  "Server: rserve.rb\r\n"
  "Connection: close\r\n";


const char bad_request_str[] =
  "HTTP/1.1 404 Not Found\r\n\r\n"
  "404 File Not Found\r\n\r\n";

const char fail_header[] =
  "HTTP/1.1 404 Not Found\r\n\r\n404 File Not Found\r\n\r\n";


void get_timestr(int len, char buf[]) {
  time_t t;
  struct tm timeinfo;
  time(&t);
  gmtime_r(&t, &timeinfo);
  strftime(buf, len, "%a, %d %b %Y %T GMT", &timeinfo);
#if DEBUG
  printf("time : %s\n", buf);
#endif
}

/* Invoked if bad request was sent. */
void bad_req(int sock) {
  send(sock, (void *) bad_request_str, sizeof bad_request_str, 0);
}

int get_serve(const char *docroot, char *uri, int sock) {
  int dlen = strlen(docroot);
  int ulen = strlen(uri);
  char *tmp = (char *) malloc(dlen + 2 + ulen);
  tmp[0] = '\0';
  strncat(tmp, docroot, dlen);
  strncat(tmp, uri, ulen);
#if DEBUG
  printf("file path: %s\n", tmp);
#endif
  int filesize;
  //checking file status
  {
    struct stat buf;
    if (stat(tmp, &buf) < 0) {
      perror("stat");
      goto fail;
    }
    if (! S_ISREG(buf.st_mode)) {
      fprintf(stderr, "Not a file : %s\n", tmp);
      goto fail;
    }
    filesize = buf.st_size;
    (void) filesize;
  }
  char outbuf[1000]; // buffer for string to send to client
#define O(str) send(sock, (void *) str, strlen(str), 0)
  O(ok_header);
  O("Date: ");
  get_timestr(1000, outbuf);
  O(outbuf);
  O("\r\n");
  O("\r\n");
  FILE *fp = fopen(tmp, "r");
  char *content = (char *) malloc(filesize);
  (void) fread(content, filesize, 1, fp);
  fclose(fp);
  O(content);
  free(tmp);
  return 0;
 fail:
  free(tmp);
  send(sock, (void*) fail_header, sizeof fail_header, 0);
  return -1;
}


int fd_getline(int fd, size_t len, char buf[]) {
  int i;
  int cnt = 0;
  for (i = 0; i < len - 1; ++i) {
    char ch;
    int res = read(fd, (void *) &ch, 1);
    if (res < 0){
      buf[0] = '\0';
      return -1;
    }
    if (res == 0) {
      break;
    }
    if (ch == '\r') {
      cnt++;
      i--;
      continue;
    }
    if (ch == '\n') {
      break;
    }
    buf[i] = ch;
  }
  buf[i] = '\0';
  return i + cnt;
}

void create_server(const char *docroot, int sock, int cpid) {
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
    ssize_t fst_size = fd_getline(clisock, SIZE, buf);
    if (fst_size <= 0) {
      //error
      bad_req(clisock);
      close(clisock);
      continue;
    }
    printf("=== %s\n", buf);
    char method[10], uri[1000], http_ver[20];
    int res = sscanf(buf, "%9s %999s %19s", method, uri, http_ver);
    if (res != 3) {
      // not a good request
      bad_req(clisock);
      close(clisock);
      continue;
    }
    if (strcmp(method, "GET") == 0) {
#if DEBUG
      printf("***** GET *****\n  uri = %s, http-version = %s\n", uri, http_ver);
#endif
    }
    if (strcmp(method, "HEAD") == 0) {
#if DEBUG
      printf("***** GET *****\n  uri = %s, http-version = %s\n", uri, http_ver);
#endif
    }
    while (1) {
      ssize_t size = fd_getline(clisock, SIZE, buf);
      if (size == 0) { /* Encountered eof */ 
        break;
      }
      if (size < 0) {
        perror("recv");
        exit(3);
      }
      if (buf[0] == '\0') { // empty line
        break;
      }
#if DEBUG
      printf(" input = \"%s\"", buf);
#endif
    }
    get_serve(docroot, uri, clisock);
    close(clisock);
#if DEBUG
    printf("closed (sock=%d, cpid=%d, cliaddr=%s)\n", clisock, cpid, addrstr);
#endif
  }
}

int serve(uint16_t port, const char *docroot) {
  printf("port = %d, docroot = %s\n", port, docroot);
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
      create_server(docroot, sock, i);
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
