/*
 * config.c:
 *
 * See the README file for copyright information and how to reach the author.
 *
 */

#include <string.h>

#include "common.h"
#include "config.h"

cEPG2VDRConfig EPG2VDRConfig;

cEPG2VDRConfig::cEPG2VDRConfig(void) 
{
   mainmenuVisible = yes;
   mainmenuFullupdate = 0;

   useproxy = no;
   sstrcpy(httpproxy, "127.0.0.1:8000", sizeof(httpproxy));
   sstrcpy(username, "", sizeof(username));
   sstrcpy(password, "", sizeof(password));

   checkInitial = yes;
   updatetime = 6;        // hours
   days = 8;
   upddays = 2;
   storeXmlToFs = no;
   blacklist = no;
   masterMode = 0;

   getepgimages = yes;
   maximagesperevent = 1;
   epgImageSize = 2;

   seriesEnabled = yes;
   sstrcpy(seriesUrl, "eplists.constabel.net", sizeof(seriesUrl));
   seriesPort = 2006;
   storeSeriesToFs = no;

#ifdef VDR_PLUGIN
   activeOnEpgd = no;
   scheduleBoot = no;
#else
   sstrcpy(cachePath, "/var/cache/epgd", sizeof(cachePath));
   sstrcpy(pluginPath, PLGDIR, sizeof(pluginPath));
   sstrcpy(epgView, "eventsview.sql", sizeof(epgView));
   updateThreshold = 200;
   maintanance = no;
#endif

   sstrcpy(dbHost, "localhost", sizeof(dbHost));
   dbPort = 3306;
   sstrcpy(dbName, "epg2vdr", sizeof(dbName));
   sstrcpy(dbUser, "epg2vdr", sizeof(dbUser));
   sstrcpy(dbPass, "epg", sizeof(dbPass));

   logstdout = no;
   loglevel = 1;

   uuid[0] = 0;
} 
