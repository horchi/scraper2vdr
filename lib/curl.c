/*
 * curl.c: 
 *
 * See the README file for copyright information and how to reach the author.
 *
 */

#include <curl/curl.h>

#include "common.h"
#include "config.h"

//***************************************************************************
// Callbacks
//***************************************************************************

static size_t WriteMemoryCallback(void* ptr, size_t size, size_t nmemb, void* data)
{
   size_t realsize = size * nmemb;
   struct MemoryStruct* mem = (struct MemoryStruct*)data;
   
   if (mem->memory)
      mem->memory = (char*)realloc(mem->memory, mem->size + realsize + 1);
   else
      mem->memory = (char*)malloc(mem->size + realsize + 1);
   
   if (mem->memory)
   {
      memcpy (&(mem->memory[mem->size]), ptr, realsize);
      mem->size += realsize;
      mem->memory[mem->size] = 0;
   }

   return realsize;
}

static size_t WriteHeaderCallback(void* ptr, size_t size, size_t nmemb, void* data)
{
   size_t realsize = size * nmemb;
   struct MemoryStruct* mem = (struct MemoryStruct*)data;
   char* p;

   if (ptr)
   {
      // get filename
      {
         // Content-Disposition: attachment; filename="20140103_20140103_de_qy.zip"

         const char* attribute = "Content-disposition: ";
         
         if ((p = strcasestr((char*)ptr, attribute)))
         {
            if (p = strcasestr(p, "filename="))
            {
               p += strlen("filename=");

               tell(4, "found filename at [%s]", p);

               if (*p == '"')
                  p++;

               sprintf(mem->name, "%s", p);
               
               if ((p = strchr(mem->name, '\n')))
                  *p = 0;
               
               if ((p = strchr(mem->name, '\r')))
                  *p = 0;
               
               if ((p = strchr(mem->name, '"')))
                  *p = 0;

               tell(4, "set name to '%s'", mem->name);
            }
         }
      }
      
      // since some sources update "ETag" an "Last-Modified:" without changing the contents
      //  we have to use "Content-Length:" to check for updates :(
      {
         const char* attribute = "Content-Length: ";
         
         if ((p = strcasestr((char*)ptr, attribute)))
         {
            sprintf(mem->tag, "%s", p+strlen(attribute));
            
            if ((p = strchr(mem->tag, '\n')))
               *p = 0;
            
            if ((p = strchr(mem->tag, '\r')))
               *p = 0;
            
            if ((p = strchr(mem->tag, '"')))
               *p = 0;
         }
      }
   }

   return realsize;
}

//***************************************************************************
// Download File
//***************************************************************************

int downloadFile(const char* url, int& size, MemoryStruct* data, int timeout, const char* userAgent)
{
   CURL* curl_handle;
   long code;
   CURLcode res = CURLE_OK;

   size = 0;

   // init curl

   if (curl_global_init(CURL_GLOBAL_ALL) != 0)
   {
      tell(0, "Error, something went wrong with curl_global_init()");
      
      return fail;
   }
   
   curl_handle = curl_easy_init();
   
   if (!curl_handle)
   {
      tell(0, "Error, unable to get handle from curl_easy_init()");
      
      return fail;
   }
  
   if (EPG2VDRConfig.useproxy)
   {
      curl_easy_setopt(curl_handle, CURLOPT_PROXYTYPE, CURLPROXY_HTTP);
      curl_easy_setopt(curl_handle, CURLOPT_PROXY, EPG2VDRConfig.httpproxy);   // Specify HTTP proxy
   }

   curl_easy_setopt(curl_handle, CURLOPT_URL, url);                            // Specify URL to get   
   curl_easy_setopt(curl_handle, CURLOPT_FOLLOWLOCATION, 0);                   // don't follow redirects
   curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);  // Send all data to this function
   curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void*)data);              // Pass our 'data' struct to the callback function
   curl_easy_setopt(curl_handle, CURLOPT_HEADERFUNCTION, WriteHeaderCallback); // Send header to this function
   curl_easy_setopt(curl_handle, CURLOPT_WRITEHEADER, (void*)data);            // Pass some header details to this struct
   curl_easy_setopt(curl_handle, CURLOPT_MAXFILESIZE, 100*1024*1024);          // Set maximum file size to get (bytes)
   curl_easy_setopt(curl_handle, CURLOPT_NOPROGRESS, 1);                       // No progress meter
   curl_easy_setopt(curl_handle, CURLOPT_NOSIGNAL, 1);                         // No signaling
   curl_easy_setopt(curl_handle, CURLOPT_TIMEOUT, timeout);                    // Set timeout
   curl_easy_setopt(curl_handle, CURLOPT_NOBODY, data->headerOnly ? 1 : 0);    // 
   curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, userAgent);                // Some servers don't like requests 
                                                                               // that are made without a user-agent field
   // perform http-get

   if ((res = curl_easy_perform(curl_handle)) != 0)
   {
      data->clear();

      tell(1, "Error, download failed; %s (%d)",
           curl_easy_strerror(res), res);
      curl_easy_cleanup(curl_handle);  // Cleanup curl stuff

      return fail;
   }

   curl_easy_getinfo(curl_handle, CURLINFO_HTTP_CODE, &code); 
   curl_easy_cleanup(curl_handle);     // cleanup curl stuff

   if (code == 404)
   {
      data->clear();
      return fail;
   }

   size = data->size;

   return success;
}
