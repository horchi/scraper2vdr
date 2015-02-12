/*
 * config.c:
 *
 * See the README file for copyright information and how to reach the author.
 *
 */

#include "plgconfig.h"

cScraper2VdrConfig scraper2VdrConfig;

//***************************************************************************
// cEpg2VdrConfig
//***************************************************************************

cScraper2VdrConfig::cScraper2VdrConfig() 
   : cEpgConfig()
{
   mainMenuEntry = yes;
   headless = no;
   imgDirSet = no;
   imageDir = "";
   recScrapInfoName = "scrapinfo";
   fastmode = yes;
   thumbHeight = 200;
}

void cScraper2VdrConfig::SetUuid(cPlugin *plug) 
{
   if (isEmpty(uuid)) 
   { 
      sstrcpy(uuid, getUniqueId(), sizeof(uuid));
      plug->SetupStore("uuid", uuid);
   }
   
   tell(0, "epgd uuid: %s", uuid);
}

void cScraper2VdrConfig::SetImageDir(cString dir) 
{
   imageDir = *dir;
   imgDirSet = true;
}

void cScraper2VdrConfig::SetDefaultImageDir(void) 
{
   if (!imgDirSet)
      imageDir = cPlugin::CacheDirectory(PLUGIN_NAME_I18N);

   tell (0, "using image directory %s", imageDir.c_str());
}

void cScraper2VdrConfig::SetMode(string mode) 
{
   if (!mode.compare("headless"))
      headless = true;
}

bool cScraper2VdrConfig::SetupParse(const char *Name, const char *Value) 
{ 
   if      (strcasecmp(Name, "uuid") == 0)          sstrcpy(uuid, Value, sizeof(uuid));
   else if (strcasecmp(Name, "mainMenuEntry") == 0) mainMenuEntry = atoi(Value);
   else if (strcasecmp(Name, "DbHost") == 0)        sstrcpy(dbHost, Value, sizeof(dbHost));
   else if (strcasecmp(Name, "DbPort") == 0)        dbPort = atoi(Value);
   else if (strcasecmp(Name, "DbName") == 0)        sstrcpy(dbName, Value, sizeof(dbName));
   else if (strcasecmp(Name, "DbUser") == 0)        sstrcpy(dbUser, Value, sizeof(dbUser));
   else if (strcasecmp(Name, "DbPass") == 0)        sstrcpy(dbPass, Value, sizeof(dbPass));
   else if (strcasecmp(Name, "fastmode") == 0)      fastmode = atoi(Value);
   else if (strcasecmp(Name, "thumbHeight") == 0)   thumbHeight = atoi(Value);
   else
      return false;

   return true;
}
