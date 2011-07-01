//Author: Hao Wu <haowu@cs.utexas.edu>

#include <sstream>

#include <replay/FDManager.h>
using namespace std;

int FDManager::addOld(int fd, String path)
{
    oldFDMap.insert(valueType(fd, path));
}

int FDManager::addNew(int fd, String path)
{
    newFDMap.insert(valueType(fd, path));
    //LOG("%d=%s", fd, newFDMap[fd].c_str());
}

int FDManager::removeOld(int fd)
{
    oldFDMap.erase(fd);
}

int FDManager::removeNew(int fd)
{
    newFDMap.erase(fd);
}

String FDManager::searchOld(int fd)
{
    String str;
    mapType::iterator it;
    it = oldFDMap.find(fd);
    if (it != oldFDMap.end())
    {
        str = it->second;
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
        str = it->second;
    }
    return str;
}

bool FDManager::equals(int oldFd, int newFd)
{
    String oldPath = searchOld(oldFd);
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
        sout << it->first << "\t" << it->second << endl;
    }
    sout << "New FDs" << endl;
    for (mapType::iterator it = newFDMap.begin(); it != newFDMap.end(); ++it)
    {
        sout << it->first << "\t" << it->second << endl;
    }
    return sout.str();
}

