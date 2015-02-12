/*
 * common.h
 *
 * See the README file for copyright information and how to reach the author.
 *
 */

#ifndef __COMMON_H
#define __COMMON_H

#include <stdint.h>      // uint_64_t
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>
#include <errno.h>
#include <string>


#ifdef USEMD5
#  include <openssl/md5.h> // MD5_*
#endif

//#ifdef VDR_PLUGIN
//#  include <vdr/tools.h>
//#  include <vdr/thread.h>
//#endif

#ifdef USELIBXML
# include <libxslt/transform.h>
# include <libxslt/xsltutils.h>
# include <libexslt/exslt.h>
#endif

class MemoryStruct;

//***************************************************************************
// Misc
//***************************************************************************

// #ifndef VDR_PLUGIN
inline long min(long a, long b) { return a < b ? a : b; }
inline long max(long a, long b) { return a > b ? a : b; }
// #endif

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

#ifdef USEMD5
   sizeMd5 = 2 * MD5_DIGEST_LENGTH,
#endif
   sizeUuid = 36,

   tmeSecondsPerMinute = 60,
   tmeSecondsPerHour = tmeSecondsPerMinute * 60,
   tmeSecondsPerDay = 24 * tmeSecondsPerHour
};

enum Case
{
   cUpper,
   cLower
};

const char* toCase(Case cs, char* str);

//***************************************************************************
// Tell
//***************************************************************************

extern const char* logPrefix;

const char* getLogPrefix();
void __attribute__ ((format(printf, 2, 3))) tell(int eloquence, const char* format, ...);

//***************************************************************************
// 
//***************************************************************************

char* srealloc(void* ptr, size_t size);

//***************************************************************************
// Gun-Zip
//***************************************************************************

ulong gzipBound(ulong size);
int gzip(Bytef* dest, uLongf* destLen, const Bytef* source, uLong sourceLen);
void tellZipError(int errorCode, const char* op, const char* msg);
int gunzip(MemoryStruct* zippedData, MemoryStruct* unzippedData);

//***************************************************************************
// MemoryStruct
//***************************************************************************

struct MemoryStruct
{
   public:

      MemoryStruct()   { expireAt = 0; memory = 0; zmemory = 0; clear(); }
      MemoryStruct(const MemoryStruct* o)
      {
         size = o->size;
         memory = (char*)malloc(size);
         memcpy(memory, o->memory, size);

         zsize = o->zsize;
         zmemory = (char*)malloc(zsize);
         memcpy(zmemory, o->zmemory, zsize);

         copyAttributes(o);
      }
      
      ~MemoryStruct()  { clear(); }

      int isEmpty()  { return memory == 0; }
      int isZipped() { return zmemory != 0 && zsize > 0; }

      int append(const char* buf, int len)
      {
         memory = srealloc(memory, size+len);
         memcpy(memory+size, buf, len);
         size += len;

         return success;
      }

      void copyAttributes(const MemoryStruct* o)
      {
         strcpy(tag, o->tag);
         strcpy(name, o->name);
         strcpy(contentType, o->contentType);
         strcpy(contentEncoding, o->contentEncoding);
         strcpy(mimeType, o->mimeType);
         headerOnly = o->headerOnly;
         modTime = o->modTime;
         expireAt = o->expireAt;
      }

      int toGzip()
      {
         free(zmemory);
         zsize = 0;

         if (isEmpty())
            return fail;

         zsize = gzipBound(size) + 512;  // the maximum calculated by the lib, will adusted at gzip() call
         zmemory = (char*)malloc(zsize);
         
         if (gzip((Bytef*)zmemory, &zsize, (Bytef*)memory, size) != success)
         {
            free(zmemory);
            zsize = 0;
            tell(0, "Error gzip failed!");

            return fail;
         }
         
         sprintf(contentEncoding, "gzip");
         
         return success;
      }
      
      void clear() 
      {
         free(memory);
         memory = 0;
         size = 0;
         free(zmemory);
         zmemory = 0;
         zsize = 0;
         *tag = 0;
         *name = 0;
         *contentType = 0;
         *contentEncoding = 0;
         *mimeType = 0;
         modTime = time(0);
         headerOnly = no;
         // expireAt = time(0); -> don't reset 'expireAt' here !!!!
      }

      // data
      
      char* memory;
      long unsigned int size;

      char* zmemory;
      long unsigned int zsize;
      
      // tag attribute
      
      char tag[100+TB];              // the tag to be compared 
      char name[100+TB];             // content name (filename)
      char contentType[100+TB];      // e.g. text/html
      char mimeType[100+TB];         // 
      char contentEncoding[100+TB];  // 
      int headerOnly;
      time_t modTime;
      time_t expireAt;
};

//***************************************************************************
// Tools
//***************************************************************************

double usNow();
unsigned int getHostId();
const char* getHostName();
const char* getFirstIp();
const char* getIpOf(const char* device);

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
time_t timeOf(time_t t);
int weekdayOf(time_t t);
const char* toWeekdayName(uint day);
time_t hhmmOf(time_t t);
int l2hhmm(time_t t);
time_t midnightOf(time_t t);
std::string l2pTime(time_t t);
std::string l2pDate(time_t t);
std::string ms2Dur(uint64_t t);
const char* c2s(char c, char* buf);
char* eos(char* s);
int urlUnescape(char* dst, const char* src, int normalize = yes);

int storeToFile(const char* filename, const char* data, int size);
int loadFromFile(const char* infile, MemoryStruct* data);

int fileExists(const char* path);
int fileSize(const char* path);
time_t fileModTime(const char* path);
int createLink(const char* link, const char* dest, int force);
int isLink(const char* path);
const char*  suffixOf(const char* path);
int isEmpty(const char* str);
const char* notNull(const char* str);
int isZero(const char* str);
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

#ifdef USELIBARCHIVE
int unzip(const char* file, const char* filter, char*& buffer, 
          int& size, char* entryName);
#endif

//***************************************************************************
// cMyMutex
//***************************************************************************

class cMyMutex 
{
      friend class cCondVar;
   private:
      pthread_mutex_t mutex;
      int locked;
   public:
      cMyMutex(void);
      ~cMyMutex();
      void Lock(void);
      void Unlock(void);
};

//***************************************************************************
// cMyTimeMs
//***************************************************************************

class cMyTimeMs 
{
   private:

      uint64_t begin;

   public:

      cMyTimeMs(int Ms = 0);
      static uint64_t Now(void);
      void Set(int Ms = 0);
      bool TimedOut(void);
      uint64_t Elapsed(void);
};

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

//***************************************************************************
// Semaphore
//***************************************************************************

#include <sys/sem.h>

class Sem
{
   public:

      Sem(key_t aKey)
      {
         locked = no;
         key = aKey;

         if ((id = semget(key, 1, 0666 | IPC_CREAT)) == -1)
            tell(0, "Error: Can't get semaphore, errno (%d) '%s'", 
                 errno, strerror(errno));
      }

      ~Sem() 
      { 
         if (locked)
            v();
      }

      // ----------------------
      // get lock

      int p()
      {
         sembuf sops[2];
         
         sops[0].sem_num = 0;
         sops[0].sem_op = 0;                        // wait for lock
         sops[0].sem_flg = SEM_UNDO;
         
         sops[1].sem_num = 0;
         sops[1].sem_op = 1;                        // increment 
         sops[1].sem_flg = SEM_UNDO | IPC_NOWAIT;
         
         if (semop(id, sops, 2) == -1)
         {
            tell(0, "Error: Can't lock semaphore, errno (%d) '%s'", 
                 errno, strerror(errno));
            
            return fail;
         }

         locked = yes;

         return success;
      }

      // ----------------------
      // increment

      int inc()
      {
         sembuf sops[1];
         
         sops[0].sem_num = 0;
         sops[0].sem_op = 1;                        // increment 
         sops[0].sem_flg = SEM_UNDO | IPC_NOWAIT;
         
         if (semop(id, sops, 1) == -1)
         {
            if (errno != EAGAIN)
               tell(0, "Error: Can't lock semaphore, errno was (%d) '%s'", 
                    errno, strerror(errno));
            
            return fail;
         }
         
         locked = yes;

         return success;
      }

      // ----------------------
      // decrement

      int dec()
      {
         return v();
      }

      // ----------------------
      // check

      int check()
      {
         sembuf sops[1];
         
         sops[0].sem_num = 0;
         sops[0].sem_op = 0; 
         sops[0].sem_flg = SEM_UNDO | IPC_NOWAIT;
         
         if (semop(id, sops, 1) == -1)
         {
            if (errno != EAGAIN)
               tell(0, "Error: Can't lock semaphore, errno was (%d) '%s'", 
                    errno, strerror(errno));
            
            return fail;
         }
         
         return success;
      }

      // ----------------------
      // release lock

      int v()
      {
         sembuf sops;
         
         sops.sem_num = 0;
         sops.sem_op = -1;                          // release control
         sops.sem_flg = SEM_UNDO | IPC_NOWAIT;
         
         if (semop(id, &sops, 1) == -1)
         {
            if (errno != EAGAIN)
               tell(0, "Error: Can't unlock semaphore, errno (%d) '%s'",
                    errno, strerror(errno));
            
            return fail;
         }
         
         locked = no;

         return success;
      }
      
   private:

      key_t key;
      int id;
      int locked;
};

//***************************************************************************
#endif //___COMMON_H
