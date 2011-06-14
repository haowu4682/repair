#pragma once

struct inode;

enum { PATHID_LEN = 20 };
void reset_pathid(void);
const char* last_pathid(void);
void store_pathid(const char* name, unsigned int len, struct inode* inode);
