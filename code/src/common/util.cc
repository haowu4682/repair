// Author: Hao Wu <haowu@cs.utexas.edu.cn>

#include <climits>
#include <cstring>
#include <sstream>
#include <sys/ptrace.h>

#include <common/util.h>

// Since PTRACE is quite low level, we need to use these functions to peek/poke data.
// Some architecture-specified code is applied in the function.
long writeToProcess(const void *buf, long addr, size_t len, pid_t pid)
{
    long pret = 0;
    long startAddr, endAddr;
    const char *charBuf = reinterpret_cast<const char *>(buf);

    if (len <= 0)
    {
        return len;
    }
    startAddr = addr & (~WORD_ALIGN);
    endAddr = (addr + len - 1) & (~WORD_ALIGN);
    //LOG("startAddr=%p, endAddr=%p", startAddr, endAddr);
    for (long alignAddr = startAddr; alignAddr <= endAddr; alignAddr += WORD_BYTES)
    {
        long wordBuf;
        char *byteBuf = reinterpret_cast<char *>(&wordBuf);
        long startAlign, endAlign;
        // When the word is not aligned, we must not damage any origin data.
        if (startAddr == endAddr)
        {
            // cut both head and tail remainder
            startAlign = addr & WORD_ALIGN;
            endAlign = (addr+len-1) & WORD_ALIGN;
            wordBuf = ptrace(PTRACE_PEEKDATA, pid, alignAddr, NULL);
        }
        else if (alignAddr == startAddr)
        {
            // cut head remainder
            startAlign = addr & WORD_ALIGN;
            endAlign = WORD_ALIGN;
            wordBuf = ptrace(PTRACE_PEEKDATA, pid, alignAddr, NULL);
        }
        else if (alignAddr == endAddr)
        {
            // cut tail remainder
            startAlign = 0;
            endAlign = (addr+len-1) & WORD_ALIGN;
            wordBuf = ptrace(PTRACE_PEEKDATA, pid, alignAddr, NULL);
        }
        else
        {
            // no need to cut remainder, no need to peek data to avoid damaging data
            startAlign = 0;
            endAlign = WORD_ALIGN;
        }
        for (int i = startAlign; i <= endAlign; i++)
        {
            byteBuf[i] = *charBuf++;
        }
        //LOG("startAlign = %ld, endAlign = %ld, wordBuf = %ld", startAlign, endAlign, wordBuf);
        //LOG("alignAddr=%ld, wordBuf=%ld, byteBuf=%c", alignAddr, wordBuf, *byteBuf);
        pret = ptrace(PTRACE_POKEDATA, pid, alignAddr, wordBuf);
        if (pret < 0)
        {
            LOG("Ptrace Poke data failed, errno=%ld", pret);
        }
    }
    return pret;
}

// Some architecture-specified code is applied in the function.
long readFromProcess(void *buf, long addr, size_t len, pid_t pid)
{
    long pret = 0;
    long startAddr, endAddr;
    char *charBuf = (char *) buf;

    if (len <= 0)
    {
        return len;
    }
    startAddr = addr & (~WORD_ALIGN);
    endAddr = (addr + len - 1) & (~WORD_ALIGN);
    //LOG("%lu %lu %lu", startAddr, endAddr, len);
    for (long alignAddr = startAddr; alignAddr <= endAddr; alignAddr += WORD_BYTES)
    {
        //LOG("%lu", alignAddr);
        long wordBuf;
        wordBuf = ptrace(PTRACE_PEEKDATA, pid, alignAddr, NULL);
        //LOG("alignAddr=%p, wordBuf=%ld", alignAddr, wordBuf);
        char *byteBuf = (char *) &wordBuf;
        long startAlign, endAlign;
        if (startAddr == endAddr)
        {
            // cut both head and tail remainder
            startAlign = addr & WORD_ALIGN;
            endAlign = (addr+len-1) & WORD_ALIGN;
        }
        else if (alignAddr == startAddr)
        {
            // cut head remainder
            startAlign = addr & WORD_ALIGN;
            endAlign = WORD_ALIGN;
        }
        else if (alignAddr == endAddr)
        {
            // cut tail remainder
            startAlign = 0;
            endAlign = (addr+len-1) & WORD_ALIGN;
        }
        else
        {
            // no need to cut remainder
            startAlign = 0;
            endAlign = WORD_ALIGN;
        }
        for (int i = startAlign; i <= endAlign; i++)
        {
            *charBuf++ = byteBuf[i];
        }
    }
    return pret;
}

Vector<String> convertCommand(int argc, char **argv)
{
    Vector<String> commands;
    for (int i = 0; i < argc; i++)
    {
        commands.push_back(String(argv[i]));
    }
    return commands;
}

// A local function to parse a string within a pair of quotations
int parseString(String &dst, String &src, size_t &pos)
{
    size_t startPos, endPos;
    startPos = src.find_first_of('"', pos);
    if (startPos == String::npos)
        return -1;
    ++startPos;
    endPos = src.find_first_of('"', startPos);
    while (endPos != String::npos && src[endPos-1] == '\\')
        endPos = src.find_first_of('"', endPos+1);
    if (endPos == String::npos)
        return -1;
    dst = src.substr(startPos, endPos - startPos);
    //LOG("%s %ld %ld", dst.c_str(), startPos, endPos - startPos);
    pos = endPos + 1;
    return 0;
}

int parseArgv(Vector<String> &command, String str)
{
    int ret;
    size_t pos;

    pos = str.find_first_of('[');
    if (pos == String::npos)
        return -1;
    String arg;
    while (parseString(arg, str, pos) == 0)
    {
        command.push_back(arg);
        size_t newPos = str.find_first_of(',', pos);
        if (newPos != String::npos)
            pos = newPos;
    }
    pos = str.find_first_of(']', pos);
    if (pos == String::npos)
    {
        return -1;
    }
    return 0;
}

String regsToStr(user_regs_struct &regs)
{
    std::stringstream ss;
    ss << "nr=" << regs.orig_rax
       << ", arg[0]=" << regs.rdi
       << ", arg[1]=" << regs.rsi
       << ", arg[2]=" << regs.rdx
       << ", arg[3]=" << regs.r10
       << ", arg[4]=" << regs.r9
       << ", arg[5]=" << regs.r8
       << ", ret=" << regs.rax;
    return ss.str();
}

int atoiE(const char *src, int base = 10, int begin = 0, int end = INT_MAX)
{
    int res = 0;
    for (int i = begin; i < end; ++i)
    {
        res *= base;
        if (src[i] >= '0' && src[i] <= '9')
            res += src[i] - '0';
        else if (src[i] >= 'A' && src[i] <= 'F')
            res += src[i] - 'A' + 10;
        else if (src[i] >= 'a' && src[i] <= 'f')
            res += src[i] - 'a' + 10;
        else
            break;
    }
    return res;
}

String bufToStr(const String &buf)
{
    String str = removeEscapeSequence(buf);
    //LOG("escaped str is: %s, after escape: %s", buf.c_str(), str.c_str());
    size_t spos, epos;
    spos = str.find('\"');
    if (spos == std::string::npos)
    {
        return str;
    }
    epos = str.rfind('\"');
    if (epos == std::string::npos)
    {
        return str;
    }
    assert(spos < epos);
    return str.substr(spos + 1, epos - spos - 1);
}

String readEscapeString(Istream &is)
{
}

String addEscapeSequence(const String &src)
{
    std::stringstream ss;
    for (int i = 0; i < src.size(); ++i)
    {
        char c = src[i];
        if (c == '\\' || c < 0x20)
        {
            ss << "\\x" << std::hex << int(c) << std::dec;
        }
        else
        {
            ss << c;
        }
    }
    return ss.str();
}

String removeEscapeSequence(const String &src)
{
    int i = 0, e = src.size();
    long aux;
    char c;
    std::stringstream ss;
    while (i < e)
    {
        if (src[i] == '\\')
        {
            if (i+1 == e)
            {
                LOG("Bad string: %s", src.c_str());
                break;
            }
            switch (src[i+1])
            {
                case '\"':
                    ss << '\"';
                    i += 2;
                    break;
                case '\'':
                    ss << '\'';
                    i += 2;
                    break;
                case '\\':
                    ss << '\\';
                    i += 2;
                    break;
                case '0':
                    ss << '\0';
                    i += 2;
                    break;
                case 'a':
                    ss << '\a';
                    i += 2;
                    break;
                case 'b':
                    ss << '\b';
                    i += 2;
                    break;
                case 'f':
                    ss << '\f';
                    i += 2;
                    break;
                case 'n':
                    ss << '\n';
                    i += 2;
                    break;
                case 'r':
                    ss << '\r';
                    i += 2;
                    break;
                case 't':
                    ss << '\t';
                    i += 2;
                    break;
                case 'v':
                    ss << '\v';
                    i += 2;
                    break;
                case 'x':
                case 'X':
                    if (i+4 > e)
                    {
                        LOG("Bad string: %s", src.c_str());
                        i += 4;
                        break;
                    }
                    aux = atoiE(src.c_str(), 16, i+2, i+4);
                    c = (char) aux;
                    ss << c;
                    i += 4;
                    break;
                default:
                    if (src[i+1] >= 0 && src[i+1] <= 7)
                    {
                        if (i+4 > e)
                        {
                            LOG("Bad string: %s", src.c_str());
                            i += 4;
                            break;
                        }
                        aux = atoiE(src.c_str(), 8, i+1, i+4);
                        c = (char) aux;
                        ss << c;
                        i += 4;
                        break;
                    }
                    LOG("Unknown escape sequence inside: %s", src.c_str());
                    break;
            }
        }
        else
        {
            ss << src[i++];
        }
    }
    return ss.str();
}

void printMsg(const String &msg, std::ostream &os /* = std::cout */)
{
    os << msg << std::endl;
}

void printMsg(const char *msg, std::ostream &os /* = std::cout */)
{
    os << msg << std::endl;
}

char retrieveChar(const char *alphabet /* = NULL */, const char *noticeMsg
        /* = NULL */, std::istream &is /* = std::cin */, std::ostream &os
        /* = std::cout */)
{
    char c;
    String str;
//    getline(is, str);
//    c = str[0];
    is.get(c);
    if (alphabet != NULL)
    {
        while (strchr(alphabet, c) == NULL)
        {
            if (noticeMsg != NULL)
            {
                printMsg(noticeMsg, os);
            }
            is.get(c);
//            getline(is, str);
//            c = str[0];
        }
    }
    return c;
}

