#include <stdio.h>
#include <stdlib.h>
#include "./serve.h"

int main(int argc, const char *argv[]) {
  if (argc <= 2) {
    fprintf(stderr, "usage:\nhttpserver [port] [documentroot]\n");
    return 1;
  }
  int port = atoi(argv[1]);
  const char* docroot = argv[2];
  serve(port, docroot);
  return 0;
}
