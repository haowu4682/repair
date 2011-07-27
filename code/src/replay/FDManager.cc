//Author: Hao Wu <haowu@cs.utexas.edu>

#include <sstream>

#include <replay/FDManager.h>
using namespace std;

int FDManager::addOld(int fd, String path, long seqNum)
{
    //oldFDMap.insert(valueType(fd, path));
    oldFDMap[fd].push_back(FDItem(seqNum, path));
}

int FDManager::addNew(int fd, String path, long seqNum)
{
    //newFDMap.insert(valueType(fd, path));
    newFDMap[fd].push_back(FDItem(seqNum, path));
}

int FDManager::removeOld(int fd)
{
    oldFDMap.erase(fd);
}

int FDManager::removeNew(int fd)
{
    newFDMap.erase(fd);
}

String FDManager::searchOld(int fd, long seqNum)
{
    String str;
    mapType::iterator it;
    it = oldFDMap.find(fd);
    if (it != oldFDMap.end())
    {
        for (Vector<FDItem>::iterator jt = it->second.begin(); jt != it->second.end(); ++jt)
        {
            if (jt->seqNum < seqNum)
            {
                str = jt->path;
            }
            else
            {
                break;
            }
        }
    }
    return str;
}

String FDManager::searchNew(int fd)
{
    String str;
    mapType::iterator it;
    it = newFDMap.find(fd);
    if (it != newFDMap.end())
    {
        str = it->second.back().path;
    }
    return str;
}

int FDManager::oldToNew(int oldFd, long seqNum)
{
    String path = searchOld(oldFd, seqNum);
    LOG1(path.c_str());
    if (!path.empty())
    {
        for (mapType::iterator it = newFDMap.begin(); it != newFDMap.end(); ++it)
        {
            // XXX: Use the most recent one now. It may cause a problem if it is called
            //      while the most recent one has expired.
            if (it->second.back().path == path)
                return it->first;
        }
    }
    return oldFd;
}

// XXX: temporarily invalid
/*
int FDManager::newToOld(int newFd, long seqNum)
{
    String path = searchNew(newFd);
    if (!path.empty())
    {
        for (mapType::iterator it = oldFDMap.begin(); it != oldFDMap.end(); ++it)
        {
            if (it->second == path)
                return it->first;
        }
    }
    return newFd;
}
*/

bool FDManager::equals(int oldFd, int newFd, long seqNum)
{
    String oldPath = searchOld(oldFd, seqNum);
    String newPath = searchNew(newFd);
    if (oldPath.empty())
        return false;
    if (newPath.empty())
        return false;
    if (oldPath != newPath)
        return false;
    return true;
}

String FDManager::toString()
{
    ostringstream sout;
    sout << "Old FDs" << endl;
    for (mapType::iterator it = oldFDMap.begin(); it != oldFDMap.end(); ++it)
    {
        for (Vector<FDItem>::iterator jt = it->second.begin(); jt != it->second.end(); ++jt)
        {
            sout << it->first << "\t" << jt->seqNum << "\t" << jt->path << endl;
        }
    }
    sout << "New FDs" << endl;
    for (mapType::iterator it = newFDMap.begin(); it != newFDMap.end(); ++it)
    {
        for (Vector<FDItem>::iterator jt = it->second.begin(); jt != it->second.end(); ++jt)
        {
            sout << it->first << "\t" << jt->seqNum << "\t" << jt->path << endl;
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

