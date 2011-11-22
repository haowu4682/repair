// Author: Hao Wu <haowu@cs.utexas.edu.cn>

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
        LOG("alignAddr=%ld, wordBuf=%ld", alignAddr, wordBuf);
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

