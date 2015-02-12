/*
 * curlfuncs.h
 *
 * See the README file for copyright information and how to reach the author.
 *
 */

#ifndef __LIB_CURL__
#define __LIB_CURL__

#include <curl/curl.h>
#include <curl/easy.h>

#include <string>

#include "common.h"
#include "config.h"

using namespace std;

#define CURL_USERAGENT "Mozilla/4.0 (compatible; MSIE 6.0; Windows NT 5.0; Mayukh's libcurl wrapper http://www.mayukhbose.com/)"

//***************************************************************************
// CURL
//***************************************************************************

class cCurl
{
   public:

      cCurl();
      ~cCurl();

      int init(const char* httpproxy = "");
      int exit();

      static int create();
      static int destroy();

      int GetUrl(const char *url, string *sOutput, const string &sReferer="");
      int GetUrlFile(const char *url, const char *filename, const string &sReferer="");
      int SetCookieFile(char *filename);
      int PostUrl(const char *url, const string &sPost, string *sOutput, const string &sReferer = "");
      int PostRaw(const char *url, const string &sPost, string *sOutput, const string &sReferer = "");
      int DoPost(const char *url, string *sOutput, const string &sReferer,
                 struct curl_httppost *formpost, struct curl_slist *headerlist);

      char* EscapeUrl(const char *url);
      void Free(char* str);

      int downloadFile(const char* url, int& size, MemoryStruct* data, int timeout = 30, const char* userAgent = CURL_USERAGENT);

      // static stuff 

      static string sBuf;   // dirty

   protected:

      // data 

      CURL* handle;

      static size_t WriteMemoryCallback(void* ptr, size_t size, size_t nmemb, void* data);
      static size_t WriteHeaderCallback(void* ptr, size_t size, size_t nmemb, void* data);

      static int curlInitialized;
};

extern cCurl curl;

//***************************************************************************
#endif // __LIB_CURL__
