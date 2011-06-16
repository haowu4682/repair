// Author: Hao Wu <haowu@cs.utexas.edu>

#ifndef __COMMON_UTIL_H__
#define __COMMON_UTIL_H__

#include <string>

#include <common.h>

// Split a string and append it to the given vector of string
// @author haowu
// @param dstStringList The destination of the splitted string
// @param srcString The source string
int splitString(vector<String> &dstStringList, String &srcString);

#endif //__COMMON_UTIL_H__

