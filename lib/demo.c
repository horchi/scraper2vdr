

#include "config.h"
#include "common.h"

#include "db.h"
#include "tabledef.h"

cDbConnection* connection = 0;
const char* logPrefix = "demo";

//***************************************************************************
// Init Connection
//***************************************************************************

void initConnection()
{
   cDbConnection::init();

   cDbConnection::setEncoding("utf8");
   cDbConnection::setHost("localhost");

   cDbConnection::setPort(3306);
   cDbConnection::setName("epg2vdr");
   cDbConnection::setUser("epg2vdr");
   cDbConnection::setPass("epg");
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

int demoStatement()
{
   int status = success;

   cTableEvents* eventsDb = new cTableEvents(connection);

   tell(0, "------------------- attach table ---------------");

   // open table (attach)

   if (eventsDb->open() != success) 
      return fail;
   
   tell(0, "---------------- prepare select statement -------------");

   // vorbereiten (prepare) eines statement, am besten einmal bei programmstart!
   // ----------
   // select eventid, compshorttext, episodepart, episodelang 
   //   from events 
   //     where eventid > ?

   cDbStatement* selectByCompTitle = new cDbStatement(eventsDb);

   status += selectByCompTitle->build("select ");
   status += selectByCompTitle->bind(cTableEvents::fiEventId, cDBS::bndOut);
   status += selectByCompTitle->bind(cTableEvents::fiChannelId, cDBS::bndOut, ", ");
   status += selectByCompTitle->bind(cTableEvents::fiTitle, cDBS::bndOut, ", ");
   status += selectByCompTitle->build(" from %s where ", eventsDb->TableName());
   status += selectByCompTitle->bindCmp(0, cTableEvents::fiEventId, 0, ">");

   status += selectByCompTitle->prepare();   // prepare statement 

   if (status != success)
   {
      // prepare sollte oracle fehler ausgegeben haben!

      delete eventsDb;
      delete selectByCompTitle; 

      return fail;
   }

   tell(0, "------------------ prepare done ----------------------");

   tell(0, "------------------ create some rows  ----------------------");

   eventsDb->clear();     // alle values löschen   

   for (int i = 0; i < 10; i++)
   {
      char* title;
      asprintf(&title, "title %d", i);

      eventsDb->setValue(cTableEvents::fiEventId, 800 + i * 100);
      eventsDb->setValue(cTableEvents::fiChannelId, "xxx-yyyy-zzz");
      eventsDb->setValue(cTableEvents::fiTitle, title);
      
      eventsDb->store();                // store -> select mit anschl. update oder insert je nachdem ob dier PKey bereits vorhanden ist
      // eventsDb->insert();            // sofern man schon weiß das es ein insert ist
      // eventsDb->update();            // sofern man schon weiß das der Datensatz vorhanden ist

      free(title);
   }

   tell(0, "------------------ done  ----------------------");

   tell(0, "-------- select all where eventid > 1000 -------------");
   
   eventsDb->clear();     // alle values löschen
   eventsDb->setValue(cTableEvents::fiEventId, 1000);

   for (int f = selectByCompTitle->find(); f; f = selectByCompTitle->fetch())
   {
      tell(0, "id: %ld", eventsDb->getIntValue(cTableEvents::fiEventId));
      tell(0, "channel: %s", eventsDb->getStrValue(cTableEvents::fiChannelId));
      tell(0, "titel: %s", eventsDb->getStrValue(cTableEvents::fiTitle));
   }

   // freigeben der Ergebnissmenge !!

   selectByCompTitle->freeResult();

   // folgendes am programmende

   delete eventsDb;             // implizietes close (detach)
   delete selectByCompTitle;    // statement freigeben (auch gegen die DB)

   return success;
}

//***************************************************************************
// Join
//***************************************************************************

int joinDemo()
{
   int status = success;

   // grundsätzlich genügt hier auch eine Tabelle, für die anderen sind cDbValue Instanzen außreichend
   // so ist es etwas einfacher die cDbValues zu initialisieren. 
   // Ich habe statische "virtual FieldDef* getFieldDef(int f)" Methode in der Tabellenklassen geplant
   // um ohne Instanz der cTable ein Feld einfach initialisieren zu können

   cTableEvents* eventsDb = new cTableEvents(connection);
   cTableImageRefs* imageRefDb = new cTableImageRefs(connection);
   cTableImages* imageDb = new cTableImages(connection);

   tell(0, "------------------- attach table ---------------");

   // open table (attach)

   if (eventsDb->open() != success) 
      return fail;

   if (imageDb->open() != success) 
      return fail;

   if (imageRefDb->open() != success) 
      return fail;
   
   tell(0, "---------------- prepare select statement -------------");

   // all images

   cDbStatement* selectAllImages = new cDbStatement(imageRefDb);

   // prepare fields

   cDbValue imageUpdSp;
   cDbValue imageSize;
   cDbValue masterId;

   cDBS::FieldDef imageSizeDef = { "image", cDBS::ffUInt,  0, 999, cDBS::ftData };  // eine Art ein Feld zu erzeugen
   imageSize.setField(&imageSizeDef);                                               // eine andere Art ein Feld zu erzeugen ...
   imageUpdSp.setField(imageDb->getField(cTableImages::fiUpdSp));
   masterId.setField(eventsDb->getField(cTableEvents::fiMasterId));

   // select e.masterid, r.imagename, r.eventid, r.lfn, length(i.image)
   //      from imagerefs r, images i, events e 
   //      where i.imagename = r.imagename 
   //         and e.eventid = r.eventid
   //         and (i.updsp > ? or r.updsp > ?)

   selectAllImages->build("select ");
   selectAllImages->setBindPrefix("e.");
   selectAllImages->bind(&masterId, cDBS::bndOut);
   selectAllImages->setBindPrefix("r.");
   selectAllImages->bind(cTableImageRefs::fiImgName, cDBS::bndOut, ", ");
   selectAllImages->bind(cTableImageRefs::fiEventId, cDBS::bndOut, ", ");
   selectAllImages->bind(cTableImageRefs::fiLfn, cDBS::bndOut, ", ");
   selectAllImages->setBindPrefix("i.");
   selectAllImages->build(", length(");
   selectAllImages->bind(&imageSize, cDBS::bndOut);
   selectAllImages->build(")");
   selectAllImages->clrBindPrefix();
   selectAllImages->build(" from %s r, %s i, %s e where ", 
                          imageRefDb->TableName(), imageDb->TableName(), eventsDb->TableName());
   selectAllImages->build("e.%s = r.%s and i.%s = r.%s and (",
                          eventsDb->getField(cTableEvents::fiEventId)->name, 
                          imageRefDb->getField(cTableImageRefs::fiEventId)->name,
                          imageDb->getField(cTableImages::fiImgName)->name,
                          imageRefDb->getField(cTableImageRefs::fiImgName)->name);
   selectAllImages->bindCmp("i", &imageUpdSp, ">");
   selectAllImages->build(" or ");
   selectAllImages->bindCmp("r", cTableImageRefs::fiUpdSp, 0, ">");
   selectAllImages->build(")");

   status += selectAllImages->prepare();

   if (status != success)
   {
      // prepare sollte oracle fehler ausgegeben haben!

      delete eventsDb;
      delete imageDb;
      delete imageRefDb;
      delete selectAllImages; 

      return fail;
   }

   tell(0, "------------------ prepare done ----------------------");

   tell(0, "------------------ select ----------------------");

   time_t since = time(0) - 60 * 60;
   imageRefDb->clear();
   imageRefDb->setValue(cTableImageRefs::fiUpdSp, since);
   imageUpdSp.setValue(since);

   for (int res = selectAllImages->find(); res; res = selectAllImages->fetch())
   {
      // so kommst du an die Werte der unterschgiedlichen Tabellen

      // int eventid = masterId.getIntValue();
      // const char* imageName = imageRefDb->getStrValue(cTableImageRefs::fiImgName);
      // int lfn = imageRefDb->getIntValue(cTableImageRefs::fiLfn);
      // int size = imageSize.getIntValue();

      
   }

   // freigeben der Ergebnissmenge !!

   selectAllImages->freeResult();

   // folgendes am programmende

   delete eventsDb;             // implizietes close (detach)
   delete imageDb;
   delete imageRefDb;
   delete selectAllImages;      // statement freigeben (auch gegen die DB)

   return success;
}


//***************************************************************************
// Main
//***************************************************************************

int main()
{
   EPG2VDRConfig.logstdout = yes;
   EPG2VDRConfig.loglevel = 2;

   initConnection();

   // demoStatement();
   // joinDemo();

   tell(0, "uuid: '%s'", getUniqueId());

   exitConnection();

   return 0;
}
