#include <sys/stat.h>
#include <sys/types.h>
#include <sys/vfs.h>
#include <dirent.h>
#include <fstream>
#include <algorithm> 
#include <functional> 
#include <cctype>
#include <locale>
#include <Magick++.h>
#include <vdr/plugin.h>
#include "lib/common.h"
#include "tools.h"

using namespace std;
using namespace Magick;

bool CreateDirectory(string dir) {
    mkdir(dir.c_str(), 0775);
    //check if successfull
    DIR *pDir;
    bool exists = false;
    pDir = opendir(dir.c_str());
    if (pDir != NULL) {
        exists = true;    
        closedir(pDir);
    }
    return exists;
}

bool FileExists(string filename, bool isImage) {
    ifstream ifile(filename.c_str());
    if (ifile) {
        //a valid image should be larger then 500 bytes
        ifile.seekg (0, ifile.end);
        int length = ifile.tellg();
        int minimumLength = isImage ? 500 : 1;
        if (length > minimumLength)
            return true;
    }
    return false;
}

bool CheckDirExists(const char* dirName) {
    struct statfs statfsbuf;
    if (statfs(dirName,&statfsbuf)==-1) return false;
    if ((statfsbuf.f_type!=0x01021994) && (statfsbuf.f_type!=0x28cd3d45)) return false;
    if (access(dirName,R_OK|W_OK)==-1) return false;
    return true;

}

void DeleteFile(string filename) {
    remove(filename.c_str());
}

void DeleteDirectory(string dirname) {
    DIR *dir;
    struct dirent *entry;
    if ((dir = opendir (dirname.c_str())) != NULL) {
        while ((entry = readdir (dir)) != NULL) {
            string file = entry->d_name;
            if (!file.compare("."))
                continue;
            if (!file.compare(".."))
                continue;
            string delFile = dirname + "/" + file;
            DeleteFile(delFile);
        }
        closedir (dir);
    }
    rmdir(dirname.c_str());
}

string TwoFoldersHigher(string folder) {
    unsigned found = folder.find_last_of("/");
    if (found != string::npos) {
        string firstDirRemoved = folder.substr(0,found);
        unsigned found2 = firstDirRemoved.find_last_of("/");
        if (found2 != string::npos) {
            return firstDirRemoved.substr(0,found2);
        }
    }
    return "";
}


// trim from start
string &ltrim(string &s) {
    s.erase(s.begin(), find_if(s.begin(), s.end(), not1(ptr_fun<int, int>(isspace))));
    return s;
}

// trim from end
string &rtrim(string &s) {
    s.erase(find_if(s.rbegin(), s.rend(), not1(ptr_fun<int, int>(isspace))).base(), s.end());
    return s;
}

// trim from both ends
string &trim(string &s) {
    return ltrim(rtrim(s));
}

void toLower(string &s) {
    transform(s.begin(), s.end(), s.begin(), ::tolower);
}

bool isNumber(const string& s) {
    string::const_iterator it = s.begin();
    while (it != s.end() && isdigit(*it)) ++it;
    return !s.empty() && it == s.end();
}

string replaceString(string content, string search, string repl) {
    size_t pos = 0;
    while((pos = content.find(search, pos)) != std::string::npos) {
        if (pos > 3 && pos < content.size() - 2) {
            content.replace(pos, search.length(), repl);
        } else {
            content.replace(pos, search.length(), "");            
        }
        pos += repl.length();
    }
    return content;
}


/****************************************************************************************
*            SPLTSTRING
****************************************************************************************/
// split: receives a char delimiter; returns a vector of strings
// By default ignores repeated delimiters, unless argument rep == 1.
vector<string>& splitstring::split(char delim, int rep) {
    if (!flds.empty()) flds.clear();  // empty vector if necessary
    string work = data();
    string buf = "";
    int i = 0;
    while (i < work.length()) {
        if (work[i] != delim)
            buf += work[i];
        else if (rep == 1) {
            flds.push_back(buf);
            buf = "";
        } else if (buf.length() > 0) {
            flds.push_back(buf);
            buf = "";
        }
        i++;
    }
    if (!buf.empty())
        flds.push_back(buf);
    return flds;
}

void CreateThumbnail(string sourcePath, string destPath, int origWidth, int origHeight, int shrinkFactor) {
    if (sourcePath.size() < 5 || destPath.size() < 5 || shrinkFactor < 2)
        return;
    
    int thumbWidth = origWidth / shrinkFactor;
    int thumbHeight = origHeight / shrinkFactor;

    InitializeMagick(NULL);
    Image buffer;
    try {
        buffer.read(sourcePath.c_str());
        buffer.sample(Geometry(thumbWidth, thumbHeight)); 
        buffer.write(destPath.c_str());
    } catch( ... ) {}
}