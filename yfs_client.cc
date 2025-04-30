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
#include <vector>

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

int limited_rand(int limit)
{
  int r, d = RAND_MAX / limit;
  limit *= d;
  do
  {
    r = rand();
  } while (r >= limit);
  return r / d;
}

yfs_client::inum
yfs_client::new_inum(bool isfile)
{
  int rand_num = limited_rand(0x7FFFFFFF);

  yfs_client::inum finum;

  if (isfile)
    finum = rand_num | 0x0000000080000000;

  printf("new_inum %016llx \n", finum);
  return finum;
}

std::string
yfs_client::filename(inum inum)
{
  std::ostringstream ost;
  ost << inum;
  return ost.str();
}

bool yfs_client::isfile(inum inum)
{
  if (inum & 0x80000000)
    return true;
  return false;
}

bool yfs_client::isdir(inum inum)
{
  return !isfile(inum);
}

int yfs_client::getfile(inum inum, fileinfo &fin)
{
  int r = OK;

  printf("[yfs_client::getfile] File: %016llx\n", inum);

  extent_protocol::attr a;
  if (ec->getattr(inum, a) != extent_protocol::OK)
  {
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

int yfs_client::getdir(inum inum, dirinfo &din)
{
  int r = OK;

  printf("[yfs_client::getdir] Directory: %016llx\n", inum);
  extent_protocol::attr a;
  if (ec->getattr(inum, a) != extent_protocol::OK)
  {
    r = IOERR;
    goto release;
  }
  din.atime = a.atime;
  din.mtime = a.mtime;
  din.ctime = a.ctime;

release:
  return r;
}

int yfs_client::setattr(inum inum, fileinfo &fin)
{
  int r = OK;
  std::string file_buf;

  printf("[yfs_client::setattr] File: %016llx \n", inum);
  if (ec->put(inum, fin.size, file_buf) != extent_protocol::OK)
  {
    r = IOERR;
    goto release;
  }

release:
  return r;
}

int yfs_client::createfile(inum previous_inum, const char *name, inum &current_inum)
{
  int r = OK;
  int count = 0;
  std::string previous_buf;
  char *cstr, *p;
  inum file_inum;
  std::string file_buf("");

  // Read parent directory and check if name already exists
  if (ec->get(previous_inum, -1, 0, previous_buf) != extent_protocol::OK)
  {
    printf("[yfs_client::createfile] %016llx parent directory does not exist\n", previous_inum);
    r = NOENT;
    goto release;
  }

  cstr = new char[previous_buf.size() + 1];
  strcpy(cstr, previous_buf.c_str());
  p = strtok(cstr, "/");

  while (p != NULL)
  {
    printf("createfile: p %c\n", *p);

    // Skip its own directory name and inum
    if (count != 1 && count % 2 == 1)
    {
      if (!strncmp(p, name, strlen(name)))
      {
        delete[] cstr;
        r = EXIST;
        goto release;
      }
    }
    p = strtok(NULL, "/");
    count++;
  }

  delete[] cstr;
  file_inum = new_inum(true);

  // Create an empty extent for ina
  if (ec->put(file_inum, -1, file_buf) != extent_protocol::OK)
  {
    r = IOERR;
    goto release;
  }

  // Add a <name, ina> entry into @parent
  previous_buf.append("/" + filename(file_inum) + "/" + name);
  if (ec->put(previous_inum, -1, previous_buf) != extent_protocol::OK)
  {
    r = IOERR;
    goto release;
  }

  current_inum = file_inum;

release:
  return r;
}

int yfs_client::createroot(inum inum, const char *name)
{
  int r = OK;
  std::string file_buf('/' + filename(inum) + '/' + name);

  if (ec->put(inum, -1, file_buf) != extent_protocol::OK)
  {
    r = IOERR;
    return r;
  }
  return r;
}

int yfs_client::lookup(inum previous_inum, const char *name, inum &current_inum)
{
  int r = OK, count = 0;
  std::string previous_buf;
  char *cstr, *p;
  inum file_inum;
  std::string file_buf("");

  // Read parent directory and check if name already exists
  if (ec->get(previous_inum, -1, 0, previous_buf) != extent_protocol::OK)
  {
    r = NOENT;
    goto release;
  }

  cstr = new char[previous_buf.size() + 1];
  strcpy(cstr, previous_buf.c_str());
  p = strtok(cstr, "/");

  while (p != NULL)
  {
    // Skip its own directory name and inum
    if (count != 1 && count % 2 == 1)
    {
      if (!strncmp(p, name, strlen(name)))
      {
        delete[] cstr;
        r = OK;
        current_inum = file_inum;
        goto release;
      }
    }
    else
    {
      file_buf = p;
      file_inum = n2i(file_buf);
    }
    p = strtok(NULL, "/");
    count++;
  }

  delete[] cstr;
  r = NOENT;

release:
  return r;
}

int yfs_client::readdir(inum previous_inum, std::vector<dirent> &r_dirent)
{
  int r = OK;
  int count = 0;
  std::string previous_buf;
  char *cstr, *p;
  std::string file_buf;
  dirent current_dirent;

  // Read parent directory and check if name already exists
  if (ec->get(previous_inum, -1, 0, previous_buf) != extent_protocol::OK)
  {
    r = NOENT;
    goto release;
  }

  cstr = new char[previous_buf.size() + 1];
  strcpy(cstr, previous_buf.c_str());
  p = strtok(cstr, "/");

  while (p != NULL)
  {
    // Skip its own directory name and inum
    if ((count && count != 1) && count % 2 == 1) {
      current_dirent.name = p;
      r_dirent.push_back(current_dirent);
    }
    else {
      file_buf = p;
      current_dirent.inum = n2i(file_buf);
    }
    p = strtok(NULL, "/");
    count++;
  }

  delete[] cstr;
  r = OK;
  release:
    return r;
}

int yfs_client::read(inum in_inum, off_t off, size_t size, std::string &buf)
{
  int r = OK;
  printf("[yfs_client::read] File: %016llx, off: %ld, size: %lu\n", in_inum, (long int)off, size);

  if (ec->get(in_inum, (int)off, (unsigned int)size, buf) != extent_protocol::OK)
  {
    r = IOERR;
    goto release;
  }

release:
  return r;
}

int yfs_client::write(inum in_inum, const char *buf, off_t off, size_t size)
{
  std::string file_buf;
  int r = OK;
  printf("[yfs_client::write] File: %016llx, off: %ld, size: %lu\n", in_inum, (long int)off, size);

  file_buf.append(buf, size);
  if (ec->put(in_inum, (int)off, file_buf) != extent_protocol::OK)
  {
    r = IOERR;
    goto release;
  }

release:
  return r;
}