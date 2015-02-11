#ifndef __UPDATE_H
#define __UPDATE_H

#include <map>

#include <vdr/thread.h>
#include "lib/common.h"
#include "lib/db.h"
#include "lib/tabledef.h"
#include "scrapmanager.h"
#include "filedatemanager.h"

#define EPGDNAME "epgd"
#define LogPeriode 15000 // write log entry after each 15s

class cUpdate : public cThread  {
    private:
        cScrapManager *scrapManager;
        string TempEpisodeTableName;
        string TempEpisodeCreateStatement;
        string TempEpisodeDeleteStatement;
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
        cFileDateManager fileDateManager;
        int lastScrap;
        long MaxScrspSeries; // max scrsp of known events/recordings for series
        long MaxScrspMovies; // max scrsp of known events/recordings for movies
        cCondVar waitCondition;
        cMutex mutex;
        long lastWait; // when did we call waitCondition.TimedWait last time (in ms)
        bool forceUpdate;
        bool forceRecordingUpdate;
        bool forceVideoDirUpdate;
        bool forceScrapInfoUpdate;
        bool forceCleanupRecordingDb;
        bool forceFullUpdate; // force full update (information and images)
        int exitDb();
        int dbConnected(int force = no) { return connection && (!force || connection->check() == success); };
        int CheckConnection(int& timeout);
        bool CheckEpgdBusy(void);
        bool CheckRunningAndWait(void); // check if we should abort execution (thread stoped), also check if we should call waitCondition.TimedWait (so other processes can use the CPU) 
        void Action(void);
        //EVENTS
        int ReadScrapedEvents(void);
        //SERIES
        int ReadSeries(bool isRec);
        int ReadSeriesFast(long &maxscrsp); // read all series with event or recording from sql-db
        int ReadSeriesContentFast(int &newImages, int &newSeasonPoster); // read all episodes/actors/media from all series where series->updatecontent = true
        int ReadEpisodesContentFast(cTVDBSeries *series); // read all episodes for current series with event or recording from sql-db
        void ReadSeriesMediaFast(cTVDBSeries *series, int &newImages, int &newSeasonPoster); // read all images of series (also create actors if not available yet)
        void CheckSeriesMedia(cTVDBSeries *series, int mediaType, int imgWidth, int imgHeight, string imgName, int season, bool needRefresh); // create media if not exists in series, update size, path, needRefresh of media
        void ReadSeriesImagesFast(int &newImages, int &emptySeasonPosters); // read all images with needrefresh from sql-db
        bool ReadSeriesImageFast(int seriesId, int season, int episodeId, int actorId, int mediaType, cTVDBMedia *media, cTVDBMedia *mediaThumb, int shrinkFactor); // read real image data from sql-db
        void ReadEpisode(int episodeId, cTVDBSeries *series, string path);
        void LoadEpisodeImage(cTVDBSeries *series, int episodeId, string path);
        void LoadSeasonPoster(cTVDBSeries *series, int season, string path);
        void ReadSeriesActors(cTVDBSeries *series, string path);
        void LoadSeriesMedia(cTVDBSeries *series, string path);
        string LoadMediaSeries(int seriesId, int mediaType, string path, int width, int height);
        void LoadSeriesActorThumb(cTVDBSeries *series, int actorId, string path);
        //MOVIES
        int ReadMovies(bool isRec);
        int ReadMoviesFast(long &maxscrsp); // read all movies with event or recording from sql-db
        int ReadMoviesContentFast(void); // read all actors/media from all movies where movie->updateimages = true
        int ReadMovieActorsFast(cMovieDbMovie *movie); // read all actor for current movie from sql-db
        int ReadMovieMediaFast(cMovieDbMovie *movie); // read all images for current movie from sql-db
        void CheckMovieMedia(cMovieDbMovie *movie, int mediaType, int imgWidth, int imgHeight, string imgName, bool needRefresh); // create media if not exists in movie, update size, path, needRefresh of media
        int ReadMovieImagesFast(void); // read all images with needrefresh from sql-db
        bool ReadMovieImageFast(int movieId, int actorId, int mediaType, cMovieMedia *media, cMovieMedia *mediaThumb, int shrinkFactor); // read real image data from sql-db
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

      // statements

      cDbStatement* selectReadScrapedEventsInit;
      cDbStatement* selectReadScrapedEvents;
      cDbStatement* selectImg;
      cDbStatement* selectEpisodeImg;
      cDbStatement* selectSeasonPoster;
      cDbStatement* selectActors;
      cDbStatement* selectActorThumbs;
      cDbStatement* selectSeriesMedia;
      cDbStatement* selectMovieActors;
      cDbStatement* selectMovieActorThumbs;
      cDbStatement* selectMovieMedia;
      cDbStatement* selectMediaMovie;
      cDbStatement* selectRecordings;
      cDbStatement* selectCleanupRecordings;
      cDbStatement* clearTempEpisodeTable;
      cDbStatement* fillTempEpisodeTable;
      cDbStatement* selectReadSeriesInit; // statement to select all series which are used from min one event
      cDbStatement* selectReadSeriesLastScrsp; // statement to select all series which are used from min one event (filtered by > scrsp)
      cDbStatement* selectReadEpisodes; // statement to select all episodes for one series which are used from min one event/recording
      cDbStatement* selectSeriesMediaActors; // statement to select all media and actor information for one series (but without binary image data)
      cDbStatement* selectSeriesImage; // statement to select one single image from series_media (with binary image data)
      cDbStatement* selectReadMoviesInit; // statement to select all movies which are used from min one event
      cDbStatement* selectReadMoviesLastScrsp; // statement to select all movies which are used from min one event (filtered by > scrsp)
      cDbStatement* selectMovieMediaFast; // statement to select all media information for one movie (but without binary image data)
      cDbStatement* selectMovieImage; // statement to select one single image from movie_media (with binary image data)

      cDbValue imageSize;
      cDbValue episodeImageSize;
      cDbValue posterSize;
      cDbValue series_id;
      cDbValue event_scrsp;
      cDbValue events_ScrSeriesId;
      cDbValue episode_LastUpdate;
      cDbValue vdr_uuid;
      cDbValue actorImageSize;

      cDbValue actorRole;
      cDbValue actorMovie;
      cDbValue thbWidth;
      cDbValue thbHeight;

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
        void ForceFullUpdate(void);
};

//***************************************************************************
#endif //__UPDATE_H
