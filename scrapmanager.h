#ifndef __SCRAPMANAGER_H
#define __SCRAPMANAGER_H

#include <vector>
#include <map>
#include <set>
#include <utility>
#include <algorithm>

#include "lib/common.h"
#include "lib/db.h"

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
        map<int, cTVDBSeries*>::iterator seriesIterator;
        map<int, cMovieDbMovie*> movies;
        map<int, cMovieDbMovie*>::iterator moviesIterator;
        map<int, cMovieMedia*> movieActorsThumbs; // map with all movie actor thumbs
        map<int, cMovieMedia*>::iterator movieActorsThumbsIterator;
    public:
        cScrapManager(void);
        virtual ~cScrapManager(void);
        //Series and Movies Handling
        void AddEvent(int eventId, string channelId, int seriesId, int episodeId, int movieId);
        void InitIterator(bool isRec);
        int GetNumSeries(void) { return series.size(); };
        int GetNumMovies(void) { return movies.size(); };
        int GetNumMovieActors(void) { return movieActorsThumbs.size(); };
        sEventsValue GetEventInformation(int eventId, string channelId);
        bool GetNextSeries(bool isRec, int &seriesId, int &episodeId);
        bool GetNextMovie(bool isRec, int &movieId);
        int GetSeriesFirst(cTVDBSeries* &seriesval); // get first series (also init series iterator)
        int GetSeriesNext(cTVDBSeries* &seriesval); // get next series from iterator
        int GetMoviesFirst(cMovieDbMovie* &movieval); // get first movie (also init series iterator)
        int GetMoviesNext(cMovieDbMovie* &movieval); // get next movie from iterator
        cTVDBSeries *GetSeries(int seriesId);
        cMovieDbMovie *GetMovie(int movieId);
        cTVDBSeries *AddSeries(cDbTable* tSeries);
        void UpdateSeries(cTVDBSeries *seriesval, cDbTable* tSeries); // update series using values from current db row
        cMovieDbMovie *AddMovie(cDbTable* tMovies);
        void UpdateMovie(cMovieDbMovie *movieval, cDbTable* tMovies); // update movie using values from current db row
        void AddSeriesEpisode(cTVDBSeries *series, cDbTable* tEpisodes);
        void UpdateSeriesEpisode(cTVDBEpisode *episode, cDbTable* tEpisodes); // update episode using values from current db row
        cTVDBActor *AddSeriesActor(cTVDBSeries *series, cDbTable* tActors);
        void UpdateSeriesActor(cTVDBActor *actor, cDbTable* tActors); // update actor using values from current db row
        cMovieActor *AddMovieActor(cMovieDbMovie *movie, cDbTable* tActor, string role, bool noActorThumb);
        void UpdateMovieActor(cMovieActor *actor, cDbTable* tActor, string role); // update actor using values from current db row
        void AddMovieMedia(cMovieDbMovie *movie, cDbTable* tMovieMedia, string path);
        cMovieMedia *GetMovieActorThumb(int actorId); // try to find actor in global movie actor thumbs map
        cMovieMedia *AddMovieActorThumb(int actorId, int imgWidth, int imgHeight, string path, bool needrefresh); // insert movie actor thumb
        int GetMovieActorThumbFirst(int &actorId, cMovieMedia* &movieActorThumb); // get first movie actor (also init series iterator)
        int GetMovieActorThumbNext(int &actorId, cMovieMedia* &movieActorThumb); // get next movie actor from iterator
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
