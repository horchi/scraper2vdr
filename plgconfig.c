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
   thumbHeight = 200;
   useFixPosterSize = no;
   fixPosterWidth = 500;
   fixPosterHeight = 735;
   fixSeasonPosterWidth = 400;
   fixSeasonPosterHeight = 588;
   maxPosterDistortion = 5;
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
        if (strcasecmp(Name, "mainMenuEntry") == 0) mainMenuEntry = atoi(Value);
   else if (strcasecmp(Name, "DbHost") == 0)        sstrcpy(dbHost, Value, sizeof(dbHost));
   else if (strcasecmp(Name, "DbPort") == 0)        dbPort = atoi(Value);
   else if (strcasecmp(Name, "DbName") == 0)        sstrcpy(dbName, Value, sizeof(dbName));
   else if (strcasecmp(Name, "DbUser") == 0)        sstrcpy(dbUser, Value, sizeof(dbUser));
   else if (strcasecmp(Name, "DbPass") == 0)        sstrcpy(dbPass, Value, sizeof(dbPass));
   else if (strcasecmp(Name, "thumbHeight") == 0)   thumbHeight = atoi(Value);
   else if (strcasecmp(Name, "useFixPosterSize") == 0)   useFixPosterSize = atoi(Value);
   else if (strcasecmp(Name, "fixPosterWidth") == 0)   fixPosterWidth = atoi(Value);
   else if (strcasecmp(Name, "fixPosterHeight") == 0)   fixPosterHeight = atoi(Value);
   else if (strcasecmp(Name, "fixSeasonPosterWidth") == 0)   fixSeasonPosterWidth = atoi(Value);
   else if (strcasecmp(Name, "fixSeasonPosterHeight") == 0)   fixSeasonPosterHeight = atoi(Value);
   else if (strcasecmp(Name, "maxPosterDistortion") == 0)   maxPosterDistortion = atoi(Value);
   else if (strcasecmp(Name, "LogLevel") == 0)      loglevel = atoi(Value);
   else
      return false;

   return true;
}
