/*
 * configuration.h
 *
 * See the README file for copyright information and how to reach the author.
 *
 */

#ifndef __CONFIG_H
#define __CONFIG_H

extern const char* confDir;

//***************************************************************************
// Configuration (#TODO move to lib)
//***************************************************************************

class Configuration
{
   public:

      Configuration() {};
      virtual ~Configuration() {};

      virtual int atConfigItem(const char* Name, const char* Value) = 0;

      virtual int readConfig()
      {
         int count = 0;
         FILE* f;
         char* line = 0;
         size_t size = 0;
         char* value;
         char* name;
         char* fileName;
         
         asprintf(&fileName, "%s/epgd.conf", confDir);
         
         if (access(fileName, F_OK) != 0)
         {
            fprintf(stderr, "Cannot access configuration file '%s'\n", fileName);
            free(fileName);
            return fail;
         }
         
         f = fopen(fileName, "r");
         
         while (getline(&line, &size, f) > 0)
         {
            char* p = strchr(line, '#');
            if (p) *p = 0;
            
            allTrim(line);
            
            if (isEmpty(line))
               continue;
            
            if (!(value = strchr(line, '=')))
               continue;
            
            *value = 0;
            value++;
            lTrim(value);
            name = line;
            allTrim(name);
            
            if (atConfigItem(name, value) != success)
            {
               fprintf(stderr, "Found unexpected parameter '%s', aborting\n", name);
               free(fileName);
               return fail;
            }
            
            count++;
         }
         
         free(line);
         fclose(f);
         
         tell(0, "Read %d option from %s", count , fileName);
         
         free(fileName);
         
         return success;
      }
};

//***************************************************************************

#endif //  __CONFIG_H
