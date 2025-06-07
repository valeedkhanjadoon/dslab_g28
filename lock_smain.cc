#include "rpc.h"
#include <arpa/inet.h>
#include <stdlib.h>
#include <stdio.h>
#include "lock_server_cache.h"
#include "paxos.h"
#include "rsm.h"

#include "jsl_log.h"

// Main loop of lock_server

int main(int argc, char *argv[])
{
  int count = 0;

  setvbuf(stdout, NULL, _IONBF, 0);
  setvbuf(stderr, NULL, _IONBF, 0);

  srandom(getpid());

  if (argc != 3)
  {
    fprintf(stderr, "Usage: %s [master:]port [me:]port\n", argv[0]);
    exit(1);
  }

  char *count_env = getenv("RPC_COUNT");
  if (count_env != NULL)
  {
    count = atoi(count_env);
  }

  // jsl_set_debug(2);

#define RSM
#ifdef RSM
  rsm rsm(argv[1], argv[2]);
#endif // RSM

  while (1)
    sleep(1000);
}
