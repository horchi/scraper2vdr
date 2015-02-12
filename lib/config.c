/*
 * config.c:
 *
 * See the README file for copyright information and how to reach the author.
 *
 */

#include "config.h"

//***************************************************************************
// Statics
//***************************************************************************

int cEpgConfig::logstdout = no;
int cEpgConfig::loglevel = 1;

//***************************************************************************
// Common EPG Service Configuration
//***************************************************************************

cEpgConfig::cEpgConfig() 
{
   // database connection

   sstrcpy(dbHost, "localhost", sizeof(dbHost));
   dbPort = 3306;
   sstrcpy(dbName, "epg2vdr", sizeof(dbName));
   sstrcpy(dbUser, "epg2vdr", sizeof(dbUser));
   sstrcpy(dbPass, "epg", sizeof(dbPass));

   sstrcpy(netDevice, "", sizeof(netDevice));

   uuid[0] = 0;

   getepgimages = yes;
} 
