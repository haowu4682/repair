// Author: Hao Wu <haowu@cs.utexas.edu>

#include <syscall/SystemCallArg.h>
using namespace std;

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

// An aux function to get the path from a path record. It's not elegant.
String getPathString(const String &value)
{
    size_t startPos = value.find('"');
    if (startPos >= value.length())
        return "";
    size_t endPos = value.find('"', startPos + 1);
    if (endPos >= value.length())
        return "";
    return value.substr(startPos+1, endPos - startPos - 1);
}

String SystemCallArgument::getValue() const
{
    // TODO: no ad-hoc here.
    if (type->name == "path")
    {
        return getPathString(value);
    }
    else
    {
        return value;
    }
}

bool SystemCallArgument::operator == (SystemCallArgument &another)
{
    if (type != another.type)
        return false;
    if (value != another.value)
        return false;
    return true;
}

bool SystemCallArgument::operator != (SystemCallArgument &another)
{
    return !operator ==(another);
}

bool SystemCallArgument::operator < (SystemCallArgument &another)
{
    if (type != another.type)
        return false;
    if (another.value.find(value) != 0)
        return false;
    return true;
}

String SystemCallArgument::toString() const
{
    String str;
    str = name + "=" + value;
    return str;
}

int SystemCallArgument::overwrite(pid_t pid) const
{
}

