#define __STL_CONFIG_H
#include <locale.h>
#include "lib/common.h"

#include <vdr/videodir.h>
#include <vdr/tools.h>
#include <vdr/plugin.h>

#include "lib/config.h"

#include "config.h"
#include "tools.h"
#include "update.h"

extern cScraper2VdrConfig config;

cUpdate::cUpdate(cScrapManager *manager) : cThread("update thread started", true) {
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
    tRecordings = NULL;

    selectReadScrapedEventsInit = 0;
    selectReadScrapedEvents = 0;
    selectImg = 0;
    selectEpisodeImg = 0;
    selectSeasonPoster = 0;
    selectActors = 0;
    selectActorThumbs = 0;
    selectSeriesMedia = 0;
    selectMovieActors = 0;
    selectMovieActorThumbs = 0;
    selectMovieMedia = 0;
    selectMediaMovie = 0;
    selectRecordings = 0;
    selectCleanupRecordings = 0;
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
    imgPathSeries = config.imageDir + "/series";
    imgPathMovies = config.imageDir + "/movies";
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
    forceCleanupRecordingDb = false;
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

    cDbConnection::setEncoding(withutf8 ? "utf8": "latin1");
    cDbConnection::setHost(config.mysqlHost.c_str());
    cDbConnection::setPort(config.mysqlPort);
    cDbConnection::setName(config.mysqlDBName.c_str());
    cDbConnection::setUser(config.mysqlDBUser.c_str());
    cDbConnection::setPass(config.mysqlDBPass.c_str());
    cDbTable::setConfPath(cPlugin::ConfigDirectory("epg2vdr/"));

    EPG2VDRConfig.loglevel = config.debug ? 2 : 1;
}

cUpdate::~cUpdate() {
    if (loopActive)
        Stop();
}

// global field definitions

cDBS::FieldDef imageSizeDef = { "media_content", cDBS::ffUInt,  0, 999, cDBS::ftData };
cDBS::FieldDef uuIDDef = { "uuid", cDBS::ffAscii, 40, 998,  cDBS::ftData };

int cUpdate::initDb() {
    int status = success;
    if (!connection)
        connection = new cDbConnection();
    if (!connection)
      return fail;
    vdrDb = new cTableVdrs(connection);
    if (vdrDb->open() != success) 
        return fail;
    tEvents = new cTableEvents(connection);
    if (tEvents->open() != success) 
        return fail;
    tSeries = new cTableSeries(connection);
    if (tSeries->open() != success) 
        return fail;
    tEpisodes = new cTableSeriesEpisode(connection);
    if (tEpisodes->open() != success) 
        return fail;
    tSeriesMedia = new cTableSeriesMedia(connection);
    if (tSeriesMedia->open() != success) 
        return fail;
    tSeriesActors = new cTableSeriesActor(connection);
    if (tSeriesActors->open() != success) 
        return fail;
    tMovies = new cTableMovies(connection);
    if (tMovies->open() != success) 
        return fail;
    tMovieActor = new cTableMovieActor(connection);
    if (tMovieActor->open() != success) 
        return fail;
    tMovieActors = new cTableMovieActors(connection);
    if (tMovieActors->open() != success) 
        return fail;
    tMovieMedia = new cTableMovieMedia(connection);
    if (tMovieMedia->open() != success) 
        return fail;
    tRecordings = new cTableRecordings(connection);
    if (tRecordings->open() != success) 
        return fail;

    // try to create temp. episode cache table
    if (connection->query("%s",TempEpisodeCreateStatement.c_str()) != success)
        return fail;
    
    
    // --------------------
    // prepare statements

    // 
    series_id.setField(tSeriesMedia->getField(cTableSeriesMedia::fiSeriesId));
    event_scrsp.setField(tEvents->getField(cTableEvents::fiScrSp));
    events_ScrSeriesId.setField(tEvents->getField(cTableEvents::fiScrSeriesId));
    episode_LastUpdate.setField(tEpisodes->getField(cTableSeriesEpisode::fiEpisodeLastUpdated));
    vdr_uuid.setField(&uuIDDef);
    imageSize.setField(&imageSizeDef);
    episodeImageSize.setField(&imageSizeDef);
    posterSize.setField(&imageSizeDef);
    actorImageSize.setField(&imageSizeDef);
    actorRole.setField(tMovieActors->getField(cTableMovieActors::fiRole));
    actorMovie.setField(tMovieActors->getField(cTableMovieActors::fiMovieId));
    thbWidth.setField(tMovieMedia->getField(cTableMovieMedia::fiMediaWidth));
    thbHeight.setField(tMovieMedia->getField(cTableMovieMedia::fiMediaHeight));

    selectReadScrapedEventsInit = new cDbStatement(tEvents);
    selectReadScrapedEventsInit->build("select ");
    selectReadScrapedEventsInit->bind(cTableEvents::fiEventId, cDBS::bndOut);
    selectReadScrapedEventsInit->bind(cTableEvents::fiChannelId, cDBS::bndOut, ", ");
    selectReadScrapedEventsInit->bind(cTableEvents::fiMasterId, cDBS::bndOut, ", ");
    selectReadScrapedEventsInit->bind(cTableEvents::fiScrSeriesId, cDBS::bndOut, ", ");
    selectReadScrapedEventsInit->bind(cTableEvents::fiScrSeriesEpisode, cDBS::bndOut, ", ");
    selectReadScrapedEventsInit->bind(cTableEvents::fiScrMovieId, cDBS::bndOut, ", ");
    selectReadScrapedEventsInit->bind(cTableEvents::fiScrSp, cDBS::bndOut, ", ");
    selectReadScrapedEventsInit->build(" from %s where ", tEvents->TableName());
    selectReadScrapedEventsInit->build(" ((%s is not null and %s > 0) ", 
                                   tEvents->getField(cTableEvents::fiScrSeriesId)->name, 
                                   tEvents->getField(cTableEvents::fiScrSeriesId)->name);
    selectReadScrapedEventsInit->build(" or (%s is not null and %s > 0)) ", 
                                   tEvents->getField(cTableEvents::fiScrMovieId)->name, 
                                   tEvents->getField(cTableEvents::fiScrMovieId)->name);
    selectReadScrapedEventsInit->build(" order by %s", tEvents->getField(cTableEvents::fiInsSp)->name);
    
    status += selectReadScrapedEventsInit->prepare();

    // 

    selectReadScrapedEvents = new cDbStatement(tEvents);
    selectReadScrapedEvents->build("select ");
    selectReadScrapedEvents->bind(cTableEvents::fiEventId, cDBS::bndOut);
    selectReadScrapedEvents->bind(cTableEvents::fiChannelId, cDBS::bndOut, ", ");
    selectReadScrapedEvents->bind(cTableEvents::fiMasterId, cDBS::bndOut, ", ");
    selectReadScrapedEvents->bind(cTableEvents::fiScrSeriesId, cDBS::bndOut, ", ");
    selectReadScrapedEvents->bind(cTableEvents::fiScrSeriesEpisode, cDBS::bndOut, ", ");
    selectReadScrapedEvents->bind(cTableEvents::fiScrMovieId, cDBS::bndOut, ", ");
    selectReadScrapedEvents->bind(cTableEvents::fiScrSp, cDBS::bndOut, ", ");
    selectReadScrapedEvents->build(" from %s where ", tEvents->TableName());
    selectReadScrapedEvents->build(" ((%s is not null and %s > 0) ", 
                                   tEvents->getField(cTableEvents::fiScrSeriesId)->name, 
                                   tEvents->getField(cTableEvents::fiScrSeriesId)->name);
    selectReadScrapedEvents->build(" or (%s is not null and %s > 0)) ", 
                                   tEvents->getField(cTableEvents::fiScrMovieId)->name, 
                                   tEvents->getField(cTableEvents::fiScrMovieId)->name);
    selectReadScrapedEvents->bindCmp(0, cTableEvents::fiScrSp, 0, ">", " and ");
    selectReadScrapedEvents->build(" order by %s", tEvents->getField(cTableEvents::fiInsSp)->name);
   
    status += selectReadScrapedEvents->prepare();
    
    // select image

    selectImg = new cDbStatement(tSeriesMedia);
    selectImg->build("select ");
    selectImg->bind(cTableSeriesMedia::fiMediaWidth, cDBS::bndOut);
    selectImg->bind(cTableSeriesMedia::fiMediaHeight, cDBS::bndOut, ", ");
    selectImg->bind(cTableSeriesMedia::fiMediaContent, cDBS::bndOut, ", ");
    selectImg->build(", length(");
    selectImg->bind(&imageSize, cDBS::bndOut);
    selectImg->build(")");
    selectImg->build(" from %s where ", tSeriesMedia->TableName());
    selectImg->bind(cTableSeriesMedia::fiSeriesId, cDBS::bndIn | cDBS::bndSet);
    selectImg->bind(cTableSeriesMedia::fiMediaType, cDBS::bndIn | cDBS::bndSet, " and ");
    status += selectImg->prepare();

    // select episode image

    selectEpisodeImg = new cDbStatement(tSeriesMedia);
    selectEpisodeImg->build("select ");
    selectEpisodeImg->bind(cTableSeriesMedia::fiMediaWidth, cDBS::bndOut);
    selectEpisodeImg->bind(cTableSeriesMedia::fiMediaHeight, cDBS::bndOut, ", ");
    selectEpisodeImg->bind(cTableSeriesMedia::fiMediaContent, cDBS::bndOut, ", ");
    selectEpisodeImg->build(", length(");
    selectEpisodeImg->bind(&episodeImageSize, cDBS::bndOut);
    selectEpisodeImg->build(")");
    selectEpisodeImg->build(" from %s where ", tSeriesMedia->TableName());
    selectEpisodeImg->bind(cTableSeriesMedia::fiSeriesId, cDBS::bndIn | cDBS::bndSet);
    selectEpisodeImg->bind(cTableSeriesMedia::fiEpisodeId, cDBS::bndIn | cDBS::bndSet, " and ");
    status += selectEpisodeImg->prepare();

    // select poster image

    selectSeasonPoster = new cDbStatement(tSeriesMedia);
    selectSeasonPoster->build("select ");
    selectSeasonPoster->bind(cTableSeriesMedia::fiMediaWidth, cDBS::bndOut);
    selectSeasonPoster->bind(cTableSeriesMedia::fiMediaHeight, cDBS::bndOut, ", ");
    selectSeasonPoster->bind(cTableSeriesMedia::fiMediaContent, cDBS::bndOut, ", ");
    selectSeasonPoster->build(", length(");
    selectSeasonPoster->bind(&posterSize, cDBS::bndOut);
    selectSeasonPoster->build(")");
    selectSeasonPoster->build(" from %s where ", tSeriesMedia->TableName());
    selectSeasonPoster->bind(cTableSeriesMedia::fiSeriesId, cDBS::bndIn | cDBS::bndSet);
    selectSeasonPoster->bind(cTableSeriesMedia::fiSeasonNumber, cDBS::bndIn | cDBS::bndSet, " and ");
    selectSeasonPoster->bind(cTableSeriesMedia::fiMediaType, cDBS::bndIn | cDBS::bndSet, " and ");
    status += selectSeasonPoster->prepare();

    // select actor

    selectActors = new cDbStatement(tSeriesActors);
    selectActors->build("select ");
    selectActors->setBindPrefix("series_actor.");
    selectActors->bind(cTableSeriesActor::fiActorId, cDBS::bndOut);
    selectActors->bind(cTableSeriesActor::fiActorName, cDBS::bndOut, ", ");
    selectActors->bind(cTableSeriesActor::fiActorRole, cDBS::bndOut, ", ");
    selectActors->clrBindPrefix();
    selectActors->build(" from %s, %s where ", tSeriesActors->TableName(), tSeriesMedia->TableName());
    selectActors->build(" %s.%s  = %s.%s ", tSeriesActors->TableName(),
                                            tSeriesActors->getField(cTableSeriesActor::fiActorId)->name,
                                            tSeriesMedia->TableName(),
                                            tSeriesMedia->getField(cTableSeriesMedia::fiActorId)->name);
    selectActors->setBindPrefix("series_media.");
    selectActors->bind(&series_id, cDBS::bndIn | cDBS::bndSet, " and ");
    selectActors->build(" order by %s, %s asc", tSeriesActors->getField(cTableSeriesActor::fiSortOrder)->name, 
                                                tSeriesActors->getField(cTableSeriesActor::fiActorRole)->name);    
    status += selectActors->prepare();

    // select actor thumbs

    selectActorThumbs = new cDbStatement(tSeriesMedia);
    selectActorThumbs->build("select ");
    selectActorThumbs->bind(cTableSeriesMedia::fiMediaWidth, cDBS::bndOut);
    selectActorThumbs->bind(cTableSeriesMedia::fiMediaHeight, cDBS::bndOut, ", ");
    selectActorThumbs->bind(cTableSeriesMedia::fiMediaContent, cDBS::bndOut, ", ");
    selectActorThumbs->build(", length(");
    selectActorThumbs->bind(&actorImageSize, cDBS::bndOut);
    selectActorThumbs->build(")");
    selectActorThumbs->build(" from %s where ", tSeriesMedia->TableName());
    selectActorThumbs->bind(cTableSeriesMedia::fiActorId, cDBS::bndIn | cDBS::bndSet);
    status += selectActorThumbs->prepare();
    
    // 

    selectSeriesMedia = new cDbStatement(tSeriesMedia);
    selectSeriesMedia->build("select ");
    selectSeriesMedia->bind(cTableSeriesMedia::fiMediaWidth, cDBS::bndOut);
    selectSeriesMedia->bind(cTableSeriesMedia::fiMediaHeight, cDBS::bndOut, ", ");
    selectSeriesMedia->bind(cTableSeriesMedia::fiMediaType, cDBS::bndOut, ", ");
    selectSeriesMedia->build(" from %s where ", tSeriesMedia->TableName());
    selectSeriesMedia->bind(cTableSeriesMedia::fiSeriesId, cDBS::bndIn | cDBS::bndSet);
    selectSeriesMedia->build(" and %s in (%d, %d, %d, %d, %d, %d, %d, %d, %d)",
                             tSeriesMedia->getField(cTableSeriesMedia::fiMediaType)->name,
                             msPoster1, msPoster2, msPoster3,
                             msFanart1, msFanart2, msFanart3,
                             msBanner1, msBanner2, msBanner3);
    status += selectSeriesMedia->prepare();

    // 

    selectMovieActors = new cDbStatement(tMovieActor);
    selectMovieActors->build("select ");
    selectMovieActors->setBindPrefix("act.");
    selectMovieActors->bind(cTableMovieActor::fiActorId, cDBS::bndOut);
    selectMovieActors->bind(cTableMovieActor::fiActorName, cDBS::bndOut, ", ");
    selectMovieActors->setBindPrefix("role.");
    selectMovieActors->bind(&actorRole, cDBS::bndOut, ", ");
    selectMovieActors->setBindPrefix("thumb.");
    selectMovieActors->bind(&thbWidth, cDBS::bndOut, ", ");
    selectMovieActors->bind(&thbHeight, cDBS::bndOut, ", ");
    selectMovieActors->clrBindPrefix();
    selectMovieActors->build(" from %s act, %s role, %s thumb where ", 
                        tMovieActor->TableName(), tMovieActors->TableName(), tMovieMedia->TableName());
    selectMovieActors->build("act.%s = role.%s ",
                        tMovieActor->getField(cTableMovieActor::fiActorId)->name, 
                        tMovieActors->getField(cTableMovieActors::fiActorId)->name);
    selectMovieActors->build(" and role.%s = thumb.%s ",
                        tMovieActors->getField(cTableMovieActors::fiActorId)->name, 
                        tMovieMedia->getField(cTableMovieMedia::fiActorId)->name);
    selectMovieActors->setBindPrefix("role.");
    selectMovieActors->bind(&actorMovie, cDBS::bndIn | cDBS::bndSet, " and ");
    status += selectMovieActors->prepare();

    // 

    selectMovieActorThumbs = new cDbStatement(tMovieMedia);
    selectMovieActorThumbs->build("select ");
    selectMovieActorThumbs->bind(cTableMovieMedia::fiMediaContent, cDBS::bndOut);
    selectMovieActorThumbs->build(", length(");
    selectMovieActorThumbs->bind(&imageSize, cDBS::bndOut);
    selectMovieActorThumbs->build(")");
    selectMovieActorThumbs->build(" from %s where ", tMovieMedia->TableName());
    selectMovieActorThumbs->bind(cTableMovieMedia::fiActorId, cDBS::bndIn | cDBS::bndSet);
    status += selectMovieActorThumbs->prepare();

    //

    selectMovieMedia = new cDbStatement(tMovieMedia);
    selectMovieMedia->build("select ");
    selectMovieMedia->bind(cTableMovieMedia::fiMediaWidth, cDBS::bndOut);
    selectMovieMedia->bind(cTableMovieMedia::fiMediaHeight, cDBS::bndOut, ", ");
    selectMovieMedia->bind(cTableMovieMedia::fiMediaType, cDBS::bndOut, ", ");
    selectMovieMedia->build(" from %s where ", tMovieMedia->TableName());
    selectMovieMedia->bind(cTableMovieMedia::fiMovieId, cDBS::bndIn | cDBS::bndSet);
    selectMovieMedia->build(" and %s in (%d, %d, %d, %d)",
                        tMovieMedia->getField(cTableMovieMedia::fiMediaType)->name,
                        mmPoster,
                        mmFanart,
                        mmCollectionPoster,
                        mmCollectionFanart);
    status += selectMovieMedia->prepare();

    // 

    selectMediaMovie = new cDbStatement(tMovieMedia);
    selectMediaMovie->build("select ");
    selectMediaMovie->bind(cTableMovieMedia::fiMediaContent, cDBS::bndOut);
    selectMediaMovie->build(", length(");
    selectMediaMovie->bind(&imageSize, cDBS::bndOut);
    selectMediaMovie->build(")");
    selectMediaMovie->build(" from %s where ", tMovieMedia->TableName());
    selectMediaMovie->bind(cTableMovieMedia::fiMovieId, cDBS::bndIn | cDBS::bndSet);
    selectMediaMovie->bind(cTableMovieMedia::fiMediaType, cDBS::bndIn | cDBS::bndSet, " and ");
    status += selectMediaMovie->prepare();

    // 

    selectRecordings = new cDbStatement(tRecordings);
    selectRecordings->build("select ");
    selectRecordings->bind(cTableRecordings::fiRecPath, cDBS::bndOut);
    selectRecordings->bind(cTableRecordings::fiRecStart, cDBS::bndOut, ", ");
    selectRecordings->bind(cTableRecordings::fiMovieId, cDBS::bndOut, ", ");
    selectRecordings->bind(cTableRecordings::fiSeriesId, cDBS::bndOut, ", ");
    selectRecordings->bind(cTableRecordings::fiEpisodeId, cDBS::bndOut, ", ");
    selectRecordings->build(" from %s where ", tRecordings->TableName());
    selectRecordings->bind(cTableRecordings::fiUuid, cDBS::bndIn | cDBS::bndSet);
    selectRecordings->build(" and %s = 0", tRecordings->getField(cTableRecordings::fiScrapNew)->name);
    status += selectRecordings->prepare();
    
    // 

    selectCleanupRecordings = new cDbStatement(tRecordings);
    selectCleanupRecordings->build("select ");
    selectCleanupRecordings->bind(cTableRecordings::fiRecPath, cDBS::bndOut);
    selectCleanupRecordings->bind(cTableRecordings::fiRecStart, cDBS::bndOut, ", ");
    selectCleanupRecordings->build(" from %s where ", tRecordings->TableName());
    selectCleanupRecordings->bind(cTableRecordings::fiUuid, cDBS::bndIn | cDBS::bndSet);
    status += selectCleanupRecordings->prepare();

    // clear temp episode cache table

    clearTempEpisodeTable = new cDbStatement(connection);
    clearTempEpisodeTable->build("truncate table %s",
                                 TempEpisodeTableName.c_str());
    status += clearTempEpisodeTable->prepare();
    
    // fill temp episode cache table using current series id
/*  insert into TempEpisodeTableName select distinct A.scrseriesepisode as episode_id from
               (select scrseriesepisode from epg2vdr.events where (scrseriesepisode > 0) and (scrseriesid = SeriesID)
                union all 
                select episode_id as scrseriesepisode from epg2vdr.recordings 
                where (episode_id > 0) and (series_id = SeriesID) and (uuid = UUID)) A */
    
    fillTempEpisodeTable = new cDbStatement(connection);
    fillTempEpisodeTable->build("insert into %s select distinct A.%s as %s from ",
                                TempEpisodeTableName.c_str(),
                                tEvents->getField(cTableEvents::fiScrSeriesEpisode)->name,
                                tEpisodes->getField(cTableSeriesEpisode::fiEpisodeId)->name);
    fillTempEpisodeTable->build("(select %s from %s where (%s > 0) and (",
                                tEvents->getField(cTableEvents::fiScrSeriesEpisode)->name,
                                tEvents->TableName(),
                                tEvents->getField(cTableEvents::fiScrSeriesEpisode)->name);
    fillTempEpisodeTable->bind(&events_ScrSeriesId, cDBS::bndIn | cDBS::bndSet);
    fillTempEpisodeTable->build(") union all select %s as %s from %s ",
                                tRecordings->getField(cTableRecordings::fiEpisodeId)->name,
                                tEvents->getField(cTableEvents::fiScrSeriesEpisode)->name,
                                tRecordings->TableName());
    fillTempEpisodeTable->build("where (episode_id > 0) and (",
                                tRecordings->getField(cTableRecordings::fiEpisodeId)->name);
    fillTempEpisodeTable->bind(&series_id, cDBS::bndIn | cDBS::bndSet);
    fillTempEpisodeTable->build(") and (");
    fillTempEpisodeTable->bind(&vdr_uuid, cDBS::bndIn | cDBS::bndSet);
    fillTempEpisodeTable->build(")) A");
    status += fillTempEpisodeTable->prepare();
    
    // get all used series (events/recordings), last_update of used episodes and max(scrsp) of all events 
/*  select A.series_id, A.series_name, A.series_last_scraped, A.series_last_updated, A.series_overview, A.series_firstaired, A.series_network,
     A.series_imdb_id, A.series_genre,A.series_rating, A.series_status, B.episode_last_updated, B.scrsp from epg2vdr.series A
    right join (select C.series_id,C.episode_id,max(D.episode_last_updated) as episode_last_updated,max(C.scrsp) as scrsp from 
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
    selectReadSeriesInit->bind(cTableSeries::fiSeriesId, cDBS::bndOut, "A.");
    selectReadSeriesInit->bind(cTableSeries::fiSeriesName, cDBS::bndOut, ", A.");
    selectReadSeriesInit->bind(cTableSeries::fiSeriesLastScraped, cDBS::bndOut, ", A.");
    selectReadSeriesInit->bind(cTableSeries::fiSeriesLastUpdated, cDBS::bndOut, ", A.");
    selectReadSeriesInit->bind(cTableSeries::fiSeriesOverview, cDBS::bndOut, ", A.");
    selectReadSeriesInit->bind(cTableSeries::fiSeriesFirstAired, cDBS::bndOut, ", A.");
    selectReadSeriesInit->bind(cTableSeries::fiSeriesNetwork, cDBS::bndOut, ", A.");
    selectReadSeriesInit->bind(cTableSeries::fiSeriesIMDBId, cDBS::bndOut, ", A.");
    selectReadSeriesInit->bind(cTableSeries::fiSeriesGenre, cDBS::bndOut, ", A.");
    selectReadSeriesInit->bind(cTableSeries::fiSeriesRating, cDBS::bndOut, ", A.");
    selectReadSeriesInit->bind(cTableSeries::fiSeriesStatus, cDBS::bndOut, ", A.");
    selectReadSeriesInit->bind(&episode_LastUpdate, cDBS::bndOut, ", B."); // max last update time of all episodes of current series with event/recording
    selectReadSeriesInit->bind(&event_scrsp, cDBS::bndOut, ", B."); // max last scrsp time of all events/recordings of current series 
    selectReadSeriesInit->build(" from %s A ",
                                tSeries->TableName());
    selectReadSeriesInit->build("right join (select C.%s, C.%s, max(D.%s) as %s, max(C.%s) as %s from ",
                                tSeries->getField(cTableSeries::fiSeriesId)->name,
                                tEpisodes->getField(cTableSeriesEpisode::fiEpisodeId)->name,
                                tEpisodes->getField(cTableSeriesEpisode::fiEpisodeLastUpdated)->name,
                                tEpisodes->getField(cTableSeriesEpisode::fiEpisodeLastUpdated)->name,
                                tEvents->getField(cTableEvents::fiScrSp)->name,
                                tEvents->getField(cTableEvents::fiScrSp)->name);
    selectReadSeriesInit->build("(select E.%s,E.%s, max(E.%s) as %s from ",
                                tSeries->getField(cTableSeries::fiSeriesId)->name,
                                tEpisodes->getField(cTableSeriesEpisode::fiEpisodeId)->name,
                                tEvents->getField(cTableEvents::fiScrSp)->name,
                                tEvents->getField(cTableEvents::fiScrSp)->name);    
    selectReadSeriesInit->build("(select %s as %s, %s as %s, %s from %s where (%s>0)",
                                tEvents->getField(cTableEvents::fiScrSeriesId)->name,
                                tSeries->getField(cTableSeries::fiSeriesId)->name,
                                tEvents->getField(cTableEvents::fiScrSeriesEpisode)->name,
                                tEpisodes->getField(cTableSeriesEpisode::fiEpisodeId)->name,
                                tEvents->getField(cTableEvents::fiScrSp)->name,
                                tEvents->TableName(),
                                tEvents->getField(cTableEvents::fiScrSeriesId)->name);
    selectReadSeriesInit->build(" union all ");
    selectReadSeriesInit->build("select %s, %s, %s from %s where (",
                                tRecordings->getField(cTableRecordings::fiSeriesId)->name,
                                tRecordings->getField(cTableRecordings::fiEpisodeId)->name,
                                tEvents->getField(cTableEvents::fiScrSp)->name,
                                tRecordings->TableName());
    selectReadSeriesInit->bind(&vdr_uuid, cDBS::bndIn | cDBS::bndSet);
    selectReadSeriesInit->build(") and (%s > 0)",
                                tRecordings->getField(cTableRecordings::fiSeriesId)->name);
    selectReadSeriesInit->build(") E group by %s,%s) C ",
                                tSeries->getField(cTableSeries::fiSeriesId)->name,
                                tEpisodes->getField(cTableSeriesEpisode::fiEpisodeId)->name);
    selectReadSeriesInit->build("left join %s D on (C.%s = D.%s and C.%s>0) ",
                                tEpisodes->TableName(),
                                tEpisodes->getField(cTableSeriesEpisode::fiEpisodeId)->name,
                                tEpisodes->getField(cTableSeriesEpisode::fiEpisodeId)->name,
                                tEpisodes->getField(cTableSeriesEpisode::fiEpisodeId)->name);
    selectReadSeriesInit->build("group by C.%s) B on (A.%s = B.%s) order by A.%s",
                                tSeries->getField(cTableSeries::fiSeriesId)->name,
                                tSeries->getField(cTableSeries::fiSeriesId)->name,
                                tSeries->getField(cTableSeries::fiSeriesId)->name,
                                tSeries->getField(cTableSeries::fiSeriesId)->name);
    status += selectReadSeriesInit->prepare();
    

    // get all used series (events/recordings), last_update of used episodes and max(scrsp) of all events, filtered by lastscrsp
/*  select A.series_id, A.series_name, A.series_last_scraped, A.series_last_updated, A.series_overview, A.series_firstaired, A.series_network,
     A.series_imdb_id, A.series_genre,A.series_rating, A.series_status, B.episode_last_updated, B.scrsp from epg2vdr.series A
    right join (select C.series_id,C.episode_id,max(D.episode_last_updated) as episode_last_updated,max(C.scrsp) as scrsp from 
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
    selectReadSeriesLastScrsp->bind(cTableSeries::fiSeriesId, cDBS::bndOut, "A.");
    selectReadSeriesLastScrsp->bind(cTableSeries::fiSeriesName, cDBS::bndOut, ", A.");
    selectReadSeriesLastScrsp->bind(cTableSeries::fiSeriesLastScraped, cDBS::bndOut, ", A.");
    selectReadSeriesLastScrsp->bind(cTableSeries::fiSeriesLastUpdated, cDBS::bndOut, ", A.");
    selectReadSeriesLastScrsp->bind(cTableSeries::fiSeriesOverview, cDBS::bndOut, ", A.");
    selectReadSeriesLastScrsp->bind(cTableSeries::fiSeriesFirstAired, cDBS::bndOut, ", A.");
    selectReadSeriesLastScrsp->bind(cTableSeries::fiSeriesNetwork, cDBS::bndOut, ", A.");
    selectReadSeriesLastScrsp->bind(cTableSeries::fiSeriesIMDBId, cDBS::bndOut, ", A.");
    selectReadSeriesLastScrsp->bind(cTableSeries::fiSeriesGenre, cDBS::bndOut, ", A.");
    selectReadSeriesLastScrsp->bind(cTableSeries::fiSeriesRating, cDBS::bndOut, ", A.");
    selectReadSeriesLastScrsp->bind(cTableSeries::fiSeriesStatus, cDBS::bndOut, ", A.");
    selectReadSeriesLastScrsp->bind(&episode_LastUpdate, cDBS::bndOut, ", B."); // max last update time of all episodes of current series with event/recording
    selectReadSeriesLastScrsp->bind(&event_scrsp, cDBS::bndOut, ", B."); // max last scrsp time of all events/recordings of current series 
    selectReadSeriesLastScrsp->build(" from %s A ",
                                     tSeries->TableName());
    selectReadSeriesLastScrsp->build("right join (select C.%s, C.%s, max(D.%s) as %s, max(C.%s) as %s from ",
                                     tSeries->getField(cTableSeries::fiSeriesId)->name,
                                     tEpisodes->getField(cTableSeriesEpisode::fiEpisodeId)->name,
                                     tEpisodes->getField(cTableSeriesEpisode::fiEpisodeLastUpdated)->name,
                                     tEpisodes->getField(cTableSeriesEpisode::fiEpisodeLastUpdated)->name,
                                     tEvents->getField(cTableEvents::fiScrSp)->name,
                                     tEvents->getField(cTableEvents::fiScrSp)->name);
    selectReadSeriesLastScrsp->build("(select E.%s,E.%s, max(E.%s) as %s from ",
                                     tSeries->getField(cTableSeries::fiSeriesId)->name,
                                     tEpisodes->getField(cTableSeriesEpisode::fiEpisodeId)->name,
                                     tEvents->getField(cTableEvents::fiScrSp)->name,
                                     tEvents->getField(cTableEvents::fiScrSp)->name);
    selectReadSeriesLastScrsp->build("(select %s as %s, %s as %s, %s from %s where (%s>0) and (",
                                     tEvents->getField(cTableEvents::fiScrSeriesId)->name,
                                     tSeries->getField(cTableSeries::fiSeriesId)->name,
                                     tEvents->getField(cTableEvents::fiScrSeriesEpisode)->name,
                                     tEpisodes->getField(cTableSeriesEpisode::fiEpisodeId)->name,
                                     tEvents->getField(cTableEvents::fiScrSp)->name,
                                     tEvents->TableName(),
                                     tEvents->getField(cTableEvents::fiScrSeriesId)->name);
    selectReadSeriesLastScrsp->bindCmp(0, &event_scrsp, ">");
    selectReadSeriesLastScrsp->build(") union all ");
    selectReadSeriesLastScrsp->build("select %s, %s, %s from %s where (",
                                     tRecordings->getField(cTableRecordings::fiSeriesId)->name,
                                     tRecordings->getField(cTableRecordings::fiEpisodeId)->name,
                                     tEvents->getField(cTableEvents::fiScrSp)->name,
                                     tRecordings->TableName());
    selectReadSeriesLastScrsp->bind(&vdr_uuid, cDBS::bndIn | cDBS::bndSet);
    selectReadSeriesLastScrsp->build(") and (%s > 0) and (",
                                     tRecordings->getField(cTableRecordings::fiSeriesId)->name);
    selectReadSeriesLastScrsp->bindCmp(0, &event_scrsp, ">");
    selectReadSeriesLastScrsp->build(")) E group by %s,%s) C ",
                                     tSeries->getField(cTableSeries::fiSeriesId)->name,
                                     tEpisodes->getField(cTableSeriesEpisode::fiEpisodeId)->name);
    selectReadSeriesLastScrsp->build("left join %s D on (C.%s = D.%s and C.%s>0) ",
                                     tEpisodes->TableName(),
                                     tEpisodes->getField(cTableSeriesEpisode::fiEpisodeId)->name,
                                     tEpisodes->getField(cTableSeriesEpisode::fiEpisodeId)->name,
                                     tEpisodes->getField(cTableSeriesEpisode::fiEpisodeId)->name);
    selectReadSeriesLastScrsp->build("group by C.%s) B on (A.%s = B.%s) order by A.%s",
                                     tSeries->getField(cTableSeries::fiSeriesId)->name,
                                     tSeries->getField(cTableSeries::fiSeriesId)->name,
                                     tSeries->getField(cTableSeries::fiSeriesId)->name,
                                     tSeries->getField(cTableSeries::fiSeriesId)->name);
    status += selectReadSeriesLastScrsp->prepare();
    
    // get all used episodes (with events/recordings) of one series
/*  select A.episode_id,A.episode_number,A.season_number,A.episode_name,A.episode_overview,A.episode_firstaired,A.episode_gueststars from epg2vdr.series_episode A 
    right join epg2vdr.usedepisodes B 
    on (B.episode_id = A.episode_id) where A.episode_id is not null */
    
    selectReadEpisodes = new cDbStatement(tEpisodes);
    selectReadEpisodes->build("select ");
    selectReadEpisodes->bind(cTableSeriesEpisode::fiEpisodeId, cDBS::bndOut, "A.");
    selectReadEpisodes->bind(cTableSeriesEpisode::fiEpisodeNumber, cDBS::bndOut, ", A.");
    selectReadEpisodes->bind(cTableSeriesEpisode::fiSeasonNumber, cDBS::bndOut, ", A.");
    selectReadEpisodes->bind(cTableSeriesEpisode::fiEpisodeName, cDBS::bndOut, ", A.");
    selectReadEpisodes->bind(cTableSeriesEpisode::fiEpisodeOverview, cDBS::bndOut, ", A.");
    selectReadEpisodes->bind(cTableSeriesEpisode::fiEpisodeFirstAired, cDBS::bndOut, ", A.");
    selectReadEpisodes->bind(cTableSeriesEpisode::fiEpisodeGuestStars, cDBS::bndOut, ", A.");
    selectReadEpisodes->bind(cTableSeriesEpisode::fiEpisodeRating, cDBS::bndOut, ", A.");
    selectReadEpisodes->bind(cTableSeriesEpisode::fiEpisodeLastUpdated, cDBS::bndOut, ", A.");
    selectReadEpisodes->bind(cTableSeriesEpisode::fiSeriesId, cDBS::bndOut, ", A.");
    selectReadEpisodes->build(" from %s A right join %s B ",
                              tEpisodes->TableName(),
                              TempEpisodeTableName.c_str());
    selectReadEpisodes->build("on (B.%s = A.%s) where A.%s is not null",
                              tEpisodes->getField(cTableSeriesEpisode::fiEpisodeId)->name,
                              tEpisodes->getField(cTableSeriesEpisode::fiEpisodeId)->name,
                              tEpisodes->getField(cTableSeriesEpisode::fiEpisodeId)->name);
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
    selectSeriesMediaActors->bind(cTableSeriesMedia::fiSeriesId, cDBS::bndOut, "A.");
    selectSeriesMediaActors->bind(cTableSeriesMedia::fiEpisodeId, cDBS::bndOut, ", A.");
    selectSeriesMediaActors->bind(cTableSeriesMedia::fiSeasonNumber, cDBS::bndOut, ", A.");
    selectSeriesMediaActors->bind(cTableSeriesMedia::fiActorId, cDBS::bndOut, ", A.");
    selectSeriesMediaActors->bind(cTableSeriesMedia::fiMediaType, cDBS::bndOut, ", A.");
    selectSeriesMediaActors->bind(cTableSeriesMedia::fiMediaWidth, cDBS::bndOut, ", A.");
    selectSeriesMediaActors->bind(cTableSeriesMedia::fiMediaHeight, cDBS::bndOut, ", A.");
    selectSeriesMediaActors->bind(tSeriesActors,cTableSeriesActor::fiActorId, cDBS::bndOut, ", C.");
    selectSeriesMediaActors->bind(tSeriesActors,cTableSeriesActor::fiActorName, cDBS::bndOut, ", C.");
    selectSeriesMediaActors->bind(tSeriesActors,cTableSeriesActor::fiActorRole, cDBS::bndOut, ", C.");
    selectSeriesMediaActors->bind(tSeriesActors,cTableSeriesActor::fiSortOrder, cDBS::bndOut, ", C.");
    selectSeriesMediaActors->build(" from %s A left join %s B on (B.%s = A.%s) ",
                                   tSeriesMedia->TableName(),
                                   TempEpisodeTableName.c_str(),
                                   tEpisodes->getField(cTableSeriesEpisode::fiEpisodeId)->name,
                                   tSeriesMedia->getField(cTableSeriesMedia::fiEpisodeId)->name);
    selectSeriesMediaActors->build("left join (select D.%s,D.%s,D.%s,D.%s from %s D ",
                                   tSeriesActors->getField(cTableSeriesActor::fiActorId)->name,                                    
                                   tSeriesActors->getField(cTableSeriesActor::fiActorName)->name,                                    
                                   tSeriesActors->getField(cTableSeriesActor::fiActorRole)->name,                                    
                                   tSeriesActors->getField(cTableSeriesActor::fiSortOrder)->name,                                    
                                   tSeriesActors->TableName());
    selectSeriesMediaActors->build("right join (select %s from %s where (",
                                   tSeriesMedia->getField(cTableSeriesMedia::fiActorId)->name,
                                   tSeriesMedia->TableName());
    selectSeriesMediaActors->bind(&series_id, cDBS::bndIn | cDBS::bndSet);
    selectSeriesMediaActors->build(") and (%s>0)) E on (D.%s = E.%s)) C on (A.%s=C.%s) where (",
                                   tSeriesMedia->getField(cTableSeriesMedia::fiActorId)->name,
                                   tSeriesActors->getField(cTableSeriesActor::fiActorId)->name,
                                   tSeriesMedia->getField(cTableSeriesMedia::fiActorId)->name,
                                   tSeriesMedia->getField(cTableSeriesMedia::fiActorId)->name,
                                   tSeriesMedia->getField(cTableSeriesMedia::fiActorId)->name);
    selectSeriesMediaActors->bind(&series_id, cDBS::bndIn | cDBS::bndSet);
    selectSeriesMediaActors->build(") and ((B.%s is not null) or (A.%s = 0))  order by %s,%s",
                                   tEpisodes->getField(cTableSeriesEpisode::fiEpisodeId)->name,
                                   tSeriesMedia->getField(cTableSeriesMedia::fiEpisodeId)->name,
                                   tSeriesMedia->getField(cTableSeriesMedia::fiSeasonNumber)->name,
                                   tSeriesActors->getField(cTableSeriesActor::fiSortOrder)->name);
    status += selectSeriesMediaActors->prepare();
    
    selectSeriesImage = new cDbStatement(tSeriesMedia);
    selectSeriesImage->build("select ");
    selectSeriesImage->bind(cTableSeriesMedia::fiMediaWidth, cDBS::bndOut);
    selectSeriesImage->bind(cTableSeriesMedia::fiMediaHeight, cDBS::bndOut, ", ");
    selectSeriesImage->bind(cTableSeriesMedia::fiMediaContent, cDBS::bndOut, ", ");
    selectSeriesImage->build(", length(");
    selectSeriesImage->bind(&imageSize, cDBS::bndOut);
    selectSeriesImage->build(")");
    selectSeriesImage->build(" from %s where ", tSeriesMedia->TableName());
    selectSeriesImage->bind(cTableSeriesMedia::fiSeriesId, cDBS::bndIn | cDBS::bndSet);
    selectSeriesImage->bind(cTableSeriesMedia::fiSeasonNumber, cDBS::bndIn | cDBS::bndSet, " and ");
    selectSeriesImage->bind(cTableSeriesMedia::fiEpisodeId, cDBS::bndIn | cDBS::bndSet, " and ");
    selectSeriesImage->bind(cTableSeriesMedia::fiActorId, cDBS::bndIn | cDBS::bndSet, " and ");
    selectSeriesImage->bind(cTableSeriesMedia::fiMediaType, cDBS::bndIn | cDBS::bndSet, " and ");
    status += selectSeriesImage->prepare();
    
    // get all used movies (events/recordings) and max(scrsp) of all events
/*  select A.movie_id, A.movie_title, A.movie_original_title, A.movie_tagline, A.movie_overview, A.movie_adult, A.movie_collection_name, A.movie_budget,
     A.movie_revenue, A.movie_genres, A.movie_homepage, A.movie_release_date, A.movie_runtime, A.movie_popularity, A.movie_vote_average, B.scrsp from epg2vdr.movie A
    right join (select C.movie_id, max(C.scrsp) as scrsp from 
                           (select scrmovieid as movie_id, scrsp from epg2vdr.events where (scrmovieid>0)
                            union all 
                            select movie_id, scrsp from epg2vdr.recordings
                            where (uuid = "4E231345-9219-4FEC-8666-09C60D536E68") and (movie_id > 0)) C group by movie_id) B
    on (A.movie_id = B.movie_id)
    order by A.movie_id */
    
    selectReadMoviesInit = new cDbStatement(tMovies);
    selectReadMoviesInit->build("select ");
    selectReadMoviesInit->bind(cTableMovies::fiMovieId, cDBS::bndOut, "A.");
    selectReadMoviesInit->bind(cTableMovies::fiTitle, cDBS::bndOut, ", A.");
    selectReadMoviesInit->bind(cTableMovies::fiOriginalTitle, cDBS::bndOut, ", A.");
    selectReadMoviesInit->bind(cTableMovies::fiTagline, cDBS::bndOut, ", A.");
    selectReadMoviesInit->bind(cTableMovies::fiOverview, cDBS::bndOut, ", A.");
    selectReadMoviesInit->bind(cTableMovies::fiIsAdult, cDBS::bndOut, ", A.");
    selectReadMoviesInit->bind(cTableMovies::fiCollectionId, cDBS::bndOut, ", A.");
    selectReadMoviesInit->bind(cTableMovies::fiCollectionName, cDBS::bndOut, ", A.");
    selectReadMoviesInit->bind(cTableMovies::fiBudget, cDBS::bndOut, ", A.");
    selectReadMoviesInit->bind(cTableMovies::fiRevenue, cDBS::bndOut, ", A.");
    selectReadMoviesInit->bind(cTableMovies::fiGenres, cDBS::bndOut, ", A.");
    selectReadMoviesInit->bind(cTableMovies::fiHomepage, cDBS::bndOut, ", A.");
    selectReadMoviesInit->bind(cTableMovies::fiReleaaseDate, cDBS::bndOut, ", A.");
    selectReadMoviesInit->bind(cTableMovies::fiRuntime, cDBS::bndOut, ", A.");
    selectReadMoviesInit->bind(cTableMovies::fiPopularity, cDBS::bndOut, ", A.");
    selectReadMoviesInit->bind(cTableMovies::fiVoteAverage, cDBS::bndOut, ", A.");
    selectReadMoviesInit->bind(&event_scrsp, cDBS::bndOut, ", B."); // max last scrsp time of all events/recordings of current movie 
    selectReadMoviesInit->build(" from %s A ",
                                tMovies->TableName());
    selectReadMoviesInit->build("right join (select C.%s, max(C.%s) as %s from ",
                                tMovies->getField(cTableMovies::fiMovieId)->name,
                                tEvents->getField(cTableEvents::fiScrSp)->name,
                                tEvents->getField(cTableEvents::fiScrSp)->name);
    selectReadMoviesInit->build("(select %s as %s, %s from %s where (%s>0)",
                                tEvents->getField(cTableEvents::fiScrMovieId)->name,
                                tMovies->getField(cTableMovies::fiMovieId)->name,
                                tEvents->getField(cTableEvents::fiScrSp)->name,
                                tEvents->TableName(),
                                tEvents->getField(cTableEvents::fiScrMovieId)->name);
    selectReadMoviesInit->build(" union all ");
    selectReadMoviesInit->build("select %s, %s from %s where (",
                                tRecordings->getField(cTableRecordings::fiMovieId)->name,
                                tEvents->getField(cTableEvents::fiScrSp)->name,
                                tRecordings->TableName());
    selectReadMoviesInit->bind(&vdr_uuid, cDBS::bndIn | cDBS::bndSet);
    selectReadMoviesInit->build(") and (%s > 0)",
                                tRecordings->getField(cTableRecordings::fiMovieId)->name);
    selectReadMoviesInit->build(") C group by %s) B on (A.%s = B.%s) order by A.%s",
                                tRecordings->getField(cTableRecordings::fiMovieId)->name,
                                tRecordings->getField(cTableRecordings::fiMovieId)->name,
                                tRecordings->getField(cTableRecordings::fiMovieId)->name,
                                tRecordings->getField(cTableRecordings::fiMovieId)->name);
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
    selectReadMoviesLastScrsp->bind(cTableMovies::fiMovieId, cDBS::bndOut, "A.");
    selectReadMoviesLastScrsp->bind(cTableMovies::fiTitle, cDBS::bndOut, ", A.");
    selectReadMoviesLastScrsp->bind(cTableMovies::fiOriginalTitle, cDBS::bndOut, ", A.");
    selectReadMoviesLastScrsp->bind(cTableMovies::fiTagline, cDBS::bndOut, ", A.");
    selectReadMoviesLastScrsp->bind(cTableMovies::fiOverview, cDBS::bndOut, ", A.");
    selectReadMoviesLastScrsp->bind(cTableMovies::fiIsAdult, cDBS::bndOut, ", A.");
    selectReadMoviesLastScrsp->bind(cTableMovies::fiCollectionId, cDBS::bndOut, ", A.");
    selectReadMoviesLastScrsp->bind(cTableMovies::fiCollectionName, cDBS::bndOut, ", A.");
    selectReadMoviesLastScrsp->bind(cTableMovies::fiBudget, cDBS::bndOut, ", A.");
    selectReadMoviesLastScrsp->bind(cTableMovies::fiRevenue, cDBS::bndOut, ", A.");
    selectReadMoviesLastScrsp->bind(cTableMovies::fiGenres, cDBS::bndOut, ", A.");
    selectReadMoviesLastScrsp->bind(cTableMovies::fiHomepage, cDBS::bndOut, ", A.");
    selectReadMoviesLastScrsp->bind(cTableMovies::fiReleaaseDate, cDBS::bndOut, ", A.");
    selectReadMoviesLastScrsp->bind(cTableMovies::fiRuntime, cDBS::bndOut, ", A.");
    selectReadMoviesLastScrsp->bind(cTableMovies::fiPopularity, cDBS::bndOut, ", A.");
    selectReadMoviesLastScrsp->bind(cTableMovies::fiVoteAverage, cDBS::bndOut, ", A.");
    selectReadMoviesLastScrsp->bind(&event_scrsp, cDBS::bndOut, ", B."); // max last scrsp time of all events/recordings of current movie 
    selectReadMoviesLastScrsp->build(" from %s A ",
                                     tMovies->TableName());
    selectReadMoviesLastScrsp->build("right join (select C.%s, max(C.%s) as %s from ",
                                     tMovies->getField(cTableMovies::fiMovieId)->name,
                                     tEvents->getField(cTableEvents::fiScrSp)->name,
                                     tEvents->getField(cTableEvents::fiScrSp)->name);
    selectReadMoviesLastScrsp->build("(select %s as %s, %s from %s where (%s>0) and (",
                                     tEvents->getField(cTableEvents::fiScrMovieId)->name,
                                     tMovies->getField(cTableMovies::fiMovieId)->name,
                                     tEvents->getField(cTableEvents::fiScrSp)->name,
                                     tEvents->TableName(),
                                     tEvents->getField(cTableEvents::fiScrMovieId)->name);
    selectReadMoviesLastScrsp->bindCmp(0, &event_scrsp, ">");
    selectReadMoviesLastScrsp->build(") union all ");
    selectReadMoviesLastScrsp->build("select %s, %s from %s where (",
                                     tRecordings->getField(cTableRecordings::fiMovieId)->name,
                                     tEvents->getField(cTableEvents::fiScrSp)->name,
                                     tRecordings->TableName());
    selectReadMoviesLastScrsp->bind(&vdr_uuid, cDBS::bndIn | cDBS::bndSet);
    selectReadMoviesLastScrsp->build(") and (%s > 0) and (",
                                     tRecordings->getField(cTableRecordings::fiMovieId)->name);
    selectReadMoviesLastScrsp->bindCmp(0, &event_scrsp, ">");
    selectReadMoviesLastScrsp->build(")) C group by %s) B on (A.%s = B.%s) order by A.%s",
                                     tRecordings->getField(cTableRecordings::fiMovieId)->name,
                                     tRecordings->getField(cTableRecordings::fiMovieId)->name,
                                     tRecordings->getField(cTableRecordings::fiMovieId)->name,
                                     tRecordings->getField(cTableRecordings::fiMovieId)->name);
    status += selectReadMoviesLastScrsp->prepare();

    selectMovieMediaFast = new cDbStatement(tMovieMedia);
    selectMovieMediaFast->build("select ");
    selectMovieMediaFast->bind(cTableMovieMedia::fiActorId, cDBS::bndOut);
    selectMovieMediaFast->bind(cTableMovieMedia::fiMediaType, cDBS::bndOut, ", ");
    selectMovieMediaFast->bind(cTableMovieMedia::fiMediaWidth, cDBS::bndOut, ", ");
    selectMovieMediaFast->bind(cTableMovieMedia::fiMediaHeight, cDBS::bndOut, ", ");
    selectMovieMediaFast->build(" from %s where ", tMovieMedia->TableName());
    selectMovieMediaFast->bind(cTableMovieMedia::fiMovieId, cDBS::bndIn | cDBS::bndSet);
    status += selectMovieMediaFast->prepare();
    
    selectMovieImage = new cDbStatement(tMovieMedia);
    selectMovieImage->build("select ");
    selectMovieImage->bind(cTableMovieMedia::fiMediaContent, cDBS::bndOut);
    selectMovieImage->bind(cTableMovieMedia::fiMediaWidth, cDBS::bndOut, ", ");
    selectMovieImage->bind(cTableMovieMedia::fiMediaHeight, cDBS::bndOut, ", ");
    selectMovieImage->build(", length(");
    selectMovieImage->bind(&imageSize, cDBS::bndOut);
    selectMovieImage->build(")");
    selectMovieImage->build(" from %s where ", tMovieMedia->TableName());
    selectMovieImage->bind(cTableMovieMedia::fiMovieId, cDBS::bndIn | cDBS::bndSet);
    selectMovieImage->bind(cTableMovieMedia::fiActorId, cDBS::bndIn | cDBS::bndSet, " and ");
    selectMovieImage->bind(cTableMovieMedia::fiMediaType, cDBS::bndIn | cDBS::bndSet, " and ");
    status += selectMovieImage->prepare();
    
    return status;
}

int cUpdate::exitDb() {

    delete selectReadScrapedEvents;     selectReadScrapedEvents = 0;
    delete selectReadScrapedEventsInit; selectReadScrapedEventsInit = 0;
    delete selectImg;                   selectImg = 0;
    delete selectEpisodeImg;            selectEpisodeImg = 0;
    delete selectSeasonPoster;          selectSeasonPoster = 0;
    delete selectActors;                selectActors = 0;
    delete selectActorThumbs;           selectActorThumbs = 0;
    delete selectSeriesMedia;           selectSeriesMedia = 0;
    delete selectMovieActors;           selectMovieActors = 0;
    delete selectMovieActorThumbs;      selectMovieActorThumbs = 0;
    delete selectMovieMedia;            selectMovieMedia = 0;
    delete selectMediaMovie;            selectMediaMovie = 0;
    delete selectRecordings;            selectRecordings = 0;
    delete selectCleanupRecordings;     selectCleanupRecordings = 0;
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

    delete vdrDb;         vdrDb = 0;
    delete tEvents;       tEvents = 0;
    delete tSeries;       tSeries = 0;
    delete tEpisodes;     tEpisodes = 0;
    delete tSeriesMedia;  tSeriesMedia = 0;
    delete tSeriesActors; tSeriesActors = 0;
    delete tMovies;       tMovies = 0;
    delete tMovieActor;   tMovieActor = 0;
    delete tMovieActors;  tMovieActors = 0;
    delete tMovieMedia;   tMovieMedia = 0;
    delete tRecordings;   tRecordings = 0;

    // try to delete temp. episode cache table
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

int cUpdate::CheckConnection(int& timeout) {
    static int retry = 0;
    timeout = retry < 5 ? 10 : 60;
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
    vdrDb->setValue(cTableVdrs::fiUuid, EPGDNAME);

    if (vdrDb->find()) {
        Es::State epgdState = cEpgdState::toState(vdrDb->getStrValue(cTableVdrs::fiState));
        // ignore esBusyImages until we don't write this table
        if (epgdState >= cEpgdState::esBusy && epgdState < cEpgdState::esBusyImages)
            busy = true;
    }

    vdrDb->reset();
    return busy;
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
    tEvents->setValue(cTableEvents::fiScrSp, lastScrap);
    
    lastWait = GetTimems(); // init time for CheckRunningAndWait
    for (int res = select->find(); res; res = select->fetch() && CheckRunningAndWait()) {
        eventId = tEvents->getIntValue(cTableEvents::fiMasterId);
        channelId = tEvents->getStrValue(cTableEvents::fiChannelId);
        seriesId = tEvents->getIntValue(cTableEvents::fiScrSeriesId);
        episodeId = tEvents->getIntValue(cTableEvents::fiScrSeriesEpisode);
        movieId = tEvents->getIntValue(cTableEvents::fiScrMovieId);
        scrapManager->AddEvent(eventId, channelId, seriesId, episodeId, movieId);
        lastScrap = max(lastScrap, (int)tEvents->getIntValue(cTableEvents::fiScrSp));
        numNew++;
    }

    select->freeResult();

    return numNew;
}

//***************************************************************************
// SERIES
//***************************************************************************

int cUpdate::ReadSeries(bool isRec) {
    scrapManager->InitIterator(isRec);
    int seriesId = 0;
    int episodeId = 0;
    
    if (!CreateDirectory(config.imageDir))
        return 0;
    if (!CreateDirectory(imgPathSeries))
        return 0;

    bool isNew = false;
    int numNew = 0;
    int i = 0;
    lastWait = GetTimems(); // init time for CheckRunningAndWait
    while (scrapManager->GetNextSeries(isRec, seriesId, episodeId) && CheckRunningAndWait()) {
        cTVDBSeries *series = scrapManager->GetSeries(seriesId);
        if (!series) {
            tSeries->clear();
            tSeries->setValue(cTableSeries::fiSeriesId, seriesId);
            int res = tSeries->find();
            if (res) {
                series = scrapManager->AddSeries(tSeries);
            }
            isNew = true;
        } else {
            isNew = false;
        }
        if (series) {
            stringstream sPath("");
            sPath << imgPathSeries << "/" << seriesId;
            string seriesPath = sPath.str();
            if (episodeId) {
                ReadEpisode(episodeId, series, seriesPath);
            }
            if (isNew) {
                ReadSeriesActors(series, seriesPath);
                LoadSeriesMedia(series, seriesPath);
            }
        }
        if (++i % 500 == 0)
            tell(0, "Loaded %d series, continuing...", i);
        numNew++;
    }
    return numNew;
}

// read all series with event from sql-db
int cUpdate::ReadSeriesFast(long &maxscrsp) {
    // try to create directorys first (if not exists)
    if (!CreateDirectory(config.imageDir))
        return 0;
    if (!CreateDirectory(imgPathSeries))
        return 0;

    cTVDBSeries *series;
    bool isNew = false;
    int numNew = 0;
    int i = 0;

    vdr_uuid.setValue(config.uuid.c_str()); // search all recordigs of this vdr uuid
    tSeries->clear();
    event_scrsp.setValue(maxscrsp); // use last max scrsp as start value for filter
    
    cDbStatement* select = maxscrsp > 0 ? selectReadSeriesLastScrsp : selectReadSeriesInit;
    
    lastWait = GetTimems(); // init time for CheckRunningAndWait
    long lastLog = GetTimems(); // init time for logging
    for (int res = select->find(); res; res = select->fetch() && CheckRunningAndWait()) {
        series = scrapManager->GetSeries(tSeries->getIntValue(cTableSeries::fiSeriesId));
        if (!series) {
            // add new series with information of current row
            series = scrapManager->AddSeries(tSeries); 
            series->lastepisodeupdate = episode_LastUpdate.getIntValue();
            series->lastscraped = event_scrsp.getIntValue(); 
            isNew = true;
        } else {
            // check if something changed for series
            isNew = false;
            series->updateimages = ((series->lastupdate < tSeries->getIntValue(cTableSeries::fiSeriesLastUpdated)) || // series get updated on thetvdb
                                    (series->lastepisodeupdate < episode_LastUpdate.getIntValue())); // min one used episode get updated on thetvdb         
            series->updatecontent = (series->updateimages) || (series->lastscraped < event_scrsp.getIntValue()); // new events or recodings with this series 
            if ((series->updateimages) || (series->updatecontent)) { // something to refresh for series
                scrapManager->UpdateSeries(series,tSeries); // update series
                series->lastepisodeupdate = episode_LastUpdate.getIntValue();
                series->lastscraped = event_scrsp.getIntValue(); 
                isNew = true;
            }
        }
        maxscrsp = max(maxscrsp, series->lastscraped);
        i++;
        if (GetTimeDiffms(lastLog)>LogPeriode) {
            tell(0, "Loaded %d series, continuing...", i);
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
            tell(0, "Handled %d of %d series, continuing...", i, seriescount);
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
    series_id.setValue(series->id); // we would search for all episodes for this series
    events_ScrSeriesId.setValue(series->id); // we would search for all events for this series
    vdr_uuid.setValue(config.uuid.c_str()); // search all recordigs of this vdr uuid

    clearTempEpisodeTable->find(); // clear temp table
    fillTempEpisodeTable->find(); // fill temp table using current series id
    for (int res = selectReadEpisodes->find(); res; res = selectReadEpisodes->fetch() && CheckRunningAndWait()) {
        // search for episode (I could call AddEpisode, because this will overwrite existing values, but then I could not count new episodes)
        episode = series->GetEpisode(tEpisodes->getIntValue(cTableSeriesEpisode::fiEpisodeId));
        if (!episode) {
            scrapManager->AddSeriesEpisode(series,tEpisodes);
            numNew++;
        } else {
            if (episode->lastupdate < tEpisodes->getIntValue(cTableSeriesEpisode::fiEpisodeLastUpdated)) {
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
    stringstream imageName("");
    imageName << imgPathSeries << "/" << series->id;
    string seriesPath = imageName.str();
    string thumbName = "";

    bool imgNeedRefresh = false;
    int imgWidth = 0;
    int imgHeight = 0;
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
        
        mediaType = tSeriesMedia->getIntValue(cTableSeriesMedia::fiMediaType);
        imgWidth = tSeriesMedia->getIntValue(cTableSeriesMedia::fiMediaWidth);
        imgHeight = tSeriesMedia->getIntValue(cTableSeriesMedia::fiMediaHeight);
        imgNeedRefresh = false;
        // check type of media first
        switch (mediaType) {
            case msBanner1:
                imgNeedRefresh = fileDateManager.CheckImageNeedRefresh("banner1.jpg", series->lastupdate);
                CheckSeriesMedia(series, mediaType, imgWidth, imgHeight,"banner1.jpg", 0, imgNeedRefresh);
                break;
            case msBanner2:
                imgNeedRefresh = fileDateManager.CheckImageNeedRefresh("banner2.jpg", series->lastupdate);
                CheckSeriesMedia(series, mediaType, imgWidth, imgHeight,"banner2.jpg", 0, imgNeedRefresh);
                break;
            case msBanner3:
                imgNeedRefresh = fileDateManager.CheckImageNeedRefresh("banner3.jpg", series->lastupdate);
                CheckSeriesMedia(series, mediaType, imgWidth, imgHeight,"banner3.jpg", 0, imgNeedRefresh);
                break;
            case msPoster1:
                imgNeedRefresh = fileDateManager.CheckImageNeedRefreshThumb("poster1.jpg", "poster_thumb.jpg", series->lastupdate);
                CheckSeriesMedia(series, mediaType, imgWidth, imgHeight, "poster1.jpg", 0, imgNeedRefresh);
                CheckSeriesMedia(series, msPosterThumb, imgWidth/5, imgHeight/5, "poster_thumb.jpg", 0, imgNeedRefresh);
                break;
            case msPoster2:
                imgNeedRefresh = fileDateManager.CheckImageNeedRefresh("poster2.jpg", series->lastupdate);
                CheckSeriesMedia(series, mediaType, imgWidth, imgHeight,"poster2.jpg", 0, imgNeedRefresh);
                break;
            case msPoster3:
                imgNeedRefresh = fileDateManager.CheckImageNeedRefresh("poster3.jpg", series->lastupdate);
                CheckSeriesMedia(series, mediaType, imgWidth, imgHeight,"poster3.jpg", 0, imgNeedRefresh);
                break;
            case msFanart1:
                imgNeedRefresh = fileDateManager.CheckImageNeedRefresh("fanart1.jpg", series->lastupdate);
                CheckSeriesMedia(series, mediaType, imgWidth, imgHeight,"fanart1.jpg", 0, imgNeedRefresh);
                break;
            case msFanart2:
                imgNeedRefresh = fileDateManager.CheckImageNeedRefresh("fanart2.jpg", series->lastupdate);
                CheckSeriesMedia(series, mediaType, imgWidth, imgHeight,"fanart2.jpg", 0, imgNeedRefresh);
                break;
            case msFanart3:
                imgNeedRefresh = fileDateManager.CheckImageNeedRefresh("fanart3.jpg", series->lastupdate);
                CheckSeriesMedia(series, mediaType, imgWidth, imgHeight,"fanart3.jpg", 0, imgNeedRefresh);
                break;
            case msSeasonPoster:
                season = tSeriesMedia->getIntValue(cTableSeriesMedia::fiSeasonNumber);
                imageName.str("");
                imageName << "season_" << season << "_thumb.jpg";
                thumbName = imageName.str();
                imageName.str("");
                imageName << "season_" << season << ".jpg";
                imgNeedRefresh = fileDateManager.CheckImageNeedRefreshThumb(imageName.str(),thumbName,series->lastupdate);
                CheckSeriesMedia(series, mediaType, imgWidth, imgHeight, imageName.str(), season, imgNeedRefresh);
                CheckSeriesMedia(series, msSeasonPosterThumb, imgWidth/2, imgHeight/2, thumbName, season, imgNeedRefresh);
                break;
            case msEpisodePic:
                episode = series->GetEpisode(tSeriesMedia->getIntValue(cTableSeriesMedia::fiEpisodeId));
                if (episode) {
                    // only continue if episode exist
                    imageName.str("");
                    imageName << "episode_" << episode->id << ".jpg";
                    imgNeedRefresh = fileDateManager.CheckImageNeedRefresh(imageName.str(),episode->lastupdate); // use lastupdate from episode for episode images
                    imageName.str("");
                    imageName << seriesPath << "/" << "episode_" << episode->id << ".jpg";
                    series->SetEpisodeImage(episode->id, imgWidth, imgHeight, imageName.str(), imgNeedRefresh);
                }    
                break;
            case msActorThumb:
                actor = series->GetActor(tSeriesMedia->getIntValue(cTableSeriesMedia::fiActorId));
                if (!actor) {    
                    actor = scrapManager->AddSeriesActor(series,tSeriesActors); // can use all columns of tSeriesActors (get filled from select too)
                } else {    
                    scrapManager->UpdateSeriesActor(actor,tSeriesActors); // update actor
                }
                imageName.str("");
                imageName << "actor_" << actor->id << ".jpg";
                imgNeedRefresh = fileDateManager.CheckImageNeedRefresh(imageName.str(),series->lastupdate);
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
void cUpdate::ReadSeriesImagesFast(int &newImages, int &emptySeasonPosters) { 
    newImages = 0;
    emptySeasonPosters = 0;
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
                        if (ReadSeriesImageFast(series->id,0,0,actor->id,msActorThumb,actor->actorThumb,NULL,1)) {
                            imageName.str("");
                            imageName << "actor_" << actor->id << ".jpg";
                            fileDateManager.SetLastUpdated(imageName.str(),series->lastupdate); // use lastupdated of series for actor images
                            newImages++;
                        } else {
                           tell(0,"failed to read image (series %d, actor %d)",series->id,actor->id);
                        }    
                    }     
                }    
            }    
            
            // continue with epiode images
            for (int res = series->GetEpisodeFirst(episode); res; res = series->GetEpisodeNext(episode) && CheckRunningAndWait()) {
                if (episode->episodeImage) {
                    if (episode->episodeImage->needrefresh) {
                        // have to load this image from DB
                        if (ReadSeriesImageFast(series->id,episode->season,episode->id,0,msEpisodePic,episode->episodeImage,NULL,1)) {
                            imageName.str("");
                            imageName << "episode_" << episode->id << ".jpg";
                            fileDateManager.SetLastUpdated(imageName.str(),episode->lastupdate); // use lastupdated of episode for episode images
                            newImages++;
                        } else {
                           tell(0,"failed to read image (series %d, episode %d)",series->id,episode->id);
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
                        if (ReadSeriesImageFast(series->id,season,0,0,msSeasonPoster,media,mediaThumb,2)) {
                            imageName.str("");
                            imageName << "season_" << season << ".jpg";
                            fileDateManager.SetLastUpdated(imageName.str(),series->lastupdate); // use lastupdated of series for season images
                            newImages++;
                        } else {
                            // delete season posters from fileDateManager which are not available
                            imageName.str("");
                            imageName << "season_" << season << ".jpg";
                            fileDateManager.DeleteImage(imageName.str());
                            emptySeasonPosters++;
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
                        if (ReadSeriesImageFast(series->id,0,0,0,mediaType,media,mediaThumb,5)) {
                            fileDateManager.SetLastUpdated(mediaName,series->lastupdate); // use lastupdated of series for series images
                            newImages++;
                        } else {
                           tell(0,"failed to read image (series %d, mediatype %d)",series->id,mediaType);
                        }    
                    }    
                }
            }
            fileDateManager.SaveFileDateList(); // save updated list
            
            series->updateimages = false; // have updated all images
        }

        i++;
        if (GetTimeDiffms(lastLog)>LogPeriode) {
            tell(0, "Handled %d images (for %d series), continuing...", newImages,i);
            tell(0, "Handled %d of %d series (loaded %d new images), continuing...", i, seriesCount, newImages);
            lastLog = GetTimems();
        }    
    }  
}

// read real image data from sql-db
bool cUpdate::ReadSeriesImageFast(int seriesId, int season, int episodeId, int actorId, int mediaType, cTVDBMedia *media, cTVDBMedia *mediaThumb, int shrinkFactor) {
    bool imageSaved = false;
    tSeriesMedia->clear();
    tSeriesMedia->setValue(cTableSeriesMedia::fiSeriesId, seriesId);
    tSeriesMedia->setValue(cTableSeriesMedia::fiSeasonNumber, season);
    tSeriesMedia->setValue(cTableSeriesMedia::fiEpisodeId, episodeId);
    tSeriesMedia->setValue(cTableSeriesMedia::fiActorId, actorId);
    tSeriesMedia->setValue(cTableSeriesMedia::fiMediaType, mediaType);
    //tell(0,"serie: %d / season: %d / episode: %d / actor: %d / media: %d / pfad: %s",seriesId,season,episodeId,actorId,mediaType,media->path.c_str());
    int res = selectSeriesImage->find();
    if (res) {
        media->width = tSeriesMedia->getIntValue(cTableSeriesMedia::fiMediaWidth);
        media->height = tSeriesMedia->getIntValue(cTableSeriesMedia::fiMediaHeight);
        int size = imageSize.getIntValue();
        if (size > 0) {
            if (FILE* fh = fopen(media->path.c_str(), "w")) {
                fwrite(tSeriesMedia->getStrValue(cTableSeriesMedia::fiMediaContent), 1, size, fh);
                fclose(fh);
                media->needrefresh = false; // reset update flag
                media->mediavalid = true; // we have a image file for this media
                imageSaved = true;
            }
            if (mediaThumb) {
                CreateThumbnail(media->path, mediaThumb->path, media->width, media->height, shrinkFactor);
                mediaThumb->needrefresh = false;
                mediaThumb->mediavalid = true; // we have a image file for this media
            }
        } else {
            media->needrefresh = false; // reset update flag for empty images also
        }    
    }
    selectSeriesImage->freeResult();
    return imageSaved;
}

void cUpdate::ReadEpisode(int episodeId, cTVDBSeries *series, string path) {
    tEpisodes->clear();
    tEpisodes->setValue(cTableSeriesEpisode::fiEpisodeId, episodeId);
    int res = tEpisodes->find();
    if (res) {
        scrapManager->AddSeriesEpisode(series, tEpisodes);
        LoadEpisodeImage(series, episodeId, path);
        int season = tEpisodes->getIntValue(cTableSeriesEpisode::fiSeasonNumber);
        if (season > 0)
            LoadSeasonPoster(series, season, path);
    }
    return;
}

void cUpdate::LoadEpisodeImage(cTVDBSeries *series, int episodeId, string path) {
    stringstream iPath("");
    iPath << path << "/" << "episode_" << episodeId << ".jpg";
    string imgPath = iPath.str();
    bool imgExists = FileExists(imgPath);
    if (!imgExists)
        if (!CreateDirectory(path))
            return;

    tSeriesMedia->clear();
    tSeriesMedia->setValue(cTableSeriesMedia::fiSeriesId, series->id);
    tSeriesMedia->setValue(cTableSeriesMedia::fiEpisodeId, episodeId);

    int res = selectEpisodeImg->find();

    if (res) {
        if (!imgExists) {
            int size = episodeImageSize.getIntValue();
            if (FILE* fh = fopen(imgPath.c_str(), "w")) {
                fwrite(tSeriesMedia->getStrValue(cTableSeriesMedia::fiMediaContent), 1, size, fh);
                fclose(fh);
            }
        }
        int imgWidth = tSeriesMedia->getIntValue(cTableSeriesMedia::fiMediaWidth);
        int imgHeight = tSeriesMedia->getIntValue(cTableSeriesMedia::fiMediaHeight);
        series->InsertEpisodeImage(episodeId, imgWidth, imgHeight, imgPath);
    }

    selectEpisodeImg->freeResult();
}

void cUpdate::LoadSeasonPoster(cTVDBSeries *series, int season, string path) {
    stringstream iPath("");
    iPath << path << "/" << "season_" << season << ".jpg";
    stringstream tPath("");
    tPath << path << "/" << "season_" << season << "_thumb.jpg";
    string imgPath = iPath.str();
    string thumbPath = tPath.str();
    bool imgExists = FileExists(imgPath);
    if (!imgExists)
        if (!CreateDirectory(path))
            return;

    tSeriesMedia->clear();
    tSeriesMedia->setValue(cTableSeriesMedia::fiSeriesId, series->id);
    tSeriesMedia->setValue(cTableSeriesMedia::fiSeasonNumber, season);
    tSeriesMedia->setValue(cTableSeriesMedia::fiMediaType, msSeasonPoster);

    int res = selectSeasonPoster->find();

    if (res) {
        if (!imgExists) {
            int size = posterSize.getIntValue();
            if (FILE* fh = fopen(imgPath.c_str(), "w")) {
                fwrite(tSeriesMedia->getStrValue(cTableSeriesMedia::fiMediaContent), 1, size, fh);
                fclose(fh);
            }
        }
        int imgWidth = tSeriesMedia->getIntValue(cTableSeriesMedia::fiMediaWidth);
        int imgHeight = tSeriesMedia->getIntValue(cTableSeriesMedia::fiMediaHeight);
        if (!FileExists(thumbPath)) {
            CreateThumbnail(imgPath, thumbPath, imgWidth, imgHeight, 2);
        }
        series->InsertMedia(msSeasonPoster, imgWidth, imgHeight, imgPath, season);
        series->InsertMedia(msSeasonPosterThumb, imgWidth/2, imgHeight/2, thumbPath, season);
    }

    selectSeasonPoster->freeResult();
}

void cUpdate::ReadSeriesActors(cTVDBSeries *series, string path) {
    tSeriesActors->clear();
    series_id.setValue(series->id);

    for (int res = selectActors->find(); res; res = selectActors->fetch()) {
        scrapManager->AddSeriesActor(series, tSeriesActors);
        LoadSeriesActorThumb(series, tSeriesActors->getIntValue(cTableSeriesActor::fiActorId), path);
    }
    selectActors->freeResult();
}

void cUpdate::LoadSeriesActorThumb(cTVDBSeries *series, int actorId, string path) {
    stringstream iPath("");
    iPath << path << "/" << "actor_" << actorId << ".jpg";
    string imgPath = iPath.str();
    bool imgExists = FileExists(imgPath);
    if (!imgExists)
        if (!CreateDirectory(path))
            return;

    tSeriesMedia->clear();
    tSeriesMedia->setValue(cTableSeriesMedia::fiActorId, actorId);

    int res = selectActorThumbs->find();

    if (res) {
        if (!imgExists) {
            int size = actorImageSize.getIntValue();
            if (FILE* fh = fopen(imgPath.c_str(), "w")) {
                fwrite(tSeriesMedia->getStrValue(cTableSeriesMedia::fiMediaContent), 1, size, fh);
                fclose(fh);
            }
        }
        int tmbWidth = tSeriesMedia->getIntValue(cTableSeriesMedia::fiMediaWidth);
        int tmbHeight = tSeriesMedia->getIntValue(cTableSeriesMedia::fiMediaHeight);
        series->SetActorThumb(actorId, tmbWidth, tmbHeight, imgPath);
    }

    selectActorThumbs->freeResult();
}

void cUpdate::LoadSeriesMedia(cTVDBSeries *series, string path) {  
   tSeriesMedia->clear();  
   tSeriesMedia->setValue(cTableSeriesMedia::fiSeriesId, series->id);
   
   for (int res = selectSeriesMedia->find(); res; res = selectSeriesMedia->fetch()) {
      int mediaType = tSeriesMedia->getIntValue(cTableSeriesMedia::fiMediaType);
      int mediaWidth = tSeriesMedia->getIntValue(cTableSeriesMedia::fiMediaWidth);
      int mediaHeight = tSeriesMedia->getIntValue(cTableSeriesMedia::fiMediaHeight);
      string mediaPath = LoadMediaSeries(series->id, mediaType, path, mediaWidth, mediaHeight);
      series->InsertMedia(mediaType, mediaWidth, mediaHeight, mediaPath);
      if (mediaType == msPoster1) {
         string thumbPath = path + "/poster_thumb.jpg";
         series->InsertMedia(msPosterThumb, mediaWidth/5, mediaHeight/5, thumbPath);
      }
   }

   selectSeriesMedia->freeResult();
}

string cUpdate::LoadMediaSeries(int seriesId, int mediaType, string path, int width, int height) {
    stringstream iPath("");
    iPath << path << "/";
    bool createThumb = false;
    stringstream tPath("");
    tPath << path << "/";
    switch (mediaType) {
        case msPoster1:
            iPath << "poster1.jpg";
            createThumb = true;
            tPath << "poster_thumb.jpg";
            break;
        case msPoster2:
            iPath << "poster2.jpg";
            break;
        case msPoster3:
            iPath << "poster3.jpg";
            break;
        case msFanart1:
            iPath << "fanart1.jpg";
            break;
        case msFanart2:
            iPath << "fanart2.jpg";
            break;
        case msFanart3:
            iPath << "fanart3.jpg";
            break;
        case msBanner1:
            iPath << "banner1.jpg";
            break;
        case msBanner2:
            iPath << "banner2.jpg";
            break;
        case msBanner3:
            iPath << "banner3.jpg";
            break;
        default:
        break;
    }   
    string imgPath = iPath.str();
    string thumbPath = tPath.str();
    if (FileExists(imgPath)) {
        if (createThumb && !FileExists(thumbPath)) {
            CreateThumbnail(imgPath, thumbPath, width, height, 5);
        }
        return imgPath;
    }
    if (!CreateDirectory(path))
        return "";

    tSeriesMedia->clear();
    tSeriesMedia->setValue(cTableSeriesMedia::fiSeriesId, seriesId);
    tSeriesMedia->setValue(cTableSeriesMedia::fiMediaType, mediaType);
    int res = selectImg->find();
    if (res) {
        int size = imageSize.getIntValue();
        if (FILE* fh = fopen(imgPath.c_str(), "w")) {
            fwrite(tSeriesMedia->getStrValue(cTableSeriesMedia::fiMediaContent), 1, size, fh);
            fclose(fh);
        }
        if (createThumb && !FileExists(thumbPath)) {
            CreateThumbnail(imgPath, thumbPath, width, height, 5);
        }
    }

    selectImg->freeResult();
    return imgPath;
}

//***************************************************************************
// MOVIES
//***************************************************************************

int cUpdate::ReadMovies(bool isRec) {
    scrapManager->InitIterator(isRec);
    int movieId = 0;
   
    if (!CreateDirectory(config.imageDir))
        return 0;
    if (!CreateDirectory(imgPathMovies))
        return 0;

    int numNew = 0;
    lastWait = GetTimems(); // init time for CheckRunningAndWait
    while (scrapManager->GetNextMovie(isRec, movieId) && CheckRunningAndWait()) {
        cMovieDbMovie *movie = scrapManager->GetMovie(movieId);
        if (movie)
            continue;
        tMovies->clear();
        tMovies->setValue(cTableMovies::fiMovieId, movieId);
        int res = tMovies->find();
        if (!res)
            continue;
        movie = scrapManager->AddMovie(tMovies);
        stringstream mPath("");
        mPath << imgPathMovies << "/" << movieId;
        string moviePath = mPath.str();
        ReadMovieActors(movie);
        LoadMovieActorThumbs(movie);
        LoadMovieMedia(movie, moviePath);
        numNew++;
    }
    return numNew;
}

// read all movies with event or recording from sql-db
int cUpdate::ReadMoviesFast(long &maxscrsp) {
    // try to create directorys first (if not exists)
    if (!CreateDirectory(config.imageDir))
        return 0;
    if (!CreateDirectory(imgPathMovies))
        return 0;

    cMovieDbMovie *movie;
    bool isNew = false;
    int numNew = 0;
    int i = 0;

    vdr_uuid.setValue(config.uuid.c_str()); // search all recordigs of this vdr uuid
    tMovies->clear();
    event_scrsp.setValue(maxscrsp); // use last max scrsp as start value for filter
    
    cDbStatement* select = maxscrsp > 0 ? selectReadMoviesLastScrsp : selectReadMoviesInit;
    
    lastWait = GetTimems(); // init time for CheckRunningAndWait
    long lastLog = GetTimems(); // init time for logging
    for (int res = select->find(); res; res = select->fetch() && CheckRunningAndWait()) {
        movie = scrapManager->GetMovie(tMovies->getIntValue(cTableMovies::fiMovieId));
        if (!movie)
        {
            // add new movie with information of current row
            movie = scrapManager->AddMovie(tMovies); 
            movie->lastscraped = event_scrsp.getIntValue(); 
            isNew = true;
        } else {
            // only update lastscraped of known movie
            movie->lastscraped = event_scrsp.getIntValue(); 
            isNew = false;
        }
        maxscrsp = max(maxscrsp, movie->lastscraped);
        i++;
        if (GetTimeDiffms(lastLog)>LogPeriode) {
            tell(0, "Loaded %d movies, continuing...", i);
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
            tell(0, "Handled %d of %d movies, continuing...", i, moviecount);
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
        actor = movie->GetActor(tSeriesMedia->getIntValue(cTableSeriesMedia::fiActorId));
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
            imgNeedRefresh = !FileExists(imageName.str(),true);
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

    tMovieMedia->clear();
    tMovieMedia->setValue(cTableMovieMedia::fiMovieId, movie->id);

    for (int res = selectMovieMediaFast->find(); res; res = selectMovieMediaFast->fetch() && CheckRunningAndWait()) {
        mediaType = tMovieMedia->getIntValue(cTableMovieMedia::fiMediaType);
        imgWidth = tMovieMedia->getIntValue(cTableMovieMedia::fiMediaWidth);
        imgHeight = tMovieMedia->getIntValue(cTableMovieMedia::fiMediaHeight);

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

                CheckMovieMedia(movie, mediaType, imgWidth, imgHeight, imageName.str(), imgNeedRefresh);
                CheckMovieMedia(movie, mmPosterThumb, imgWidth/4, imgHeight/4, thumbName, imgNeedRefresh);
                break;
            case mmFanart:
                imageName.str("");
                imageName << moviePath << "/fanart.jpg";
                imgNeedRefresh = !FileExists(imageName.str(),true);
                CheckMovieMedia(movie, mediaType, imgWidth, imgHeight, imageName.str(), imgNeedRefresh);
                break;
            case mmCollectionPoster:
                imageName.str("");
                imageName << moviePath << "/collectionPoster.jpg";
                imgNeedRefresh = !FileExists(imageName.str(),true);
                CheckMovieMedia(movie, mediaType, imgWidth, imgHeight, imageName.str(), imgNeedRefresh);
                break;
            case mmCollectionFanart:
                imageName.str("");
                imageName << moviePath << "/collectionFanart.jpg";
                imgNeedRefresh = !FileExists(imageName.str(),true);
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

void cUpdate::ReadMovieActors(cMovieDbMovie *movie) {
    tMovieActor->clear();
    tMovieMedia->clear();
    actorMovie.setValue(movie->id);

    for (int res = selectMovieActors->find(); res; res = selectMovieActors->fetch()) {
        scrapManager->AddMovieActor(movie, tMovieActor, actorRole.getStrValue(), false);
        int tmbWidth = thbWidth.getIntValue();
        int tmbHeight = thbHeight.getIntValue();
        movie->SetActorThumbSize(tMovieActor->getIntValue(cTableMovieActor::fiActorId), tmbWidth, tmbHeight);
    }

    selectMovieActors->freeResult();
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
            if (ReadMovieImageFast(0,actorId,mmActorThumb,media,NULL,1)) {
                newActors++;
            } else {
                tell(0,"failed to read image (movie actor id %d)",actorId);
            }    
        }
        i++;
        if (GetTimeDiffms(lastLog)>LogPeriode) {
            tell(0, "Handled %d of %d actors (loaded %d new images), continuing...", i, actorCount, newActors);
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
                        if (ReadMovieImageFast(movie->id,0,mediaType,media,mediaThumb,4)) {
                            newImages++;
                        } else {
                           tell(0,"failed to read image (movie %d, mediatype %d)",movie->id,mediaType);
                        }    
                    }    
                }
            }
            movie->updateimages = false; // have updated all images
        }

        i++;
        if (GetTimeDiffms(lastLog)>LogPeriode) {
            tell(0, "Handled %d of %d movies (loaded %d new images), continuing...", i, movieCount, newImages);
            lastLog = GetTimems();
        }    
    }  
    return newActors + newImages;
}

// read real image data from sql-db
bool cUpdate::ReadMovieImageFast(int movieId, int actorId, int mediaType, cMovieMedia *media, cMovieMedia *mediaThumb, int shrinkFactor) {
    bool imageSaved = false;
    tMovieMedia->clear();
    tMovieMedia->setValue(cTableMovieMedia::fiMovieId, movieId);
    tMovieMedia->setValue(cTableMovieMedia::fiActorId, actorId);
    tMovieMedia->setValue(cTableMovieMedia::fiMediaType, mediaType);
    int res = selectMovieImage->find();
    if (res) {
        media->width = tMovieMedia->getIntValue(cTableMovieMedia::fiMediaWidth);
        media->height = tMovieMedia->getIntValue(cTableMovieMedia::fiMediaHeight);
        int size = imageSize.getIntValue();
        if (size > 0) {
            if (FILE* fh = fopen(media->path.c_str(), "w")) {
                fwrite(tMovieMedia->getStrValue(cTableMovieMedia::fiMediaContent), 1, size, fh);
                fclose(fh);
                media->needrefresh = false; // reset update flag
                media->mediavalid = true; // we have a image file for this media
                imageSaved = true;
            }
            if (mediaThumb) {
                CreateThumbnail(media->path, mediaThumb->path, media->width, media->height, shrinkFactor);
                mediaThumb->needrefresh = false;
                mediaThumb->mediavalid = true; // we have a image file for this media
            }
        } else {
            media->needrefresh = false; // reset update flag for empty images also
        }    
    }
    selectMovieImage->freeResult();
    return imageSaved;
}

void cUpdate::LoadMovieActorThumbs(cMovieDbMovie *movie) {
    tMovieMedia->clear();

    string movieActorsPath = imgPathMovies + "/actors";
    if (!CreateDirectory(movieActorsPath))
        return;

    vector<int> IDs = movie->GetActorIDs();
    for (vector<int>::iterator it = IDs.begin(); it != IDs.end(); it++) {
        int actorId = (int)*it;
        stringstream tName("");
        tName << "actor_" << actorId << ".jpg";
        string thumbName = tName.str();
        string thumbFullPath = movieActorsPath + "/" + thumbName; 
        if (!FileExists(thumbFullPath)) {
            tMovieMedia->setValue(cTableMovieMedia::fiActorId, actorId);
            int res = selectMovieActorThumbs->find();
            if (res) {
                int size = imageSize.getIntValue();
                if (FILE* fh = fopen(thumbFullPath.c_str(), "w")) {
                    fwrite(tMovieMedia->getStrValue(cTableMovieMedia::fiMediaContent), 1, size, fh);
                    fclose(fh);
                }
                movie->SetActorPath(actorId, thumbFullPath);
            }
        } else {
            movie->SetActorPath(actorId, thumbFullPath);
        }
    }

    selectMovieActorThumbs->freeResult();
}        

void cUpdate::LoadMovieMedia(cMovieDbMovie *movie, string moviePath) {
    tMovieMedia->clear();
    tMovieMedia->setValue(cTableMovieMedia::fiMovieId, movie->id);

    for (int res = selectMovieMedia->find(); res; res = selectMovieMedia->fetch()) {
        int mediaType = tMovieMedia->getIntValue(cTableMovieMedia::fiMediaType);
        int mediaWidth = tMovieMedia->getIntValue(cTableMovieMedia::fiMediaWidth);
        int mediaHeight = tMovieMedia->getIntValue(cTableMovieMedia::fiMediaHeight);
        string imgPath = LoadMediaMovie(movie->id, mediaType, moviePath, mediaWidth, mediaHeight);
        if (imgPath.size() > 0)
            scrapManager->AddMovieMedia(movie, tMovieMedia, imgPath);
        if (mediaType == mmPoster) {
            cMovieMedia *m = new cMovieMedia();
            m->mediaType = mmPosterThumb;
            m->width = mediaWidth/4;
            m->height = mediaHeight/4;
            m->path = moviePath + "/poster_thumb.jpg";
            movie->InsertMedia(m);
        }
    }

    selectMovieMedia->freeResult();
}

string cUpdate::LoadMediaMovie(int movieId, int mediaType, string path, int width, int height) {
    stringstream iPath("");
    iPath << path << "/";
    bool createThumb = false;
    stringstream tPath("");
    tPath << path << "/";
    switch (mediaType) {
        case mmPoster:
            iPath << "poster.jpg";
            createThumb = true;
            tPath << "poster_thumb.jpg";
            break;
        case mmFanart:
            iPath << "fanart.jpg";
            break;
        case mmCollectionPoster:
            iPath << "collectionPoster.jpg";
            break;
        case mmCollectionFanart:
            iPath << "collectionFanart.jpg";
            break;
        default:
        break;
    }   
    string imgPath = iPath.str();
    string thumbPath = tPath.str();
    if (FileExists(imgPath)) {
        if (createThumb && !FileExists(thumbPath)) {
            CreateThumbnail(imgPath, thumbPath, width, height, 4);
        }
        return imgPath;
    }
    if (!CreateDirectory(path))
        return imgPath;

    tMovieMedia->clear();
    tMovieMedia->setValue(cTableMovieMedia::fiMovieId, movieId);
    tMovieMedia->setValue(cTableMovieMedia::fiMediaType, mediaType);

    int res = selectMediaMovie->find();
    if (res) {
        int size = imageSize.getIntValue();
        if (FILE* fh = fopen(imgPath.c_str(), "w")) {
            fwrite(tMovieMedia->getStrValue(cTableMovieMedia::fiMediaContent), 1, size, fh);
            fclose(fh);
        }
        if (createThumb && !FileExists(thumbPath)) {
            CreateThumbnail(imgPath, thumbPath, width, height, 4);
        }
    }

    selectMediaMovie->freeResult();
    return imgPath;
}

//***************************************************************************
// RECORDINGS
//***************************************************************************

int cUpdate::ReadRecordings(void) {
    tRecordings->clear();
    tRecordings->setValue(cTableRecordings::fiUuid, config.uuid.c_str());
    int numRecs = 0;
    for (int res = selectRecordings->find(); res; res = selectRecordings->fetch()) {
        int recStart = tRecordings->getIntValue(cTableRecordings::fiRecStart);
        string recPath = tRecordings->getStrValue(cTableRecordings::fiRecPath);
        int movieId = tRecordings->getIntValue(cTableRecordings::fiMovieId);
        int seriesId = tRecordings->getIntValue(cTableRecordings::fiSeriesId);
        int episodeId = tRecordings->getIntValue(cTableRecordings::fiEpisodeId);
        bool isNew = scrapManager->AddRecording(recStart, recPath, seriesId, episodeId, movieId);
        if (isNew)
            numRecs++;
    }

    selectRecordings->freeResult();
    return numRecs;
}

int cUpdate::ScanVideoDir(void) {
    int newRecs = 0;
    for (cRecording *rec = Recordings.First(); rec; rec = Recordings.Next(rec)) {
        string recPath = getRecPath(rec);
        int recStart = rec->Start();
        if (!scrapManager->RecordingExists(recStart, recPath)) {
            newRecs++;
            int scrapInfoMovieID = 0;
            int scrapInfoSeriesID = 0;        
            int scrapInfoEpisodeID = 0;
            ReadScrapInfo(rec->FileName(), scrapInfoMovieID, scrapInfoSeriesID, scrapInfoEpisodeID);
            int eventId = 0;
            string channelId = "";
            string title = rec->Name();
            //remove directory
            size_t posDelim = title.find_last_of('~');
            if (posDelim != string::npos) {
                title = title.substr(posDelim+1);
            }
            string subTitle = "";
            const cRecordingInfo *recInfo = rec->Info();
            if (recInfo) {
                const cEvent *recEvent = recInfo->GetEvent();
                if (recEvent) {
                    eventId = recEvent->EventID();
                    channelId = *(recInfo->ChannelID().ToString());
                    if (recInfo->Title())
                        title = recInfo->Title();
                    subTitle = (recInfo->ShortText())?(recInfo->ShortText()):"";
                }
            }
            tRecordings->clear();
            tRecordings->setValue(cTableRecordings::fiUuid, config.uuid.c_str());
            tRecordings->setValue(cTableRecordings::fiRecPath, recPath.c_str());
            tRecordings->setValue(cTableRecordings::fiRecStart, recStart);
            
            tRecordings->setValue(cTableRecordings::fiEventId, eventId);
            tRecordings->setValue(cTableRecordings::fiChannelId, channelId.c_str());
            tRecordings->setValue(cTableRecordings::fiScrapInfoMovieId, scrapInfoMovieID);
            tRecordings->setValue(cTableRecordings::fiScrapInfoSeriesId, scrapInfoSeriesID);
            tRecordings->setValue(cTableRecordings::fiScrapInfoEpisodeId, scrapInfoEpisodeID);
            tRecordings->setValue(cTableRecordings::fiScrapNew, 1);
            tRecordings->setValue(cTableRecordings::fiRecTitle, title.c_str());
            tRecordings->setValue(cTableRecordings::fiRecSubTitle, subTitle.c_str());
            tRecordings->setValue(cTableRecordings::fiRecDuration, rec->LengthInSeconds()/60);
            tRecordings->store();
        }
    }
    return newRecs;
}

int cUpdate::ScanVideoDirScrapInfo(void) {
    int numUpdated = 0;
    for (cRecording *rec = Recordings.First(); rec; rec = Recordings.Next(rec)) {
        int recStart = rec->Start();
        string recPath = getRecPath(rec);
        /* bool recExists = */ LoadRecording(recStart, recPath);  
        int scrapInfoMovieID = 0;
        int scrapInfoSeriesID = 0;        
        int scrapInfoEpisodeID = 0;
        ReadScrapInfo(rec->FileName(), scrapInfoMovieID, scrapInfoSeriesID, scrapInfoEpisodeID);
        if (ScrapInfoChanged(scrapInfoMovieID, scrapInfoSeriesID, scrapInfoEpisodeID)) {
            tRecordings->setValue(cTableRecordings::fiScrapNew, 1);
            tRecordings->setValue(cTableRecordings::fiScrapInfoMovieId, scrapInfoMovieID);
            tRecordings->setValue(cTableRecordings::fiScrapInfoSeriesId, scrapInfoSeriesID);
            tRecordings->setValue(cTableRecordings::fiScrapInfoEpisodeId, scrapInfoEpisodeID);
            tRecordings->update();
            numUpdated++;
        }
    }
    return numUpdated;
}

bool cUpdate::LoadRecording(int recStart, string recPath) {
    tRecordings->clear();
    tRecordings->setValue(cTableRecordings::fiUuid, config.uuid.c_str());
    tRecordings->setValue(cTableRecordings::fiRecStart, recStart);
    tRecordings->setValue(cTableRecordings::fiRecPath, recPath.c_str());
    int found = tRecordings->find();
    if (found == yes) {
        return true;
    }
    return false;
}

bool cUpdate::ScrapInfoChanged(int scrapInfoMovieID, int scrapInfoSeriesID, int scrapInfoEpisodeID) {
    int movieIdCurrent = tRecordings->getIntValue(cTableRecordings::fiScrapInfoMovieId);
    int seriesIdCurrent = tRecordings->getIntValue(cTableRecordings::fiScrapInfoSeriesId);
    int episodeIdCurrent = tRecordings->getIntValue(cTableRecordings::fiScrapInfoEpisodeId);
    if ((movieIdCurrent != scrapInfoMovieID) ||
        (seriesIdCurrent != scrapInfoSeriesID) ||
        (episodeIdCurrent != scrapInfoEpisodeID))
        return true;
    return false;
}

void cUpdate::ReadScrapInfo(string recDir, int &scrapInfoMovieID, int &scrapInfoSeriesID, int &scrapInfoEpisodeID) {
    stringstream sInfoName("");
    sInfoName << recDir << "/" << config.recScrapInfoName;
    string scrapInfoName = sInfoName.str();
    if (!FileExists(scrapInfoName, false)) {
        string twoHigher = TwoFoldersHigher(recDir);
        if (twoHigher.size() > 0) {
            stringstream sInfoNameAlt("");
            sInfoNameAlt << twoHigher << "/" << config.recScrapInfoName;
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

int cUpdate::CleanupRecordings(void) {
    // delete all not anymore existing recordings in database

    tRecordings->clear();
    tRecordings->setValue(cTableRecordings::fiUuid, config.uuid.c_str());
    int numRecsDeleted = 0;

    for (int res = selectCleanupRecordings->find(); res; res = selectCleanupRecordings->fetch()) {
        int recStart = tRecordings->getIntValue(cTableRecordings::fiRecStart);
        string recPath = tRecordings->getStrValue(cTableRecordings::fiRecPath);
        if (!Recordings.GetByName(recPath.c_str())) {
            char escapedPath[recPath.size()+1];
            mysql_real_escape_string(connection->getMySql(), escapedPath, recPath.c_str(), recPath.size());            
            stringstream delWhere("");
            delWhere << "uuid = '" << config.uuid << "' and rec_path = '" << escapedPath << "' and rec_start = " << recStart;
            tRecordings->deleteWhere(delWhere.str().c_str());
            numRecsDeleted++;
        }
    }
    selectCleanupRecordings->freeResult();
    return numRecsDeleted;
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

//***************************************************************************
// Action
//***************************************************************************

void cUpdate::Action() 
{
    tell(0, "Update thread started (pid=%d)", getpid());
    mutex.Lock();
    loopActive = yes;

    int worked = no;
    int sleep = 60;
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
    bool init = true;

    time_t now = time(0);
    int dur = 0;
    int numNewRecs = 0;
    int numValues = 0;
    int numNewSeries = 0;
    int numNewMovies = 0;
    int numNewImages = 0;
    int numNewImages2 = 0;

    while (loopActive && Running()) 
    {
        int reconnectTimeout; // set by checkConnection

        waitCondition.TimedWait(mutex, init ? sleep*500 : sleep*1000);

        if (CheckConnection(reconnectTimeout) != success) 
           continue;

        // auch beim init auf den epgd warten, wenn der gerade busy ist müssen die sich User etwas gedulden ;) 

        if (CheckEpgdBusy())
        {
           tell(1, "epgd busy, trying again in %d seconds ...", sleep);
           continue;
        }

        // Update Recordings from Database
        if (forceRecordingUpdate || (time(0) - lastScanNewRecDB > scanNewRecDBFreq) && Running()) 
        {
            worked++;
            numNewRecs = ReadRecordings();
            lastScanNewRecDB = time(0);

            if (!config.fastmode) {
                // scan for new recordings not neccessarry in fastmode (get done together with events)
                if (numNewRecs > 0) 
                {
                    numNewSeries = ReadSeries(true);
                    numNewMovies = ReadMovies(true);
                    tell(0, "Loaded %d new Recordings from Database, %d series, %d movies", numNewRecs, numNewSeries, numNewMovies);
                }
            }           
            forceRecordingUpdate = false;
        }
        
        // Update Events

        if (!config.headless && (forceUpdate || (time(0) - lastScan > scanFreq)) && Running()) 
        {
           worked++;
           int numNewEvents = ReadScrapedEvents();

           if (numNewEvents > 0) 
           {
              tell(0, "Loaded %d new scraped Events from Database", numNewEvents);
           } 
           else 
           {
              lastScan = time(0);
              forceUpdate = false;
              init = false;
              if (!config.fastmode)
                continue;
           }
           
           worked++;
           if (config.fastmode) {
               // new mode
               tell(0, "Loading Movies information from Database...");
               now = time(0);
               numValues = ReadMoviesFast(MaxScrspMovies);
               dur = time(0) - now; 
               tell(0, "Loaded %d new/updated Movies in %ds from Database (new max scrsp: %ld)", numValues, dur, MaxScrspMovies);
               
               tell(0, "Loading Movies content from Database...");
               now = time(0);
               numValues = ReadMoviesContentFast();
               dur = time(0) - now; 
               tell(0, "Loaded %d new/updated Image information in %ds from Database", numValues, dur);
               
               tell(0, "Loading Series information from Database...");
               now = time(0);
               numValues = ReadSeriesFast(MaxScrspSeries);
               dur = time(0) - now; 
               tell(0, "Loaded %d new/updated Series in %ds from Database (new max scrsp: %ld)", numValues, dur, MaxScrspSeries);
              
               tell(0, "Loading Series content from Database...");
               now = time(0);
               numValues = ReadSeriesContentFast(numNewImages,numNewImages2);
               dur = time(0) - now; 
               tell(0, "Loaded %d new/updated Episodes and %d new/updated Image information (including %d possible not available season poster) in %ds from Database", numValues, numNewImages+numNewImages2, numNewImages2, dur);
              
               tell(0, "Loading Image content from Database...");
               now = time(0);
               ReadSeriesImagesFast(numNewImages,numNewImages2);
               numNewImages = numNewImages + ReadMovieImagesFast();
               dur = time(0) - now; 
               tell(0, "Loaded %d new/updated Images (found %d not available season posters) in %ds from Database", numNewImages, numNewImages2, dur);
           } else {
               // old mode
               tell(0, "Loading new Movies from Database...");
               now = time(0);
               numValues = ReadMovies(false);
               dur = time(0) - now;
               tell(0, "Loaded %d new Movies in %ds from Database", numValues, dur);

               tell(0, "Loading new Series and Episodes from Database...");
               now = time(0);
               numValues = ReadSeries(false);
               dur = time(0) - now; 
               tell(0, "Loaded %d new Series and Episodes in %ds from Database", numValues, dur);
           }   
           lastScan = time(0);
           forceUpdate = false;
        }
        
        // Scan new recordings

        if ((init || forceVideoDirUpdate || (time(0) - lastScanNewRec > scanNewRecFreq)) && Running()) 
        {
           static int recState = 0;

           if (Recordings.StateChanged(recState)) 
           {
              tell(0, "Searching for new recordings because of Recordings State Change...");
              worked++;
              numNewRecs = ScanVideoDir();
              tell(0, "found %d new recordings", numNewRecs);
           }
           
           lastScanNewRec = time(0);
           forceVideoDirUpdate = false;
        }
        
        init = false;
        
        // Scan Video dir for scrapinfo files

        if (forceScrapInfoUpdate) 
        {
           worked++;
           tell(0, "Checking for new or updated scrapinfo files in recordings...");
           int numUpdated = ScanVideoDirScrapInfo();
           tell(0, "found %d new or updated scrapinfo files", numUpdated);
           forceScrapInfoUpdate = false;
        }
        
        // Cleanup

        if ((time(0) - lastCleanup > cleanUpFreq) && Running())
        {
           worked++;
           int seriesDeleted = CleanupSeries();
           int moviesDeleted = CleanupMovies();

           if (seriesDeleted > 0 || moviesDeleted > 0) 
           {
              tell(0, "Deleted %d outdated series image folders", seriesDeleted);            
              tell(0, "Deleted %d outdated movie image folders", moviesDeleted);
           }
           
           lastCleanup = time(0);
        }
        
        // Cleanup Recording DB
        
        if (forceCleanupRecordingDb) 
        {
            worked++;
            tell(0, "Cleaning up recordings in database...");
            int recsDeleted = CleanupRecordings();
            tell(0, "Deleted %d not anymore existing recordings in database", recsDeleted);
            forceCleanupRecordingDb = false;
        }
        
        if (worked && config.debug) 
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

void cUpdate::TriggerCleanRecordingsDB(void) {
    tell(0, "cleanup of recording DB triggered");
    forceCleanupRecordingDb = true;
}

