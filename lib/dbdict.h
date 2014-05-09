/*
 * dbdict.h
 *
 * See the README file for copyright information and how to reach the author.
 *
 */

#ifndef __DBDICT_H
#define __DBDICT_H

#include "db.h"

//***************************************************************************
// cDbDict
//***************************************************************************

class cDbDict : public cDbService
{
      
   public:

      // declarations

      enum DictToken
      {
         dtName,
         dtDescription,
         dtFormat,
         dtSize,
         dtType,

         dtCount
      };

      cDbDict();
      virtual ~cDbDict();

      int in(const char* file);

   protected:

      int atLine(const char* line);
      int parseField(const char* line);

      // data

      int inside;
      static FieldDef fields[];

};

#endif //  __DBDICT_H
