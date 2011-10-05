//Author: Hao Wu <haowu@cs.utexas.edu>

#include <sstream>

#include <replay/FDManager.h>
using namespace std;

File FDManager::standardFiles[] = {
        File::STDIN,
        File::STDOUT,
        File::STDERR
};
unsigned FDManager::standardFilesSize = 3;

void FDManager::init()
{
    for (unsigned i = 0; i < standardFilesSize; i++)
    {
        addOldFile(&standardFiles[i], 0);
        addNewFile(&standardFiles[i], 0);
    }
}

int FDManager::addOldFile(File *file, long seqNum)
{
    int fd = file->getFD();
    oldFDMap[fd].push_back(FDItem(seqNum, file));
}

int FDManager::addNewFile(File *file, long seqNum)
{
    int fd = file->getFD();
    newFDMap[fd].push_back(FDItem(seqNum, file));
}

/*
int FDManager::removeOldFile(int fd)
{
    oldFDMap.erase(fd);
}

int FDManager::removeNew(int fd)
{
    newFDMap.erase(fd);
}
*/

File *FDManager::searchOld(int fd, long seqNum) const
{
    File *str = NULL;
    mapType::const_iterator it;
    it = oldFDMap.find(fd);
    if (it != oldFDMap.end())
    {
        for (Vector<FDItem>::const_iterator jt = it->second.begin(); jt != it->second.end(); ++jt)
        {
            if (jt->seqNum < seqNum)
            {
                str = jt->file;
            }
            else
            {
                break;
            }
        }
    }
    return str;
}

File *FDManager::searchNew(int fd) const
{
    File *str = NULL;
    mapType::const_iterator it;
    it = newFDMap.find(fd);
    if (it != newFDMap.end())
    {
        str = it->second.back().file;
    }
    return str;
}

int FDManager::oldToNew(int oldFd, long seqNum) const
{
    File *file = searchOld(oldFd, seqNum);
    if (file != NULL)
    {
        for (mapType::const_iterator it = newFDMap.begin(); it != newFDMap.end(); ++it)
        {
            // XXX: Use the most recent one now. It may cause a problem if it is called
            //      while the most recent one has expired.
            if (it->second.back().file == file)
                return it->first;
        }
    }
    return -1;
}

// XXX: temporarily invalid
/*
int FDManager::newToOld(int newFd, long seqNum)
{
    String file = searchNew(newFd);
    if (!file.empty())
    {
        for (mapType::iterator it = oldFDMap.begin(); it != oldFDMap.end(); ++it)
        {
            if (it->second == file)
                return it->first;
        }
    }
    return newFd;
}
*/

bool FDManager::equals(int oldFd, int newFd, long seqNum) const
{
    File *oldPath = searchOld(oldFd, seqNum);
    File *newPath = searchNew(newFd);
    if (oldPath == NULL)
        return false;
    if (newPath == NULL)
        return false;
    if (oldPath != newPath)
        return false;
    return true;
}

String FDManager::toString() const
{
    ostringstream sout;
    sout << "Old FDs" << endl;
    for (mapType::const_iterator it = oldFDMap.begin(); it != oldFDMap.end(); ++it)
    {
        for (Vector<FDItem>::const_iterator jt = it->second.begin(); jt != it->second.end(); ++jt)
        {
            sout << it->first << "\t" << jt->seqNum << "\t" << jt->file << endl;
        }
    }
    sout << "New FDs" << endl;
    for (mapType::const_iterator it = newFDMap.begin(); it != newFDMap.end(); ++it)
    {
        for (Vector<FDItem>::const_iterator jt = it->second.begin(); jt != it->second.end(); ++jt)
        {
            sout << it->first << "\t" << jt->seqNum << "\t" << jt->file << endl;
        }
    }
    return sout.str();
}

void FDManager::clone(FDManager *another)
{
    for (mapType::iterator it = another->oldFDMap.begin(); it != another->oldFDMap.end(); ++it)
    {
        oldFDMap.insert(*it);
    }
    for (mapType::iterator it = another->newFDMap.begin(); it != another->newFDMap.end(); ++it)
    {
        newFDMap.insert(*it);
    }
}

