
#define __STL_CONFIG_H
#include <locale.h>

#include <vdr/videodir.h>
#include <vdr/tools.h>
#include <vdr/plugin.h>

#include "lib/common.h"
#include "lib/epgservice.h"

#include "plgconfig.h"
#include "tools.h"
#include "update.h"

#include <sstream>

cUpdate::cUpdate(cScrapManager* manager)
   : cThread("scraper2vdr-update", true)
{
    connection = NULL;
    vdrDb = NULL;
    tEvents = NULL;
    tSeries = NULL;
    tEpisodes = NULL;
    tSeriesMedia = NULL;
    tSeriesActors = NULL;
    tMovies = NULL;
    tMovieActor = NULL;
    tMovieActors = NULL;
    tMovieMedia = NULL;
    tRecordingList = NULL;

    selectReadScrapedEventsInit = 0;
    selectReadScrapedEvents = 0;
    selectMovieActors = 0;
    selectRecordings = 0;
    clearTempEpisodeTable = 0;
    fillTempEpisodeTable = 0;

    selectReadSeriesInit = 0;
    selectReadSeriesLastScrsp = 0;
    selectReadEpisodes = 0;
    selectSeriesMediaActors = 0;
    selectSeriesImage = 0;
    selectReadMoviesInit = 0;
    selectReadMoviesLastScrsp = 0;
    selectMovieMediaFast = 0;
    selectMovieImage = 0;

    scrapManager = manager;
    imgPathSeries = scraper2VdrConfig.imageDir + "/series";
    imgPathMovies = scraper2VdrConfig.imageDir + "/movies";
    TempEpisodeTableName = "TempEpisodeCache_"+replaceString(getUniqueId(),"-","");
    TempEpisodeCreateStatement = "CREATE TEMPORARY TABLE IF NOT EXISTS "+TempEpisodeTableName+" (episode_id int(11) NOT NULL, PRIMARY KEY (episode_id)) ENGINE=Memory";
    TempEpisodeDeleteStatement = "DROP TEMPORARY TABLE "+TempEpisodeTableName;
    lastScrap = 0;
    MaxScrspSeries = 0; // max scrsp of known events/recordings for series
    MaxScrspMovies = 0; // max scrsp of known events/recordings for movies
    lastWait = GetTimems(); // when did we call waitCondition.TimedWait last time
    forceUpdate = false;
    forceRecordingUpdate = false;
    forceVideoDirUpdate = false;
    forceScrapInfoUpdate = false;
    forceFullUpdate = false;
    forceReconnect = false;
    scrspRemoved = false;
}

int cUpdate::init()
{
   char* lang;
   lang = setlocale(LC_CTYPE, 0);

   if (lang) {
      tell(0, "Set locale to '%s'", lang);
      if ((strcasestr(lang, "UTF-8") != 0) || (strcasestr(lang, "UTF8") != 0)){
         tell(0, "detected UTF-8");
         withutf8 = yes;
      }
   } else {
      tell(0, "Reseting locale for LC_CTYPE failed.");
   }

   // initialize the dictionary

   if (LoadDBDict() != success) return fail;

   // init database ...

   cDbConnection::setEncoding(withutf8 ? "utf8": "latin1"); // mysql uses latin1 for ISO8851-1
   cDbConnection::setHost(scraper2VdrConfig.dbHost);
   cDbConnection::setPort(scraper2VdrConfig.dbPort);
   cDbConnection::setName(scraper2VdrConfig.dbName);
   cDbConnection::setUser(scraper2VdrConfig.dbUser);
   cDbConnection::setPass(scraper2VdrConfig.dbPass);
   cDbConnection::setConfPath(cPlugin::ConfigDirectory("scraper2vdr/"));

   return success;
}

cUpdate::~cUpdate() {
    if (loopActive)
        Stop();
}

// global field definitions

cDbFieldDef imageSizeDef("MEDIACONTENT", "media_content", cDBS::ffUInt, 0, cDBS::ftData);
cDbFieldDef uuIDDef("VDRUUID", "vdruuid", cDBS::ffAscii, 40, cDBS::ftData);

int cUpdate::GetEnvironment(cEnvironment* env)
{
   env->basePath = scraper2VdrConfig.imageDir;
   env->seriesPath = imgPathSeries;
   env->moviesPath = imgPathMovies;

   return done;
}

int cUpdate::initDb()
{
    int status = success;

    if (!connection)
        connection = new cDbConnection();

    if (scrspRemoved) {
        // have to reload dbdict, because we removed scrsp field from recordings table
        if (LoadDBDict() != success) return fail;
        scrspRemoved = false;
    }

    vdrDb = new cDbTable(connection, "vdrs");
    if (vdrDb->open() != success) return fail;

    tEvents = new cDbTable(connection, "events");
    if (tEvents->open() != success) return fail;

    tSeries = new cDbTable(connection, "series");
    if (tSeries->open() != success) return fail;

    tEpisodes = new cDbTable(connection, "series_episode");
    if (tEpisodes->open() != success) return fail;

    tSeriesMedia = new cDbTable(connection, "series_media");
    if (tSeriesMedia->open() != success) return fail;

    tSeriesActors = new cDbTable(connection, "series_actor");
    if (tSeriesActors->open() != success) return fail;

    tMovies = new cDbTable(connection, "movie");
    if (tMovies->open() != success) return fail;

    tMovieActor = new cDbTable(connection, "movie_actor");
    if (tMovieActor->open() != success) return fail;

    tMovieActors = new cDbTable(connection, "movie_actors");
    if (tMovieActors->open() != success) return fail;

    tMovieMedia = new cDbTable(connection, "movie_media");
    if (tMovieMedia->open() != success) return fail;

    tRecordingList = new cDbTable(connection, "recordinglist");
    if (tRecordingList->open() != success) return fail;

    // try to create temp. episode cache table

    if (connection->query("%s", TempEpisodeCreateStatement.c_str()) != success)
        return fail;

    // --------------------
    // prepare statements

    //
    series_id.setField(tSeriesMedia->getField("SERIESID"));
    recSeriesid.setField(tRecordingList->getField("SCRSERIESID"));
    event_scrsp.setField(tEvents->getField("SCRSP"));
    events_ScrSeriesId.setField(tEvents->getField("SCRSERIESID"));
    episode_LastUpdate.setField(tEpisodes->getField("EPISODELASTUPDATED"));
    vdr_uuid.setField(&uuIDDef);
    imageSize.setField(&imageSizeDef);
    actorRole.setField(tMovieActors->getField("ROLE"));
    actorMovie.setField(tMovieActors->getField("MOVIEID"));
    thbWidth.setField(tMovieMedia->getField("MEDIAWIDTH"));
    thbHeight.setField(tMovieMedia->getField("MEDIAHEIGHT"));

    selectReadScrapedEventsInit = new cDbStatement(tEvents);
    selectReadScrapedEventsInit->build("select ");
    selectReadScrapedEventsInit->bind("EVENTID", cDBS::bndOut);
    selectReadScrapedEventsInit->bind("CHANNELID", cDBS::bndOut, ", ");
    selectReadScrapedEventsInit->bind("MASTERID", cDBS::bndOut, ", ");
    selectReadScrapedEventsInit->bind("SCRSERIESID", cDBS::bndOut, ", ");
    selectReadScrapedEventsInit->bind("SCRSERIESEPISODE", cDBS::bndOut, ", ");
    selectReadScrapedEventsInit->bind("SCRMOVIEID", cDBS::bndOut, ", ");
    selectReadScrapedEventsInit->bind("SCRSP", cDBS::bndOut, ", ");
    selectReadScrapedEventsInit->build(" from %s where ", tEvents->TableName());
    selectReadScrapedEventsInit->build(" ((%s is not null and %s > 0) ",
                                       tEvents->getField("SCRSERIESID")->getDbName(),
                                       tEvents->getField("SCRSERIESID")->getDbName());
    selectReadScrapedEventsInit->build(" or (%s is not null and %s > 0)) ",
                                   tEvents->getField("SCRMOVIEID")->getDbName(),
                                   tEvents->getField("SCRMOVIEID")->getDbName());
    selectReadScrapedEventsInit->build(" order by %s", tEvents->getField("INSSP")->getDbName());

    status += selectReadScrapedEventsInit->prepare();

    //

    selectReadScrapedEvents = new cDbStatement(tEvents);
    selectReadScrapedEvents->build("select ");
    selectReadScrapedEvents->bind("EVENTID", cDBS::bndOut);
    selectReadScrapedEvents->bind("CHANNELID", cDBS::bndOut, ", ");
    selectReadScrapedEvents->bind("MASTERID", cDBS::bndOut, ", ");
    selectReadScrapedEvents->bind("SCRSERIESID", cDBS::bndOut, ", ");
    selectReadScrapedEvents->bind("SCRSERIESEPISODE", cDBS::bndOut, ", ");
    selectReadScrapedEvents->bind("SCRMOVIEID", cDBS::bndOut, ", ");
    selectReadScrapedEvents->bind("SCRSP", cDBS::bndOut, ", ");
    selectReadScrapedEvents->build(" from %s where ", tEvents->TableName());
    selectReadScrapedEvents->build(" ((%s is not null and %s > 0) ",
                                   tEvents->getField("SCRSERIESID")->getDbName(),
                                   tEvents->getField("SCRSERIESID")->getDbName());
    selectReadScrapedEvents->build(" or (%s is not null and %s > 0)) ",
                                   tEvents->getField("SCRMOVIEID")->getDbName(),
                                   tEvents->getField("SCRMOVIEID")->getDbName());
    selectReadScrapedEvents->bindCmp(0, "SCRSP", 0, ">", " and ");
    selectReadScrapedEvents->build(" order by %s", tEvents->getField("INSSP")->getDbName());

    status += selectReadScrapedEvents->prepare();

    //

    selectMovieActors = new cDbStatement(tMovieActor);
    selectMovieActors->build("select ");
    selectMovieActors->setBindPrefix("act.");
    selectMovieActors->bind("ACTORID", cDBS::bndOut);
    selectMovieActors->bind("ACTORNAME", cDBS::bndOut, ", ");
    selectMovieActors->setBindPrefix("role.");
    selectMovieActors->bind(&actorRole, cDBS::bndOut, ", ");
    selectMovieActors->setBindPrefix("thumb.");
    selectMovieActors->bind(&thbWidth, cDBS::bndOut, ", ");
    selectMovieActors->bind(&thbHeight, cDBS::bndOut, ", ");
    selectMovieActors->clrBindPrefix();
    selectMovieActors->build(" from %s act, %s role, %s thumb where ",
                        tMovieActor->TableName(), tMovieActors->TableName(), tMovieMedia->TableName());
    selectMovieActors->build("act.%s = role.%s ",
                        tMovieActor->getField("ACTORID")->getDbName(),
                        tMovieActors->getField("ACTORID")->getDbName());
    selectMovieActors->build(" and role.%s = thumb.%s ",
                        tMovieActors->getField("ACTORID")->getDbName(),
                        tMovieMedia->getField("ACTORID")->getDbName());
    selectMovieActors->setBindPrefix("role.");
    selectMovieActors->bind(&actorMovie, cDBS::bndIn | cDBS::bndSet, " and ");
    status += selectMovieActors->prepare();

    //

    selectRecordings = new cDbStatement(tRecordingList);
    selectRecordings->build("select ");
    selectRecordings->bind("PATH", cDBS::bndOut);
    selectRecordings->bind("STARTTIME", cDBS::bndOut, ", ");
    selectRecordings->bind("SCRMOVIEID", cDBS::bndOut, ", ");
    selectRecordings->bind("SCRSERIESID", cDBS::bndOut, ", ");
    selectRecordings->bind("SCRSERIESEPISODE", cDBS::bndOut, ", ");
    selectRecordings->build(" from %s where ", tRecordingList->TableName());
    selectRecordings->build("%s = 0", tRecordingList->getField("SCRNEW")->getDbName());
    status += selectRecordings->prepare();

    // clear temp episode cache table

    clearTempEpisodeTable = new cDbStatement(connection);
    clearTempEpisodeTable->build("truncate table %s",
                                 TempEpisodeTableName.c_str());
    status += clearTempEpisodeTable->prepare();

    // fill temp episode cache table using current series id
/*  insert into TempEpisodeTableName select distinct A.scrseriesepisode as episode_id
               from
               (select scrseriesepisode from epg2vdr.events where (scrseriesepisode > 0) and (scrseriesid = SeriesID)
                union all
                select episode_id as scrseriesepisode from epg2vdr.recordinglist
                where (episode_id > 0) and (series_id = SeriesID) and (uuid = VDRUUID)) A */

    fillTempEpisodeTable = new cDbStatement(connection);
    fillTempEpisodeTable->build("insert into %s select distinct A.%s as %s from ",
                                TempEpisodeTableName.c_str(),
                                tEvents->getField("SCRSERIESEPISODE")->getDbName(),
                                tEpisodes->getField("EPISODEID")->getDbName());
    fillTempEpisodeTable->build("(select %s from %s where (%s > 0) and (",
                                tEvents->getField("SCRSERIESEPISODE")->getDbName(),
                                tEvents->TableName(),
                                tEvents->getField("SCRSERIESEPISODE")->getDbName());
    fillTempEpisodeTable->bind(&events_ScrSeriesId, cDBS::bndIn | cDBS::bndSet);
    fillTempEpisodeTable->build(") union all select %s as %s from %s ",
                                tRecordingList->getField("SCRSERIESEPISODE")->getDbName(),
                                tEvents->getField("SCRSERIESEPISODE")->getDbName(),
                                tRecordingList->TableName());
    fillTempEpisodeTable->build("where (%s > 0) and (",
                                tRecordingList->getField("SCRSERIESEPISODE")->getDbName());
    fillTempEpisodeTable->bind(&recSeriesid, cDBS::bndIn | cDBS::bndSet);
    fillTempEpisodeTable->build(") and (");
    fillTempEpisodeTable->bind(&vdr_uuid, cDBS::bndIn | cDBS::bndSet);
    fillTempEpisodeTable->build(")) A");
    status += fillTempEpisodeTable->prepare();

    // get all used series (events/recordings), last_update of used episodes and max(scrsp) of all events
/*  select A.series_id, A.series_name, A.series_last_scraped, A.series_last_updated, A.series_overview, A.series_firstaired, A.series_network,
     A.series_imdb_id, A.series_genre,A.series_rating, A.series_status, B.episode_last_updated, B.scrsp from epg2vdr.series A
    right join (select C.series_id,max(D.episode_last_updated) as episode_last_updated,max(C.scrsp) as scrsp from
                       (select E.series_id,E.episode_id, max(E.scrsp) as scrsp from
                           (select scrseriesid as series_id, scrseriesepisode as episode_id, scrsp from epg2vdr.events where (scrseriesid>0)
                            union all
                            select series_id, episode_id, scrsp from epg2vdr.recordings
                            where (uuid = "4E231345-9219-4FEC-8666-09C60D536E68") and (series_id > 0)) E group by series_id,episode_id) C
                left join epg2vdr.series_episode D
                on (C.episode_id = D.episode_id and C.episode_id>0)
                group by C.series_id) B
    on (A.series_id = B.series_id)
    order by A.series_id */

    selectReadSeriesInit = new cDbStatement(tSeries);
    selectReadSeriesInit->build("select ");
    selectReadSeriesInit->bind("SERIESID", cDBS::bndOut, "A.");
    selectReadSeriesInit->bind("SERIESNAME", cDBS::bndOut, ", A.");
    selectReadSeriesInit->bind("SERIESLASTSCRAPED", cDBS::bndOut, ", A.");
    selectReadSeriesInit->bind("SERIESLASTUPDATED", cDBS::bndOut, ", A.");
    selectReadSeriesInit->bind("SERIESOVERVIEW", cDBS::bndOut, ", A.");
    selectReadSeriesInit->bind("SERIESFIRSTAIRED", cDBS::bndOut, ", A.");
    selectReadSeriesInit->bind("SERIESNETWORK", cDBS::bndOut, ", A.");
    selectReadSeriesInit->bind("SERIESIMDBID", cDBS::bndOut, ", A.");
    selectReadSeriesInit->bind("SERIESGENRE", cDBS::bndOut, ", A.");
    selectReadSeriesInit->bind("SERIESRATING", cDBS::bndOut, ", A.");
    selectReadSeriesInit->bind("SERIESSTATUS", cDBS::bndOut, ", A.");
    selectReadSeriesInit->bind(&episode_LastUpdate, cDBS::bndOut, ", B."); // max last update time of all episodes of current series with event/recording
    selectReadSeriesInit->bind(&event_scrsp, cDBS::bndOut, ", B."); // max last scrsp time of all events/recordings of current series
    selectReadSeriesInit->build(" from %s A ",
                                tSeries->TableName());
    selectReadSeriesInit->build("right join (select C.%s, max(D.%s) as %s, max(C.%s) as %s from ",
                                tSeries->getField("SERIESID")->getDbName(),
                                tEpisodes->getField("EPISODELASTUPDATED")->getDbName(),
                                tEpisodes->getField("EPISODELASTUPDATED")->getDbName(),
                                tEvents->getField("SCRSP")->getDbName(),
                                tEvents->getField("SCRSP")->getDbName());
    selectReadSeriesInit->build("(select E.%s,E.%s, max(E.%s) as %s from ",
                                tSeries->getField("SERIESID")->getDbName(),
                                tEpisodes->getField("EPISODEID")->getDbName(),
                                tEvents->getField("SCRSP")->getDbName(),
                                tEvents->getField("SCRSP")->getDbName());
    selectReadSeriesInit->build("(select %s as %s, %s as %s, %s from %s where (%s>0)",
                                tEvents->getField("SCRSERIESID")->getDbName(),
                                tSeries->getField("SERIESID")->getDbName(),
                                tEvents->getField("SCRSERIESEPISODE")->getDbName(),
                                tEpisodes->getField("EPISODEID")->getDbName(),
                                tEvents->getField("SCRSP")->getDbName(),
                                tEvents->TableName(),
                                tEvents->getField("SCRSERIESID")->getDbName());
    selectReadSeriesInit->build(" union all ");
    selectReadSeriesInit->build("select %s, %s, %s from %s where (",
                                tRecordingList->getField("SCRSERIESID")->getDbName(),
                                tRecordingList->getField("SCRSERIESEPISODE")->getDbName(),
                                tEvents->getField("SCRSP")->getDbName(),
                                tRecordingList->TableName());
    selectReadSeriesInit->bind(&vdr_uuid, cDBS::bndIn | cDBS::bndSet);
    selectReadSeriesInit->build(") and (%s > 0)",
                                tRecordingList->getField("SCRSERIESID")->getDbName());
    selectReadSeriesInit->build(") E group by %s,%s) C ",
                                tSeries->getField("SERIESID")->getDbName(),
                                tEpisodes->getField("EPISODEID")->getDbName());
    selectReadSeriesInit->build("left join %s D on (C.%s = D.%s and C.%s>0) ",
                                tEpisodes->TableName(),
                                tEpisodes->getField("EPISODEID")->getDbName(),
                                tEpisodes->getField("EPISODEID")->getDbName(),
                                tEpisodes->getField("EPISODEID")->getDbName());
    selectReadSeriesInit->build("group by C.%s) B on (A.%s = B.%s) order by A.%s",
                                tSeries->getField("SERIESID")->getDbName(),
                                tSeries->getField("SERIESID")->getDbName(),
                                tSeries->getField("SERIESID")->getDbName(),
                                tSeries->getField("SERIESID")->getDbName());
    status += selectReadSeriesInit->prepare();


    // get all used series (events/recordings), last_update of used episodes and max(scrsp) of all events, filtered by lastscrsp
/*  select A.series_id, A.series_name, A.series_last_scraped, A.series_last_updated, A.series_overview, A.series_firstaired, A.series_network,
     A.series_imdb_id, A.series_genre,A.series_rating, A.series_status, B.episode_last_updated, B.scrsp from epg2vdr.series A
    right join (select C.series_id,max(D.episode_last_updated) as episode_last_updated,max(C.scrsp) as scrsp from
                       (select E.series_id,E.episode_id, max(E.scrsp) as scrsp from
                           (select scrseriesid as series_id, scrseriesepisode as episode_id, scrsp from epg2vdr.events where (scrseriesid>0) and (scrsp>LASTSCRSP)
                            union all
                            select series_id, episode_id, scrsp from epg2vdr.recordings
                            where (uuid = "4E231345-9219-4FEC-8666-09C60D536E68") and (series_id > 0) and (scrsp>LASTSCRSP)) E group by series_id,episode_id) C
                left join epg2vdr.series_episode D
                on (C.episode_id = D.episode_id and C.episode_id>0)
                group by C.series_id) B
    on (A.series_id = B.series_id)
    order by A.series_id */

    selectReadSeriesLastScrsp = new cDbStatement(tSeries);
    selectReadSeriesLastScrsp->build("select ");
    selectReadSeriesLastScrsp->bind("SERIESID", cDBS::bndOut, "A.");
    selectReadSeriesLastScrsp->bind("SERIESNAME", cDBS::bndOut, ", A.");
    selectReadSeriesLastScrsp->bind("SERIESLASTSCRAPED", cDBS::bndOut, ", A.");
    selectReadSeriesLastScrsp->bind("SERIESLASTUPDATED", cDBS::bndOut, ", A.");
    selectReadSeriesLastScrsp->bind("SERIESOVERVIEW", cDBS::bndOut, ", A.");
    selectReadSeriesLastScrsp->bind("SERIESFIRSTAIRED", cDBS::bndOut, ", A.");
    selectReadSeriesLastScrsp->bind("SERIESNETWORK", cDBS::bndOut, ", A.");
    selectReadSeriesLastScrsp->bind("SERIESIMDBID", cDBS::bndOut, ", A.");
    selectReadSeriesLastScrsp->bind("SERIESGENRE", cDBS::bndOut, ", A.");
    selectReadSeriesLastScrsp->bind("SERIESRATING", cDBS::bndOut, ", A.");
    selectReadSeriesLastScrsp->bind("SERIESSTATUS", cDBS::bndOut, ", A.");
    selectReadSeriesLastScrsp->bind(&episode_LastUpdate, cDBS::bndOut, ", B."); // max last update time of all episodes of current series with event/recording
    selectReadSeriesLastScrsp->bind(&event_scrsp, cDBS::bndOut, ", B."); // max last scrsp time of all events/recordings of current series
    selectReadSeriesLastScrsp->build(" from %s A ",
                                     tSeries->TableName());
    selectReadSeriesLastScrsp->build("right join (select C.%s, max(D.%s) as %s, max(C.%s) as %s from ",
                                     tSeries->getField("SERIESID")->getDbName(),
                                     tEpisodes->getField("EPISODELASTUPDATED")->getDbName(),
                                     tEpisodes->getField("EPISODELASTUPDATED")->getDbName(),
                                     tEvents->getField("SCRSP")->getDbName(),
                                     tEvents->getField("SCRSP")->getDbName());
    selectReadSeriesLastScrsp->build("(select E.%s,E.%s, max(E.%s) as %s from ",
                                     tSeries->getField("SERIESID")->getDbName(),
                                     tEpisodes->getField("EPISODEID")->getDbName(),
                                     tEvents->getField("SCRSP")->getDbName(),
                                     tEvents->getField("SCRSP")->getDbName());
    selectReadSeriesLastScrsp->build("(select %s as %s, %s as %s, %s from %s where (%s>0) and (",
                                     tEvents->getField("SCRSERIESID")->getDbName(),
                                     tSeries->getField("SERIESID")->getDbName(),
                                     tEvents->getField("SCRSERIESEPISODE")->getDbName(),
                                     tEpisodes->getField("EPISODEID")->getDbName(),
                                     tEvents->getField("SCRSP")->getDbName(),
                                     tEvents->TableName(),
                                     tEvents->getField("SCRSERIESID")->getDbName());
    selectReadSeriesLastScrsp->bindCmp(0, &event_scrsp, ">");
    selectReadSeriesLastScrsp->build(") union all ");
    selectReadSeriesLastScrsp->build("select %s, %s, %s from %s where (",
                                     tRecordingList->getField("SCRSERIESID")->getDbName(),
                                     tRecordingList->getField("SCRSERIESEPISODE")->getDbName(),
                                     tEvents->getField("SCRSP")->getDbName(),
                                     tRecordingList->TableName());
    selectReadSeriesLastScrsp->bind(&vdr_uuid, cDBS::bndIn | cDBS::bndSet);
    selectReadSeriesLastScrsp->build(") and (%s > 0) and (",
                                     tRecordingList->getField("SCRSERIESID")->getDbName());
    selectReadSeriesLastScrsp->bindCmp(0, &event_scrsp, ">");
    selectReadSeriesLastScrsp->build(")) E group by %s,%s) C ",
                                     tSeries->getField("SERIESID")->getDbName(),
                                     tEpisodes->getField("EPISODEID")->getDbName());
    selectReadSeriesLastScrsp->build("left join %s D on (C.%s = D.%s and C.%s>0) ",
                                     tEpisodes->TableName(),
                                     tEpisodes->getField("EPISODEID")->getDbName(),
                                     tEpisodes->getField("EPISODEID")->getDbName(),
                                     tEpisodes->getField("EPISODEID")->getDbName());
    selectReadSeriesLastScrsp->build("group by C.%s) B on (A.%s = B.%s) order by A.%s",
                                     tSeries->getField("SERIESID")->getDbName(),
                                     tSeries->getField("SERIESID")->getDbName(),
                                     tSeries->getField("SERIESID")->getDbName(),
                                     tSeries->getField("SERIESID")->getDbName());
    status += selectReadSeriesLastScrsp->prepare();

    // get all used episodes (with events/recordings) of one series
/*  select A.episode_id,A.episode_number,A.season_number,A.episode_name,A.episode_overview,A.episode_firstaired,A.episode_gueststars from epg2vdr.series_episode A
    right join epg2vdr.usedepisodes B
    on (B.episode_id = A.episode_id) where A.episode_id is not null */

    selectReadEpisodes = new cDbStatement(tEpisodes);
    selectReadEpisodes->build("select ");
    selectReadEpisodes->bind("EPISODEID", cDBS::bndOut, "A.");
    selectReadEpisodes->bind("EPISODENUMBER", cDBS::bndOut, ", A.");
    selectReadEpisodes->bind("SEASONNUMBER", cDBS::bndOut, ", A.");
    selectReadEpisodes->bind("EPISODENAME", cDBS::bndOut, ", A.");
    selectReadEpisodes->bind("EPISODEOVERVIEW", cDBS::bndOut, ", A.");
    selectReadEpisodes->bind("EPISODEFIRSTAIRED", cDBS::bndOut, ", A.");
    selectReadEpisodes->bind("EPISODEGUESTSTARS", cDBS::bndOut, ", A.");
    selectReadEpisodes->bind("EPISODERATING", cDBS::bndOut, ", A.");
    selectReadEpisodes->bind("EPISODELASTUPDATED", cDBS::bndOut, ", A.");
    selectReadEpisodes->bind("SERIESID", cDBS::bndOut, ", A.");
    selectReadEpisodes->build(" from %s A right join %s B ",
                              tEpisodes->TableName(),
                              TempEpisodeTableName.c_str());
    selectReadEpisodes->build("on (B.%s = A.%s) where A.%s is not null",
                              tEpisodes->getField("EPISODEID")->getDbName(),
                              tEpisodes->getField("EPISODEID")->getDbName(),
                              tEpisodes->getField("EPISODEID")->getDbName());
    status += selectReadEpisodes->prepare();

    // get all media data and actor informations of one series (including only episode images for episodes with events/recordings )
/*  select A.series_id,A.episode_id,A.season_number,A.actor_id, A.media_type,A.media_width,A.media_height,C.actor_id,C.actor_name,C.actor_role,C.actor_sortorder
    from epg2vdr.series_media A
    left join epg2vdr.usedepisodes B
    on (B.episode_id = A.episode_id)
    left join (select D.actor_id,D.actor_name,D.actor_role,D.actor_sortorder from epg2vdr.series_actor D
               right join (select actor_id from epg2vdr.series_media where (series_id=SeriesID) and (actor_id>0)) E
               on (D.actor_id = E.actor_id)) C
    on (A.actor_id=C.actor_id)
    where (series_id = SeriesID) and ((B.episode_id is not null) or (A.episode_id = 0))
    order by season_number,actor_sortorder */

    selectSeriesMediaActors = new cDbStatement(tSeriesMedia);
    selectSeriesMediaActors->build("select ");
    selectSeriesMediaActors->bind("SERIESID", cDBS::bndOut, "A.");
    selectSeriesMediaActors->bind("EPISODEID", cDBS::bndOut, ", A.");
    selectSeriesMediaActors->bind("SEASONNUMBER", cDBS::bndOut, ", A.");
    selectSeriesMediaActors->bind("ACTORID", cDBS::bndOut, ", A.");
    selectSeriesMediaActors->bind("MEDIATYPE", cDBS::bndOut, ", A.");
    selectSeriesMediaActors->bind("MEDIAWIDTH", cDBS::bndOut, ", A.");
    selectSeriesMediaActors->bind("MEDIAHEIGHT", cDBS::bndOut, ", A.");
    selectSeriesMediaActors->bind(tSeriesActors,"ACTORID", cDBS::bndOut, ", C.");
    selectSeriesMediaActors->bind(tSeriesActors,"ACTORNAME", cDBS::bndOut, ", C.");
    selectSeriesMediaActors->bind(tSeriesActors,"ACTORROLE", cDBS::bndOut, ", C.");
    selectSeriesMediaActors->bind(tSeriesActors,"SORTORDER", cDBS::bndOut, ", C.");
    selectSeriesMediaActors->build(" from %s A left join %s B on (B.%s = A.%s) ",
                                   tSeriesMedia->TableName(),
                                   TempEpisodeTableName.c_str(),
                                   tEpisodes->getField("EPISODEID")->getDbName(),
                                   tSeriesMedia->getField("EPISODEID")->getDbName());
    selectSeriesMediaActors->build("left join (select D.%s,D.%s,D.%s,D.%s from %s D ",
                                   tSeriesActors->getField("ACTORID")->getDbName(),
                                   tSeriesActors->getField("ACTORNAME")->getDbName(),
                                   tSeriesActors->getField("ACTORROLE")->getDbName(),
                                   tSeriesActors->getField("SORTORDER")->getDbName(),
                                   tSeriesActors->TableName());
    selectSeriesMediaActors->build("right join (select %s from %s where (",
                                   tSeriesMedia->getField("ACTORID")->getDbName(),
                                   tSeriesMedia->TableName());
    selectSeriesMediaActors->bind(&series_id, cDBS::bndIn | cDBS::bndSet);
    selectSeriesMediaActors->build(") and (%s>0)) E on (D.%s = E.%s)) C on (A.%s=C.%s) where (",
                                   tSeriesMedia->getField("ACTORID")->getDbName(),
                                   tSeriesActors->getField("ACTORID")->getDbName(),
                                   tSeriesMedia->getField("ACTORID")->getDbName(),
                                   tSeriesMedia->getField("ACTORID")->getDbName(),
                                   tSeriesMedia->getField("ACTORID")->getDbName());
    selectSeriesMediaActors->bind(&series_id, cDBS::bndIn | cDBS::bndSet);
    selectSeriesMediaActors->build(") and ((B.%s is not null) or (A.%s = 0))  order by %s,%s",
                                   tEpisodes->getField("EPISODEID")->getDbName(),
                                   tSeriesMedia->getField("EPISODEID")->getDbName(),
                                   tSeriesMedia->getField("SEASONNUMBER")->getDbName(),
                                   tSeriesActors->getField("SORTORDER")->getDbName());
    status += selectSeriesMediaActors->prepare();

    selectSeriesImage = new cDbStatement(tSeriesMedia);
    selectSeriesImage->build("select ");
    selectSeriesImage->bind("MEDIAWIDTH", cDBS::bndOut);
    selectSeriesImage->bind("MEDIAHEIGHT", cDBS::bndOut, ", ");
    selectSeriesImage->bind("MEDIACONTENT", cDBS::bndOut, ", ");
    selectSeriesImage->build(", length(");
    selectSeriesImage->bind(&imageSize, cDBS::bndOut);
    selectSeriesImage->build(")");
    selectSeriesImage->build(" from %s where ", tSeriesMedia->TableName());
    selectSeriesImage->bind("SERIESID", cDBS::bndIn | cDBS::bndSet);
    selectSeriesImage->bind("SEASONNUMBER", cDBS::bndIn | cDBS::bndSet, " and ");
    selectSeriesImage->bind("EPISODEID", cDBS::bndIn | cDBS::bndSet, " and ");
    selectSeriesImage->bind("ACTORID", cDBS::bndIn | cDBS::bndSet, " and ");
    selectSeriesImage->bind("MEDIATYPE", cDBS::bndIn | cDBS::bndSet, " and ");
    status += selectSeriesImage->prepare();

    // get all used movies (events/recordings) and max(scrsp) of all events
/*  select A.movie_id, A.movie_title, A.movie_original_title, A.movie_tagline, A.movie_overview, A.movie_adult, A.movie_collection_name, A.movie_budget,
     A.movie_revenue, A.movie_genres, A.movie_homepage, A.movie_release_date, A.movie_runtime, A.movie_popularity, A.movie_vote_average, B.scrsp from epg2vdr.movie A
    right join (select C.movie_id, max(C.scrsp) as scrsp from
                           (select scrmovieid as movie_id, scrsp from events where (scrmovieid > 0)
                            union all
                            select scrmovieid, scrsp from recordinglist
                            where  (movie_id > 0)) C
                              group by movie_id) B on (A.movie_id = B.movie_id)
                            order by A.movie_id */

    selectReadMoviesInit = new cDbStatement(tMovies);
    selectReadMoviesInit->build("select ");
    selectReadMoviesInit->bind("MOVIEID", cDBS::bndOut, "A.");
    selectReadMoviesInit->bind("TITLE", cDBS::bndOut, ", A.");
    selectReadMoviesInit->bind("ORIGINALTITLE", cDBS::bndOut, ", A.");
    selectReadMoviesInit->bind("TAGLINE", cDBS::bndOut, ", A.");
    selectReadMoviesInit->bind("OVERVIEW", cDBS::bndOut, ", A.");
    selectReadMoviesInit->bind("ISADULT", cDBS::bndOut, ", A.");
    selectReadMoviesInit->bind("COLLECTIONID", cDBS::bndOut, ", A.");
    selectReadMoviesInit->bind("COLLECTIONNAME", cDBS::bndOut, ", A.");
    selectReadMoviesInit->bind("BUDGET", cDBS::bndOut, ", A.");
    selectReadMoviesInit->bind("REVENUE", cDBS::bndOut, ", A.");
    selectReadMoviesInit->bind("GENRES", cDBS::bndOut, ", A.");
    selectReadMoviesInit->bind("HOMEPAGE", cDBS::bndOut, ", A.");
    selectReadMoviesInit->bind("RELEAASEDATE", cDBS::bndOut, ", A.");
    selectReadMoviesInit->bind("RUNTIME", cDBS::bndOut, ", A.");
    selectReadMoviesInit->bind("POPULARITY", cDBS::bndOut, ", A.");
    selectReadMoviesInit->bind("VOTEAVERAGE", cDBS::bndOut, ", A.");
    selectReadMoviesInit->bind(&event_scrsp, cDBS::bndOut, ", B."); // max last scrsp time of all events/recordings of current movie
    selectReadMoviesInit->build(" from %s A ",
                                tMovies->TableName());
    selectReadMoviesInit->build("right join (select C.%s, max(C.%s) as %s from ",
                                tMovies->getField("MOVIEID")->getDbName(),
                                tEvents->getField("SCRSP")->getDbName(),
                                tEvents->getField("SCRSP")->getDbName());
    selectReadMoviesInit->build("(select %s as %s, %s from %s where (%s>0)",
                                tEvents->getField("SCRMOVIEID")->getDbName(),
                                tMovies->getField("MOVIEID")->getDbName(),
                                tEvents->getField("SCRSP")->getDbName(),
                                tEvents->TableName(),
                                tEvents->getField("SCRMOVIEID")->getDbName());
    selectReadMoviesInit->build(" union all ");
    selectReadMoviesInit->build("select %s, %s from %s where ",
                                tRecordingList->getField("SCRMOVIEID")->getDbName(),
                                tEvents->getField("SCRSP")->getDbName(),
                                tRecordingList->TableName());
    selectReadMoviesInit->build(" (%s > 0)",
                                tRecordingList->getField("SCRMOVIEID")->getDbName());
    selectReadMoviesInit->build(") C group by %s) B on (A.%s = B.%s) order by A.%s",
                                tMovies->getField("MOVIEID")->getDbName(),
                                tMovies->getField("MOVIEID")->getDbName(),
                                tMovies->getField("MOVIEID")->getDbName(),
                                tMovies->getField("MOVIEID")->getDbName());
    status += selectReadMoviesInit->prepare();

    // get all used movies (events/recordings) and max(scrsp) of all events, filtered by lastscrsp
/*  select A.movie_id, A.movie_title, A.movie_original_title, A.movie_tagline, A.movie_overview, A.movie_adult, A.movie_collection_name, A.movie_budget,
     A.movie_revenue, A.movie_genres, A.movie_homepage, A.movie_release_date, A.movie_runtime, A.movie_popularity, A.movie_vote_average, B.scrsp from epg2vdr.movie A
    right join (select C.movie_id, max(C.scrsp) as scrsp from
                           (select scrmovieid as movie_id, scrsp from epg2vdr.events where (scrmovieid>0) and (scrsp>LASTSCRSP)
                            union all
                            select movie_id, scrsp from epg2vdr.recordings
                            where (uuid = "4E231345-9219-4FEC-8666-09C60D536E68") and (movie_id > 0) and (scrsp>LASTSCRSP)) C group by movie_id) B
    on (A.movie_id = B.movie_id)
    order by A.movie_id */

    selectReadMoviesLastScrsp = new cDbStatement(tMovies);
    selectReadMoviesLastScrsp->build("select ");
    selectReadMoviesLastScrsp->bind("MOVIEID", cDBS::bndOut, "A.");
    selectReadMoviesLastScrsp->bind("TITLE", cDBS::bndOut, ", A.");
    selectReadMoviesLastScrsp->bind("ORIGINALTITLE", cDBS::bndOut, ", A.");
    selectReadMoviesLastScrsp->bind("TAGLINE", cDBS::bndOut, ", A.");
    selectReadMoviesLastScrsp->bind("OVERVIEW", cDBS::bndOut, ", A.");
    selectReadMoviesLastScrsp->bind("ISADULT", cDBS::bndOut, ", A.");
    selectReadMoviesLastScrsp->bind("COLLECTIONID", cDBS::bndOut, ", A.");
    selectReadMoviesLastScrsp->bind("COLLECTIONNAME", cDBS::bndOut, ", A.");
    selectReadMoviesLastScrsp->bind("BUDGET", cDBS::bndOut, ", A.");
    selectReadMoviesLastScrsp->bind("REVENUE", cDBS::bndOut, ", A.");
    selectReadMoviesLastScrsp->bind("GENRES", cDBS::bndOut, ", A.");
    selectReadMoviesLastScrsp->bind("HOMEPAGE", cDBS::bndOut, ", A.");
    selectReadMoviesLastScrsp->bind("RELEAASEDATE", cDBS::bndOut, ", A.");
    selectReadMoviesLastScrsp->bind("RUNTIME", cDBS::bndOut, ", A.");
    selectReadMoviesLastScrsp->bind("POPULARITY", cDBS::bndOut, ", A.");
    selectReadMoviesLastScrsp->bind("VOTEAVERAGE", cDBS::bndOut, ", A.");
    selectReadMoviesLastScrsp->bind(&event_scrsp, cDBS::bndOut, ", B."); // max last scrsp time of all events/recordings of current movie
    selectReadMoviesLastScrsp->build(" from %s A ",
                                     tMovies->TableName());
    selectReadMoviesLastScrsp->build("right join (select C.%s, max(C.%s) as %s from ",
                                     tMovies->getField("MOVIEID")->getDbName(),
                                     tEvents->getField("SCRSP")->getDbName(),
                                     tEvents->getField("SCRSP")->getDbName());
    selectReadMoviesLastScrsp->build("(select %s as %s, %s from %s where (%s>0) and (",
                                     tEvents->getField("SCRMOVIEID")->getDbName(),
                                     tMovies->getField("MOVIEID")->getDbName(),
                                     tEvents->getField("SCRSP")->getDbName(),
                                     tEvents->TableName(),
                                     tEvents->getField("SCRMOVIEID")->getDbName());
    selectReadMoviesLastScrsp->bindCmp(0, &event_scrsp, ">");
    selectReadMoviesLastScrsp->build(") union all ");
    selectReadMoviesLastScrsp->build("select %s, %s from %s where ",
                                     tRecordingList->getField("SCRMOVIEID")->getDbName(),
                                     tEvents->getField("SCRSP")->getDbName(),
                                     tRecordingList->TableName());
    selectReadMoviesLastScrsp->build(" (%s > 0) and (",
                                     tRecordingList->getField("SCRMOVIEID")->getDbName());
    selectReadMoviesLastScrsp->bindCmp(0, &event_scrsp, ">");
    selectReadMoviesLastScrsp->build(")) C group by %s) B on (A.%s = B.%s) order by A.%s",
                                     tMovies->getField("MOVIEID")->getDbName(),
                                     tMovies->getField("MOVIEID")->getDbName(),
                                     tMovies->getField("MOVIEID")->getDbName(),
                                     tMovies->getField("MOVIEID")->getDbName());
    status += selectReadMoviesLastScrsp->prepare();

    selectMovieMediaFast = new cDbStatement(tMovieMedia);
    selectMovieMediaFast->build("select ");
    selectMovieMediaFast->bind("ACTORID", cDBS::bndOut);
    selectMovieMediaFast->bind("MEDIATYPE", cDBS::bndOut, ", ");
    selectMovieMediaFast->bind("MEDIAWIDTH", cDBS::bndOut, ", ");
    selectMovieMediaFast->bind("MEDIAHEIGHT", cDBS::bndOut, ", ");
    selectMovieMediaFast->build(" from %s where ", tMovieMedia->TableName());
    selectMovieMediaFast->bind("MOVIEID", cDBS::bndIn | cDBS::bndSet);
    status += selectMovieMediaFast->prepare();

    selectMovieImage = new cDbStatement(tMovieMedia);
    selectMovieImage->build("select ");
    selectMovieImage->bind("MEDIACONTENT", cDBS::bndOut);
    selectMovieImage->bind("MEDIAWIDTH", cDBS::bndOut, ", ");
    selectMovieImage->bind("MEDIAHEIGHT", cDBS::bndOut, ", ");
    selectMovieImage->build(", length(");
    selectMovieImage->bind(&imageSize, cDBS::bndOut);
    selectMovieImage->build(")");
    selectMovieImage->build(" from %s where ", tMovieMedia->TableName());
    selectMovieImage->bind("MOVIEID", cDBS::bndIn | cDBS::bndSet);
    selectMovieImage->bind("ACTORID", cDBS::bndIn | cDBS::bndSet, " and ");
    selectMovieImage->bind("MEDIATYPE", cDBS::bndIn | cDBS::bndSet, " and ");
    status += selectMovieImage->prepare();

    return status;
}

// load database dictionary
int cUpdate::LoadDBDict(void) {
   char* dictPath = 0;

   dbDict.forget();       // clear all old data

   asprintf(&dictPath, "%sepg.dat", cPlugin::ConfigDirectory("scraper2vdr/"));
   dbDict.setFilterFromNameFct(toFieldFilter);

   if (dbDict.in(dictPath, ffScraper2Vdr) != success)
   {
      tell(0, "Fatal: Dictionary not loaded, aborting!");
      return fail;
   }

   tell(0, "Dictionary '%s' loaded", dictPath);
   free(dictPath);

   return success;
}

int cUpdate::exitDb() {

    delete selectReadScrapedEvents;     selectReadScrapedEvents = 0;
    delete selectReadScrapedEventsInit; selectReadScrapedEventsInit = 0;
    delete selectMovieActors;           selectMovieActors = 0;
    delete selectRecordings;            selectRecordings = 0;
    delete clearTempEpisodeTable;       clearTempEpisodeTable = 0;
    delete fillTempEpisodeTable;        fillTempEpisodeTable = 0;
    delete selectReadSeriesInit;        selectReadSeriesInit = 0;
    delete selectReadSeriesLastScrsp;   selectReadSeriesLastScrsp = 0;
    delete selectReadEpisodes;          selectReadEpisodes = 0;
    delete selectSeriesMediaActors;     selectSeriesMediaActors = 0;
    delete selectSeriesImage;           selectSeriesImage = 0;
    delete selectReadMoviesInit;        selectReadMoviesInit = 0;
    delete selectReadMoviesLastScrsp;   selectReadMoviesLastScrsp = 0;
    delete selectMovieMediaFast;        selectMovieMediaFast = 0;
    delete selectMovieImage;            selectMovieImage = 0;

    delete vdrDb;          vdrDb = 0;
    delete tEvents;        tEvents = 0;
    delete tSeries;        tSeries = 0;
    delete tEpisodes;      tEpisodes = 0;
    delete tSeriesMedia;   tSeriesMedia = 0;
    delete tSeriesActors;  tSeriesActors = 0;
    delete tMovies;        tMovies = 0;
    delete tMovieActor;    tMovieActor = 0;
    delete tMovieActors;   tMovieActors = 0;
    delete tMovieMedia;    tMovieMedia = 0;
    delete tRecordingList; tRecordingList = 0;

    // try to delete temp. episode cache table

    if (dbConnected())
        connection->query("%s",TempEpisodeDeleteStatement.c_str());

    delete connection;    connection = 0;

    return done;
}

void cUpdate::Stop() {
    loopActive = false;
    waitCondition.Broadcast();
    Cancel(1);
    while (Active())
        cCondWait::SleepMs(10);
}

int cUpdate::CheckConnection(bool init, int& timeout) {
    if (forceReconnect) {
        forceReconnect = false;
        if (dbConnected(false)) {
            tell(0, "Force re-connect to database!");
            exitDb(); // force closed DB
        }
        // apply new settings
        cDbConnection::setEncoding(withutf8 ? "utf8": "latin1"); // mysql uses latin1 for ISO8851-1
        cDbConnection::setHost(scraper2VdrConfig.dbHost);
        cDbConnection::setPort(scraper2VdrConfig.dbPort);
        cDbConnection::setName(scraper2VdrConfig.dbName);
        cDbConnection::setUser(scraper2VdrConfig.dbUser);
        cDbConnection::setPass(scraper2VdrConfig.dbPass);
        cDbConnection::setConfPath(cPlugin::ConfigDirectory("scraper2vdr/"));
    }
    static int retry = 0;
    timeout = init ? 30 : 60; // 30s during init, otherwise 60s (this is the intervall of update thread)
    // check connection
    if (!dbConnected(yes)) {
        // try to connect
        tell(0, "Trying to re-connect to database!");
        retry++;
        if (initDb() != success) {
            tell(0, "Retry #%d failed, retrying in %d seconds!", retry, timeout);
            exitDb();
            return fail;
        }
        retry = 0;
        tell(0, "Connection established successfull!");
    }
    return success;
}

bool cUpdate::CheckEpgdBusy(void) {
    int busy = false;
    vdrDb->clear();
    vdrDb->setValue("UUID", EPGDNAME);

    if (vdrDb->find()) {
        cEpgdState::State epgdState = cEpgdState::toState(vdrDb->getStrValue("STATE"));
        // ignore esBusyImages until we don't write this table
        if (epgdState >= cEpgdState::esBusy && epgdState < cEpgdState::esBusyImages)
            busy = true;
    }

    vdrDb->reset();
    return busy;
}

//***************************************************************************
// Init Uuid
//***************************************************************************

int cUpdate::initUuid(int timeout)
{
   if (isEmpty(scraper2VdrConfig.uuid))
   {
      Epg2vdr_UUID_v1_0 req;

      cPlugin* epg2vdrPlugin = cPluginManager::GetPlugin("epg2vdr");

      if (!epg2vdrPlugin)
      {
         tell(0, "Warning: Can't find %s to query uuid, retrying in %d seconds",
              epg2vdrPlugin ? "service " EPG2VDR_UUID_SERVICE : "plugin epg2vdr", timeout);
         return fail;
      }

      req.uuid = 0;

      if (!epg2vdrPlugin->Service(EPG2VDR_UUID_SERVICE, &req))
      {
         tell(0, "Error: Call of service '%s' failed, retrying in %d seconds",
              EPG2VDR_UUID_SERVICE, timeout);
         return fail;
      }

      tell(0, "Got UUID '%s' by epg2vdr", req.uuid);

      sstrcpy(scraper2VdrConfig.uuid, req.uuid, sizeUuid+TB);
   }

   return done;
}

int cUpdate::ReadScrapedEvents(void) {
    int eventId = 0;
    int seriesId = 0;
    int episodeId = 0;
    int movieId = 0;
    string channelId = "";
    int numNew = 0;

    cDbStatement* select = lastScrap > 0 ? selectReadScrapedEvents : selectReadScrapedEventsInit;

    tEvents->clear();
    tEvents->setValue("SCRSP", lastScrap);

    lastWait = GetTimems(); // init time for CheckRunningAndWait
    for (int res = select->find(); res; res = select->fetch() && CheckRunningAndWait()) {
        eventId = tEvents->getIntValue("MASTERID");
        channelId = tEvents->getStrValue("CHANNELID");
        seriesId = tEvents->getIntValue("SCRSERIESID");
        episodeId = tEvents->getIntValue("SCRSERIESEPISODE");
        movieId = tEvents->getIntValue("SCRMOVIEID");
        scrapManager->AddEvent(eventId, channelId, seriesId, episodeId, movieId);
        lastScrap = std::max(lastScrap, (int)tEvents->getIntValue("SCRSP"));
        numNew++;
    }

    select->freeResult();

    return numNew;
}

//***************************************************************************
// SERIES
//***************************************************************************

// read all series with event from sql-db
int cUpdate::ReadSeriesFast(long &maxscrsp) {
    // try to create directorys first (if not exists)
    if (!CreateDirectory(scraper2VdrConfig.imageDir))
        return 0;
    if (!CreateDirectory(imgPathSeries))
        return 0;

    cTVDBSeries *series;
    bool isNew = false;
    int numNew = 0;
    int i = 0;

    vdr_uuid.setValue(scraper2VdrConfig.uuid); // search all recordigs of this vdr uuid
    tSeries->clear();
    event_scrsp.setValue(maxscrsp); // use last max scrsp as start value for filter

    cDbStatement* select = maxscrsp > 0 ? selectReadSeriesLastScrsp : selectReadSeriesInit;

    lastWait = GetTimems(); // init time for CheckRunningAndWait
    long lastLog = GetTimems(); // init time for logging
    for (int res = select->find(); res; res = select->fetch() && CheckRunningAndWait()) {
        series = scrapManager->GetSeries(tSeries->getIntValue("SERIESID"));
        if (!series) {
            // add new series with information of current row
            series = scrapManager->AddSeries(tSeries);
            series->lastepisodeupdate = episode_LastUpdate.getIntValue();
            series->lastscraped = event_scrsp.getIntValue();
            isNew = true;
        } else {
            // check if something changed for series
            isNew = false;
            series->updateimages = (series->lastupdate < tSeries->getIntValue("SERIESLASTUPDATED")) || // series get updated on thetvdb
                                   (series->lastepisodeupdate < episode_LastUpdate.getIntValue()) || // min one used episode get updated on thetvdb
                                   forceFullUpdate; // force reload everything
            series->updatecontent = (series->updateimages) || (series->lastscraped < event_scrsp.getIntValue()) || forceFullUpdate; // new events or recodings with this series
            if ((series->updateimages) || (series->updatecontent)) { // something to refresh for series
                scrapManager->UpdateSeries(series,tSeries); // update series
                series->lastepisodeupdate = episode_LastUpdate.getIntValue();
                series->lastscraped = event_scrsp.getIntValue();
                isNew = true;
            }
        }
        maxscrsp = std::max(maxscrsp, series->lastscraped);
        i++;
        if (GetTimeDiffms(lastLog)>LogPeriode) {
            tell(1, "Got %d series, continuing...", i);
            lastLog = GetTimems();
        }
        if (isNew)
            numNew++;
    }
    select->freeResult();
    return numNew;
}

// read all episodes/actors/media from all series where series->updatecontent = true
int cUpdate::ReadSeriesContentFast(int &newImages, int &newSeasonPoster) {
    newImages = 0;
    newSeasonPoster = 0;
    cTVDBSeries *series;
    int newValues = 0;
    int newValues2 = 0;
    int numNew = 0;
    int seriescount = scrapManager->GetNumSeries();
    int i = 0;
    tSeries->clear();
    lastWait = GetTimems(); // init time for CheckRunningAndWait
    long lastLog = GetTimems(); // init time for logging
    for (int res = scrapManager->GetSeriesFirst(series); res; res = scrapManager->GetSeriesNext(series) && CheckRunningAndWait()) {
        if (series->updatecontent) {
            // update existing episodes first
            newValues = ReadEpisodesContentFast(series);
            if (newValues > 0)
                series->updateimages = true; // have to search for new images, because we found new episodes
            numNew = numNew + newValues;
            series->updatecontent = false; // reset update flag
        }

        // check if we have to update images
        if (series->updateimages) {
            ReadSeriesMediaFast(series,newValues,newValues2);
            if (newValues+newValues2 == 0)
                series->updateimages = false; // no new images for this series, reset load image flag -> series get not used in ReadSeriesImagesFast
            newImages = newImages + newValues;
            newSeasonPoster = newSeasonPoster + newValues2;
        }

        i++;
        if (GetTimeDiffms(lastLog)>LogPeriode) {
            tell(1, "Handled %d of %d series, continuing...", i, seriescount);
            lastLog = GetTimems();
        }
    }
    return numNew;
}

// read all episodes for current series with event or recording from sql-db
int cUpdate::ReadEpisodesContentFast(cTVDBSeries *series) {
    cTVDBEpisode *episode;
    int numNew = 0;

    tEpisodes->clear();
    recSeriesid.setValue(series->id); // we would search for all episodes for this series
    events_ScrSeriesId.setValue(series->id); // we would search for all events for this series
    vdr_uuid.setValue(scraper2VdrConfig.uuid); // search all recordigs of this vdr uuid

    clearTempEpisodeTable->find(); // clear temp table
    fillTempEpisodeTable->find(); // fill temp table using current series id
    for (int res = selectReadEpisodes->find(); res; res = selectReadEpisodes->fetch() && CheckRunningAndWait()) {
        // search for episode (I could call AddEpisode, because this will overwrite existing values, but then I could not count new episodes)
        episode = series->GetEpisode(tEpisodes->getIntValue("EPISODEID"));
        if (!episode) {
            scrapManager->AddSeriesEpisode(series,tEpisodes);
            numNew++;
        } else {
            if ((episode->lastupdate < tEpisodes->getIntValue("EPISODELASTUPDATED")) || forceFullUpdate) {
                // updated series
                scrapManager->UpdateSeriesEpisode(episode,tEpisodes); // update episode
                numNew++;
            }
        }
    }
    selectReadEpisodes->freeResult();
    return numNew;
}

// read all images of series (also create actors if not available yet)
void cUpdate::ReadSeriesMediaFast(cTVDBSeries *series, int &newImages, int &newSeasonPoster) {
    newImages = 0;
    newSeasonPoster = 0;
    bool isFirst = true;
    int mediaType = 0;
    std::stringstream imageName("");
    imageName << imgPathSeries << "/" << series->id;
    string seriesPath = imageName.str();
    string thumbName = "";

    bool imgNeedRefresh = false;
    int imgWidth = 0;
    int imgHeight = 0;
    int originalWidth = 0;
    int originalHeight = 0;
    int thbWidth = 0;
    int thbHeight = 0;
    int season;

    cTVDBActor *actor;
    cTVDBEpisode *episode;

    tSeriesActors->clear();
    tSeriesMedia->clear();
    series_id.setValue(series->id); // we would search for all media for this series

    for (int res = selectSeriesMediaActors->find(); res; res = selectSeriesMediaActors->fetch() && CheckRunningAndWait()) {
        if (isFirst) {
            if (!CreateDirectory(seriesPath))
                return;
            // fill lastupdated list first
            fileDateManager.LoadFileDateList(seriesPath,false); // ignore read errors (try to overwrite with new file) and mark all old values from file as not used
        }
        isFirst = false;

        mediaType = tSeriesMedia->getIntValue("MEDIATYPE");
        originalWidth = tSeriesMedia->getIntValue("MEDIAWIDTH");
        originalHeight = tSeriesMedia->getIntValue("MEDIAHEIGHT");
        GetUsedImageSize(originalWidth, originalHeight,
                        (mediaType >= msPoster1) && (mediaType <= msPoster3),mediaType == msSeasonPoster,
                         imgWidth, imgHeight);
        imgNeedRefresh = false;
        // check type of media first
        switch (mediaType) {
            case msBanner1:
                imgNeedRefresh = fileDateManager.CheckImageNeedRefresh("banner1.jpg", series->lastupdate) || forceFullUpdate;
                CheckSeriesMedia(series, mediaType, imgWidth, imgHeight,"banner1.jpg", 0, imgNeedRefresh);
                break;
            case msBanner2:
                imgNeedRefresh = fileDateManager.CheckImageNeedRefresh("banner2.jpg", series->lastupdate) || forceFullUpdate;
                CheckSeriesMedia(series, mediaType, imgWidth, imgHeight,"banner2.jpg", 0, imgNeedRefresh);
                break;
            case msBanner3:
                imgNeedRefresh = fileDateManager.CheckImageNeedRefresh("banner3.jpg", series->lastupdate) || forceFullUpdate;
                CheckSeriesMedia(series, mediaType, imgWidth, imgHeight,"banner3.jpg", 0, imgNeedRefresh);
                break;
            case msPoster1:
                imgNeedRefresh = fileDateManager.CheckImageNeedRefreshThumb("poster1.jpg", "poster_thumb.jpg", series->lastupdate) || forceFullUpdate;
                CheckSeriesMedia(series, mediaType, imgWidth, imgHeight, "poster1.jpg", 0, imgNeedRefresh);
                CalcThumbSize(imgWidth, imgHeight, scraper2VdrConfig.thumbHeight, thbWidth, thbHeight);
                CheckSeriesMedia(series, msPosterThumb, thbWidth, thbHeight, "poster_thumb.jpg", 0, imgNeedRefresh);
                break;
            case msPoster2:
                imgNeedRefresh = fileDateManager.CheckImageNeedRefresh("poster2.jpg", series->lastupdate) || forceFullUpdate;
                CheckSeriesMedia(series, mediaType, imgWidth, imgHeight,"poster2.jpg", 0, imgNeedRefresh);
                break;
            case msPoster3:
                imgNeedRefresh = fileDateManager.CheckImageNeedRefresh("poster3.jpg", series->lastupdate) || forceFullUpdate;
                CheckSeriesMedia(series, mediaType, imgWidth, imgHeight,"poster3.jpg", 0, imgNeedRefresh);
                break;
            case msFanart1:
                imgNeedRefresh = fileDateManager.CheckImageNeedRefresh("fanart1.jpg", series->lastupdate) || forceFullUpdate;
                CheckSeriesMedia(series, mediaType, imgWidth, imgHeight,"fanart1.jpg", 0, imgNeedRefresh);
                break;
            case msFanart2:
                imgNeedRefresh = fileDateManager.CheckImageNeedRefresh("fanart2.jpg", series->lastupdate) || forceFullUpdate;
                CheckSeriesMedia(series, mediaType, imgWidth, imgHeight,"fanart2.jpg", 0, imgNeedRefresh);
                break;
            case msFanart3:
                imgNeedRefresh = fileDateManager.CheckImageNeedRefresh("fanart3.jpg", series->lastupdate) || forceFullUpdate;
                CheckSeriesMedia(series, mediaType, imgWidth, imgHeight,"fanart3.jpg", 0, imgNeedRefresh);
                break;
            case msSeasonPoster:
                season = tSeriesMedia->getIntValue("SEASONNUMBER");
                imageName.str("");
                imageName << "season_" << season << "_thumb.jpg";
                thumbName = imageName.str();
                imageName.str("");
                imageName << "season_" << season << ".jpg";
                imgNeedRefresh = fileDateManager.CheckImageNeedRefreshThumb(imageName.str(),thumbName,series->lastupdate) || forceFullUpdate;
                CheckSeriesMedia(series, mediaType, imgWidth, imgHeight, imageName.str(), season, imgNeedRefresh);
                CalcThumbSize(imgWidth, imgHeight, scraper2VdrConfig.thumbHeight, thbWidth, thbHeight);
                CheckSeriesMedia(series, msSeasonPosterThumb, thbWidth, thbHeight, thumbName, season, imgNeedRefresh);
                break;
            case msEpisodePic:
                episode = series->GetEpisode(tSeriesMedia->getIntValue("EPISODEID"));
                if (episode) {
                    // only continue if episode exist
                    imageName.str("");
                    imageName << "episode_" << episode->id << ".jpg";
                    imgNeedRefresh = fileDateManager.CheckImageNeedRefresh(imageName.str(),episode->lastupdate) || forceFullUpdate; // use lastupdate from episode for episode images
                    imageName.str("");
                    imageName << seriesPath << "/" << "episode_" << episode->id << ".jpg";
                    series->SetEpisodeImage(episode->id, imgWidth, imgHeight, imageName.str(), imgNeedRefresh);
                }
                break;
            case msActorThumb:
                actor = series->GetActor(tSeriesMedia->getIntValue("ACTORID"));
                if (!actor) {
                    actor = scrapManager->AddSeriesActor(series,tSeriesActors); // can use all columns of tSeriesActors (get filled from select too)
                } else {
                    scrapManager->UpdateSeriesActor(actor,tSeriesActors); // update actor
                }
                imageName.str("");
                imageName << "actor_" << actor->id << ".jpg";
                imgNeedRefresh = fileDateManager.CheckImageNeedRefresh(imageName.str(),series->lastupdate) || forceFullUpdate;
                imageName.str("");
                imageName << seriesPath << "/" << "actor_" << actor->id << ".jpg";
                series->SetActorThumb(actor->id, imgWidth, imgHeight, imageName.str(), imgNeedRefresh); // update/insert actor thumb
                break;
            default:
                break;
        }
        if (imgNeedRefresh) {
            if (mediaType == msSeasonPoster) {
                newSeasonPoster++;
            } else {
                newImages++;
            }
        }
    }
    selectSeriesMediaActors->freeResult();

    if (!isFirst) {
        // save new file if min one image was available in DB
        fileDateManager.SaveFileDateList();
    }
}

// create media if not exists in series, update size, path, needRefresh of media
void cUpdate::CheckSeriesMedia(cTVDBSeries *series, int mediaType, int imgWidth, int imgHeight, string imgName, int season, bool needRefresh) {
    stringstream imageName("");
    imageName << imgPathSeries << "/" << series->id << "/" << imgName;
    cTVDBMedia *media = series->GetMedia(mediaType,season);
    if (!media) {
        series->InsertMedia(mediaType,imgWidth,imgHeight,imageName.str(),season,needRefresh);
    } else {
        media->width = imgWidth;
        media->height = imgHeight;
        media->path = imageName.str();
        media->needrefresh = needRefresh;
    }
}

// read all images with needrefresh from sql-db
void cUpdate::ReadSeriesImagesFast(int &newImages, int &emptyImages) {
    newImages = 0;
    emptyImages = 0;
    int i = 0;
    cTVDBMedia *media = NULL;
    cTVDBMedia *mediaThumb;
    cTVDBSeries *series;
    cTVDBActor *actor;
    cTVDBEpisode *episode;
    stringstream sPath("");
    stringstream imageName("");
    string seriesPath = "";
    string mediaName = "";
    int season = 0;
    int seriesCount = scrapManager->GetNumSeries();
    lastWait = GetTimems(); // init time for CheckRunningAndWait
    long lastLog = GetTimems(); // init time for logging
    for (int res = scrapManager->GetSeriesFirst(series); res; res = scrapManager->GetSeriesNext(series) && CheckRunningAndWait()) {
        if (series->updateimages)
        {
            // series used and have new images
            sPath.str("");
            sPath << imgPathSeries << "/" << series->id;
            string seriesPath = sPath.str();
            if (!CreateDirectory(seriesPath))
                return;

            // fill lastupdated list first
            fileDateManager.LoadFileDateList(seriesPath,true); // mark all old values from file as used (we only touch changed images in this function and we do not want to delete something from list)

            // go trough actors first
            for (int res = series->GetActorFirst(actor); res; res = series->GetActorNext(actor) && CheckRunningAndWait()) {
                if (actor->actorThumb) {
                    if (actor->actorThumb->needrefresh) {
                        // have to load this image from DB
                        if (ReadSeriesImageFast(series->id,0,0,actor->id,msActorThumb,actor->actorThumb,NULL)) {
                            imageName.str("");
                            imageName << "actor_" << actor->id << ".jpg";
                            fileDateManager.SetLastUpdated(imageName.str(),series->lastupdate); // use lastupdated of series for actor images
                            newImages++;
                        } else {
                            tell(2,"failed to read image (series %d, actor %d)",series->id,actor->id);
                            emptyImages++;
                        }
                    }
                }
            }

            // continue with epiode images
            for (int res = series->GetEpisodeFirst(episode); res; res = series->GetEpisodeNext(episode) && CheckRunningAndWait()) {
                if (episode->episodeImage) {
                    if (episode->episodeImage->needrefresh) {
                        // have to load this image from DB
                        if (ReadSeriesImageFast(series->id,episode->season,episode->id,0,msEpisodePic,episode->episodeImage,NULL)) {
                            imageName.str("");
                            imageName << "episode_" << episode->id << ".jpg";
                            fileDateManager.SetLastUpdated(imageName.str(),episode->lastupdate); // use lastupdated of episode for episode images
                            newImages++;
                        } else {
                            tell(2,"failed to read image (series %d, episode %d)",series->id,episode->id);
                            emptyImages++;
                        }
                    }
                }
            }

            // load season posters
            for (int res = series->GetSeasonPosterFirst(season, media); res; res = series->GetSeasonPosterNext(season, media) && CheckRunningAndWait()) {
                if (media) {
                    if (media->needrefresh) {
                        // have to load this image from DB
                        mediaThumb = series->GetMedia(msSeasonPosterThumb, season); // try to get season thumb for this season
                        if (ReadSeriesImageFast(series->id,season,0,0,msSeasonPoster,media,mediaThumb)) {
                            imageName.str("");
                            imageName << "season_" << season << ".jpg";
                            fileDateManager.SetLastUpdated(imageName.str(),series->lastupdate); // use lastupdated of series for season images
                            newImages++;
                        } else {
                            // delete season posters from fileDateManager which are not available
                            imageName.str("");
                            imageName << "season_" << season << ".jpg";
                            fileDateManager.DeleteImage(imageName.str());
                            tell(2,"failed to read image (series %d, season %d)",series->id,season);
                            emptyImages++;
                        }
                    }
                }
            }

            // now load all series images (ignore season poster in switch)
            for (int mediaType = msBanner1; mediaType <= msFanart3; ++mediaType) {
                mediaName = "";
                media = NULL;
                mediaThumb = NULL;
                switch (mediaType) {
                    case msBanner1:
                        mediaName = "banner1.jpg";
                        media = series->GetMedia(mediaType,0);
                        break;
                    case msBanner2:
                        mediaName = "banner2.jpg";
                        media = series->GetMedia(mediaType,0);
                        break;
                    case msBanner3:
                        mediaName = "banner3.jpg";
                        media = series->GetMedia(mediaType,0);
                        break;
                    case msPoster1:
                        mediaName = "poster1.jpg";
                        media = series->GetMedia(mediaType,0);
                        mediaThumb = series->GetMedia(msPosterThumb,0);
                        break;
                    case msPoster2:
                        mediaName = "poster2.jpg";
                        media = series->GetMedia(mediaType,0);
                        break;
                    case msPoster3:
                        mediaName = "poster3.jpg";
                        media = series->GetMedia(mediaType,0);
                        break;
                    case msFanart1:
                        mediaName = "fanart1.jpg";
                        media = series->GetMedia(mediaType,0);
                        break;
                    case msFanart2:
                        mediaName = "fanart2.jpg";
                        media = series->GetMedia(mediaType,0);
                        break;
                    case msFanart3:
                        mediaName = "fanart3.jpg";
                        media = series->GetMedia(mediaType,0);
                        break;
                    default:
                        break;
                }
                if (media) {
                    if (media->needrefresh) {
                        // have to load this image from DB
                        if (ReadSeriesImageFast(series->id,0,0,0,mediaType,media,mediaThumb)) {
                            fileDateManager.SetLastUpdated(mediaName,series->lastupdate); // use lastupdated of series for series images
                            newImages++;
                        } else {
                            tell(2,"failed to read image (series %d, mediatype %d)",series->id,mediaType);
                            emptyImages++;
                        }
                    }
                }
            }
            fileDateManager.SaveFileDateList(); // save updated list

            series->updateimages = false; // have updated all images
        }

        i++;
        if (GetTimeDiffms(lastLog)>LogPeriode) {
            tell(1, "Handled %d of %d series (loaded %d new images), continuing...", i, seriesCount, newImages);
            lastLog = GetTimems();
        }
    }
}

// read real image data from sql-db
bool cUpdate::ReadSeriesImageFast(int seriesId, int season, int episodeId, int actorId, int mediaType, cTVDBMedia *media, cTVDBMedia *mediaThumb) {
    bool imageSaved = false;
    bool forceImageSize;
    tSeriesMedia->clear();
    tSeriesMedia->setValue("SERIESID", seriesId);
    tSeriesMedia->setValue("SEASONNUMBER", season);
    tSeriesMedia->setValue("EPISODEID", episodeId);
    tSeriesMedia->setValue("ACTORID", actorId);
    tSeriesMedia->setValue("MEDIATYPE", mediaType);
    //tell(0,"serie: %d / season: %d / episode: %d / actor: %d / media: %d / pfad: %s",seriesId,season,episodeId,actorId,mediaType,media->path.c_str());
    int res = selectSeriesImage->find();
    if (res) {
        int originalWidth = tSeriesMedia->getIntValue("MEDIAWIDTH");
        int originalHeight = tSeriesMedia->getIntValue("MEDIAHEIGHT");
        forceImageSize = GetUsedImageSize(originalWidth, originalHeight,
                                          (mediaType >= msPoster1) && (mediaType <= msPoster3),mediaType == msSeasonPoster,
                                          media->width, media->height);
        int size = imageSize.getIntValue();
        if (size > 0) {
            if (FILE* fh = fopen(media->path.c_str(), "w")) {
                fwrite(tSeriesMedia->getStrValue("MEDIACONTENT"), 1, size, fh);
                fclose(fh);
                bool handleThumb = false;
                string thumbPath = "";
                if (mediaThumb) {
                   handleThumb = true;
                   thumbPath = mediaThumb->path;
                }
                HandleImage(media->path, originalWidth, originalHeight,
                            forceImageSize, media->width, media->height, float(scraper2VdrConfig.maxPosterDistortion)/100,
                            handleThumb, thumbPath, scraper2VdrConfig.thumbHeight);

                media->needrefresh = false; // reset update flag
                media->mediavalid = true; // we have a image file for this media
                if (mediaThumb) {
                    mediaThumb->needrefresh = false;
                    mediaThumb->mediavalid = true; // we have a image file for this media
                }

                imageSaved = true;
            }
        } else {
            media->needrefresh = false; // reset update flag for empty images also
        }
    }
    selectSeriesImage->freeResult();
    return imageSaved;
}

//***************************************************************************
// MOVIES
//***************************************************************************

// read all movies with event or recording from sql-db
int cUpdate::ReadMoviesFast(long &maxscrsp) {
    // try to create directorys first (if not exists)
    if (!CreateDirectory(scraper2VdrConfig.imageDir))
        return 0;
    if (!CreateDirectory(imgPathMovies))
        return 0;

    cMovieDbMovie *movie;
    bool isNew = false;
    int numNew = 0;
    int i = 0;

    vdr_uuid.setValue(scraper2VdrConfig.uuid); // search all recordigs of this vdr uuid
    tMovies->clear();
    event_scrsp.setValue(maxscrsp); // use last max scrsp as start value for filter

    cDbStatement* select = maxscrsp > 0 ? selectReadMoviesLastScrsp : selectReadMoviesInit;

    lastWait = GetTimems(); // init time for CheckRunningAndWait
    long lastLog = GetTimems(); // init time for logging
    for (int res = select->find(); res; res = select->fetch() && CheckRunningAndWait()) {
        movie = scrapManager->GetMovie(tMovies->getIntValue("MOVIEID"));
        if (!movie)
        {
            // add new movie with information of current row
            movie = scrapManager->AddMovie(tMovies);
            movie->lastscraped = event_scrsp.getIntValue();
            isNew = true;
        } else {
            // only update lastscraped of known movie
            movie->lastscraped = event_scrsp.getIntValue();
            if (forceFullUpdate) {
                movie->updateimages = true; // force update of images
                isNew = true;
            } else {
                isNew = false;
            }
        }
        maxscrsp = std::max(maxscrsp, movie->lastscraped);
        i++;
        if (GetTimeDiffms(lastLog)>LogPeriode) {
            tell(1, "Got %d movies, continuing...", i);
            lastLog = GetTimems();
        }
        if (isNew)
            numNew++;
    }
    select->freeResult();
    return numNew;
}

// read all actors/media from all movies where movie->updateimages = true
int cUpdate::ReadMoviesContentFast(void) {
    cMovieDbMovie *movie;
    int newNew = 0;
    int newActors = 0;
    int newMedia = 0;
    int moviecount = scrapManager->GetNumMovies();
    int i = 0;

    lastWait = GetTimems(); // init time for CheckRunningAndWait
    long lastLog = GetTimems(); // init time for logging
    for (int res = scrapManager->GetMoviesFirst(movie); res; res = scrapManager->GetMoviesNext(movie) && CheckRunningAndWait()) {
        if (movie->updateimages)
        {
            // have to update images of this movie
            newActors = ReadMovieActorsFast(movie); // read actors first
            newMedia = ReadMovieMediaFast(movie); // read all movie media
            if (newMedia == 0)
                movie->updateimages = false; // no new images for this movie, reset load image flag -> movie get not used in ReadMovieImagesFast
            newNew = newNew + newActors + newMedia;
        }

        i++;
        if (GetTimeDiffms(lastLog)>LogPeriode) {
            tell(1, "Handled %d of %d movies, continuing...", i, moviecount);
            lastLog = GetTimems();
        }
    }
    return newNew;
}

// read all actor for current movie from sql-db
int cUpdate::ReadMovieActorsFast(cMovieDbMovie *movie) {
    int numNew = 0;
    stringstream imageName("");
    bool imgNeedRefresh = false;
    int imgWidth = 0;
    int imgHeight = 0;
    cMovieActor *actor;
    cMovieMedia *actorThumb;

    tMovieActor->clear();
    tMovieMedia->clear();
    actorMovie.setValue(movie->id);

    for (int res = selectMovieActors->find(); res; res = selectMovieActors->fetch() && CheckRunningAndWait()) {
        actor = movie->GetActor(tSeriesMedia->getIntValue("ACTORID"));
        if (!actor) {
            actor = scrapManager->AddMovieActor(movie, tMovieActor, actorRole.getStrValue(), true); // do not add actor thumb
            // check if we already have actor in global actor list
        } else {
            scrapManager->UpdateMovieActor(actor, tMovieActor, actorRole.getStrValue()); // update actor
        }
        actorThumb = scrapManager->GetMovieActorThumb(actor->id);
        if (!actorThumb) {
            // new actor, add to global actor thumb map
            imageName.str("");
            imageName << imgPathMovies << "/actors/" << "actor_" << actor->id << ".jpg";
            imgNeedRefresh = (!FileExists(imageName.str(),true)) || forceFullUpdate;
            imgWidth = thbWidth.getIntValue();
            imgHeight = thbHeight.getIntValue();
            actorThumb = scrapManager->AddMovieActorThumb(actor->id, imgWidth, imgHeight, imageName.str(), imgNeedRefresh);
            if (imgNeedRefresh)
              numNew++; // only count new actors/images
        }
        actor->actorThumbExternal = actorThumb; // assign actor thumb from global map to movie actor
    }

    selectMovieActors->freeResult();
    return numNew;
}

// read all images for current movie from sql-db
int cUpdate::ReadMovieMediaFast(cMovieDbMovie *movie) {
    int newImages = 0;
    int mediaType = 0;
    stringstream imageName("");
    imageName << imgPathMovies << "/" << movie->id;
    string moviePath = imageName.str();
    string thumbName = "";

    bool imgNeedRefresh = false;
    int imgWidth = 0;
    int imgHeight = 0;
    int originalWidth = 0;
    int originalHeight = 0;
    int thbWidth = 0;
    int thbHeight = 0;

    tMovieMedia->clear();
    tMovieMedia->setValue("MOVIEID", movie->id);

    for (int res = selectMovieMediaFast->find(); res; res = selectMovieMediaFast->fetch() && CheckRunningAndWait()) {
        mediaType = tMovieMedia->getIntValue("MEDIATYPE");
        originalWidth = tMovieMedia->getIntValue("MEDIAWIDTH");
        originalHeight = tMovieMedia->getIntValue("MEDIAHEIGHT");
        GetUsedImageSize(originalWidth, originalHeight,
                         (mediaType == mmPoster) || (mediaType == mmCollectionPoster), false,
                         imgWidth, imgHeight);

        imgNeedRefresh = false;
        // check type of media first
        switch (mediaType) {
            case mmPoster:
                imageName.str("");
                imageName << moviePath << "/poster_thumb.jpg";
                thumbName = imageName.str();
                imageName.str("");
                imageName << moviePath << "/poster.jpg";
                imgNeedRefresh = !(FileExists(imageName.str(),true) && FileExists(thumbName,true)); // refresh image if one file not exist
                imgNeedRefresh = imgNeedRefresh || forceFullUpdate;
                CheckMovieMedia(movie, mediaType, imgWidth, imgHeight, imageName.str(), imgNeedRefresh);
                CalcThumbSize(imgWidth, imgHeight, scraper2VdrConfig.thumbHeight, thbWidth, thbHeight);
                CheckMovieMedia(movie, mmPosterThumb, thbWidth, thbHeight, thumbName, imgNeedRefresh);
                break;
            case mmFanart:
                imageName.str("");
                imageName << moviePath << "/fanart.jpg";
                imgNeedRefresh = (!FileExists(imageName.str(),true)) || forceFullUpdate;
                CheckMovieMedia(movie, mediaType, imgWidth, imgHeight, imageName.str(), imgNeedRefresh);
                break;
            case mmCollectionPoster:
                imageName.str("");
                imageName << moviePath << "/collectionPoster.jpg";
                imgNeedRefresh = (!FileExists(imageName.str(),true)) || forceFullUpdate;
                CheckMovieMedia(movie, mediaType, imgWidth, imgHeight, imageName.str(), imgNeedRefresh);
                break;
            case mmCollectionFanart:
                imageName.str("");
                imageName << moviePath << "/collectionFanart.jpg";
                imgNeedRefresh = (!FileExists(imageName.str(),true)) || forceFullUpdate;
                CheckMovieMedia(movie, mediaType, imgWidth, imgHeight, imageName.str(), imgNeedRefresh);
                break;
            default:
                break;
        }
        if (imgNeedRefresh)
            newImages++;
    }

    selectMovieMediaFast->freeResult();
    return newImages;
}

// create media if not exists in movie, update size, path, needRefresh of media
void cUpdate::CheckMovieMedia(cMovieDbMovie *movie, int mediaType, int imgWidth, int imgHeight, string imgName, bool needRefresh) {
    cMovieMedia *media = movie->GetMediaObj(mediaType);
    if (!media) {
        movie->InsertMedia(mediaType,imgWidth,imgHeight,imgName,needRefresh);
    } else {
        media->width = imgWidth;
        media->height = imgHeight;
        media->path = imgName;
        media->needrefresh = needRefresh;
    }
}

// read all images with needrefresh from sql-db
int cUpdate::ReadMovieImagesFast(void) {
    string movieActorsPath = imgPathMovies + "/actors";
    if (!CreateDirectory(movieActorsPath))
        return 0;

    int newActors = 0;
    int newImages = 0;
    int i = 0;
    int actorId = 0;
    stringstream mPath("");
    cMovieMedia *media = NULL;
    cMovieMedia *mediaThumb;
    cMovieDbMovie *movie;

    int actorCount = scrapManager->GetNumMovieActors();
    int movieCount = scrapManager->GetNumMovies();
    lastWait = GetTimems(); // init time for CheckRunningAndWait
    long lastLog = GetTimems(); // init time for logging
    // go trough actors first
    for (int res = scrapManager->GetMovieActorThumbFirst(actorId, media); res; res = scrapManager->GetMovieActorThumbNext(actorId, media) && CheckRunningAndWait()) {
        if (media->needrefresh) {
            // have to load this image from DB
            if (ReadMovieImageFast(0,actorId,mmActorThumb,media,NULL)) {
                newActors++;
            } else {
                tell(2,"failed to read image (movie actor id %d)",actorId);
            }
        }
        i++;
        if (GetTimeDiffms(lastLog)>LogPeriode) {
            tell(1, "Handled %d of %d actors (loaded %d new images), continuing...", i, actorCount, newActors);
            lastLog = GetTimems();
        }
    }

    i = 0;
    for (int res = scrapManager->GetMoviesFirst(movie); res; res = scrapManager->GetMoviesNext(movie) && CheckRunningAndWait()) {
        if (movie->updateimages) {
            // movie have new images
            mPath.str("");
            mPath << imgPathMovies << "/" << movie->id;
            if (!CreateDirectory(mPath.str()))
                return 0;

            // load all movie images
            for (int mediaType = mmPoster; mediaType <= mmCollectionFanart; ++mediaType) {
                media = NULL;
                mediaThumb = NULL;
                switch (mediaType) {
                    case mmPoster:
                        media = movie->GetMediaObj(mediaType);
                        mediaThumb = movie->GetMediaObj(mmPosterThumb);
                    break;
                    case mmFanart:
                        media = movie->GetMediaObj(mediaType);
                    break;
                    case mmCollectionPoster:
                        media = movie->GetMediaObj(mediaType);
                    break;
                    case mmCollectionFanart:
                        media = movie->GetMediaObj(mediaType);
                    break;
                    default:
                        break;
                }
                if (media) {
                    if (media->needrefresh) {
                        // have to load this image from DB
                        if (ReadMovieImageFast(movie->id,0,mediaType,media,mediaThumb)) {
                            newImages++;
                        } else {
                           tell(2,"failed to read image (movie %d, mediatype %d)",movie->id,mediaType);
                        }
                    }
                }
            }
            movie->updateimages = false; // have updated all images
        }

        i++;
        if (GetTimeDiffms(lastLog)>LogPeriode) {
            tell(1, "Handled %d of %d movies (loaded %d new images), continuing...", i, movieCount, newImages);
            lastLog = GetTimems();
        }
    }
    return newActors + newImages;
}

// read real image data from sql-db
bool cUpdate::ReadMovieImageFast(int movieId, int actorId, int mediaType, cMovieMedia *media, cMovieMedia *mediaThumb) {
    bool imageSaved = false;
    bool forceImageSize;
    tMovieMedia->clear();
    tMovieMedia->setValue("MOVIEID", movieId);
    tMovieMedia->setValue("ACTORID", actorId);
    tMovieMedia->setValue("MEDIATYPE", mediaType);
    int res = selectMovieImage->find();
    if (res) {
        int originalWidth = tMovieMedia->getIntValue("MEDIAWIDTH");
        int originalHeight = tMovieMedia->getIntValue("MEDIAHEIGHT");
        forceImageSize = GetUsedImageSize(originalWidth, originalHeight,
                                          (mediaType == mmPoster) || (mediaType == mmCollectionPoster), false,
                                          media->width, media->height);
        int size = imageSize.getIntValue();
        if (size > 0) {
            if (FILE* fh = fopen(media->path.c_str(), "w")) {
                fwrite(tMovieMedia->getStrValue("MEDIACONTENT"), 1, size, fh);
                fclose(fh);
                bool handleThumb = false;
                string thumbPath = "";
                if (mediaThumb) {
                   handleThumb = true;
                   thumbPath = mediaThumb->path;
                }
                HandleImage(media->path, originalWidth, originalHeight,
                            forceImageSize, media->width, media->height, float(scraper2VdrConfig.maxPosterDistortion)/100,
                            handleThumb, thumbPath, scraper2VdrConfig.thumbHeight);

                media->needrefresh = false; // reset update flag
                media->mediavalid = true; // we have a image file for this media
                if (mediaThumb) {
                    mediaThumb->needrefresh = false;
                    mediaThumb->mediavalid = true; // we have a image file for this media
                }

                imageSaved = true;
            }
        } else {
            media->needrefresh = false; // reset update flag for empty images also
        }
    }
    selectMovieImage->freeResult();
    return imageSaved;
}

//***************************************************************************
// RECORDINGS
//***************************************************************************

int cUpdate::ReadRecordings(void) {
    tRecordingList->clear();
    int numRecs = 0;
    for (int res = selectRecordings->find(); res; res = selectRecordings->fetch()) {
        int recStart = tRecordingList->getIntValue("STARTTIME");
        string recPath = tRecordingList->getStrValue("PATH");
        int movieId = tRecordingList->getIntValue("SCRMOVIEID");
        int seriesId = tRecordingList->getIntValue("SCRSERIESID");
        int episodeId = tRecordingList->getIntValue("SCRSERIESEPISODE");
        bool isNew = scrapManager->AddRecording(recStart, recPath, seriesId, episodeId, movieId);
        if (isNew)
            numRecs++;
    }

    selectRecordings->freeResult();
    return numRecs;
}

//***************************************************************************
//
//***************************************************************************

/*int cUpdate::ScanVideoDir(void) {
   int newRecs = 0;

   // --------------------------
   // get recordings lock

#if defined (APIVERSNUM) && (APIVERSNUM >= 20301)
   LOCK_RECORDINGS_READ;
   const cRecordings* recordings = Recordings;
   // cRecordingsLock recordingsLock(false);
   // const cRecordings* recordings = recordingsLock.Recordings();
#else
   const cRecordings* recordings = &Recordings;
#endif

   // --------
   // iterate

    for (const cRecording *rec = recordings->First(); rec; rec = recordings->Next(rec))
    {
        string recPath = getRecPath(rec);
        int recStart = rec->Start();

        if (!scrapManager->RecordingExists(recStart, recPath))
        {
            newRecs++;
            int scrapInfoMovieID = 0;
            int scrapInfoSeriesID = 0;
            int scrapInfoEpisodeID = 0;
            ReadScrapInfo(rec->FileName(), scrapInfoMovieID, scrapInfoSeriesID, scrapInfoEpisodeID);
            // string channelId = "";
            string title = rec->Name();
            size_t posDelim = title.find_last_of('~');
            if (posDelim != string::npos) {
                title = title.substr(posDelim+1);
            }
            string subTitle = "";
            const cRecordingInfo *recInfo = rec->Info();
            if (recInfo) {
                const cEvent *recEvent = recInfo->GetEvent();
                if (recEvent) {
                   //channelId = *(recInfo->ChannelID().ToString());
                    if (recInfo->Title())
                        title = recInfo->Title();
                    subTitle = (recInfo->ShortText())?(recInfo->ShortText()):"";
                }
            }

            // update also recordinglist table for epghttpd

            md5Buf md5path;
            int pathOffset = 0;

#if APIVERSNUM > 20103
            const char* videoBasePath = cVideoDirectory::Name();
#else
            const char* videoBasePath = VideoDirectory;
#endif

            if (strncmp(rec->FileName(), videoBasePath, strlen(videoBasePath)) == 0)
            {
               pathOffset = strlen(videoBasePath);

               if (*(rec->FileName()+pathOffset) == '/')
                  pathOffset++;
            }

            createMd5(rec->FileName()+pathOffset, md5path);

            tRecordingList->clear();
            tRecordingList->setValue("MD5PATH", md5path);
            tRecordingList->setValue("STARTTIME", recStart);

            if (tRecordingList->find())
            {
               tRecordingList->clearChanged();
               tRecordingList->setValue("SCRINFOMOVIEID", scrapInfoMovieID);
               tRecordingList->setValue("SCRINFOSERIESID", scrapInfoSeriesID);
               tRecordingList->setValue("SCRINFOEPISODEID", scrapInfoEpisodeID);
               tRecordingList->setValue("SCRNEW", 1);

               if (tRecordingList->getChanges())
                  tRecordingList->update();
            }

            tRecordingList->reset();
        }
    }
    return newRecs;
}

int cUpdate::ScanVideoDirScrapInfo(void) {
    int numUpdated = 0;

   // --------------------------
   // get recordings lock

#if defined (APIVERSNUM) && (APIVERSNUM >= 20301)
    LOCK_RECORDINGS_READ;
    const cRecordings* recordings = Recordings;

    // cRecordingsLock recordingsLock(false);
    // const cRecordings* recordings = recordingsLock.Recordings();
#else
    const cRecordings* recordings = &Recordings;
#endif

   // --------
   // iterate

    for (const cRecording *rec = recordings->First(); rec; rec = recordings->Next(rec))
    {
        int recStart = rec->Start();
        string recPath = getRecPath(rec);
        int scrapInfoMovieID = 0;
        int scrapInfoSeriesID = 0;
        int scrapInfoEpisodeID = 0;

        ReadScrapInfo(rec->FileName(), scrapInfoMovieID, scrapInfoSeriesID, scrapInfoEpisodeID);

        md5Buf md5path;
#if APIVERSNUM > 20103
        const char* videoBasePath = cVideoDirectory::Name();
#else
        const char* videoBasePath = VideoDirectory;
#endif

        if (strncmp(rec->FileName(), videoBasePath, strlen(videoBasePath)) == 0)
           createMd5(rec->FileName()+strlen(videoBasePath), md5path);
        else
           createMd5(rec->FileName(), md5path);

        tRecordingList->clear();
        tRecordingList->setValue("MD5PATH", md5path);
        tRecordingList->setValue("STARTTIME", recStart);

        if (tRecordingList->find())
        {
           tRecordingList->clearChanged();
           tRecordingList->setValue("SCRINFOMOVIEID", scrapInfoMovieID);
           tRecordingList->setValue("SCRINFOSERIESID", scrapInfoSeriesID);
           tRecordingList->setValue("SCRINFOEPISODEID", scrapInfoEpisodeID);
           tRecordingList->setValue("SCRNEW", 1);

           if (tRecordingList->getChanges())
              tRecordingList->update();
        }

        tRecordingList->reset();

        numUpdated++;
    }

    return numUpdated;
}
*/
void cUpdate::ReadScrapInfo(string recDir, int &scrapInfoMovieID, int &scrapInfoSeriesID, int &scrapInfoEpisodeID) {
    stringstream sInfoName("");
    sInfoName << recDir << "/" << scraper2VdrConfig.recScrapInfoName;
    string scrapInfoName = sInfoName.str();
    if (!FileExists(scrapInfoName, false)) {
        string twoHigher = TwoFoldersHigher(recDir);
        if (twoHigher.size() > 0) {
            stringstream sInfoNameAlt("");
            sInfoNameAlt << twoHigher << "/" << scraper2VdrConfig.recScrapInfoName;
            scrapInfoName = sInfoNameAlt.str();
            if (!FileExists(scrapInfoName, false)) {
                return;
            }
        } else
            return;
    }
    vector<string> scrapInfoLines;
    FILE *f = fopen(scrapInfoName.c_str(), "r");
    if (!f)
        return;
    cReadLine ReadLine;
    char *line;
    while ((line = ReadLine.Read(f)) != NULL) {
        scrapInfoLines.push_back(line);
    }
    fclose(f);
    int numLines = scrapInfoLines.size();
    if (numLines < 2) {
        tell(0, "invalid scrapinfo file in %s", recDir.c_str());
        return;
    }
    for (int line=0; line < numLines; line++) {
        scrapInfoLines[line] = trim(scrapInfoLines[line]);
        toLower(scrapInfoLines[line]);
    }
    bool isMovie = false;
    bool isSeries = false;
    if (!scrapInfoLines[0].compare("movie"))
        isMovie = true;
    else if (!scrapInfoLines[0].compare("series"))
        isSeries = true;
    else
        tell(0, "invalid scrapinfo file in %s", recDir.c_str());

    int id1 = 0, id2 = 0;
    string key = "", value = "";

    splitstring s(scrapInfoLines[1].c_str());
    vector<string> flds = s.split('=');
    if (flds.size() == 2) {
        key = trim(flds[0]);
        value = trim(flds[1]);
    } else {
        tell(0, "invalid scrapinfo file in %s", recDir.c_str());
    }
    if (!key.compare("id")) {
        id1 = atoi(value.c_str());
        if (numLines > 2) {
            splitstring s2(scrapInfoLines[2].c_str());
            vector<string> flds2 = s2.split('=');
            if (flds2.size() == 2) {
                key = trim(flds2[0]);
                toLower(key);
                value = trim(flds2[1]);
            }
            if (!key.compare("episode")) {
                id2 = atoi(value.c_str());
            }
        }
    } else
        tell(0, "invalid scrapinfo file in %s", recDir.c_str());
    if (isSeries) {
        scrapInfoSeriesID = id1;
        scrapInfoEpisodeID = id2;
    }
    if (isMovie) {
        scrapInfoMovieID = id1;
    }
}

//***************************************************************************
// Cleanup
//***************************************************************************

int cUpdate::CleanupSeries(void) {
    //read existing series in file system
    vector<string> storedSeries;
    DIR *dir;
    struct dirent *entry;
    if ((dir = opendir (imgPathSeries.c_str())) != NULL) {
        while ((entry = readdir (dir)) != NULL) {
            string dirName = entry->d_name;
            if (isNumber(dirName)) {
                storedSeries.push_back(dirName);
            }
        }
        closedir (dir);
    } else return 0;
    int deletedSeries = 0;
    //aviod complete delete if no series are available
    int numSeriesAvailable = scrapManager->GetNumSeries();
    if (numSeriesAvailable == 0)
        return 0;
    for (vector<string>::iterator seriesDir = storedSeries.begin(); seriesDir != storedSeries.end(); seriesDir++) {
        int seriesId = atoi(((string)*seriesDir).c_str());
        if (!scrapManager->SeriesInUse(seriesId)) {
            string delDir = imgPathSeries + "/" + ((string)*seriesDir);
            DeleteDirectory(delDir);
            deletedSeries++;
        }
    }
    return deletedSeries;
}

int cUpdate::CleanupMovies(void) {
    //read existing movies in file system
    vector<string> storedMovies;
    DIR *dir;
    struct dirent *entry;
    if ((dir = opendir (imgPathMovies.c_str())) != NULL) {
        while ((entry = readdir (dir)) != NULL) {
            string dirName = entry->d_name;
            if (isNumber(dirName)) {
                storedMovies.push_back(dirName);
            }
        }
        closedir (dir);
    } else return 0;
    int deletedMovies = 0;
    //aviod complete delete if no movies are available
    int numMoviesAvailable = scrapManager->GetNumMovies();
    if (numMoviesAvailable == 0)
        return 0;
    for (vector<string>::iterator movieDir = storedMovies.begin(); movieDir != storedMovies.end(); movieDir++) {
        int movieId = atoi(((string)*movieDir).c_str());
        if (!scrapManager->MovieInUse(movieId)) {
            string delDir = imgPathMovies + "/" + ((string)*movieDir);
            DeleteDirectory(delDir);
            deletedMovies++;
        }
    }
    return deletedMovies;
}

// check if we should abort execution (thread stoped), also check if we should call waitCondition.TimedWait (so other processes can use the CPU)
bool cUpdate::CheckRunningAndWait(void)  {
  if (Running())
  {
    // check for intervall to call
    if (GetTimeDiffms(lastWait) > 30)
    {
      // wait/sleep after 50ms
      waitCondition.TimedWait(mutex,3);
      lastWait = GetTimems();
    }
    return true; // not aborted
  }
  else
  {
    return false; // should abort
  }
}

// get used image size (depends on isPoster/isSeasonPoster and useFixPosterSize)
bool cUpdate::GetUsedImageSize(int originalWidth, int originalHeight, bool isPoster, bool isSeasonPoster, int& usedWidth, int& usedHeight) {
    if (isPoster && (scraper2VdrConfig.useFixPosterSize)) {
        // use fix size for posters
        usedWidth = scraper2VdrConfig.fixPosterWidth;
        usedHeight = scraper2VdrConfig.fixPosterHeight;
        return true;
    } else {
        if (isSeasonPoster && (scraper2VdrConfig.useFixPosterSize)) {
            // use fix size for season posters
            usedWidth = scraper2VdrConfig.fixSeasonPosterWidth;
            usedHeight = scraper2VdrConfig.fixSeasonPosterHeight;
            return true;
        } else {
            // no fixed size for this image, use size values from database
            usedWidth = originalWidth;
            usedHeight = originalHeight;
            return false;
        }
    }
}

//***************************************************************************
// Action
//***************************************************************************

void cUpdate::Action()
{
    mutex.Lock();
    loopActive = yes;

    int worked = no;
    int sleep = 60;
    int initSleep = 10;
    int scanFreq = 60 * 2;
    int scanNewRecFreq = 60 * 5;
    int scanNewRecDBFreq = 60 * 5;
    int cleanUpFreq = 60 * 10;

    forceUpdate = true;
    forceRecordingUpdate = true;

    time_t lastScan = time(0);
    time_t lastScanNewRec = time(0);
    time_t lastScanNewRecDB = time(0);
    time_t lastCleanup = time(0);
    bool init = yes;

    time_t now = time(0);
    int dur = 0;
    //int numNewRecs = 0;
    int numValues = 0;
    int numNewImages = 0;
    int numNewImages2 = 0;
    bool showlog = false;

    while (loopActive && Running())
    {
        int reconnectTimeout; // set by checkConnection

        waitCondition.TimedWait(mutex, init ? initSleep*1000 : sleep*1000);

        // at first init the uuid
        //  - read uuid from vdrs table to use the same than the epg2vdr plugin

        if (isEmpty(scraper2VdrConfig.uuid))
           initUuid(initSleep);

        if (isEmpty(scraper2VdrConfig.uuid))
           continue;

        if (CheckConnection(init, reconnectTimeout) != success)
           continue;

        // auch beim init auf den epgd warten, wenn der gerade busy
        // ist müssen die sich User etwas gedulden ;)

        if (CheckEpgdBusy())
        {
           init = no;
           tell(1, "epgd busy, trying again in %d seconds ...", sleep);
           continue;
        }

        // Update Recordings from Database

        if (forceRecordingUpdate || (time(0) - lastScanNewRecDB > scanNewRecDBFreq) && Running())
        {
            worked++;
            /*numNewRecs = */ ReadRecordings();
            lastScanNewRecDB = time(0);
            forceRecordingUpdate = false;
        }

        // Update Events

        if (!scraper2VdrConfig.headless && (forceUpdate || (time(0) - lastScan > scanFreq)) && Running())
        {
           worked++;
           int numNewEvents = ReadScrapedEvents();

           if (numNewEvents > 0) {
              tell(1, "Got %d new scraped Events from Database", numNewEvents);
           } else {
              lastScan = time(0);
              init = no;
           }

           worked++;
           if (forceFullUpdate) {
               // force loading of all data
               MaxScrspMovies = 0;
               MaxScrspSeries = 0;
           }

           // new mode
           showlog = (MaxScrspMovies == 0) || (MaxScrspSeries == 0) || numNewEvents || forceUpdate || (scraper2VdrConfig.loglevel>1); // force log if: first run, found new events, forced update or loglevel > 1
           if (showlog)
               tell(1, "Loading Movies information from Database...");
           now = time(0);
           numValues = ReadMoviesFast(MaxScrspMovies);
           dur = time(0) - now;
           showlog = showlog || (numValues>0);
           if (showlog) {
               tell(1, "Got %d new/updated Movies in %ds from Database (new max scrsp: %ld)", numValues, dur, MaxScrspMovies);
               tell(1, "Loading Movies content from Database...");
           }

           now = time(0);
           numValues = ReadMoviesContentFast();
           dur = time(0) - now;
           showlog = showlog || (numValues>0);
           if (showlog) {
               tell(1, "Got %d new/updated Image information in %ds from Database", numValues, dur);
               tell(1, "Loading Series information from Database...");
           }

           now = time(0);
           numValues = ReadSeriesFast(MaxScrspSeries);
           dur = time(0) - now;
           showlog = showlog || (numValues>0);
           if (showlog) {
               tell(1, "Got %d new/updated Series in %ds from Database (new max scrsp: %ld)", numValues, dur, MaxScrspSeries);
               tell(1, "Loading Series content from Database...");
           }
           now = time(0);
           numValues = ReadSeriesContentFast(numNewImages,numNewImages2);
           dur = time(0) - now;
           showlog = showlog || (numValues>0) || (numNewImages-numNewImages2>0); // ignore season thumbs here
           if (showlog) {
               tell(1, "Got %d new/updated Episodes and %d new/updated Image information (including %d possible not available season poster) in %ds from Database", numValues, numNewImages+numNewImages2, numNewImages2, dur);
               tell(1, "Loading Image content from Database...");
           }
           now = time(0);
           ReadSeriesImagesFast(numNewImages,numNewImages2);
           numNewImages = numNewImages + ReadMovieImagesFast();
           dur = time(0) - now;
           showlog = showlog || (numNewImages>0);
           if (showlog)
               tell(1, "Got %d new/updated Images (found %d not available images) in %ds from Database", numNewImages, numNewImages2, dur);

           lastScan = time(0);
           forceUpdate = false;
           forceFullUpdate = false;
        }

        // Scan new recordings

        if ((init || forceVideoDirUpdate || (time(0) - lastScanNewRec > scanNewRecFreq)) && Running())
        {
//            // ---------------------------------------
//            // check if any recording has changed ...

// #if defined (APIVERSNUM) && (APIVERSNUM >= 20301)
//            static cStateKey* recordingsKey = new cStateKey();
//            cRecordings* recordings = cRecordings::GetRecordingsWrite(*recordingsKey, 500/*ms*/);
//            int recStateChanged = recordings != 0;

//            if (recordings)
//               recordingsKey->Remove();
// #else
//            static int recState = 0;
//            int recStateChanged = Recordings.StateChanged(recState);
// #endif

//            if (recStateChanged)
//            {
//               tell(1, "Searching for new recordings because of Recordings State Change...");
//               worked++;
//               numNewRecs = ScanVideoDir();
//               tell(1, "found %d new recordings", numNewRecs);
//            }

           lastScanNewRec = time(0);
           forceVideoDirUpdate = false;
        }

        init = no;

//         // Scan Video dir for scrapinfo files

//         if (forceScrapInfoUpdate)
//         {
//            worked++;
//            tell(1, "Checking for new or updated scrapinfo files in recordings...");
//            int numUpdated = ScanVideoDirScrapInfo();
//            tell(1, "found %d new or updated scrapinfo files", numUpdated);
//            forceScrapInfoUpdate = false;
//         }

        // Cleanup

        if ((time(0) - lastCleanup > cleanUpFreq) && Running())
        {
           worked++;
           int seriesDeleted = CleanupSeries();
           int moviesDeleted = CleanupMovies();

           if (seriesDeleted > 0 || moviesDeleted > 0)
           {
              tell(1, "Deleted %d outdated series image folders", seriesDeleted);
              tell(1, "Deleted %d outdated movie image folders", moviesDeleted);
           }

           lastCleanup = time(0);
        }

        if (worked && scraper2VdrConfig.loglevel > 2)
           connection->showStat();

        worked = no;
    }

    exitDb();             // don't call exit in dtor, outside from thread!!
    loopActive = no;
    tell(0, "Update thread ended (pid=%d)", getpid());
}

//***************************************************************************
// External trigggering of Actions
//***************************************************************************

void cUpdate::ForceUpdate(void) {
    tell(0, "full update from database forced");
    forceUpdate = true;
}

void cUpdate::ForceRecordingUpdate(void) {
    tell(0, "scanning of recordings in database triggered");
    forceRecordingUpdate = true;
}

void cUpdate::ForceVideoDirUpdate(void) {
    tell(0, "scanning for new recordings in video directory triggered");
    forceVideoDirUpdate = true;
}

void cUpdate::ForceScrapInfoUpdate(void) {
    tell(0, "scanning of recording scrapinfo files triggered");
    forceScrapInfoUpdate = true;
}

void cUpdate::ForceFullUpdate(void) {
    tell(0, "full update (including image data) from database forced");
    forceUpdate = true; // this trigger the update
    forceFullUpdate = true; // this trigger reset of old data
}

// forec DB reconnect (e.g. after changing Host/DB/User/Port)
void cUpdate::ForceReconnect(void) {
    tell(0, "settings changed, force re-connect to DB");
    forceReconnect = true; // this trigger the update
}
