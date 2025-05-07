// this is the extent server

#ifndef extent_server_h
#define extent_server_h

#include <string>
#include <map>
#include "extent_protocol.h"

class extent_server {

 public:
  struct extent_record {
    std::string file_data;
    extent_protocol::attr file_attributes;
  };

  // We have to use a key value store to book-keep files.
  std::map<extent_protocol::extentid_t, extent_record *> extent_store;
  pthread_mutex_t extent_server_mutex;

  extent_server();

  int put(extent_protocol::extentid_t id, std::string, int &);
  int get(extent_protocol::extentid_t id, std::string &);
  int getattr(extent_protocol::extentid_t id, extent_protocol::attr &);
  int remove(extent_protocol::extentid_t id, int &);
};

#endif 







