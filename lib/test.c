
#include <stdint.h>   // uint_64_t
#include <sys/time.h>
#include <time.h>

#include <stdio.h>
#include <string>

#include "config.h"
#include "common.h"
#include "db.h"
#include "tabledef.h"
#include "dbdict.h"

cDbConnection* connection = 0;
const char* logPrefix = "test";

//***************************************************************************
// Init Connection
//***************************************************************************

void initConnection()
{
   cDbConnection::init(0x3db00011);

   cDbConnection::setEncoding("utf8");
   cDbConnection::setHost("localhost");
   cDbConnection::setPort(EPG2VDRConfig.dbPort);
   cDbConnection::setName(EPG2VDRConfig.dbName);
   cDbConnection::setUser(EPG2VDRConfig.dbUser);
   cDbConnection::setPass(EPG2VDRConfig.dbPass);
   cDbTable::setConfPath("/etc/epgd/");

   connection = new cDbConnection();
}

void exitConnection()
{
   cDbConnection::exit();
   
   if (connection)
      delete connection;
}

//***************************************************************************
// 
//***************************************************************************

void chkCompress()
{
   std::string s = "_+*!#?=&%$< Hallo TEIL Hallo Folge ";

   printf("'%s'\n", s.c_str());
   prepareCompressed(s);
   printf("'%s'\n", s.c_str());

   s = "Place Vendôme - Heiße Diamanten";
   printf("'%s'\n", s.c_str());
   prepareCompressed(s);
   printf("'%s'\n", s.c_str());

   s = "Halöö älter";
   printf("'%s'\n", s.c_str());
   prepareCompressed(s);
   printf("'%s'\n", s.c_str());
}

//***************************************************************************
// 
//***************************************************************************

void chkStatement1()
{
   cDbTable* epgDb = new cTableEvents(connection);

   if (epgDb->open() != success)
   { 
      tell(0, "Could not access database '%s:%d' (%s)", 
           cDbConnection::getHost(), cDbConnection::getPort(), epgDb->TableName());

      return ;
   }

   tell(0, "---------------------------------------------------");

   // prepare statement to mark wasted DVB events
   
   cDbValue* endTime = new cDbValue("starttime+duration", cDBS::ffInt, 10);
   cDbStatement* updateDelFlg = new cDbStatement(epgDb);

   // update events set delflg = ?, updsp = ? 
   //   where channelid = ? and source = ? 
   //      and starttime+duration > ? 
   //      and starttime < ? 
   //      and (tableid > ? or (tableid = ? and version <> ?))

   updateDelFlg->build("update %s set ", epgDb->TableName());
   updateDelFlg->bind(cTableEvents::fiDelFlg, cDBS::bndIn | cDBS::bndSet);
   updateDelFlg->bind(cTableEvents::fiUpdSp, cDBS::bndIn | cDBS::bndSet, ", ");
   updateDelFlg->build(" where ");
   updateDelFlg->bind(cTableEvents::fiChannelId, cDBS::bndIn | cDBS::bndSet);
   updateDelFlg->bind(cTableEvents::fiSource, cDBS::bndIn | cDBS::bndSet, " and ");
   
   updateDelFlg->bindCmp(0, endTime, ">", " and ");
   
   updateDelFlg->bindCmp(0, cTableEvents::fiStartTime, 0, "<" ,  " and ");
   updateDelFlg->bindCmp(0, cTableEvents::fiTableId,   0, ">" ,  " and (");
   updateDelFlg->bindCmp(0, cTableEvents::fiTableId,   0, "=" ,  " or (");
   updateDelFlg->bindCmp(0, cTableEvents::fiVersion,   0, "<>" , " and ");
   updateDelFlg->build("));");

   updateDelFlg->prepare();

   tell(0, "---------------------------------------------------");
}

//***************************************************************************
// 
//***************************************************************************

void chkStatement2()
{
   cDbTable* imageRefDb = new cTableImageRefs(connection);
   cDbTable* imageDb = new cTableImages(connection);

   if (imageRefDb->open() != success)
      return ;

   if (imageDb->open() != success)
      return ;

   tell(0, "---------------------------------------------------");
 
   cDbStatement* selectAllImages = new cDbStatement(imageRefDb);

   cDbValue imageData; 
   imageData.setField(imageDb->getField(cTableImages::fiImage));

   // select r.imagename, r.eventid, r.lfn, i.image from imagerefs r, images i 
   //    where r.imagename = i.imagename and i.image is not null;

   selectAllImages->build("select ");
   selectAllImages->setBindPrefix("r.");
   selectAllImages->bind(cTableImageRefs::fiImgName, cDBS::bndOut);
   selectAllImages->bind(cTableImageRefs::fiEventId, cDBS::bndOut, ", ");
   selectAllImages->bind(cTableImageRefs::fiLfn, cDBS::bndOut, ", ");
   selectAllImages->setBindPrefix("i.");
   selectAllImages->bind(&imageData, cDBS::bndOut, ",");
   selectAllImages->clrBindPrefix();
   selectAllImages->build(" from %s r, %s i where ", imageRefDb->TableName(), imageDb->TableName());
   selectAllImages->build("r.%s = i.%s and i.%s is not null;",
            imageRefDb->getField(cTableImageRefs::fiImgName)->name,
            imageDb->getField(cTableImages::fiImgName)->name,
            imageDb->getField(cTableImages::fiImage)->name);

   selectAllImages->prepare();


   tell(0, "---------------------------------------------------");

   //delete s;
   delete imageRefDb;
   delete imageDb;
}

//***************************************************************************
// 
//***************************************************************************

void chkStatement3()
{
   int count = 0;
   int lcount = 0;

   cDbTable* epgDb = new cTableEvents(connection);
   cDbTable* mapDb = new cTableChannelMap(connection);

   if (epgDb->open() != success)
      return ;

   if (mapDb->open() != success)
      return ;

   tell(0, "---------------------------------------------------");
 
   cDbStatement* s = new cDbStatement(epgDb);

   s->build("select ");
   s->setBindPrefix("e.");
   s->bind(cTableEvents::fiEventId, cDBS::bndOut);
   s->bind(cTableEvents::fiChannelId, cDBS::bndOut, ", ");
   s->bind(cTableEvents::fiSource, cDBS::bndOut, ", ");
   s->bind(cTableEvents::fiDelFlg, cDBS::bndOut, ", ");
   s->bind(cTableEvents::fiFileRef, cDBS::bndOut, ", ");
   s->bind(cTableEvents::fiTableId, cDBS::bndOut, ", ");
   s->bind(cTableEvents::fiVersion, cDBS::bndOut, ", ");
   s->bind(cTableEvents::fiTitle, cDBS::bndOut, ", ");
   s->bind(cTableEvents::fiShortText, cDBS::bndOut, ", ");
   s->bind(cTableEvents::fiStartTime, cDBS::bndOut, ", ");
   s->bind(cTableEvents::fiDuration, cDBS::bndOut, ", ");
   s->bind(cTableEvents::fiParentalRating, cDBS::bndOut, ", ");
   s->bind(cTableEvents::fiVps,  cDBS::bndOut, ", ");
   s->bind(cTableEvents::fiDescription, cDBS::bndOut, ", ");
   s->clrBindPrefix();
   s->build(" from eventsview e, %s m where ", mapDb->TableName());
   s->build("e.%s = m.%s and e.%s = m.%s and ",
                          epgDb->getField(cTableEvents::fiChannelId)->name,
                          mapDb->getField(cTableChannelMap::fiChannelName)->name,
                          epgDb->getField(cTableEvents::fiSource)->name,
                          mapDb->getField(cTableChannelMap::fiSource)->name);
   s->bindCmp("e", cTableEvents::fiUpdSp, 0, ">");
   s->build(" order by m.%s;", mapDb->getField(cTableChannelMap::fiChannelName)->name);

   s->prepare();

   epgDb->clear();
   epgDb->setValue(cTableEvents::fiUpdSp, (double)0);
   epgDb->setValue(cTableEvents::fiSource, "vdr");                             // used by selectUpdEventsByChannel
   epgDb->setValue(cTableEvents::fiChannelId, "xxxxxxxxxxxxx");    // used by selectUpdEventsByChannel

   int channels = 0;
   char chan[100]; *chan = 0;

   tell(0, "---------------------------------------------------");

   for (int found = s->find(); found; found = s->fetch())
   {
      if (!*chan || strcmp(chan, epgDb->getStrValue(cTableEvents::fiChannelId)) != 0)
      {
         if (*chan)
            tell(0, "processed %-20s with %d events", chan, count - lcount);

         lcount = count;
         channels++;
         strcpy(chan, epgDb->getStrValue(cTableEvents::fiChannelId));

         tell(0, "processing %-20s now", chan);
      }

      tell(0, "-> '%s' - (%ld)", epgDb->getStrValue(cTableEvents::fiChannelId),
           epgDb->getIntValue(cTableEvents::fiEventId));


      count++;
   }

   s->freeResult();

   tell(0, "---------------------------------------------------");
   tell(0, "updated %d channels and %d events", channels, count);
   tell(0, "---------------------------------------------------");

   delete s;
   delete epgDb;
   delete mapDb;
}

//***************************************************************************
// 
//***************************************************************************

void chkStatement4()
{
   cDbTable* eventDb = new cTableEvents(connection);
   if (eventDb->open() != success) return;

   cDbTable* imageRefDb = new cTableImageRefs(connection);
   if (imageRefDb->open() != success) return;

   cDbTable* imageDb = new cTableImages(connection);
   if (imageDb->open() != success) return;

   // select e.masterid, r.imagename, r.eventid, r.lfn, i.image 
   //      from imagerefs r, images i, events e 
   //      where r.imagename = i.imagename 
   //         and e.eventid = r.eventid,
   //         and i.image is not null 
   //         and (i.updsp > ? or r.updsp > ?);

   cDBS::FieldDef masterFld = { "masterid", cDBS::ffUInt,  0, 999, cDBS::ftData };
   cDbValue masterId;
   cDbValue imageData;
   cDbValue imageUpdSp;

   masterId.setField(&masterFld);
   imageData.setField(imageDb->getField(cTableImages::fiImage));
   imageUpdSp.setField(imageDb->getField(cTableImages::fiUpdSp));

   cDbStatement* selectAllImages = new cDbStatement(imageRefDb);

   selectAllImages->build("select ");
   selectAllImages->setBindPrefix("e.");
   selectAllImages->bind(&masterId, cDBS::bndOut);
   selectAllImages->setBindPrefix("r.");
   selectAllImages->bind(cTableImageRefs::fiImgName, cDBS::bndOut, ", ");
   selectAllImages->bind(cTableImageRefs::fiEventId, cDBS::bndOut, ", ");
   selectAllImages->bind(cTableImageRefs::fiLfn, cDBS::bndOut, ", ");
   selectAllImages->setBindPrefix("i.");
   selectAllImages->bind(&imageData, cDBS::bndOut, ", ");
   selectAllImages->clrBindPrefix();
   selectAllImages->build(" from %s r, %s i, %s e where ", 
                          imageRefDb->TableName(), imageDb->TableName(), eventDb->TableName());
   selectAllImages->build("e.%s = r.%s and i.%s = r.%s and i.%s is not null and (",
                          eventDb->getField(cTableEvents::fiEventId)->name, 
                          imageRefDb->getField(cTableImageRefs::fiEventId)->name,
                          imageDb->getField(cTableImageRefs::fiImgName)->name,
                          imageRefDb->getField(cTableImageRefs::fiImgName)->name,
                          imageDb->getField(cTableImages::fiImage)->name);
   selectAllImages->bindCmp("i", &imageUpdSp, ">");
   selectAllImages->build(" or ");
   selectAllImages->bindCmp("r", cTableImageRefs::fiUpdSp, 0, ">");
   selectAllImages->build(");");

   selectAllImages->prepare();

   imageRefDb->clear();
   imageRefDb->setValue(cTableImageRefs::fiUpdSp, 1377733333L);
   imageUpdSp.setValue(1377733333L);
   
   int count = 0;
   for (int res = selectAllImages->find(); res; res = selectAllImages->fetch())
   {
      count ++;
   }
   tell(0,"%d", count);
}

//***************************************************************************
// Main
//***************************************************************************

int main()
{
   EPG2VDRConfig.logstdout = yes;
   EPG2VDRConfig.loglevel = 2;

   setlocale(LC_CTYPE, "");
   char* lang = setlocale(LC_CTYPE, 0);
   
   if (lang)
   {
      tell(0, "Set locale to '%s'", lang);
      
      if ((strcasestr(lang, "UTF-8") != 0) || (strcasestr(lang, "UTF8") != 0))
         tell(0, "detected UTF-8");
      else
         tell(0, "no UTF-8");
   }
   else
   {
      tell(0, "Reseting locale for LC_CTYPE failed.");
   }

   
   cDbDict* dict = new cDbDict();

   dict->in("../epg.dat");

   delete dict;

   return 0;

   initConnection();

   chkCompress();

   tell(0, "duration was: '%s'", ms2Dur(2340).c_str());

   // chkStatement1();
   // chkStatement2();
   // chkStatement3();
   // chkStatement4();
  
   exitConnection();

   return 0;
}
