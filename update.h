#ifndef __UPDATE_H
#define __UPDATE_H

#include <mysql/mysql.h>
#include <map>

#include <vdr/thread.h>
#include "lib/common.h"
#include "lib/db.h"
#include "lib/tabledef.h"
#include "scrapmanager.h"

#define EPGDNAME "epgd"

class cUpdate : public cThread  {
    private:
        cScrapManager *scrapManager;
        string imgPathSeries;
        string imgPathMovies;
        bool withutf8;
        bool loopActive;
        cDbConnection* connection;
        cTableVdrs* vdrDb;
        cTableEvents* tEvents;
        cTableSeries* tSeries;
        cTableSeriesEpisode* tEpisodes;
        cTableSeriesMedia* tSeriesMedia;
        cTableSeriesActor* tSeriesActors;
        cTableMovies* tMovies;
        cTableMovieActor* tMovieActor;
        cTableMovieActors* tMovieActors;
        cTableMovieMedia* tMovieMedia;
        cTableRecordings* tRecordings;
        int lastScrap;
        cCondVar waitCondition;
        cMutex mutex;
        bool forceUpdate;
        bool forceRecordingUpdate;
        bool forceVideoDirUpdate;
        bool forceScrapInfoUpdate;
        bool forceCleanupRecordingDb;
        int exitDb();
        int dbConnected(int force = no) { return connection && (!force || connection->check() == success); };
        int CheckConnection(int& timeout);
        bool CheckEpgdBusy(void);
        void Action(void);
        int ReadScrapedEvents(void);
        //SERIES
        int ReadSeries(bool isRec);
        void ReadEpisode(int episodeId, cTVDBSeries *series, string path);
        void LoadEpisodeImage(cTVDBSeries *series, int episodeId, string path);
        void LoadSeasonPoster(cTVDBSeries *series, int season, string path);
        void ReadSeriesActors(cTVDBSeries *series, string path);
        void LoadSeriesMedia(cTVDBSeries *series, string path);
        string LoadMediaSeries(int seriesId, int mediaType, string path, int width, int height);
        void LoadSeriesActorThumb(cTVDBSeries *series, int actorId, string path);
        //MOVIES
        int ReadMovies(bool isRec);
        void ReadMovieActors(cMovieDbMovie *movie);
        void LoadMovieActorThumbs(cMovieDbMovie *movie);
        void LoadMovieMedia(cMovieDbMovie *movie, string moviePath);
        string LoadMediaMovie(int movieId, int mediaType, string path, int width, int height);
        //RECORDINGS
        int ReadRecordings(void);
        int ScanVideoDir(void);
        int ScanVideoDirScrapInfo(void);
        bool LoadRecording(int eventId, string recName);
        bool ScrapInfoChanged(int scrapInfoMovieID, int scrapInfoSeriesID, int scrapInfoEpisodeID);
        void ReadScrapInfo(string recDir, int &scrapInfoMovieID, int &scrapInfoSeriesID, int &scrapInfoEpisodeID);
        //CLEANUP
        int CleanupSeries(void);
        int CleanupMovies(void);
        int CleanupRecordings(void);
    public:
        cUpdate(cScrapManager *manager);
        virtual ~cUpdate(void);
        int initDb();
        void Stop(void);
        void ForceUpdate(void);
        void ForceRecordingUpdate(void);
        void ForceVideoDirUpdate(void);
        void ForceScrapInfoUpdate(void);
        void TriggerCleanRecordingsDB(void);
};

//***************************************************************************
#endif //__UPDATE_H
