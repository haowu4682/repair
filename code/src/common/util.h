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

// Convert C-style command argvs into a vector of strings
// @param argc number of arguments
// @param argv the arguments to be converted
// @ret the converted command
Vector<String> convertCommand(int argc, char **argv);

// parse a string into argvs
// @param str the str to be parsed
// @param command the vector of string to store the argvs
int parseArgv(Vector<String> &command, String str);

// write some data to a user's memory using ptrace
// We do NOT check that process `pid' is a child process in ptrace
// by the current process!
long writeToProcess(const void *buf, long addr, size_t len, pid_t pid);

// read some data from a user's memory using ptrace
// We do NOT check that process `pid' is a child process in ptrace
// by the current process!
long readFromProcess(void *buf, long addr, size_t len, pid_t pid);

/// Convert register information to string, for debug use
String regsToStr(user_regs_struct &regs);

/// Convert out escape sequences
String removeEscapeSequence(const String &src);

#endif //__COMMON_UTIL_H__

