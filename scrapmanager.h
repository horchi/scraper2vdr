#ifndef __SCRAPMANAGER_H
#define __SCRAPMANAGER_H

#include <vector>
#include <map>
#include <set>
#include <utility>
#include <algorithm>

#include "lib/common.h"
#include "lib/db.h"
#include "lib/tabledef.h"

#include "services.h"
#include "tvdbseries.h"
#include "moviedbmovie.h"

using namespace std;

struct sEventsKey {
    int eventId;
    string channelId;
};

struct sEventsValue {
    int seriesId;
    int episodeId;
    int movieId;
    bool isNew;
};

struct sRecordingsKey {
    int recStart;
    string recPath;
};

class cScrapManager  {
    private:
        map<sEventsKey, sEventsValue> events;
        map<sEventsKey, sEventsValue>::iterator eventsIterator;
        map<sRecordingsKey, sEventsValue> recordings;
        map<sRecordingsKey, sEventsValue>::iterator recIterator;
        map<int, cTVDBSeries*> series;
        map<int, cMovieDbMovie*> movies;
    public:
        cScrapManager(void);
        virtual ~cScrapManager(void);
        //Series and Movies Handling
        void AddEvent(int eventId, string channelId, int seriesId, int episodeId, int movieId);
        void InitIterator(bool isRec);
        int GetNumSeries(void) { return series.size(); };
        int GetNumMovies(void) { return movies.size(); };
        sEventsValue GetEventInformation(int eventId, string channelId);
        bool GetNextSeries(bool isRec, int &seriesId, int &episodeId);
        bool GetNextMovie(bool isRec, int &movieId);
        cTVDBSeries *GetSeries(int seriesId);
        cMovieDbMovie *GetMovie(int movieId);
        cTVDBSeries *AddSeries(cTableSeries* tSeries);
        cMovieDbMovie *AddMovie(cTableMovies* tMovies);
        void AddSeriesEpisode(cTVDBSeries *series, cTableSeriesEpisode* tEpisodes);
        void AddSeriesActor(cTVDBSeries *series, cTableSeriesActor* tActors);
        void AddMovieActor(cMovieDbMovie *movie, cTableMovieActor* tActor, string role);
        void AddMovieMedia(cMovieDbMovie *movie, cTableMovieMedia* tMovieMedia, string path);
        //Recording Handling
        bool AddRecording(int recStart, string recPath, int seriesId, int episodeId, int movieId);
        bool RecordingExists(int recStart, string recPath);
        bool SeriesInUse(int seriesId);
        bool MovieInUse(int movieId);
        //Debug
        void DumpSeries(void);
        void DumpMovies(void);
        void DumpRecordings(void);
        //Service Calls
        bool GetEventType(ScraperGetEventType *call);
        bool GetSeries(cSeries *series);
        bool GetMovie(cMovie *movie);
        bool GetPosterBanner(ScraperGetPosterBanner *call);
        bool GetPosterBannerV2(ScraperGetPosterBannerV2 *call);
        bool GetPoster(ScraperGetPoster *call);
        bool GetPosterThumb(ScraperGetPosterThumb *call);
};
#endif //__SCRAPMANAGER_H
