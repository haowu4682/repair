// Original Author: Hao Wu <haowu@cs.utexas.edu>

#ifndef __COMMON_COMMON_H__
#define __COMMON_COMMON_H__

#include <stdio.h>
#include <cerrno>
#include <ctime>

// Type definition
#define String std::string
#define Vector std::vector

// Constant definition
#define kNanosecondsToSeconds 1e-9

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

// Returns the realtime in seconds
inline double
GetRealTime() {
    struct timespec time;
    clock_gettime(CLOCK_REALTIME, &time);
    return time.tv_sec + kNanosecondsToSeconds * time.tv_nsec;
}


#endif //__COMMON_COMMON_H__

