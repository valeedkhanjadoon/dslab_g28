//
// Lock demo
//

#include "lock_protocol.h"
#include "lock_client.h"
#include "rpc.h"
#include <arpa/inet.h>
#include <vector>
#include <stdlib.h>
#include <stdio.h>

std::string dst;
lock_client *lc;

int
main(int argc, char *argv[])
{
  int r;
  int rc, rc1, rc2, lr, rc3, rc4;

  if(argc != 2){
    fprintf(stderr, "Usage: %s [host:]port\n", argv[0]);
    exit(1);
  }

  dst = argv[1];
  lc = new lock_client(dst);
  r = lc->stat(1);
  printf ("stat returned %d\n", r);
  
  rc = lc->acquire(1);
  printf ("acquire return %d\n", rc);
  rc1 = lc->acquire(1);
  printf("acquire return %d\n", rc1);

  lr = lc->release(1);
  printf("release return %d\n", lr);

  rc2 = lc->acquire(4);
  printf("acquire return lock4  %d\n", rc2);

  rc3 = lc->acquire(2);
  printf("acquire return lock2 %d\n", rc3);

  rc4 = lc->acquire(3);
  printf("acquire return lock3 %d\n", rc4);
}
