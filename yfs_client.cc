// yfs client.  implements FS operations using extent and lock server
#include "yfs_client.h"
#include "extent_client.h"
#include <sstream>
#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


yfs_client::yfs_client(std::string extent_dst, std::string lock_dst)
{
  ec = new extent_client(extent_dst);

}

yfs_client::inum
yfs_client::n2i(std::string n)
{
  std::istringstream ist(n);
  unsigned long long finum;
  ist >> finum;
  return finum;
}

yfs_client::inum
yfs_client::generate_inum(bool isfile) {
  inum file_inum;
  file_inum = std::rand(); // generate a random integer
  if (isfile) {
    file_inum |= 0x80000000; // 10000000 00000000 00000000 00000000 = 0x80000000
  } else {
    file_inum &= 0x7FFFFFFF; // 01111111 11111111 11111111 11111111 = 0x7FFFFFFF
  }

  return file_inum;
}

std::string
yfs_client::filename(inum inum)
{
  std::ostringstream ost;
  ost << inum;
  return ost.str();
}

bool
yfs_client::isfile(inum inum)
{
  if(inum & 0x80000000)
    return true;
  return false;
}

bool
yfs_client::isdir(inum inum)
{
  return ! isfile(inum);
}

int
yfs_client::getfile(inum inum, fileinfo &fin)
{
  int r = OK;


  printf("getfile %016llx\n", inum);
  extent_protocol::attr a;
  if (ec->getattr(inum, a) != extent_protocol::OK) {
    r = IOERR;
    goto release;
  }

  fin.atime = a.atime;
  fin.mtime = a.mtime;
  fin.ctime = a.ctime;
  fin.size = a.size;
  printf("getfile %016llx -> sz %llu\n", inum, fin.size);

 release:

  return r;
}

int
yfs_client::getdir(inum inum, dirinfo &din)
{
  int r = OK;


  printf("getdir %016llx\n", inum);
  extent_protocol::attr a;
  if (ec->getattr(inum, a) != extent_protocol::OK) {
    r = IOERR;
    goto release;
  }
  din.atime = a.atime;
  din.mtime = a.mtime;
  din.ctime = a.ctime;

 release:
  return r;
}

/*
  Create (Create a file, this is only for creating a file)
  In create method, we follow the following steps:
  1. Generate a unique inum for the file
  2. Save the file
  3. Once you have saved the file, you need to update the directory
     and let it know that there is a new file.
  4. Take the former content (list) previously kept in a temp variable
     and append to it the new file addition record.
*/
int yfs_client::create(inum parent_inum, const char * name, inum & file_inum, bool isfile) {
  status ret;

  // Give a unique inum to the file/directory
  file_inum = yfs_client::generate_inum(name,isfile);

  // Initial content is for when the file is created
  std::string initial_content;

  // Former content is for saving the previous content of the directory
  std::string former_content;

  // yfs_client::dirent is the structure of how the records are stored in the directory
  struct yfs_client::dirent file_dirent;

  file_dirent.name = name;
  file_dirent.inum = file_inum;

  // [PUT] Save the file to the server
  if (ec -> put(file_inum, initial_content) != extent_protocol::OK){
    ret = IOERR;
    goto release;
  }

  // [GET] Get former records of the directory
  if (ec -> get(parent_inum, former_content) != extent_protocol::OK){
    ret = IOERR;
    goto release;
  }

  // Append the new file record to the former content
  former_content.append(yfs_client::serialize_dirent(file_dirent));

  // Update the directory structure
  if (ec -> put(parent_inum, former_content) != extent_protocol::OK){
    ret = IOERR;
    goto release;
  }

  ret = yfs_client::OK;

  release:
    return ret;
}



/*
  So, you take an object and convert it into a string.
  Similar to a key value pair.
*/
std::string
yfs_client::serialize_dirent(yfs_client::dirent dirent){
  std::string result;
  result.append(filename(dirent.inum));
  result.append(":");
  result.append(dirent.name);
  result.append(";");
  return result;
}