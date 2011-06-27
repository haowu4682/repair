// Author: Hao Wu <haowu@cs.utexas.edu.cn>

#include <sys/ptrace.h>

#include <common/util.h>

// Since PTRACE is quite low level, we need to use these functions to peek/poke data.
// Some architecture-specified code is applied in the function.
long writeToProcess(void *buf, long addr, size_t len, pid_t pid)
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
    for (int alignAddr = startAddr; alignAddr <= endAddr; alignAddr += WORD_BYTES)
    {
        long wordBuf;
        char *byteBuf = (char *) &wordBuf;
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
        pret = ptrace(PTRACE_POKEDATA, pid, alignAddr, byteBuf);
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
    for (int alignAddr = startAddr; alignAddr <= endAddr; alignAddr += WORD_BYTES)
    {
        long wordBuf;
        wordBuf = ptrace(PTRACE_PEEKDATA, pid, alignAddr, NULL);
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
