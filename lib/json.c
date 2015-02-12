/*
 * json.c
 *
 * See the README file for copyright information and how to reach the author.
 *
 */

#include "json.h"

#ifdef USEJSON

const char* charset = "utf-8";  // #TODO, move to configuration?

//***************************************************************************
// Copy JSON Object to Data Buffer
//***************************************************************************

int json2Data(json_t* obj, MemoryStruct* data, const char* encoding)
{
   int status = success;
   data->memory = json_dumps(obj, 0);     // well be freed by data dtor
   data->size = strlen(data->memory);
   sprintf(data->contentType, "application/json; charset=%s", charset);
   
   // gzip content if supported by browser ...

   if (data->size && encoding && strstr(encoding, "gzip"))
      status = data->toGzip();
   
   // json_t_to_file((char*)"last.json", obj);       // debug ...

   return status;
}

//***************************************************************************
// Add field to json object
//***************************************************************************

int addFieldToJson(json_t* obj, cDbValue* value, int ignoreEmpty, const char* extName)
{
   char* name;
   cDBS::FieldFormat type = value->getField()->getFormat();

   name = strdup(extName ? extName : value->getField()->getName());

   toCase(cLower, name);   // use always lower case names

   switch (type)
   {
      case cDBS::ffAscii:
      case cDBS::ffText:
      case cDBS::ffMText:
         if (!ignoreEmpty || !isEmpty(value->getStrValue()))
            json_object_set_new(obj, name, json_string(value->getStrValue()));
         break;

      case cDBS::ffInt:
      case cDBS::ffUInt:
         json_object_set_new(obj, name, json_integer(value->getIntValue()));
         break;

      case cDBS::ffFloat:
         json_object_set_new(obj, name, json_real(value->getFloatValue()));
         break;

      default:
         break;
   }

   free(name);

   return success;
}

//***************************************************************************
// Add field to json object
//***************************************************************************

int addFieldToJson(json_t* obj, cDbTable* table, const char* fname, int ignoreEmpty, const char* extName)
{
   return addFieldToJson(obj, table->getValue(fname), ignoreEmpty, extName);
}

//***************************************************************************
// Get Elements
//***************************************************************************

const char* getStringFromJson(json_t* obj, const char* name, const char* def)
{
   json_t* o = json_object_get(obj, name);

   if (!o)
      return def;

   return json_string_value(o);
}

int getIntFromJson(json_t* obj, const char* name, int def)
{
   json_t* o = json_object_get(obj, name);

   if (!o)
      return def;

   return json_integer_value(o);
}

#endif // USEJSON