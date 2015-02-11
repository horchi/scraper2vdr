#include <vector>
#include <string>
#include <time.h>

using namespace std;

//Filesystem Functions
bool CreateDirectory(string dir);
bool FileExists(string filename, bool isImage = true); 
bool CheckDirExists(const char* dirName);
void DeleteFile(string filename);
void DeleteDirectory(string dirname);
string TwoFoldersHigher(string folder);
long GetFileChangedTime(string filename); // get last changed timestamp of file (using st_mtime)

//String Functions
string &ltrim(string &s);
string &rtrim(string &s);
string &trim(string &s);
void toLower(string &s);
bool isNumber(const string& s);
string replaceString(string content, string search, string repl);

string getRecPath(const cRecording *rec);

class splitstring : public string {
    vector<string> flds;
public:
    splitstring(const char *s) : string(s) { };
    vector<string>& split(char delim, int rep=0);
};

//Image Functions
void CreateThumbnail(string sourcePath, string destPath, int origWidth, int origHeight, int shrinkFactor);

// create thumbnail using fix thumbnail height
void CreateThumbnailFixHeight(string sourcePath, string destPath, int origWidth, int origHeight, int thumbHeight);

// Get systemtime in ms (since unspecified starting point)
long GetTimems(void);

// Get time difference in ms between now and LastTime
int GetTimeDiffms(long LastTime);