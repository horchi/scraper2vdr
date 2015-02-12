/*
 * config.c:
 *
 * See the README file for copyright information and how to reach the author.
 *
 */

#include "plgconfig.h"

cScraper2VdrConfig Scraper2VdrConfig;

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
}
