#pragma once

#include <stdint.h>
#include "protocol.h"

// TODO: make these limits dynamic? Arena?
enum {
  MAX_RESPONSE_LEN = 10240,
};

// Options available when running a client.
struct Options {
  const char *host;
  unsigned port;
  const char *unix;
  unsigned fetches[26*2+10]; // lowercase, uppercase, digits
  unsigned stats;
  unsigned quit;
  unsigned verbose;
};

struct TableData {
  unsigned rows;
  unsigned min_id;
  unsigned max_id;
};

// A running client.
typedef struct Client {
  struct Options options;
  int fd;
  unsigned rlen;
  char rbuf[MAX_RESPONSE_LEN];
  struct TableData tables[DATA_TABLE_LAST];
} Client;

Client* client_build(void);
void client_destroy(Client* client);
unsigned client_configure(Client* client, int argc, char* argv[]);

int client_read_response(Client* client);

void client_run(Client* client);
