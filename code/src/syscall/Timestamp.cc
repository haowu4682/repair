// Author: Hao Wu

#include <syscall/Timestamp.h>
using namespace std;

bool Timestamp::operator ==(const Timestamp &ts) const
{
    return usec == ts.usec && cpuNum == ts.cpuNum;
}

bool Timestamp::operator !=(const Timestamp &ts) const
{
    return usec != ts.usec || cpuNum != ts.cpuNum;
}

bool Timestamp::operator <(const Timestamp &ts) const
{
    return usec < ts.usec;
}

bool Timestamp::operator <=(const Timestamp &ts) const
{
    return usec <= ts.usec;
}

bool Timestamp::operator >(const Timestamp &ts) const
{
    return usec > ts.usec;
}

bool Timestamp::operator >=(const Timestamp &ts) const
{
    return usec >= ts.usec;
}

void Timestamp::print(ostream &out) const
{
    out << "(" << usec << ", " << cpuNum << ")";
}

void Timestamp::read(istream &in)
{
    char auxChar;
    std::string auxStr;
    in >> auxChar;
    assert(auxChar == '(');
    in >> usec;
    in >> auxStr;
    assert(auxStr == ",");
    in >> cpuNum;
    in >> auxChar;
    assert(auxChar == ')');
}

