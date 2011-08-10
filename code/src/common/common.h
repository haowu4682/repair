// Original Author: Hao Wu <haowu@cs.utexas.edu>

#ifndef __COMMON_COMMON_H__
#define __COMMON_COMMON_H__

#include <assert.h>
#include <bits/wordsize.h>
#include <cerrno>
#include <ctime>
#include <map>
#include <stdio.h>
#include <string>
#include <sys/user.h>
#include <vector>

// Type definition
#define String std::string
#define Vector std::vector
#define Map std::map
#define Pair std::pair

// Constant definition
#define ROOT_PID -1
#define kNanosecondsToSeconds 1e-9
// The length of the max 64bit-int represented in decimals must not exceed the value
#ifndef UIO_MAXIOV
#define UIO_MAXIOV 1024
#endif

#define WORD_SIZE __WORDSIZE
#define BYTE_SIZE 8
#define WORD_BYTES (WORD_SIZE / BYTE_SIZE)
// Hard code it here
#define WORD_BYTE_LOG 3
#define WORD_ALIGN (WORD_BYTES - 1)
#define int64_MAX_LENGTH 25
#define FASTBUF PAGE_SIZE

// Debugging tools
#define LOG(fmt,args...) do { \
        fprintf(stderr, "%f:%s:%s:%d: " fmt "\n", GetRealTime(),__FILE__,__func__,__LINE__ ,##args); \
        fflush(stderr);\
        if (errno != 0 && errno != EAGAIN) { \
                    perror("errno"); \
                    errno = 0; \
                } \
} while (0);

#define LOG1(x) LOG("%s",x)

#define ASSERT(x) assert(x)

// Returns the realtime in seconds
inline double
GetRealTime() {
    struct timespec time;
    clock_gettime(CLOCK_REALTIME, &time);
    return time.tv_sec + kNanosecondsToSeconds * time.tv_nsec;
}

// Class declarations. Used to avoid circular reference
class Action;
class Actor;
class Command;
class FDManager;
class PidManager;
class Pipe;
class Process;
class ProcessManager;
class SystemManager;

class SystemCall;
class SystemCallArg;
class SystemCallList;

#endif //__COMMON_COMMON_H__

