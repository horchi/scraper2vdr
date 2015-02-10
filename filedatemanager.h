#ifndef __FILEDATEMANAGER_H
#define __FILEDATEMANAGER_H

#include <map>

#define LASTUPDATEDFILENAME "files_lastupdated.txt"

struct sFileValue {
    long lastupdated; // what is the last update timestamp of this file
    bool used; // only save used files into file
};

class cFileDateManager  {
    private:
        map<string, sFileValue> files;
        map<string, sFileValue>::iterator filesIterator;
        string curPath; // currently used path
        void AddFileValue(string fileName, long lastUpdated, bool used); // add new entry to files map
    public:
        cFileDateManager(void);
        virtual ~cFileDateManager(void);
        bool LoadFileDateList(string path, bool used); // Fill list with values from information file
        bool SaveFileDateList(void); // Save list to information file in curPath
        bool CheckImageNeedRefresh(string fileName, long lastUpdated); // result = true if image file not exist, or new lastUpdated > saved lastUpdated (function add filename to list if is new, try to read lastUpdated from filedate if available) 
        bool CheckImageNeedRefreshThumb(string fileName, string thumbfilename, long lastUpdated); // same as above, but check if thumbfile exist
        void SetLastUpdated(string fileName, long lastUpdated); // set lastUpdated for image (create new entry in list if is new)
        void DeleteImage(string fileName); // delete image from list
};

#endif //__FILEDATEMANAGER_H
