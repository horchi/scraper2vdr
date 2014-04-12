/*
 * config.h:
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id: config.h,v 1.2 2012/10/26 08:44:13 wendel Exp $
 */

#ifndef __EPG2VDR_CONFIG_H
#define __EPG2VDR_CONFIG_H

#include "common.h"

//***************************************************************************
// Config
//***************************************************************************

struct cEPG2VDRConfig
{
   public:
      
      cEPG2VDRConfig(void);

      int useproxy;      
      char httpproxy[256+TB];
      char username[100+TB];
      char password[100+TB];

      int checkInitial;
      int updatetime;
      int days;
      int upddays;
      int storeXmlToFs;
      int blacklist;         // to enable noepg feature

      int getepgimages;
      int maximagesperevent;
      int epgImageSize;

      int seriesEnabled;
      char seriesUrl[500+TB];
      int seriesPort;
      int storeSeriesToFs;

#ifdef VDR_PLUGIN
      int activeOnEpgd;
      int scheduleBoot;
#else
      char cachePath[256+TB];
      char pluginPath[256+TB];
      char epgView[100+TB];
      int updateThreshold;
      int maintanance;
#endif

      char dbHost[100+TB];
      int dbPort;
      char dbName[100+TB];
      char dbUser[100+TB];
      char dbPass[100+TB];

      int logstdout;
      int loglevel;

      int mainmenuVisible;
      int mainmenuFullupdate;
      int masterMode;
      char uuid[sizeUuid+TB];
};

extern cEPG2VDRConfig EPG2VDRConfig;

#endif // __EPG2VDR_CONFIG_H 
