// Author: Hao Wu
//
#ifndef __SYSCALL_TIMESTAMP__
#define __SYSCALL_TIMESTAMP__

#include <common/common.h>

class Timestamp
{
    public:
        void read(std::istream&);
        void print(std::ostream&) const;

        bool operator ==(const Timestamp &ts) const;
        bool operator !=(const Timestamp &ts) const;
        bool operator <(const Timestamp &ts) const;
        bool operator <=(const Timestamp &ts) const;
        bool operator >(const Timestamp &ts) const;
        bool operator >=(const Timestamp &ts) const;

    private:
        long usec;
        int cpuNum;
};

inline std::istream &operator >>(std::istream &in, Timestamp &ts)
{
    ts.read(in);
    return in;
}

inline std::ostream &operator <<(std::ostream &out, const Timestamp &ts)
{
    ts.print(out);
    return out;
}

#endif //__SYSCALL_TIMESTAMP__

