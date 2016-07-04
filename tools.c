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
#include <vdr/videodir.h>
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
        int minimumLength = isImage ? 500 : 0; // allow empty "non-image" files
        if (length >= minimumLength)
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

// get last changed timestamp of file (using st_mtime)
// long GetFileChangedTime(string filename) { 
//     struct stat st;
//     if(stat(filename.c_str() , &st) < 0)
//       return 0; // default = 0 if file not exists
//     return st.st_mtime;
// }

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

string getRecPath(const cRecording *rec) 
{
    int pathOffset = 0;

    if (!rec)
       return "";
    
#if APIVERSNUM > 20103
    const char* videoBasePath = cVideoDirectory::Name();
#else
    const char* videoBasePath = VideoDirectory;
#endif
    
    if (strncmp(rec->FileName(), videoBasePath, strlen(videoBasePath)) == 0)
    {
       pathOffset = strlen(videoBasePath);
       
       if (*(rec->FileName()+pathOffset) == '/')
          pathOffset++;
    }

    string recPath = rec->FileName()+pathOffset;

    // gute Idee das Relevante hinten abzuschneiden?

    if (recPath.size() > 200)
        recPath = recPath.substr(0, 199);

    return recPath;
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
    unsigned int i = 0;
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

// calc used thumb size (depends on thumbHeight)
void CalcThumbSize(int originalWidth, int originalHeight, int thumbHeight, int& usedWidth, int& usedHeight) {
    if ((thumbHeight < originalHeight) && (originalHeight > 0)) {
        usedHeight = thumbHeight;
        float factor = float(usedHeight)/float(originalHeight);
        usedWidth = round(float(factor)*float(originalWidth));
    } else {
        usedHeight = originalHeight;
        usedWidth = originalWidth;
    }
}

// resize/stretch/crop image to given image size (maxdisortion -> 0 = zero disortion, 0.1 = 10% disortion...) if enabled
// create thumb if enabled
void HandleImage(string imagePath, int originalWidth, int originalHeight,
                 bool forceFixImageSize, int newWidth, int newHeight, float maxDistortion,
                 bool createThumb, string thumbPath, int thumbHeight) {
    if (imagePath.size() > 5) {
        bool handleResize = forceFixImageSize && (newWidth > 0) && (newHeight > 0);
        bool handleThumb = createThumb && (thumbPath.size() > 5) && (thumbHeight > 0);
        
        if (handleResize || handleThumb) {
            // have to do something    
            InitializeMagick(NULL);
            Image buffer;
            try {
                buffer.read(imagePath.c_str()); // read source image

                // read real original size of image, because values from db are incorrect!
                originalWidth = buffer.columns();
                originalHeight = buffer.rows();
                if ((originalWidth > 0) && (originalHeight > 0)) {
                    // only not empty images 
                    handleResize = handleResize && ((originalWidth != newWidth) || (originalHeight != newHeight));
                    
                    int thbSourceWidth = originalWidth;
                    int thbSourceHeight = originalHeight;
                
                    if (handleResize) {
                        // have to resize image
                        thbSourceWidth = newWidth; // use fixed size as thumb source
                        thbSourceHeight = newHeight;
                    
                        float tempX = float(newWidth) / float(originalWidth);
                        float tempY = float(newHeight) / float(originalHeight);
                        float scaleUsed = tempX; 
                        if (tempY > tempX)
                            scaleUsed = tempY; // force side with larger scale factor get desired size
           
                        int tempWidth = round(float(originalWidth) * scaleUsed); // calc new image size using current scale factor
                        int tempHeight = round(float(originalHeight) * scaleUsed);
               
                        // calc distortion factor
                        tempX = float(tempWidth) / float(newWidth);
                        tempY = float(tempHeight) / float(newHeight);
           
                        // limit distortion factor
                        if (tempX < 1 - maxDistortion)
                            tempX = 1 - maxDistortion;
                        if (tempX > 1 + maxDistortion)
                            tempX = 1 + maxDistortion;
                        if (tempY < 1 - maxDistortion)
                            tempY = 1 - maxDistortion;
                        if (tempY > 1 + maxDistortion)
                            tempY = 1 + maxDistortion;
               
                        // calc image size using current distortion factor
                        tempWidth = round(float(tempWidth) / tempX);
                        tempHeight = round(float(tempHeight) / tempY); 
           
                        tell(3,"scale image %s from %dx%d to %dx%d",imagePath.c_str(),originalWidth,originalHeight,newWidth,newHeight);
                        if ((tempWidth != originalWidth) || (tempHeight != originalHeight)) {
                            Geometry usedGeometry;
                            usedGeometry.width(tempWidth);
                            usedGeometry.height(tempHeight);
                            usedGeometry.aspect(true); // ignore aspect ratio
                            buffer.sample(Geometry(tempWidth, tempHeight)); 
                            // buffer.resize(usedGeometry); // strech/scale to new size using max distortion factors    
                        }    
                        if ((tempWidth != newWidth) || (tempHeight != newHeight))
                            buffer.crop(Geometry(newWidth, newHeight, (tempWidth - newWidth)/2, (tempHeight - newHeight)/2)); // crop to desired size
                        if ((int(buffer.columns()) != newWidth) || (int(buffer.rows()) != newHeight)) {
                            tell(3,"wrong result image size %ldx%ld",buffer.columns(),buffer.rows());
                        }    
                        buffer.write(imagePath.c_str()); // overwrite source image
                    }    
                
                    if (handleThumb) {
                        int thbWidth;
                        int thbHeight;
                        CalcThumbSize(thbSourceWidth, thbSourceHeight, thumbHeight, thbWidth, thbHeight);
                        buffer.sample(Geometry(thbWidth, thbHeight)); 
                        buffer.write(thumbPath.c_str());
                    }    
                }    
            } catch( ... ) {}
        }    
    }
}

// Get systemtime in ms (since unspecified starting point)
long GetTimems(void) {
    struct timespec CurTime;
    if (clock_gettime(CLOCK_MONOTONIC, &CurTime) != 0)  
        return 0;
    return ((CurTime.tv_sec * 1000) + (CurTime.tv_nsec / 1000000.0));
}

// Get time difference in ms between now and LastTime
int GetTimeDiffms(long LastTime) {
    return (GetTimems() - LastTime);
}  
