#define __STL_CONFIG_H
#include <locale.h>

#include <vdr/videodir.h>
#include <vdr/tools.h>
#include <vdr/plugin.h>

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
}

cUpdate::~cUpdate() {
    if (loopActive)
        Stop();
    if (vdrDb)
        delete vdrDb;
    if (tEvents)
        delete tEvents;
    if (tSeries)
        delete tSeries;
    if (tEpisodes)
        tEpisodes;
    if (tSeriesMedia)
        delete tSeriesMedia;
    if (tSeriesActors)
        delete tSeriesActors;
    if (tMovies)
        delete tMovies;
    if (tMovieActor)
        delete tMovieActor;
    if (tMovieActors)
        delete tMovieActors;
    if (tMovieMedia)
        delete tMovieMedia;
    if (tRecordings)
        delete tRecordings;
    exitDb();
}

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
    return status;
}

int cUpdate::exitDb() {
    delete connection; 
    connection = 0;
    return done;
}

void cUpdate::Stop() { 
    loopActive = false;
    waitCondition.Broadcast();
    Cancel(3);      
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
    vdrDb->clear();
    vdrDb->setValue(cTableVdrs::fiUuid, EPGDNAME);
    if (vdrDb->find()) {
        Es::State epgdState = cEpgdState::toState(vdrDb->getStrValue(cTableVdrs::fiState));
        if (epgdState >= cEpgdState::esBusy)
            return true;
    }
    return false;
}

int cUpdate::ReadScrapedEvents(void) {
    int status = success;
    cDbStatement *select = new cDbStatement(tEvents);
    select->build("select ");
    select->bind(cTableEvents::fiEventId, cDBS::bndOut);
    select->bind(cTableEvents::fiChannelId, cDBS::bndOut, ", ");
    select->bind(cTableEvents::fiUseId, cDBS::bndOut, ", ");
    select->bind(cTableEvents::fiScrSeriesId, cDBS::bndOut, ", ");
    select->bind(cTableEvents::fiScrSeriesEpisode, cDBS::bndOut, ", ");
    select->bind(cTableEvents::fiScrMovieId, cDBS::bndOut, ", ");
    select->bind(cTableEvents::fiScrSp, cDBS::bndOut, ", ");
    select->build(" from %s where ", tEvents->TableName());
    select->build(" ((%s is not null and %s > 0) ", 
                    tEvents->getField(cTableEvents::fiScrSeriesId)->name, 
                    tEvents->getField(cTableEvents::fiScrSeriesId)->name);
    select->build(" or (%s is not null and %s > 0)) ", 
                    tEvents->getField(cTableEvents::fiScrMovieId)->name, 
                    tEvents->getField(cTableEvents::fiScrMovieId)->name);
    if (lastScrap > 0) {
        select->build(" and %s > %d", 
                    tEvents->getField(cTableEvents::fiScrSp)->name, 
                    lastScrap);
    }
    select->build(" order by %s", tEvents->getField(cTableEvents::fiInsSp)->name);
    status += select->prepare();
    if (status != success) {
        delete select;
        return 0;
    }
    int eventId = 0;
    int seriesId = 0;
    int episodeId = 0;
    int movieId = 0;
    string channelId = "";
    int numNew = 0;
    for (int res = select->find(); res; res = select->fetch()) {
        eventId = tEvents->getIntValue(cTableEvents::fiUseId);
        channelId = tEvents->getStrValue(cTableEvents::fiChannelId);
        seriesId = tEvents->getIntValue(cTableEvents::fiScrSeriesId);
        episodeId = tEvents->getIntValue(cTableEvents::fiScrSeriesEpisode);
        movieId = tEvents->getIntValue(cTableEvents::fiScrMovieId);
        scrapManager->AddEvent(eventId, channelId, seriesId, episodeId, movieId);
        lastScrap = max(lastScrap, (int)tEvents->getIntValue(cTableEvents::fiScrSp));
        numNew++;
    }
    select->freeResult();
    delete select;
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
    while (scrapManager->GetNextSeries(isRec, seriesId, episodeId)) {
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
            stringstream sPath;
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
    int status = success;
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
    int status = success;
    stringstream iPath;
    iPath << path << "/" << "episode_" << episodeId << ".jpg";
    string imgPath = iPath.str();
    bool imgExists = FileExists(imgPath);
    if (!imgExists)
        if (!CreateDirectory(path))
            return;
    tSeriesMedia->clear();
    cDbValue imageSize;
    cDBS::FieldDef imageSizeDef = { "media_content", cDBS::ffUInt,  0, 999, cDBS::ftData };
    imageSize.setField(&imageSizeDef);
    cDbStatement *selectImg = new cDbStatement(tSeriesMedia);
    selectImg->build("select ");
    selectImg->bind(cTableSeriesMedia::fiMediaWidth, cDBS::bndOut);
    selectImg->bind(cTableSeriesMedia::fiMediaHeight, cDBS::bndOut, ", ");
    if (!imgExists) {
        selectImg->bind(cTableSeriesMedia::fiMediaContent, cDBS::bndOut, ", ");
        selectImg->build(", length(");
        selectImg->bind(&imageSize, cDBS::bndOut);
        selectImg->build(")");
    }
    selectImg->build(" from %s where ", tSeriesMedia->TableName());
    selectImg->bind(cTableSeriesMedia::fiSeriesId, cDBS::bndIn | cDBS::bndSet);
    selectImg->bind(cTableSeriesMedia::fiEpisodeId, cDBS::bndIn | cDBS::bndSet, " and ");
    status += selectImg->prepare();
    if (status != success) {
        delete selectImg;
        return;
    }
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
    delete selectImg;
}

void cUpdate::LoadSeasonPoster(cTVDBSeries *series, int season, string path) {
    int status = success;
    stringstream iPath;
    iPath << path << "/" << "season_" << season << ".jpg";
    stringstream tPath;
    tPath << path << "/" << "season_" << season << "_thumb.jpg";
    string imgPath = iPath.str();
    string thumbPath = tPath.str();
    bool imgExists = FileExists(imgPath);
    if (!imgExists)
        if (!CreateDirectory(path))
            return;
    tSeriesMedia->clear();
    cDbValue imageSize;
    cDBS::FieldDef imageSizeDef = { "media_content", cDBS::ffUInt,  0, 999, cDBS::ftData };
    imageSize.setField(&imageSizeDef);
    cDbStatement *selectImg = new cDbStatement(tSeriesMedia);
    selectImg->build("select ");
    selectImg->bind(cTableSeriesMedia::fiMediaWidth, cDBS::bndOut);
    selectImg->bind(cTableSeriesMedia::fiMediaHeight, cDBS::bndOut, ", ");
    if (!imgExists) {
        selectImg->bind(cTableSeriesMedia::fiMediaContent, cDBS::bndOut, ", ");
        selectImg->build(", length(");
        selectImg->bind(&imageSize, cDBS::bndOut);
        selectImg->build(")");
    }
    selectImg->build(" from %s where ", tSeriesMedia->TableName());
    selectImg->bind(cTableSeriesMedia::fiSeriesId, cDBS::bndIn | cDBS::bndSet);
    selectImg->bind(cTableSeriesMedia::fiSeasonNumber, cDBS::bndIn | cDBS::bndSet, " and ");
    selectImg->bind(cTableSeriesMedia::fiMediaType, cDBS::bndIn | cDBS::bndSet, " and ");
    status += selectImg->prepare();
    if (status != success) {
        delete selectImg;
        return;
    }
    tSeriesMedia->setValue(cTableSeriesMedia::fiSeriesId, series->id);
    tSeriesMedia->setValue(cTableSeriesMedia::fiSeasonNumber, season);
    tSeriesMedia->setValue(cTableSeriesMedia::fiMediaType, msSeasonPoster);
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
        if (!FileExists(thumbPath)) {
            CreateThumbnail(imgPath, thumbPath, imgWidth, imgHeight, 2);
        }
        series->InsertMedia(msSeasonPoster, imgWidth, imgHeight, imgPath, season);
        series->InsertMedia(msSeasonPosterThumb, imgWidth/2, imgHeight/2, thumbPath, season);
    }
    selectImg->freeResult();
    delete selectImg;
}

void cUpdate::ReadSeriesActors(cTVDBSeries *series, string path) {
    int status = success;
    tSeriesActors->clear();
    cDbValue series_id;
    series_id.setField(tSeriesMedia->getField(cTableSeriesMedia::fiSeriesId));
    series_id.setValue(series->id);
    cDbStatement *selectActors = new cDbStatement(tSeriesActors);
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
    if (status != success) {
        delete selectActors;
        return;
    }
    for (int res = selectActors->find(); res; res = selectActors->fetch()) {
        scrapManager->AddSeriesActor(series, tSeriesActors);
        LoadSeriesActorThumb(series, tSeriesActors->getIntValue(cTableSeriesActor::fiActorId), path);
    }
    selectActors->freeResult();
    delete selectActors;
}

void cUpdate::LoadSeriesActorThumb(cTVDBSeries *series, int actorId, string path) {
    int status = success;
    stringstream iPath;
    iPath << path << "/" << "actor_" << actorId << ".jpg";
    string imgPath = iPath.str();
    bool imgExists = FileExists(imgPath);
    if (!imgExists)
        if (!CreateDirectory(path))
            return;
    cDbValue imageSize;
    cDBS::FieldDef imageSizeDef = { "media_content", cDBS::ffUInt,  0, 999, cDBS::ftData };
    imageSize.setField(&imageSizeDef);
    tSeriesMedia->clear();
    cDbStatement *selectActorThumbs = new cDbStatement(tSeriesMedia);
    selectActorThumbs->build("select ");
    selectActorThumbs->bind(cTableSeriesMedia::fiMediaWidth, cDBS::bndOut);
    selectActorThumbs->bind(cTableSeriesMedia::fiMediaHeight, cDBS::bndOut, ", ");
    if (!imgExists) {
        selectActorThumbs->bind(cTableSeriesMedia::fiMediaContent, cDBS::bndOut, ", ");
        selectActorThumbs->build(", length(");
        selectActorThumbs->bind(&imageSize, cDBS::bndOut);
        selectActorThumbs->build(")");
    }
    selectActorThumbs->build(" from %s where ", tSeriesMedia->TableName());
    selectActorThumbs->bind(cTableSeriesMedia::fiActorId, cDBS::bndIn | cDBS::bndSet);
    status += selectActorThumbs->prepare();
    if (status != success) {
        delete selectActorThumbs;
        return;
    }
    tSeriesMedia->setValue(cTableSeriesMedia::fiActorId, actorId);
    int res = selectActorThumbs->find();
    if (res) {
        if (!imgExists) {
            int size = imageSize.getIntValue();
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
    delete selectActorThumbs;
}

void cUpdate::LoadSeriesMedia(cTVDBSeries *series, string path) {
    int status = success;
    tSeriesMedia->clear();
    cDbStatement *selectImg = new cDbStatement(tSeriesMedia);
    selectImg->build("select ");
    selectImg->bind(cTableSeriesMedia::fiMediaWidth, cDBS::bndOut);
    selectImg->bind(cTableSeriesMedia::fiMediaHeight, cDBS::bndOut, ", ");
    selectImg->bind(cTableSeriesMedia::fiMediaType, cDBS::bndOut, ", ");
    selectImg->build(" from %s where ", tSeriesMedia->TableName());
    selectImg->bind(cTableSeriesMedia::fiSeriesId, cDBS::bndIn | cDBS::bndSet);
    selectImg->build(" and %s in (%d, %d, %d, %d, %d, %d, %d, %d, %d)",
                        tSeriesMedia->getField(cTableSeriesMedia::fiMediaType)->name,
                        msPoster1, msPoster2, msPoster3,
                        msFanart1, msFanart2, msFanart3,
                        msBanner1, msBanner2, msBanner3);
    status += selectImg->prepare();
    if (status != success) {
        delete selectImg;
        return;
    }
    tSeriesMedia->setValue(cTableSeriesMedia::fiSeriesId, series->id);
    for (int res = selectImg->find(); res; res = selectImg->fetch()) {
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
    selectImg->freeResult();
    delete selectImg;
}

string cUpdate::LoadMediaSeries(int seriesId, int mediaType, string path, int width, int height) {
    int status = success;
    stringstream iPath;
    iPath << path << "/";
    bool createThumb = false;
    stringstream tPath;
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
    cDbValue imageSize;
    cDBS::FieldDef imageSizeDef = { "media_content", cDBS::ffUInt,  0, 999, cDBS::ftData };
    imageSize.setField(&imageSizeDef);
    cDbStatement *selectImg = new cDbStatement(tSeriesMedia);
    selectImg->build("select ");
    selectImg->bind(cTableSeriesMedia::fiMediaContent, cDBS::bndOut);
    selectImg->build(", length(");
    selectImg->bind(&imageSize, cDBS::bndOut);
    selectImg->build(")");
    selectImg->build(" from %s where ", tSeriesMedia->TableName());
    selectImg->bind(cTableSeriesMedia::fiSeriesId, cDBS::bndIn | cDBS::bndSet);
    selectImg->bind(cTableSeriesMedia::fiMediaType, cDBS::bndIn | cDBS::bndSet, " and ");
    status += selectImg->prepare();
    if (status != success) {
        delete selectImg;
        return "";
    }
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
    delete selectImg;
    return imgPath;
}

//***************************************************************************
// MOVIES
//***************************************************************************

int cUpdate::ReadMovies(bool isRec) {
    scrapManager->InitIterator(isRec);
    int movieId = 0;
    int i=0;
    
    if (!CreateDirectory(config.imageDir))
        return 0;
    if (!CreateDirectory(imgPathMovies))
        return 0;

    int numNew = 0;
    while (scrapManager->GetNextMovie(isRec, movieId)) {
        cMovieDbMovie *movie = scrapManager->GetMovie(movieId);
        if (movie)
            continue;
        tMovies->clear();
        tMovies->setValue(cTableMovies::fiMovieId, movieId);
        int res = tMovies->find();
        if (!res)
            continue;
        movie = scrapManager->AddMovie(tMovies);
        stringstream mPath;
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
    int status = success;
    cDbValue actorRole;
    cDbValue actorMovie;
    cDbValue thbWidth;
    cDbValue thbHeight;
    actorRole.setField(tMovieActors->getField(cTableMovieActors::fiRole));
    actorMovie.setField(tMovieActors->getField(cTableMovieActors::fiMovieId));
    thbWidth.setField(tMovieMedia->getField(cTableMovieMedia::fiMediaWidth));
    thbHeight.setField(tMovieMedia->getField(cTableMovieMedia::fiMediaHeight));
    cDbStatement *selectActors = new cDbStatement(tMovieActor);
    selectActors->build("select ");
    selectActors->setBindPrefix("act.");
    selectActors->bind(cTableMovieActor::fiActorId, cDBS::bndOut);
    selectActors->bind(cTableMovieActor::fiActorName, cDBS::bndOut, ", ");
    selectActors->setBindPrefix("role.");
    selectActors->bind(&actorRole, cDBS::bndOut, ", ");
    selectActors->setBindPrefix("thumb.");
    selectActors->bind(&thbWidth, cDBS::bndOut, ", ");
    selectActors->bind(&thbHeight, cDBS::bndOut, ", ");
    selectActors->clrBindPrefix();
    selectActors->build(" from %s act, %s role, %s thumb where ", 
                        tMovieActor->TableName(), tMovieActors->TableName(), tMovieMedia->TableName());
    selectActors->build("act.%s = role.%s ",
                        tMovieActor->getField(cTableMovieActor::fiActorId)->name, 
                        tMovieActors->getField(cTableMovieActors::fiActorId)->name);
    selectActors->build(" and role.%s = thumb.%s ",
                        tMovieActors->getField(cTableMovieActors::fiActorId)->name, 
                        tMovieMedia->getField(cTableMovieMedia::fiActorId)->name);
    selectActors->setBindPrefix("role.");
    selectActors->bind(&actorMovie, cDBS::bndIn | cDBS::bndSet, " and ");
    status += selectActors->prepare();
    if (status != success) {
        delete selectActors;
        return;
    }
    actorMovie.setValue(movie->id);
    for (int res = selectActors->find(); res; res = selectActors->fetch()) {
        scrapManager->AddMovieActor(movie, tMovieActor, actorRole.getStrValue());
        int tmbWidth = thbWidth.getIntValue();
        int tmbHeight = thbHeight.getIntValue();
        movie->SetActorThumbSize(tMovieActor->getIntValue(cTableMovieActor::fiActorId), tmbWidth, tmbHeight);
    }
    selectActors->freeResult();
    delete selectActors;
}

void cUpdate::LoadMovieActorThumbs(cMovieDbMovie *movie) {
    int status = success;
    cDbValue imageSize;
    cDBS::FieldDef imageSizeDef = { "media_content", cDBS::ffUInt,  0, 999, cDBS::ftData };
    imageSize.setField(&imageSizeDef);
    tMovieMedia->clear();
    cDbStatement *selectActorThumbs = new cDbStatement(tMovieMedia);
    selectActorThumbs->build("select ");
    selectActorThumbs->bind(cTableMovieMedia::fiMediaContent, cDBS::bndOut);
    selectActorThumbs->build(", length(");
    selectActorThumbs->bind(&imageSize, cDBS::bndOut);
    selectActorThumbs->build(")");
    selectActorThumbs->build(" from %s where ", tMovieMedia->TableName());
    selectActorThumbs->bind(cTableMovieMedia::fiActorId, cDBS::bndIn | cDBS::bndSet);
    status += selectActorThumbs->prepare();
    if (status != success) {
        delete selectActorThumbs;
        return;
    }
    string movieActorsPath = imgPathMovies + "/actors";
    if (!CreateDirectory(movieActorsPath))
        return;

    vector<int> IDs = movie->GetActorIDs();
    for (vector<int>::iterator it = IDs.begin(); it != IDs.end(); it++) {
        int actorId = (int)*it;
        stringstream tName;
        tName << "actor_" << actorId << ".jpg";
        string thumbName = tName.str();
        string thumbFullPath = movieActorsPath + "/" + thumbName; 
        if (!FileExists(thumbFullPath)) {
            tMovieMedia->setValue(cTableMovieMedia::fiActorId, actorId);
            int res = selectActorThumbs->find();
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
    selectActorThumbs->freeResult();
    delete selectActorThumbs;
}        

void cUpdate::LoadMovieMedia(cMovieDbMovie *movie, string moviePath) {
    int status = success;
    tMovieMedia->clear();
    cDbStatement *selectImg = new cDbStatement(tMovieMedia);
    selectImg->build("select ");
    selectImg->bind(cTableMovieMedia::fiMediaWidth, cDBS::bndOut);
    selectImg->bind(cTableMovieMedia::fiMediaHeight, cDBS::bndOut, ", ");
    selectImg->bind(cTableMovieMedia::fiMediaType, cDBS::bndOut, ", ");
    selectImg->build(" from %s where ", tMovieMedia->TableName());
    selectImg->bind(cTableMovieMedia::fiMovieId, cDBS::bndIn | cDBS::bndSet);
    selectImg->build(" and %s in (%d, %d, %d, %d)",
                        tMovieMedia->getField(cTableMovieMedia::fiMediaType)->name,
                        mmPoster,
                        mmFanart,
                        mmCollectionPoster,
                        mmCollectionFanart);
    status += selectImg->prepare();
    if (status != success) {
        delete selectImg;
        return;
    }
    tMovieMedia->setValue(cTableMovieMedia::fiMovieId, movie->id);
    for (int res = selectImg->find(); res; res = selectImg->fetch()) {
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
    selectImg->freeResult();
    delete selectImg;
}

string cUpdate::LoadMediaMovie(int movieId, int mediaType, string path, int width, int height) {
    int status = success;
    stringstream iPath;
    iPath << path << "/";
    bool createThumb = false;
    stringstream tPath;
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
    cDbValue imageSize;
    cDBS::FieldDef imageSizeDef = { "media_content", cDBS::ffUInt,  0, 999, cDBS::ftData };
    imageSize.setField(&imageSizeDef);
    cDbStatement *selectImg = new cDbStatement(tMovieMedia);
    selectImg->build("select ");
    selectImg->bind(cTableMovieMedia::fiMediaContent, cDBS::bndOut);
    selectImg->build(", length(");
    selectImg->bind(&imageSize, cDBS::bndOut);
    selectImg->build(")");
    selectImg->build(" from %s where ", tMovieMedia->TableName());
    selectImg->bind(cTableMovieMedia::fiMovieId, cDBS::bndIn | cDBS::bndSet);
    selectImg->bind(cTableMovieMedia::fiMediaType, cDBS::bndIn | cDBS::bndSet, " and ");
    status += selectImg->prepare();
    if (status != success) {
        delete selectImg;
        return "";
    }
    tMovieMedia->setValue(cTableMovieMedia::fiMovieId, movieId);
    tMovieMedia->setValue(cTableMovieMedia::fiMediaType, mediaType);
    int res = selectImg->find();
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
    selectImg->freeResult();
    delete selectImg;
    return imgPath;
}

//***************************************************************************
// RECORDINGS
//***************************************************************************
int cUpdate::ReadRecordings(void) {
    int status = success;
    cDbStatement *select = new cDbStatement(tRecordings);
    select->build("select ");
    select->bind(cTableRecordings::fiRecPath, cDBS::bndOut);
    select->bind(cTableRecordings::fiRecStart, cDBS::bndOut, ", ");
    select->bind(cTableRecordings::fiMovieId, cDBS::bndOut, ", ");
    select->bind(cTableRecordings::fiSeriesId, cDBS::bndOut, ", ");
    select->bind(cTableRecordings::fiEpisodeId, cDBS::bndOut, ", ");
    select->build(" from %s where ", tRecordings->TableName());
    select->bind(cTableRecordings::fiUuid, cDBS::bndIn | cDBS::bndSet);
    select->build(" and %s = 0", tRecordings->getField(cTableRecordings::fiScrapNew)->name);

    status += select->prepare();
    if (status != success) {
        delete select;
        return 0;
    }

    tRecordings->clear();
    tRecordings->setValue(cTableRecordings::fiUuid, config.uuid.c_str());
    int numRecs = 0;
    for (int res = select->find(); res; res = select->fetch()) {
        int recStart = tRecordings->getIntValue(cTableRecordings::fiRecStart);
        string recPath = tRecordings->getStrValue(cTableRecordings::fiRecPath);
        int movieId = tRecordings->getIntValue(cTableRecordings::fiMovieId);
        int seriesId = tRecordings->getIntValue(cTableRecordings::fiSeriesId);
        int episodeId = tRecordings->getIntValue(cTableRecordings::fiEpisodeId);
        bool isNew = scrapManager->AddRecording(recStart, recPath, seriesId, episodeId, movieId);
        if (isNew)
            numRecs++;
    }
    select->freeResult();
    delete select;
    return numRecs;
}

int cUpdate::ScanVideoDir(void) {
    int newRecs = 0;
    for (cRecording *rec = Recordings.First(); rec; rec = Recordings.Next(rec)) {
        string recPath = rec->FileName();
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
        string recPath = rec->FileName();
        bool recExists = LoadRecording(recStart, recPath);
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
    stringstream sInfoName;
    sInfoName << recDir << "/" << config.recScrapInfoName;
    string scrapInfoName = sInfoName.str();
    if (!FileExists(scrapInfoName, false)) {
        string twoHigher = TwoFoldersHigher(recDir);
        if (twoHigher.size() > 0) {
            stringstream sInfoNameAlt;
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
    //delete all not anymore existing recordings in database
    int status = success;
    cDbStatement *select = new cDbStatement(tRecordings);
    select->build("select ");
    select->bind(cTableRecordings::fiRecPath, cDBS::bndOut);
    select->bind(cTableRecordings::fiRecStart, cDBS::bndOut, ", ");
    select->build(" from %s where ", tRecordings->TableName());
    select->bind(cTableRecordings::fiUuid, cDBS::bndIn | cDBS::bndSet);
    
    status += select->prepare();
    if (status != success) {
        delete select;
        return 0;
    }

    tRecordings->clear();
    tRecordings->setValue(cTableRecordings::fiUuid, config.uuid.c_str());
    int numRecsDeleted = 0;
    for (int res = select->find(); res; res = select->fetch()) {
        int recStart = tRecordings->getIntValue(cTableRecordings::fiRecStart);
        string recPath = tRecordings->getStrValue(cTableRecordings::fiRecPath);
        if (!Recordings.GetByName(recPath.c_str())) {
            stringstream delWhere;
            delWhere << "uuid = '" << config.uuid << "' and rec_path = '" << recPath << "' and rec_start = " << recStart;
            tRecordings->deleteWhere(delWhere.str().c_str());
            numRecsDeleted++;
        }
    }
    select->freeResult();
    delete select;
    return numRecsDeleted;
}


//***************************************************************************
// Action
//***************************************************************************

void cUpdate::Action() {
    tell(0, "Update thread started (pid=%d)", getpid());
    mutex.Lock();
    loopActive = yes;
    int sleep = 10;
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
    while (loopActive && Running()) {
        int reconnectTimeout; //set by checkConnection
        if (CheckConnection(reconnectTimeout) != success) {
            waitCondition.TimedWait(mutex, reconnectTimeout*1000);
            continue;
        }
        //Update Recordings from Database
        if (forceRecordingUpdate || (time(0) - lastScanNewRecDB > scanNewRecDBFreq)) {
            if (!init && CheckEpgdBusy())
                continue;
            int numNewRecs = ReadRecordings();
            if (numNewRecs > 0) {
                int numSeries = ReadSeries(true);
                int numMovies = ReadMovies(true);
                tell(0, "Loaded %d new Recordings from Database, %d series, %d movies", numNewRecs, numSeries, numMovies);
            }
            forceRecordingUpdate = false;
        }

        //Update Events
        if (!config.headless && (forceUpdate || (time(0) - lastScan > scanFreq))) {
            if (!init && CheckEpgdBusy())
                continue;
            int numNewEvents = ReadScrapedEvents();
            if (numNewEvents > 0) {
                tell(0, "Loaded %d new scraped Events from Database", numNewEvents);
            } else {
                lastScan = time(0);
                forceUpdate = false;
                init = false;
                continue;
            }
            tell(0, "Loading new Movies from Database...");
            time_t now = time(0);
            int numNewMovies = ReadMovies(false);
            int dur = time(0) - now;
            tell(0, "Loaded %d new Movies in %ds from Database", numNewMovies, dur);

            tell(0, "Loading new Series and Episodes from Database...");
            now = time(0);
            int numNewSeries = ReadSeries(false);
            dur = time(0) - now;
            tell(0, "Loaded %d new Series and Episodes in %ds from Database", numNewSeries, dur);

            lastScan = time(0);
            forceUpdate = false;
        }
        
        //Scan new recordings
        if (init || forceVideoDirUpdate || (time(0) - lastScanNewRec > scanNewRecFreq)) {
            if (CheckEpgdBusy()) {
                waitCondition.TimedWait(mutex, 1000);
                continue;
            }
            static int recState = 0;
            if (Recordings.StateChanged(recState)) {
                tell(0, "Searching for new recordings because of Recordings State Change...");
                int newRecs = ScanVideoDir();
                tell(0, "found %d new recordings", newRecs);
            }
            lastScanNewRec = time(0);
            forceVideoDirUpdate = false;
        }

        init = false;

        //Scan Video dir for scrapinfo files
        if (forceScrapInfoUpdate) {
            if (CheckEpgdBusy()) {
                tell(0, "epgd busy, try again in 1s...");
                waitCondition.TimedWait(mutex, 1000);
                continue;
            }
            tell(0, "Checking for new or updated scrapinfo files in recordings...");
            int numUpdated = ScanVideoDirScrapInfo();
            tell(0, "found %d new or updated scrapinfo files", numUpdated);
            forceScrapInfoUpdate = false;
        }
        
        //Cleanup
        if (time(0) - lastCleanup > cleanUpFreq) {
           if (CheckEpgdBusy()) {
                waitCondition.TimedWait(mutex, 1000);
                continue;
            }
            int seriesDeleted = CleanupSeries();
            int moviesDeleted = CleanupMovies();
            if (seriesDeleted > 0 || moviesDeleted > 0) {
                tell(0, "Deleted %d outdated series image folders", seriesDeleted);            
                tell(0, "Deleted %d outdated movie image folders", moviesDeleted);
            }
            lastCleanup = time(0);
        }
        
        //Cleanup Recording DB
        if (forceCleanupRecordingDb) {
            if (CheckEpgdBusy()) {
                waitCondition.TimedWait(mutex, 1000);
                continue;
            }
            tell(0, "Cleaning up recordings in database...");
            int recsDeleted = CleanupRecordings();
            tell(0, "Deleted %d not anymore existing recordings in database", recsDeleted);
            forceCleanupRecordingDb = false;
        }

        waitCondition.TimedWait(mutex, sleep*1000);

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
