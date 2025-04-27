// this is the extent server
/*
  The extent server manages a key-value store.
  Meaning, each file stored will have a unique key. The file content will be stored as a string in the value...
  BUTTTT... It also stores attributes of the file... Now that is confusing.
  How about we create a strcture that stores a string and a ... ahh... an array... of attributes... NO NO...
  ... It should be able to relate back... we can create a map... another structure because the values won't change.
  Yeah! So, ... the sturcture of the structure will be:
  struct extent_record {
    file_content_data: string;
    file_content_attr: struct { atime, mtime, ctime, size }
  }
*/

#ifndef extent_server_h
#define extent_server_h

#include <string>
#include <map>
#include "extent_protocol.h"

class extent_server {
 protected:
  // Basic Extent Record Strcture
  struct extent_record {
   std::string file_data;
   extent_protocol::attr file_attributes;
  };

  // We are not using a db here. So, the data will be deleted once the server is closed.
  // But, we have create a key-value store to book-keep files data.
  std::map<extent_protocol::extentid_t, extent_record *> extent_store;

 public:
  extent_server();

  int put(extent_protocol::extentid_t id, std::string, int &);
  int get(extent_protocol::extentid_t id, std::string &);
  int getattr(extent_protocol::extentid_t id, extent_protocol::attr &);
  int remove(extent_protocol::extentid_t id, int &);
};

#endif 







