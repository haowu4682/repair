// Author: Hao Wu <haowu@cs.utexas.edu>

#include <cstring>
#include <iostream>
#include <sstream>

#include <common/util.h>
#include <syscall/SystemCallArg.h>
using namespace std;

#define MAX_ARG_STRLEN  (PAGE_SIZE)

sysarg_type_t sysarg_type_list[] =
{
    void_record,
    sint_record,
    uint_record,
    sint32_record,
    uint32_record,
    intp_record,
    pid_t_record,
    buf_record,
    sha1_record,
    string_record,
    strings_record,
    iovec_record,
    fd_record,
    fd2_record,
    name_record, 
    path_record,
    path_at_record,
    rpath_record,
    rpath_at_record,
    buf_det_record, 
    struct_record,
    psize_t_record,
    msghdr_record
};

/* This function and the next serialize a variable-length
   (long) integer ABCDEFGHIJ....XYZ as
   (observe the 1's and 0's)
   1ABCDEFG ... 1MNOPQRS 0TUVWXYZ

 */
static inline size_t svarint(long v, char *buf)
{
    size_t nbits = 1 + ((v && v != -1)? (64 - __builtin_clzll((v < 0)? ~v: v)): 0);
    size_t nbytes = (nbits / 7) + ((nbits % 7)? 1: 0);
    size_t n = nbytes - 1;
    char *p = buf + n;
    *p = v & 0x7f;
    for (; n; --n) {
        v >>= 7;
        --p;
        *p = (v & 0x7f) | 0x80;
    }
    return nbytes;
}

/* See comment above */
static inline size_t uvarint(unsigned long v, char *buf)
{
    size_t nbits = (v)? (64 - __builtin_clzll(v)): 1;
    size_t nbytes = (nbits / 7) + ((nbits % 7)? 1: 0);
    size_t n = nbytes - 1;
    char *p = buf + n;
    *p = v & 0x7f;
    for (; n; --n) {
        v >>= 7;
        --p;
        *p = (v & 0x7f) | 0x80;
    }
    return nbytes;
}

// TODO: Give more precise description here.
String void_record(long argValue, SystemCallArgumentAuxilation *argAux)
{
    return "";
}

String sint_record(long argValue, SystemCallArgumentAuxilation *argAux)
{
    stringstream os;
    os << argValue;
    return os.str();
}

String uint_record(long argValue, SystemCallArgumentAuxilation *argAux)
{
    ostringstream os;
    os << argValue;
    return os.str();
}

String sint32_record(long argValue, SystemCallArgumentAuxilation *argAux)
{
    ostringstream os;
    os << argValue;
    return os.str();
}

String uint32_record(long argValue, SystemCallArgumentAuxilation *argAux)
{
    ostringstream os;
    os << argValue;
    return os.str();
}

String intp_record(long argValue, SystemCallArgumentAuxilation *argAux)
{
    // The integer shall be in user's space. So we need to use ptrace to get the value.
    // The same mechanism will apply to other similar record functions.
    long pret;
    int buf;
    ostringstream os;
    pret = readFromProcess(&buf, argValue, sizeof(int), argAux->pid);
    if (pret < 0)
    {
        LOG("Error recording syscallarg: intp %ld", argValue);
    }
    os << "[" << buf << "]";
    return os.str();
}

String pid_t_record(long argValue, SystemCallArgumentAuxilation *argAux)
{
    // XXX: According to retro's code there might be a bug here.
    sint32_record(argValue, argAux);
}

String buf_record(long argValue, SystemCallArgumentAuxilation *argAux)
{
    long pret;
    size_t len = argAux->aux;
    if (len > 0)
    {
        char *buf = new char[len];
        pret = readFromProcess(buf, argValue, len, argAux->pid);
        String str(buf, len);
        delete buf;
        //LOG1(str.c_str());
        return str;
    }
    else
    {
        return "None";
    }
}

String sha1_record(long argValue, SystemCallArgumentAuxilation *argAux)
{
    String str;
    // XXX: there might be a bug here.
    str = buf_record(argValue, argAux);
    return str;
}

String string_record(long argValue, SystemCallArgumentAuxilation *argAux)
{
    long len;
    char buf[MAX_ARG_STRLEN];
    readFromProcess(buf, argValue, MAX_ARG_STRLEN, argAux->pid);
    if ( !buf || !(len = strlen(buf))) {
        return "None";
    }
    return buf;
}

String strings_record(long argValue, SystemCallArgumentAuxilation *argAux)
{
    String str;
    //TODO: implement
    return str;
}

String iovec_record(long argValue, SystemCallArgumentAuxilation *argAux)
{
    String str;
    return str;
}

String fd_record(long argValue, SystemCallArgumentAuxilation *argAux)
{
    // XXX: We will use the way like STRACE here. However, we need code to deal with fd
    // when we compare two syscalls anyway.
    return uint32_record(argValue, argAux);
}

String fd2_record(long argValue, SystemCallArgumentAuxilation *argAux)
{
    String str;
    return str;
}

String name_record(long argValue, SystemCallArgumentAuxilation *argAux)
{
    String str;
    return str;
}

String path_record(long argValue, SystemCallArgumentAuxilation *argAux)
{
    String str;
    SystemCallArgumentAuxilation aux = *argAux;
    //aux.aux = AT_FDCWD;
    str = path_at_record(argValue, &aux);
    return str;
}

String path_at_record(long argValue, SystemCallArgumentAuxilation *argAux)
{
    String str;
    return string_record(argValue, argAux);
    return str;
}

String rpath_record(long argValue, SystemCallArgumentAuxilation *argAux)
{
    String str;
    //TODO: implement
    return string_record(argValue, argAux);
    return str;
}

String rpath_at_record(long argValue, SystemCallArgumentAuxilation *argAux)
{
    String str;
    //TODO: implement
    return string_record(argValue, argAux);
    return str;
}

String buf_det_record(long argValue, SystemCallArgumentAuxilation *argAux)
{
    //XXX: We did not check the flag here. There might be a bug here.
    return buf_record(argValue, argAux);
}

String struct_record(long argValue, SystemCallArgumentAuxilation *argAux)
{
    String str;
    return str;
}

String psize_t_record(long argValue, SystemCallArgumentAuxilation *argAux)
{
    String str;
    return str;
}

String msghdr_record(long argValue, SystemCallArgumentAuxilation *argAux)
{
    String str;
    return str;
}

void SystemCallArgument::setArg(long regValue, SystemCallArgumentAuxilation *aux,
        const SyscallArgType *syscallType /* = NULL */)
{
    if (syscallType != NULL)
    {
        type = syscallType;
    }
    value = type->record(regValue, aux);
}

void SystemCallArgument::setArg(String record, const SyscallArgType *syscallType /*=NULL*/)
{
    if (syscallType != NULL)
    {
        type = syscallType;
    }
    this->value = record;
}

void SystemCallArgument::setArg()
{
    this->value = "None";
}

bool SystemCallArgument::operator == (SystemCallArgument &another)
{
    // TODO: Implement it
    return ((type == another.type) && (value == another.value));
}

bool SystemCallArgument::operator != (SystemCallArgument &another)
{
    return !operator ==(another);
}
