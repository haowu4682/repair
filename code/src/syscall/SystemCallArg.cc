// Author: Hao Wu <haowu@cs.utexas.edu>

#include <sstream>

#include <common/util.h>
#include <syscall/SystemCallArg.h>
using namespace std;

// TODO: Give more precise description here.
String void_record(long argValue, SystemCallArgumentAuxilation *argAux)
{
    return "";
}

String sint_record(long argValue, SystemCallArgumentAuxilation *argAux)
{
    ostringstream os;
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
    pret = readFromProcess(&buf, argValue, sizeof(int));
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
    char *buf = new char[len];
    if (len > 0)
    {
        pret = readFromProcess(&buf, argValue, len);
        String str(buf, len);
        delete buf;
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
    return str;
}

String string_record(long argValue, SystemCallArgumentAuxilation *argAux)
{
    String str;
    return str;
}

String strings_record(long argValue, SystemCallArgumentAuxilation *argAux)
{
    String str;
    return str;
}

String iovec_record(long argValue, SystemCallArgumentAuxilation *argAux)
{
    String str;
    return str;
}

String fd_record(long argValue, SystemCallArgumentAuxilation *argAux)
{
    String str;
    return str;
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
    return str;
}

String path_at_record(long argValue, SystemCallArgumentAuxilation *argAux)
{
    String str;
    return str;
}

String rpath_record(long argValue, SystemCallArgumentAuxilation *argAux)
{
    String str;
    return str;
}

String rpath_at_record(long argValue, SystemCallArgumentAuxilation *argAux)
{
    String str;
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
        sysarg_type_t syscallType /* = NULL */)
{
    if (syscallType != NULL)
    {
        type = syscallType;
    }
    value = type(regValue, aux);
}

void SystemCallArgument::setArg(String record, sysarg_type_t syscallType /*=NULL*/)
{
    if (syscallType != NULL)
    {
        type = syscallType;
    }
    this->value = record;
}

bool SystemCallArgument::operator == (SystemCallArgument &another)
{
    // TODO: Implement it
    return ((type == another.type) && (value == another.value));
}

