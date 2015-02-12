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

#include "lib/config.h"

using namespace std;

//***************************************************************************
// Config
//***************************************************************************

struct cScraper2VdrConfig : public cEpgConfig
{
   public:
      
      cScraper2VdrConfig();

      int mainMenuEntry;
      bool headless;
      bool imgDirSet;
      string imageDir;
      string recScrapInfoName;
      int fastmode;
};

extern cScraper2VdrConfig scraper2VdrConfig;

#endif // __SCRAPER2VDR_CONFIG_H
