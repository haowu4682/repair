#pragma once
#include "sha1.h"

struct inode;

enum { PATHID_LEN = SHA1_SIZE };
void reset_pathid(void);
const char* last_pathid(void);
void store_pathid(const char* name, size_t len, struct inode* inode);

struct pathid_record {
  char pathid[PATHID_LEN];
  char parent_pathid[PATHID_LEN];
  char buf[128];
  size_t len;  // len of data in buf
} __attribute__((__packed__));

