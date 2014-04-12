/*
 * common.c: 
 *
 * See the README file for copyright information and how to reach the author.
 *
 */

#include <sys/stat.h>

#ifdef USEUUID
# include <uuid/uuid.h>
#endif

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>
#include <zlib.h>

#ifdef USELIBARCHIVE
# include <archive.h>
# include <archive_entry.h>
#endif

#ifdef VDR_PLUGIN
# include <vdr/thread.h>
#endif

#include "common.h"
#include "config.h"

#ifdef VDR_PLUGIN
  cMutex logMutex;
#endif

//***************************************************************************
// Debug
//***************************************************************************

void tell(int eloquence, const char* format, ...)
{
   if (EPG2VDRConfig.loglevel < eloquence)
      return ;

   const int sizeBuffer = 100000;
   char t[sizeBuffer+100]; *t = 0;
   va_list ap;

#ifdef VDR_PLUGIN
   cMutexLock lock(&logMutex);
#endif

   va_start(ap, format);

#ifdef VDR_PLUGIN
   snprintf(t, sizeBuffer, "scraper2vdr: ");
#endif

   vsnprintf(t+strlen(t), sizeBuffer-strlen(t), format, ap);
   
   if (EPG2VDRConfig.logstdout)
   {
      char buf[50+TB];
      time_t now;
      time(&now);
      strftime(buf, 50, "%y.%m.%d %H:%M:%S", localtime(&now));
      printf("%s %s\n", buf, t);
   }
   else
      syslog(LOG_ERR, "%s", t);

   va_end(ap);
}

//***************************************************************************
// Host ID
//***************************************************************************

unsigned int getHostId()
{
   static unsigned int id = gethostid() & 0xFFFFFFFF;
   return id;
}

//***************************************************************************
// String Operations
//***************************************************************************

void toUpper(std::string& str)
{
   const char* s = str.c_str();
   int lenSrc = str.length();

   char* dest = (char*)malloc(lenSrc+TB); *dest = 0;
   char* d = dest;

   int csSrc;  // size of character

   for (int ps = 0; ps < lenSrc; ps += csSrc)
   {
      csSrc = max(mblen(&s[ps], lenSrc-ps), 1);
      
      if (csSrc == 1)
         *d++ = toupper(s[ps]);
      else if (csSrc == 2 && s[ps] == (char)0xc3 && s[ps+1] >= (char)0xa0)
      {
         *d++ = s[ps];
         *d++ = s[ps+1] - 32;
      }
      else
      {
         for (int i = 0; i < csSrc; i++)
            *d++ = s[ps+i];
      }
   }

   *d = 0;

   str = dest;
   free(dest);
}

void removeChars(std::string& str, const char* ignore)
{
   const char* s = str.c_str();
   int lenSrc = str.length();
   int lenIgn = strlen(ignore);

   char* dest = (char*)malloc(lenSrc+TB); *dest = 0;
   char* d = dest;

   int csSrc;  // size of character
   int csIgn;  // 

   for (int ps = 0; ps < lenSrc; ps += csSrc)
   {
      int skip = no;

      csSrc = max(mblen(&s[ps], lenSrc-ps), 1);

      for (int pi = 0; pi < lenIgn; pi += csIgn)
      {
         csIgn = max(mblen(&ignore[pi], lenIgn-pi), 1);

         if (csSrc == csIgn && strncmp(&s[ps], &ignore[pi], csSrc) == 0)
         {
            skip = yes;
            break;
         }
      }

      if (!skip)
      {
         for (int i = 0; i < csSrc; i++)
            *d++ = s[ps+i];
      }
   }

   *d = 0;

   str = dest;
   free(dest);
}

void removeCharsExcept(std::string& str, const char* except)
{
   const char* s = str.c_str();
   int lenSrc = str.length();
   int lenIgn = strlen(except);

   char* dest = (char*)malloc(lenSrc+TB); *dest = 0;
   char* d = dest;

   int csSrc;  // size of character
   int csIgn;  // 

   for (int ps = 0; ps < lenSrc; ps += csSrc)
   {
      int skip = yes;

      csSrc = max(mblen(&s[ps], lenSrc-ps), 1);

      for (int pi = 0; pi < lenIgn; pi += csIgn)
      {
         csIgn = max(mblen(&except[pi], lenIgn-pi), 1);

         if (csSrc == csIgn && strncmp(&s[ps], &except[pi], csSrc) == 0)
         {
            skip = no;
            break;
         }
      }

      if (!skip)
      {
         for (int i = 0; i < csSrc; i++)
            *d++ = s[ps+i];
      }
   }

   *d = 0;

   str = dest;
   free(dest);
}

void removeWord(std::string& pattern, std::string word)
{
   size_t  pos;

   if ((pos = pattern.find(word)) != std::string::npos)
      pattern.swap(pattern.erase(pos, word.length()));
}

//***************************************************************************
// String Manipulation
//***************************************************************************

void prepareCompressed(std::string& pattern)
{
   // const char* ignore = " (),.;:-_+*!#?=&%$<>§/'`´@~\"[]{}"; 
   const char* notignore = "ABCDEFGHIJKLMNOPQRSTUVWXYZßÖÄÜöäü0123456789"; 

   toUpper(pattern);
   removeWord(pattern, " TEIL ");
   removeWord(pattern, " FOLGE ");
   removeCharsExcept(pattern, notignore);
}

//***************************************************************************
// Left Trim
//***************************************************************************

char* lTrim(char* buf)
{
   if (buf)
   {
      char *tp = buf;

      while (*tp && strchr("\n\r\t ",*tp)) 
         tp++;

      memmove(buf, tp, strlen(tp) +1);
   }
   
   return buf;
}

//*************************************************************************
// Right Trim
//*************************************************************************

char* rTrim(char* buf)
{
   if (buf)
   {
      char *tp = buf + strlen(buf);

      while (tp >= buf && strchr("\n\r\t ",*tp)) 
         tp--;

      *(tp+1) = 0;
   }
   
   return buf;
}

//*************************************************************************
// All Trim
//*************************************************************************

char* allTrim(char* buf)
{
   return lTrim(rTrim(buf));
}

//***************************************************************************
// Number to String
//***************************************************************************

std::string num2Str(int num)
{
   char txt[16];

   snprintf(txt, sizeof(txt), "%d", num);

   return std::string(txt);
}

//***************************************************************************
// Long to Pretty Time
//***************************************************************************

std::string l2pTime(time_t t)
{
   char txt[30];
   tm* tmp = localtime(&t);
   
   strftime(txt, sizeof(txt), "%d.%m.%Y %T", tmp);
   
   return std::string(txt);
}

//***************************************************************************
// MS to Duration
//***************************************************************************

std::string ms2Dur(uint64_t t)
{
   char txt[30];
   
   int s = t / 1000;
   int ms = t % 1000;

   snprintf(txt, sizeof(txt), "%d.%03d seconds", s, ms);
   
   return std::string(txt);
}

//***************************************************************************
// Char to Char-String
//***************************************************************************

const char* c2s(char c, char* buf)
{
   sprintf(buf, "%c", c);

   return buf;
}

//***************************************************************************
// TOOLS
//***************************************************************************

int isEmpty(const char* str)
{
   return !str || !*str;
}

char* sstrcpy(char* dest, const char* src, int max)
{
   if (!dest || !src)
      return 0;

   strncpy(dest, src, max);
   dest[max-1] = 0;
   
   return dest;
}

int isLink(const char* path)
{
   struct stat sb;

   if (lstat(path, &sb) == 0)
      return S_ISLNK(sb.st_mode);

   tell(0, "Error: Detecting state for '%s' failed, error was '%m'", path);

   return false;
}

int fileSize(const char* path)
{
   struct stat sb;

   if (lstat(path, &sb) == 0)
      return sb.st_size;

   tell(0, "Error: Detecting state for '%s' failed, error was '%m'", path);

   return false;
}


int fileExists(const char* path)
{
   return access(path, F_OK) == 0; 
}

int createLink(const char* link, const char* dest, int force)
{
   if (!fileExists(link) || force)
   {
      // may be the link exists and point to a wrong or already deleted destination ...
      //   .. therefore we delete the link at first
      
      unlink(link);
      
      if (symlink(dest, link) != 0)
      {
         tell(0, "Failed to create symlink '%s', error was '%m'", link);
         return fail;
      }
   }

   return success;
}

//***************************************************************************
// Remove File
//***************************************************************************

int removeFile(const char* filename)
{
   int lnk = isLink(filename);

   if (unlink(filename) != 0)
   {
      tell(0, "Can't remove file '%s', '%m'", filename);
      
      return 1;
   }

   tell(3, "Removed %s '%s'", lnk ? "link" : "file", filename);
   
   return 0;
}

//***************************************************************************
// Check Dir
//***************************************************************************

int chkDir(const char* path)
{
   struct stat fs;
   
   if (stat(path, &fs) != 0 || !S_ISDIR(fs.st_mode))
   {
      tell(0, "Creating directory '%s'", path);
      
      if (mkdir(path, ACCESSPERMS) == -1)
      {
         tell(0, "Can't create directory '%m'");
         return fail;
      }
   }

   return success;
}

#ifdef USELIBXML

//***************************************************************************
// Load XSLT
//***************************************************************************

xsltStylesheetPtr loadXSLT(const char* name, const char* path, int utf8)
{
   xsltStylesheetPtr stylesheet;
   char* xsltfile;

   asprintf(&xsltfile, "%s/%s-%s.xsl", path, name, utf8 ? "utf-8" : "iso-8859-1");
   
   if ((stylesheet = xsltParseStylesheetFile((const xmlChar*)xsltfile)) == 0)
      tell(0, "Error: Can't load xsltfile %s", xsltfile);
   else
      tell(0, "Info: Stylesheet '%s' loaded", xsltfile);
      
   free(xsltfile);
   return stylesheet;
}
#endif

//***************************************************************************
// Gnu Unzip
//***************************************************************************

int gunzip(MemoryStruct* zippedData, MemoryStruct* unzippedData)
{
   const int growthStep = 1024;

   z_stream stream = {0,0,0,0,0,0,0,0,0,0,0,Z_NULL,Z_NULL,Z_NULL};
   unsigned int resultSize = 0;
   int res = 0;

   unzippedData->clear();

   // determining the size in this way is taken from the sources of the gzip utility.

   memcpy(&unzippedData->size, zippedData->memory + zippedData->size -4, 4); 
   unzippedData->memory = (char*)malloc(unzippedData->size);

   // zlib initialisation

   stream.avail_in  = zippedData->size;
   stream.next_in   = (Bytef*)zippedData->memory;
   stream.avail_out = unzippedData->size;
   stream.next_out  = (Bytef*)unzippedData->memory;

   // The '+ 32' tells zlib to process zlib&gzlib headers

   res = inflateInit2(&stream, MAX_WBITS + 32);

   if (res != Z_OK)
   {
      tellZipError(res, " during zlib initialisation", stream.msg);
      inflateEnd(&stream);
      return fail;
   }

   // skip the header

   res = inflate(&stream, Z_BLOCK);

   if (res != Z_OK)
   {
      tellZipError(res, " while skipping the header", stream.msg);
      inflateEnd(&stream);
      return fail;
   }

   while (res == Z_OK)
   {
      if (stream.avail_out == 0)
      {
         unzippedData->size += growthStep;
         unzippedData->memory = (char*)realloc(unzippedData->memory, unzippedData->size);

         // Set the stream pointers to the potentially changed buffer!

         stream.avail_out = resultSize - stream.total_out;
         stream.next_out  = (Bytef*)(unzippedData + stream.total_out);
      }

      res = inflate(&stream, Z_SYNC_FLUSH);
      resultSize = stream.total_out;
   }

   if (res != Z_STREAM_END)
   {
      tellZipError(res, " during inflating", stream.msg);
      inflateEnd(&stream);
      return fail;
   }

   unzippedData->size = resultSize;
   inflateEnd(&stream);

   return success;
}

//*************************************************************************
// tellZipError
//*************************************************************************

void tellZipError(int errorCode, const char* op, const char* msg)
{
   if (!op)  op  = "";
   if (!msg) msg = "None";

   switch (errorCode)
   {
      case Z_OK:           return;
      case Z_STREAM_END:   return;
      case Z_MEM_ERROR:    tell(0, "Error: Not enough memory to unzip file%s!\n", op); return;
      case Z_BUF_ERROR:    tell(0, "Error: Couldn't unzip data due to output buffer size problem%s!\n", op); return;
      case Z_DATA_ERROR:   tell(0, "Error: Zipped input data corrupted%s! Details: %s\n", op, msg); return;
      case Z_STREAM_ERROR: tell(0, "Error: Invalid stream structure%s. Details: %s\n", op, msg); return;
      default:             tell(0, "Error: Couldn't unzip data for unknown reason (%6d)%s!\n", errorCode, op); return;
   }
}

//*************************************************************************
// Host Data
//*************************************************************************

#include <sys/utsname.h>
#include <netdb.h>
#include <ifaddrs.h>

static struct utsname info;

const char* getHostName()
{
   // get info from kernel

   if (uname(&info) == -1)
      return "";

   return info.nodename;
}

const char* getFirstIp()
{
   struct ifaddrs *ifaddr, *ifa;
   static char host[NI_MAXHOST] = "";

   if (getifaddrs(&ifaddr) == -1) 
   {
      tell(0, "getifaddrs() failed");
      return "";
   }

   // walk through linked interface list

   for (ifa = ifaddr; ifa; ifa = ifa->ifa_next) 
   {
      if (!ifa->ifa_addr)
         continue;
      
      // For an AF_INET interfaces

      if (ifa->ifa_addr->sa_family == AF_INET) //  || ifa->ifa_addr->sa_family == AF_INET6) 
      {
         int res = getnameinfo(ifa->ifa_addr, 
                               (ifa->ifa_addr->sa_family == AF_INET) ? sizeof(struct sockaddr_in) :
                               sizeof(struct sockaddr_in6),
                               host, NI_MAXHOST, 0, 0, NI_NUMERICHOST);

         if (res)
         {
            tell(0, "getnameinfo() failed: %s", gai_strerror(res));
            return "";
         }

         // skip loopback interface

         if (strcmp(host, "127.0.0.1") == 0)
            continue;

         tell(5, "%-8s %-15s %s", ifa->ifa_name, host,
              ifa->ifa_addr->sa_family == AF_INET   ? " (AF_INET)" :
              ifa->ifa_addr->sa_family == AF_INET6  ? " (AF_INET6)" : "");
      }
   }

   freeifaddrs(ifaddr);

   return host;
}

#ifdef USELIBARCHIVE

//***************************************************************************
// unzip <file> and get data of first content which name matches <filter>
//***************************************************************************

int unzip(const char* file, const char* filter, char*& buffer, int& size, char* entryName)
{
   const int step = 1024*10;

   int bufSize = 0;
   int r;
   int res;

   struct archive_entry* entry;
   struct archive* a = archive_read_new();

   *entryName = 0;
   buffer = 0;
   size = 0;

   archive_read_support_filter_all(a);
   archive_read_support_format_all(a);

   r = archive_read_open_filename(a, file, 10204);

   if (r != ARCHIVE_OK)
   {
      tell(0, "Error: Open '%s' failed - %m", file);
      return 1;
   }

   while (archive_read_next_header(a, &entry) == ARCHIVE_OK) 
   {
      strcpy(entryName, archive_entry_pathname(entry));

      if (strstr(entryName, filter))
      {
         bufSize = step;
         buffer = (char*)malloc(bufSize+1);

         while ((res = archive_read_data(a, buffer+size, step)) > 0)
         {
            size += res;
            bufSize += step;

            buffer = (char*)realloc(buffer, bufSize+1);
         }
         
         buffer[size] = 0;

         break;
      }
   }

   r = archive_read_free(a);

   if (r != ARCHIVE_OK)
   {
      size = 0;
      free(buffer);      
      return fail;
   }

   return size > 0 ? success : fail;
}

#endif

//***************************************************************************
// Class LogDuration
//***************************************************************************

#ifdef VDR_PLUGIN

# include <vdr/plugin.h>

LogDuration::LogDuration(const char* aMessage, int aLogLevel)
{
   logLevel = aLogLevel;
   strcpy(message, aMessage);
   
   // at last !

   durationStart = cTimeMs::Now();
}

LogDuration::~LogDuration()
{
   tell(logLevel, "duration '%s' was (%dms)",
     message, cTimeMs::Now() - durationStart);
}

void LogDuration::show(const char* label)
{
   tell(logLevel, "elapsed '%s' at '%s' was (%dms)",
        message, label, cTimeMs::Now() - durationStart);
}

#endif

//***************************************************************************
// Get Unique ID
//***************************************************************************

#ifdef USEUUID
const char* getUniqueId()
{
   static char uuid[sizeUuid+TB] = "";

   uuid_t id;
   uuid_generate(id);
   uuid_unparse_upper(id, uuid);

   return uuid;
}
#endif // USEUUID

//***************************************************************************
// Create MD5
//***************************************************************************

#ifdef USEMD5

int createMd5(const char* buf, md5* md5)
{
   MD5_CTX c;
   unsigned char out[MD5_DIGEST_LENGTH];

   MD5_Init(&c);
   MD5_Update(&c, buf, strlen(buf));
   MD5_Final(out, &c);

   for (int n = 0; n < MD5_DIGEST_LENGTH; n++)
      sprintf(md5+2*n, "%02x", out[n]);

   md5[sizeMd5] = 0;

   return done;
}

int createMd5OfFile(const char* path, const char* name, md5* md5)
{
   FILE* f;
   char buffer[1000];
   int nread = 0;
   MD5_CTX c;
   unsigned char out[MD5_DIGEST_LENGTH];
   char* file = 0;

   asprintf(&file, "%s/%s", path, name);
   
   if (!(f = fopen(file, "r")))
   {
      tell(0, "Fatal: Can't access '%s'; %m", file);
      free(file);
      return fail;
   }

   free(file);

   MD5_Init(&c);   
   
   while ((nread = fread(buffer, 1, 1000, f)) > 0)
      MD5_Update(&c, buffer, nread);
   
   fclose(f);

   MD5_Final(out, &c);
   
   for (int n = 0; n < MD5_DIGEST_LENGTH; n++)
      sprintf(md5+2*n, "%02x", out[n]);

   md5[sizeMd5] = 0;

   return success;
}

#endif // USEMD5
