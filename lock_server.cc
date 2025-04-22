// the lock server implementation

#include "lock_server.h"
#include <sstream>
#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>

lock_server::lock_server():
  nacquire (0)
{
  pthread_mutex_init(&locks_vault_mutex, NULL);
  pthread_cond_init(&locks_vault_state_cv, NULL);
}

lock_server::~lock_server()
{
  pthread_mutex_destroy(&locks_vault_mutex);
  pthread_cond_destroy(&locks_vault_state_cv);
}

lock_protocol::status
lock_server::stat(int clt, lock_protocol::lockid_t lid, int &r)
{
  lock_protocol::status ret = lock_protocol::OK; 
  printf("[stat] request from clt %d\n", clt);
  r = nacquire;
  return ret;
}

// Lab One Changes
lock_protocol::status
lock_server::acquire(int clt, lock_protocol::lockid_t lid, int &r)
{
  lock_protocol::status ret = lock_protocol::OK;
  printf("[acquire] request from clt %d, Lock ID: %llu\n", clt, lid);
  pthread_mutex_lock(&locks_vault_mutex);
  if (locks_vault.count(lid) > 0) {
    while (locks_vault[lid] != FREE)
	    pthread_cond_wait(&locks_vault_state_cv, &locks_vault_mutex);
  }
  locks_vault[lid] = LOCKED;
  pthread_mutex_unlock(&locks_vault_mutex);

  return ret; 
}

// Lab One changes
lock_protocol::status
lock_server::release(int clt, lock_protocol::lockid_t lid, int &r)
{
  lock_protocol::status ret = lock_protocol::OK;
  printf("[release] request from clt %d, Lock ID: %llu\n", clt, lid);
  pthread_mutex_lock(&locks_vault_mutex);
  if ((locks_vault.count(lid) > 0) && (locks_vault[lid] == LOCKED)) {
    locks_vault[lid] = FREE;
    pthread_cond_broadcast(&locks_vault_state_cv);
    pthread_mutex_unlock(&locks_vault_mutex);
  }
  else {
    pthread_mutex_unlock(&locks_vault_mutex);
    ret = lock_protocol::NOENT;
  }

  return ret;
}

