/*
 * common.h: EPG2VDR plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 */

#ifndef __COMMON_H
#define __COMMON_H

#include <stdint.h>      // uint_64_t
#include <stdlib.h>
#include <string>

#include <openssl/md5.h> // MD5_*

#ifdef VDR_PLUGIN
#  include <vdr/tools.h>
#endif

#ifdef USELIBXML
# include <libxslt/transform.h>
# include <libxslt/xsltutils.h>
# include <libexslt/exslt.h>
#endif

//***************************************************************************
// 
//***************************************************************************

#ifndef VDR_PLUGIN
  inline long min(long a, long b) { return a < b ? a : b; }
  inline long max(long a, long b) { return a > b ? a : b; }
#endif

enum Misc
{
   success  = 0,
   done     = success,
   fail     = -1,
   na       = -1,
   ignore   = -2,
   all      = -3,
   abrt     = -4,
   yes      = 1,
   on       = 1,
   off      = 0,
   no       = 0,
   TB       = 1,

   sizeMd5 = 2 * MD5_DIGEST_LENGTH,
   sizeUuid = 36,

   tmeSecondsPerMinute = 60,
   tmeSecondsPerHour = tmeSecondsPerMinute * 60,
   tmeSecondsPerDay = 24 * tmeSecondsPerHour
};

//***************************************************************************
// Tell
//***************************************************************************

void tell(int eloquence, const char* format, ...);

//***************************************************************************
// MemoryStruct for curl callbacks
//***************************************************************************

struct MemoryStruct
{
   MemoryStruct()   { memory = 0; clear(); }
   ~MemoryStruct()  { clear(); }

   // data

   char* memory;
   size_t size;

   // tag attribute

   char tag[100];           // the tag to be compared 
   char name[100];          // content name (filename)
   int headerOnly;

   int isEmpty() { return memory == 0; }

   void clear() 
   {
      free(memory);
      memory = 0;
      size = 0;
      *tag = 0;
      *name = 0;
      headerOnly = no;
   }
};

//***************************************************************************
// Tools
//***************************************************************************

unsigned int getHostId();
const char* getHostName();
const char* getFirstIp();

#ifdef USEUUID
  const char* getUniqueId();
#endif

void removeChars(std::string& str, const char* ignore);
void removeCharsExcept(std::string& str, const char* except);
void removeWord(std::string& pattern, std::string word);
void prepareCompressed(std::string& pattern);

char* rTrim(char* buf);
char* lTrim(char* buf);
char* allTrim(char* buf);
char* sstrcpy(char* dest, const char* src, int max);
std::string num2Str(int num);
std::string l2pTime(time_t t);
std::string ms2Dur(uint64_t t);
const char* c2s(char c, char* buf);

int fileExists(const char* path);
int fileSize(const char* path);
int createLink(const char* link, const char* dest, int force);
int isLink(const char* path);
int isEmpty(const char* str);
int removeFile(const char* filename);
int chkDir(const char* path);

#ifdef USELIBXML
  xsltStylesheetPtr loadXSLT(const char* name, const char* path, int utf8);
#endif

#ifdef USEMD5
  typedef char md5Buf[sizeMd5+TB];
  typedef char md5;
  int createMd5(const char* buf, md5* md5);
  int createMd5OfFile(const char* path, const char* name, md5* md5);
#endif

//***************************************************************************
// Zip
//***************************************************************************

int gunzip(MemoryStruct* zippedData, MemoryStruct* unzippedData);
void tellZipError(int errorCode, const char* op, const char* msg);

#ifdef USELIBARCHIVE
int unzip(const char* file, const char* filter, char*& buffer, 
          int& size, char* entryName);
#endif

#ifdef VDR_PLUGIN

//***************************************************************************
// Log Duration
//***************************************************************************

class LogDuration
{
   public:

      LogDuration(const char* aMessage, int aLogLevel = 2);
      ~LogDuration();

      void show(const char* label = "");

   protected:

      char message[1000];
      uint64_t durationStart;
      int logLevel;
};
#endif

//***************************************************************************
#endif //___COMMON_H
