#include <sstream>
#include <vdr/recording.h>
#include "tools.h"
#include "lib/common.h"

#include "filedatemanager.h"

cFileDateManager::cFileDateManager(void) {
    curPath = "";
}

cFileDateManager::~cFileDateManager(void) {
    files.clear();
}

// add new entry to files map
void cFileDateManager::AddFileValue(string fileName, long lastUpdated, bool used) {
    sFileValue v;
    v.lastupdated = lastUpdated;
    v.used = used;
    // search for existing entry
    map<string, sFileValue>::iterator hit = files.find(fileName);
    if (hit != files.end())
            files.erase(hit); // delete existing entry for this filename
    files.insert(pair<string, sFileValue>(fileName, v));    
}

// Fill list with values from information file
bool cFileDateManager::LoadFileDateList(string path, bool used) {
    files.clear(); // clear old values
    curPath = path;
    
    // build filename
    stringstream fPath("");
    fPath << path << "/" << LASTUPDATEDFILENAME;
    string fileName = fPath.str();
    bool fExists = FileExists(fileName,false);
    if (!fExists) {
//        tell(1,"no %s found in %s",LASTUPDATEDFILENAME,path.c_str()); 
        return true; // file not available 
    }
    
    // file available, read all non empty lines
    vector<string> lastupdatedLines;
    FILE *f = fopen(fileName.c_str(), "r");
    if (!f) {
        tell(0, "failed to read %s", fileName.c_str());
        return false;
    }    
    cReadLine ReadLine;
    char *line;
    while ((line = ReadLine.Read(f)) != NULL) {
        lastupdatedLines.push_back(line);
    }
    fclose(f);

    string newFileName = "", newLastUpdated = "";
    int numLines = lastupdatedLines.size(); 
    for (int line=0; line < numLines; line++) {
        lastupdatedLines[line] = trim(lastupdatedLines[line]);
        
        // split at =
        splitstring s(lastupdatedLines[line].c_str());
        vector<string> flds = s.split('=');
        if (flds.size() == 2) {
            AddFileValue(trim(flds[0]),atoi(trim(flds[1]).c_str()),used); // use given value as default
        } else {
            tell(0, "invalid value in %s", fileName.c_str());
            return false;
        }
    }
    return true;
}

// Save list to information file in curPath
bool cFileDateManager::SaveFileDateList(void) {
    // build filename
    stringstream fPath("");
    fPath << curPath << "/" << LASTUPDATEDFILENAME;
    string fileName = fPath.str();
    
    FILE *f = fopen(fileName.c_str(), "w"); // overwrite current file
    if (!f) {
        tell(0, "failed to write to %s", fileName.c_str());
        return false;
    }
    
    for (map<string, sFileValue>::iterator it = files.begin(); it != files.end(); it++) {
        sFileValue v = it->second;
        if (v.used) {
            // only save used values to new file
            fPath.str("");
            fPath << it->first << " = " << v.lastupdated << "\n";
            fputs(fPath.str().c_str(),f);
        }     
    }    
    fclose(f);
    return true;
}

// result = true if image not exist in files, or new lastUpdated > saved lastUpdated
// function add filename to list if is new, try to read lastUpdated from filedate if available
bool cFileDateManager::CheckImageNeedRefresh(string fileName, long lastUpdated) {
    // build filename
    stringstream fPath("");
    fPath << curPath << "/" << fileName;
    string fullFileName = fPath.str();
    bool fExists = FileExists(fullFileName,true);
    long fileLastUpdated = 0;

    // search for existing entry
    map<string, sFileValue>::iterator hit = files.find(fileName);
    if (hit != files.end()) {
        // filename exists in list, check lastUpdated
        hit->second.used = true;
        return (!fExists) || (hit->second.lastupdated < lastUpdated);
    } else {
        // new file, check if we can read filedate of existing file     
        if (fExists)
           fileLastUpdated = fileModTime(fullFileName.c_str());
        AddFileValue(fileName,fileLastUpdated,true); // add with lastupdated from file (if available)
        return fileLastUpdated < lastUpdated;
    }    
}

// same as above, but check if thumbfile exist
bool cFileDateManager::CheckImageNeedRefreshThumb(string fileName, string thumbfilename, long lastUpdated) {
    // build thumb filename
    stringstream fPath("");
    fPath << curPath << "/" << thumbfilename;
    string fullFileName = fPath.str();
    bool fExists = FileExists(fullFileName,true);

    return (CheckImageNeedRefresh(fileName, lastUpdated) || (!fExists)); // thumb file not exist, or real file need update
}

// set lastUpdated for image (create new entry in list if is new)
void cFileDateManager::SetLastUpdated(string fileName, long lastUpdated) {
    // search for existing entry
    map<string, sFileValue>::iterator hit = files.find(fileName);
    if (hit != files.end()) {
        hit->second.used = true;
        hit->second.lastupdated = lastUpdated;
    } else {
        // new file, add to list
        AddFileValue(fileName,lastUpdated,true);
    }    
}

// delete image from list
void cFileDateManager::DeleteImage(string fileName) {
    map<string, sFileValue>::iterator hit = files.find(fileName);
    if (hit != files.end()) {
        files.erase(hit);
    }    
}
