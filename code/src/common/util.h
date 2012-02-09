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

/// Read a string from input stream bounded by ""
String readEscapeString(Istream &is);

/// Convert out escape sequences
String removeEscapeSequence(const String &src);

/// Convert in escape sequences
String addEscapeSequence(const String &src);

/// Convert buffer sequence into str sequence
String bufToStr(const String &buf);

/// Print a message to the output stream
void printMsg(const String &msg, std::ostream &os = std::cout);
void printMsg(const char *msg, std::ostream &os = std::cout);

/// Retrive a single letter answer from the input stream.
/// If the input stream does not return a character that makes sense, the
/// function just try to read from it again.
/// The function is a BLOCKING function.
/// @param alphabet the available alphabet, NULL means anything is OK.
/// @param noticeMsg the noticeMsg when the input character is not in the
/// alphabet
char retrieveChar(const char *alphabet = NULL, const char *noticeMsg = NULL,
        std::istream &is = std::cin, std::ostream &os = std::cout);

#endif //__COMMON_UTIL_H__

