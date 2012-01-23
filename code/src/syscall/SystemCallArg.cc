// Author: Hao Wu <haowu@cs.utexas.edu>

#include <common/util.h>
#include <syscall/SystemCallArg.h>
using namespace std;

void SystemCallArgument::setArg(long regValue, SystemCallArgumentAuxilation *aux,
        const SyscallArgType *syscallType)
{
    type = syscallType;
    value = type->record(regValue, aux);
}

void SystemCallArgument::setArg(String record, const SyscallArgType *syscallType)
{
    type = syscallType;
    if (type != NULL && (type->record == buf_record ||
                type->record == buf_det_record ||
                type->record == path_record))
    {
        record = bufToStr(record);
    }
    this->value = record;
}

void SystemCallArgument::setArg(const SyscallArgType *syscallType)
{
    type = syscallType;
    this->value = "None";
}

// An aux function to get the path from a path record. It's not elegant.
// OBSOLETED! DO NOT USE!
#if 0
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
#endif

String SystemCallArgument::getValue() const
{
    // TODO: no ad-hoc here.
    return value;
}

bool SystemCallArgument::operator == (const SystemCallArgument &another) const
{
    if (type != another.type)
        return false;
    if (value != another.value)
        return false;
    return true;
}

bool SystemCallArgument::operator != (const SystemCallArgument &another) const
{
    return !operator ==(another);
}

bool SystemCallArgument::operator < (const SystemCallArgument &another) const
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
    str = type->name + "=" + addEscapeSequence(value);
    return str;
}

int SystemCallArgument::overwrite(pid_t pid, user_regs_struct &regs, int i) const
{
    if (type->usage & SYSARG_IFEXIT)
    {
        return type->overwrite(this, pid, regs, i);
    }
    else
    {
        return -1;
    }
}

