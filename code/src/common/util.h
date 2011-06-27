// Author: Hao Wu <haowu@cs.utexas.edu>

#ifndef __COMMON_UTIL_H__
#define __COMMON_UTIL_H__

#include <string>
//#include <sys/ptrace.h>

#include <common/common.h>

// Split a string and append it to the given vector of string
// @author haowu
// @param dstStringList The destination of the splitted string
// @param srcString The source string
// int splitString(vector<String> &dstStringList, String &srcString);

// write some data to a user's memory using ptrace
// We do NOT check that process `pid' is a child process in ptrace
// by the current process!
long writeToProcess(void *buf, long addr, size_t len, pid_t pid);

// read some data from a user's memory using ptrace
// We do NOT check that process `pid' is a child process in ptrace
// by the current process!
long readFromProcess(void *buf, long addr, size_t len, pid_t pid);

#endif //__COMMON_UTIL_H__

