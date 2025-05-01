// the extent server implementation

#include "extent_server.h"
#include <sstream>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

extent_server::extent_server() {
  pthread_mutex_init(&extent_server_mutex, NULL);
}

/*
  The put RPC is used to update an extent's content.
*/
int extent_server::put(extent_protocol::extentid_t id, std::string buf, int &)
{
  pthread_mutex_lock(&extent_server_mutex);
  extent_record *er = new extent_record();

  er->file_data = buf;
  er->file_attributes.mtime = time(NULL);
  er->file_attributes.atime = time(NULL);
  er->file_attributes.ctime = time(NULL);
  er->file_attributes.size = buf.length();

  extent_store[id] = er;
  printf("[extent_server::put] File (%016llx) added.\n", id);

  pthread_mutex_unlock(&extent_server_mutex);
  return extent_protocol::OK;
}

/*
  The get RPC is used to retrieve an extent's content.
*/
int extent_server::get(extent_protocol::extentid_t id, std::string &buf)
{
  // To [get] content of the extent, we have first check if it exists in the extent_store.
  extent_record *er = new extent_record();
  if (extent_store.count(id) > 0) {
    er = extent_store[id];
    
    // Get the data, put it in the callback variable 'buf'
    buf = er->file_data;
    
    // Update the access time
    pthread_mutex_lock(&extent_server_mutex);
    er->file_attributes.atime = time(NULL);
    extent_store[id] = er;
    pthread_mutex_unlock(&extent_server_mutex);

    printf("[extent_server::get] File (%016llx) found.\n", id);
    return extent_protocol::OK;
  }
  
  // If the file was not found. Let the user know.
  printf("[extent_server::get] File (%016llx) not found.\n", id);
  return extent_protocol::NOENT;
}

/*
  The getattr RPC is used to retrieve an extent's attribute.
  The attributes consist of:
	1. file size
	2. last modification time (mtime)
	3. change time (ctime)
	4. last access time (atime)
*/
int extent_server::getattr(extent_protocol::extentid_t id, extent_protocol::attr &a)
{
  extent_record *er = new extent_record();
  if (extent_store.count(id) > 0) {
    er = extent_store[id];

    a.mtime = er->file_attributes.mtime;
    a.atime = er->file_attributes.atime;
    a.ctime = er->file_attributes.ctime;
    a.size = er->file_attributes.size;

    printf("[extent_server::getattr] File (%016llx) found. Returning attributes.\n", id);
    return extent_protocol::OK;
  }

  /* Tester
  a.size = 0;
  a.atime = 0;
  a.mtime = 0;
  a.ctime = 0; */
  printf("[extent_server::getattr] File (%016llx) not found.\n", id);
  return extent_protocol::NOENT;
}

/*
  The remove RPC is used to remove an extent's content.  
*/
int extent_server::remove(extent_protocol::extentid_t id, int &)
{
  pthread_mutex_lock(&extent_server_mutex);
  delete(extent_store[id]); // Free the heap memory (the pointer)
  extent_store.erase(id); // Remove the key-value pair from the extent_store map
  pthread_mutex_unlock(&extent_server_mutex);

  printf("[extent_server::remove] File (%016llx) removed.\n", id);
  return extent_protocol::OK;
}

