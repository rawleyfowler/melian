#include <signal.h>
#include <stdio.h>
#include <string.h>
#include "config.h"
#include "data.h"
#include "server.h"

static void show_usage(const char* prog) {
  printf("%s -- a cache for MySQL tables\n", prog);
  printf("\n");
  printf("The program reads full tables from MySQL, stores them in memory,\n");
  printf("and serves the data over a UNIX socket based on a key value.\n");

  data_show_usage();
  config_show_usage();
}

int main(int argc, char **argv) {
  signal(SIGPIPE, SIG_IGN);
  Server* server = 0;
  do {
    if (argc >= 2 && strcmp(argv[1], "--help") == 0) {
      show_usage(argv[0]);
      break;
    }

    server = server_build();
    if (!server) break;

    if (!server_initial_load(server)) break;
    if (!server_listen(server)) break;
    if (!server_run(server)) break;
  } while (0);
  if (server) {
    server_stop(server);
    server_destroy(server);
  }
  return 0;
}
