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


int extent_server::put(extent_protocol::extentid_t id, std::string buf, int &)
{
  pthread_mutex_lock(&extent_server_mutex);

  extent_record *er = new extent_record();

  er->file_data = buf;
  er->file_attributes.mtime = time(NULL);
  er->file_attributes.ctime = time(NULL);
  er->file_attributes.atime = time(NULL);
  er->file_attributes.size = buf.length();

  extent_store[id] = er;
  printf("[extent_server::put] File (%016llx) added.\n", id);

  pthread_mutex_unlock(&extent_server_mutex);
  return extent_protocol::OK;
}

int extent_server::get(extent_protocol::extentid_t id, std::string &buf)
{
  // We create the record so we can update the last access time quickly.
  extent_record *er = new extent_record();

  if (extent_store.count(id) > 0) {
    er = extent_store[id];

    // Get the data from er instance and assign it to the call back variable 'buf'
    buf = er->file_data;

    // Update the last access time and re-update the extent store
    pthread_mutex_lock(&extent_server_mutex);
    er->file_attributes.atime = time(NULL);
    extent_store[id] = er;
    pthread_mutex_unlock(&extent_server_mutex);

    printf("[extent_server::get] File %016llx found.", id);
    return extent_protocol::OK;
  }

  // If the file was not found. Let the user know
  return extent_protocol::NOENT;
}

int extent_server::getattr(extent_protocol::extentid_t id, extent_protocol::attr &a)
{
  // You replace this with a real implementation. We send a phony response
  // for now because it's difficult to get FUSE to do anything (including
  // unmount) if getattr fails.
  extent_record *er = new extent_record();
  if (extent_store.count(id) > 0) {
    er = extent_store[id];

    a.mtime = er->file_attributes.mtime;
    a.ctime = er->file_attributes.ctime;
    a.atime = er->file_attributes.atime;
    a.size = er->file_attributes.size;

    printf("[extent_server::getattr] File (%016llx) found. Returning attributes.\n", id);
    return extent_protocol::OK;
  }

  printf("[extent_server::getattr] File (%016llx) NOT found.\n", id);
  return extent_protocol::NOENT;
}

int extent_server::remove(extent_protocol::extentid_t id, int &)
{
  if (extent_store.count(id) > 0) {
    pthread_mutex_lock(&extent_server_mutex);
    delete(extent_store[id]); // Free the heap memory (the pointer)
    extent_store.erase(id); // Remove the key-value pair from the extent_store map
    pthread_mutex_unlock(&extent_server_mutex);

    printf("[extent_server::remove] File (%016llx) found and removed.\n", id);
  }

  printf("[extent_server::remove] File (%016llx) NOT found.\n");
  return extent_protocol::NOENT;
}

