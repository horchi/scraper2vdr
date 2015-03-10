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
// long GetFileChangedTime(string filename); // get last changed timestamp of file (using st_mtime)

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

// calc used thumb size (depends on thumbHeight)
void CalcThumbSize(int originalWidth, int originalHeight, int thumbHeight, int& usedWidth, int& usedHeight);

// resize/stretch/crop image to given image size (maxdisortion -> 0 = zero disortion, 0.1 = 10% disortion...) if enabled
// create thumb if enabled
void HandleImage(string imagePath, int originalWidth, int originalHeight,
                 bool forceFixImageSize, int newWidth, int newHeight, float maxDistortion,
                 bool createThumb, string thumbPath, int thumbHeight);

// Get systemtime in ms (since unspecified starting point)
long GetTimems(void);

// Get time difference in ms between now and LastTime
int GetTimeDiffms(long LastTime);
