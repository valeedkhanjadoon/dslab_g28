// this is the lock server
// the lock client has a similar interface

#ifndef lock_server_h
#define lock_server_h

#include <string>
#include "lock_protocol.h"
#include "lock_client.h"
#include "rpc.h"
#include <map>
#include <pthread.h>

class lock_server {

 protected:
  int nacquire;
  enum lock_states {FREE, LOCKED};
  std::map<lock_protocol::lockid_t, lock_states> locks_vault;
  pthread_mutex_t locks_vault_mutex;
  pthread_cond_t locks_vault_state_cv;

 public:
  lock_server();
  ~lock_server();
  lock_protocol::status stat(int clt, lock_protocol::lockid_t lid, int &);

  // Lab 01 Changes
  lock_protocol::status acquire(int clt, lock_protocol::lockid_t lid, int &);
  lock_protocol::status release(int clt, lock_protocol::lockid_t lid, int &);
};

#endif 







