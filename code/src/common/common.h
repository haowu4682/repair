// Original Author: Hao Wu <haowu@cs.utexas.edu>

#ifndef __COMMON_COMMON_H__
#define __COMMON_COMMON_H__

#include <assert.h>
#include <cerrno>
#include <ctime>
#include <stdio.h>
#include <string>
#include <sys/user.h>
#include <vector>

// Type definition
#define String std::string
#define Vector std::vector

// Constant definition
#define kNanosecondsToSeconds 1e-9
// The length of the max 64bit-int represented in decimals must not exceed the value
#ifndef UIO_MAXIOV
#define UIO_MAXIOV 1024
#endif

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


#endif //__COMMON_COMMON_H__

