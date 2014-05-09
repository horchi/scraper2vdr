/*
 * dbdict.c
 *
 * See the README file for copyright information and how to reach the author.
 *
 */

#include "common.h"
#include "dbdict.h"

//***************************************************************************
// cDbDict
//***************************************************************************

cDbDict::cDbDict()
{
   inside = no;
}

cDbDict::~cDbDict()
{
}

//***************************************************************************
// In
//***************************************************************************

int cDbDict::in(const char* file)
{
   FILE* f;
   char* line = 0;
   size_t size = 0;
   char* path;

   asprintf(&path, "%s", file);

   f = fopen(path, "r");
   
   while (getline(&line, &size, f) > 0)
   {
      char* p = strstr(line, "//");

      if (p) *p = 0;
      
      allTrim(line);
      
      if (isEmpty(line))
         continue;
      
      if (atLine(line) != success)
      {
         tell(0, "Found unexpected definition '%s', aborting", line);
         free(path);
         return fail;
      }
   }

   fclose(f);   
   free(line);
   free(path);
   
   return success;
}

//***************************************************************************
// At Line
//***************************************************************************

int cDbDict::atLine(const char* line)
{
   const char* p;

   if (p = strcasestr(line, "Table"))
   {
      char tableName[100];

      p += strlen("Table");
      strcpy(tableName, p);
      tell(0, "Table: '%s'", tableName);
   }

   else if (strchr(line, '{'))
      inside = yes;

   else if (strchr(line, '}'))
      inside = no;

   else if (inside)
      parseField(line);

   return success;
}

//***************************************************************************
// Get Token
//***************************************************************************

int getToken(const char*& p, char* token, int size)
{
   char* dest = token;
   int num = 0;

   while (*p && *p == ' ')
      p++;

   while (*p && *p != ' ' && num < size)
   {
      if (*p == '"')
         p++;
      else
      {
         *dest++ = *p++;
         num++;
      }
   }

   *dest = 0;

   return success;
}

//***************************************************************************
// Parse Field
//***************************************************************************

int cDbDict::parseField(const char* line)
{
   const int sizeTokenMax = 100;
   FieldDef f;
   char token[sizeTokenMax+TB];
   const char* p = line;

   // tell(0, "Got: '%s'", p);

   for (int i = 0; i < dtCount; i++)
   {
      if (getToken(p, token, sizeTokenMax) != success)
      {
         tell(0, "Errot: can't parse line [%s]", line);
         return fail;
      }

      switch (i)
      {
         case dtName:        f.name = strdup(token);         break;
         case dtDescription: break;
         case dtFormat:      f.format = toDictFormat(token); break;
         case dtSize:        f.size = atoi(token);           break;
         case dtType:        f.type = toType(token);         break;
      }
      
      free((char*)f.name);  // böser cast ...

      tell(0, "token %d -> '%s'", i, token);
   }
   
   return success;
}
