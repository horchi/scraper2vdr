#define __STL_CONFIG_H
#include <locale.h>

#include <vdr/videodir.h>
#include <vdr/tools.h>
#include <vdr/plugin.h>

#include "lib/config.h"

#include "config.h"
#include "tools.h"
#include "update.h"

extern cScraper2VdrConfig config;

cUpdate::cUpdate(cScrapManager *manager) : cThread("update thread started") {
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

    scrapManager = manager;
    imgPathSeries = config.imageDir + "/series";
    imgPathMovies = config.imageDir + "/movies";
    lastScrap = 0;
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
    exitDb();
}

// global field definitions

cDBS::FieldDef imageSizeDef = { "media_content", cDBS::ffUInt,  0, 999, cDBS::ftData };

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

    // --------------------
    // prepare statements

    // 

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
    selectReadScrapedEvents->bind(cTableEvents::fiScrSp, cDBS::bndIn | cDBS::bndSet, " and ");
    selectReadScrapedEvents->build(" order by %s", tEvents->getField(cTableEvents::fiInsSp)->name);
    
    status += selectReadScrapedEvents->prepare();
    
    // select image

    imageSize.setField(&imageSizeDef);
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
    selectImg->bind(cTableSeriesMedia::fiEpisodeId, cDBS::bndIn | cDBS::bndSet, " and ");
    status += selectImg->prepare();

    // select poster image

    posterSize.setField(&imageSizeDef);
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

    series_id.setField(tSeriesMedia->getField(cTableSeriesMedia::fiSeriesId));
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

    actorImageSize.setField(&imageSizeDef);
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

    actorRole.setField(tMovieActors->getField(cTableMovieActors::fiRole));
    actorMovie.setField(tMovieActors->getField(cTableMovieActors::fiMovieId));
    thbWidth.setField(tMovieMedia->getField(cTableMovieMedia::fiMediaWidth));
    thbHeight.setField(tMovieMedia->getField(cTableMovieMedia::fiMediaHeight));

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

    return status;
}

int cUpdate::exitDb() {

    delete selectReadScrapedEvents;     selectReadScrapedEvents = 0;
    delete selectReadScrapedEventsInit; selectReadScrapedEventsInit = 0;
    delete selectImg;                   selectImg = 0;
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
    
    for (int res = select->find(); res; res = select->fetch() && Running()) {
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
    while (scrapManager->GetNextSeries(isRec, seriesId, episodeId) && Running()) {
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
        numNew++;
    }
    return numNew;
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

    int res = selectImg->find();

    if (res) {
        if (!imgExists) {
            int size = imageSize.getIntValue();
            if (FILE* fh = fopen(imgPath.c_str(), "w")) {
                fwrite(tSeriesMedia->getStrValue(cTableSeriesMedia::fiMediaContent), 1, size, fh);
                fclose(fh);
            }
        }
        int imgWidth = tSeriesMedia->getIntValue(cTableSeriesMedia::fiMediaWidth);
        int imgHeight = tSeriesMedia->getIntValue(cTableSeriesMedia::fiMediaHeight);
        series->InsertEpisodeImage(episodeId, imgWidth, imgHeight, imgPath);
    }

    selectImg->freeResult();
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
        series->InsertActorThumb(actorId, tmbWidth, tmbHeight, imgPath);
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
    while (scrapManager->GetNextMovie(isRec, movieId) && Running()) {
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

void cUpdate::ReadMovieActors(cMovieDbMovie *movie) {
    tMovieActor->clear();
    tMovieMedia->clear();
    actorMovie.setValue(movie->id);

    for (int res = selectMovieActors->find(); res; res = selectMovieActors->fetch()) {
        scrapManager->AddMovieActor(movie, tMovieActor, actorRole.getStrValue());
        int tmbWidth = thbWidth.getIntValue();
        int tmbHeight = thbHeight.getIntValue();
        movie->SetActorThumbSize(tMovieActor->getIntValue(cTableMovieActor::fiActorId), tmbWidth, tmbHeight);
    }

    selectMovieActors->freeResult();
}

void cUpdate::LoadMovieActorThumbs(cMovieDbMovie *movie) {
    tMovieMedia->clear();
    imageSize.setField(&imageSizeDef);

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
    imageSize.setField(&imageSizeDef);
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
            stringstream delWhere("");
            delWhere << "uuid = '" << config.uuid << "' and rec_path = '" << recPath << "' and rec_start = " << recStart;
            tRecordings->deleteWhere(delWhere.str().c_str());
            numRecsDeleted++;
        }
    }
    selectCleanupRecordings->freeResult();
    return numRecsDeleted;
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
           int numNewRecs = ReadRecordings();
           lastScanNewRecDB = time(0);

           if (numNewRecs > 0) 
           {
              int numSeries = ReadSeries(true);
              int numMovies = ReadMovies(true);
              tell(0, "Loaded %d new Recordings from Database, %d series, %d movies", numNewRecs, numSeries, numMovies);
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
              continue;
           }
           
           tell(0, "Loading new Movies from Database...");
           time_t now = time(0);
           worked++;
           int numNewMovies = ReadMovies(false);
           int dur = time(0) - now;
           tell(0, "Loaded %d new Movies in %ds from Database", numNewMovies, dur);
           
           tell(0, "Loading new Series and Episodes from Database...");
           now = time(0);
           worked++;
           int numNewSeries = ReadSeries(false);
           dur = time(0) - now;
           tell(0, "Loaded %d new Series and Episodes in %ds from Database", numNewSeries, dur);
           
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
              int newRecs = ScanVideoDir();
              tell(0, "found %d new recordings", newRecs);
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

