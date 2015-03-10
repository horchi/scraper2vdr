/*
 * plgconfig.h:
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id: plgconfig.h
 */

#ifndef __SCRAPER2VDR_CONFIG_H
#define __SCRAPER2VDR_CONFIG_H

#include <string.h>

#include <vdr/plugin.h>

#include "lib/config.h"

using namespace std;

//***************************************************************************
// Config
//***************************************************************************

struct cScraper2VdrConfig : public cEpgConfig
{
   private: 
      bool NewDBHost; 
      bool NewDBName; 
      bool NewDBPort; 
      bool NewDBUser; 
      bool NewDBPass; 
   public:
      
      cScraper2VdrConfig();

      bool SetupParse(const char *Name, const char *Value);
      void SetUuid(cPlugin *plug);
      void SetImageDir(cString dir);
      void SetDefaultImageDir();
      void SetMode(string mode);

      int mainMenuEntry;
      bool headless;
      bool imgDirSet;
      string imageDir;
      string recScrapInfoName;
      int fastmode;
      int thumbHeight;
      int useFixPosterSize;
      int fixPosterWidth;
      int fixPosterHeight;
      int fixSeasonPosterWidth;
      int fixSeasonPosterHeight;
      int maxPosterDistortion; 
};

extern cScraper2VdrConfig scraper2VdrConfig;

#endif // __SCRAPER2VDR_CONFIG_H
