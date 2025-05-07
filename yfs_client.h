#ifndef yfs_client_h
#define yfs_client_h

#include "extent_client.h"
#include <string>
#include <vector>


  class yfs_client {
  extent_client *ec;
 public:

  typedef unsigned long long inum;
  enum xxstatus { OK, RPCERR, NOENT, IOERR, FBIG, EXIST };
  typedef int status;

  struct fileinfo {
    unsigned long long size;
    unsigned long atime;
    unsigned long mtime;
    unsigned long ctime;
  };
  struct dirinfo {
    unsigned long atime;
    unsigned long mtime;
    unsigned long ctime;
  };
  struct dirent {
    std::string name;
    unsigned long long inum;
  };

 private:
  static std::string filename(inum);
  static inum n2i(std::string);
  static inum generate_inum(bool);
 public:

  yfs_client(std::string, std::string);

  bool isfile(inum);
  bool isdir(inum);

  int getfile(inum, fileinfo &);
  int getdir(inum, dirinfo &);

  // Lab 02 functions
  int create(inum, const char *, inum &, bool);
  int get_dir_ent(inum, std::list<dirent> &);
  int set_attr_size(inum, size_t);
  int read(inum, size_t, off_t, std::string &);
  int write(inum, const char *, size_t, off_t);

  // misc. functions
  bool is_exist(inum, const char *);
  int get_inum(inum, const char *, inum &);

  // serialization functions
  std::string serialize(std::list<dirent>);
  std::string serialize_dirent(yfs_client::dirent);
  std::list<yfs_client::dirent> unserialize(std::string);
};

#endif 
