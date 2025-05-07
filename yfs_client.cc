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
  file_inum = yfs_client::generate_inum(isfile);

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
  This is a function to get inum of a file based on it's name and which directory it resides.
*/
int
yfs_client::get_inum(inum parent_inum, const char * name, inum & file_inum) {
  yfs_client::status ret = yfs_client::OK;

  std::string directory_content;

  if (ec->get(parent_inum, directory_content) != extent_protocol::OK){
    ret = IOERR;
    return ret;
  }

  std::list<yfs_client::dirent> list = yfs_client::unserialize(directory_content);

  std::list<yfs_client::dirent>::iterator it = list.begin();
  while (it != list.end()){
    if (strcmp(name, (*it).name.c_str()) == 0){
      file_inum = (*it).inum;
      ret = yfs_client::OK;
      break;
    }
    ++it;
  }
  return ret;
}

bool
yfs_client::is_exist(inum parent_inum, const char * name){
  std::string directory_content;
  
  // The directory does not exist probably.
  if (ec->get(parent_inum, directory_content) != extent_protocol::OK) {
    printf("[IS_EXIST] Failed to get directory content.\n");
  }

  // After you have the directory contents, unserialize them and create a list
  std::list<yfs_client::dirent> list = yfs_client::unserialize(directory_content);

  // Iterate through the directory content and see if the file exists
  std::list<yfs_client::dirent>::iterator it = list.begin();
  while (it != list.end()) {
    if (strcmp(name, (*it).name.c_str()) == 0)
      return true;
    ++it;
  }
  return false;
}

std::string
yfs_client::serialize(std::list<yfs_client::dirent> dirent_list){
  std::string result;
  std::list<yfs_client::dirent>::iterator it = dirent_list.begin();
  while(it != dirent_list.end()){
    result.append(filename((*it).inum));
    result.append(":");
    result.append((*it).name);
    result.append(";");
  }
  return result;
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

std::list<yfs_client::dirent>
yfs_client::unserialize(std::string str) {
  std::list<yfs_client::dirent> result;

  std::istringstream f(str);
  std::string s;

  while (std::getline(f, s, ';')) {
    std::string inum_string = s.substr(0, s.find(":"));
    yfs_client::inum inum = n2i(inum_string);
    std::string name = s.substr(s.find(":") + 1);

    struct yfs_client::dirent dir_ent;
    dir_ent.inum = inum;
    dir_ent.name = name;

    result.push_back(dir_ent);
  }

  return result;
}

int
yfs_client::get_dir_ent(inum par_inum, std::list<dirent> &list){
  status ret;
  // std::map<yfs_client::inum, std::list<dirent> >::iterator it = dir_dirent_map.find(par_inum);
  std::string content;
  if(ec->get(par_inum, content) != extent_protocol::OK){
    ret = IOERR;
    return ret;
  }
  list = unserialize(content);
  ret = OK;
  return ret;
}

int
yfs_client::set_attr_size(inum file_inum, size_t size){
  yfs_client::status ret;
  std::string content;
  std::string new_content;
  if(!isfile(file_inum)){
    ret = NOENT;
    goto release;
  }

  if((int)size >= 0){
    
    extent_protocol::status rr = ec->get(file_inum, content);
    if(ec->get(file_inum, content) != extent_protocol::OK){
      ret = IOERR;
      goto release;
    }
    if((int)size < content.size()){
      new_content = content.substr(0, size);
    }else if((int)size > content.size()){
      new_content = content;
      new_content.append(size - content.size(), '\0');
    }else{
      ret = OK;
      goto release;
    }

    if(ec->put(file_inum, new_content) != extent_protocol::OK){
      ret = IOERR;
      goto release;
    }

  }
  ret = OK;

  release:
  return ret;

}

int 
yfs_client::read(inum file_inum, size_t size, off_t off, std::string &buf){
  yfs_client::status ret;
  std::string content;
  if(ec->get(file_inum, content) != extent_protocol::OK){
    ret = IOERR;
    goto release;
  }

  if(off < content.size()){
    buf = content.substr(off, size);
  }

  ret = yfs_client::OK;

  release:
  return ret;

}

int
yfs_client::write(inum file_inum, const char * buf, size_t size, off_t off){
  yfs_client::status ret;
  std::string content;
  std::string added_content;
  std::string new_content;
  if(ec->get(file_inum, content) != extent_protocol::OK){
    ret = IOERR;
    goto release;
  }

  added_content.append(buf, size);

  if(off <= content.size()){
    new_content.append(content.substr(0, off));
    new_content.append(added_content);
    if(off + size < content.size()){
      new_content.append(content.substr(off + size));
    }
  }else{
    new_content.append(content);
    new_content.append(off - content.size(), '\0');
    new_content.append(added_content);
  }

  if(ec->put(file_inum, new_content) != extent_protocol::OK){
    ret = IOERR;
    goto release;
  }

  ret = OK;

  release:
  return ret;
}
