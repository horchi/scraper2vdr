#define __STL_v_H
#include <vdr/recording.h>

#include "tools.h"
#include "scrapmanager.h"

using namespace std;

bool operator<(const sEventsKey& l, const sEventsKey& r) {
     if (l.eventId != r.eventId)
        return (l.eventId < r.eventId);
     int comp = l.channelId.compare(r.channelId);
     if (comp < 0)
        return true;
     return false;
}

bool operator<(const sRecordingsKey& l, const sRecordingsKey& r) {
     if (l.recStart != r.recStart)
        return (l.recStart < r.recStart);
     int comp = l.recPath.compare(r.recPath);
     if (comp < 0)
        return true;
     return false;
}

cScrapManager::cScrapManager(void) {

}

cScrapManager::~cScrapManager(void) {
    for (map<int, cTVDBSeries*>::iterator it = series.begin(); it != series.end(); it++) {
        cTVDBSeries *s = (cTVDBSeries*)it->second;
        delete s;
    }
    series.clear();
    for (map<int, cMovieDbMovie*>::iterator it = movies.begin(); it != movies.end(); it++) {
        cMovieDbMovie *m = (cMovieDbMovie*)it->second;
        delete m;
    }
    movies.clear();
    for (map<int, cMovieMedia*>::iterator it = movieActorsThumbs.begin(); it != movieActorsThumbs.end(); it++) {
        cMovieMedia *ma = (cMovieMedia*)it->second;
        delete ma;
    }
    movieActorsThumbs.clear();    
}

void cScrapManager::InitIterator(bool isRec) {
	if (!isRec)
		eventsIterator = events.begin();
	else
		recIterator = recordings.begin();
}

sEventsValue cScrapManager::GetEventInformation(int eventId, string channelId) {
	sEventsKey k;
	k.eventId = eventId;
	k.channelId = channelId;
	sEventsValue emptyVal;
	emptyVal.seriesId = 0;
	emptyVal.episodeId = 0;
	emptyVal.movieId = 0;
	emptyVal.isNew = false;
	map<sEventsKey, sEventsValue>::iterator hit = events.find(k);
	if (hit != events.end())
		return hit->second;
	return emptyVal;
}


void cScrapManager::AddEvent(int eventId, string channelId, int seriesId, int episodeId, int movieId) {
	sEventsKey k;
	k.eventId = eventId;
	k.channelId = channelId;
	sEventsValue v;
	v.seriesId = seriesId;
	v.episodeId = episodeId;
	v.movieId = movieId;
	v.isNew = true;
	events.insert(pair<sEventsKey, sEventsValue>(k, v));
}

bool cScrapManager::GetNextSeries(bool isRec, int &seriesId, int &episodeId) {
	bool next = false;
	if (!isRec) {
		while (eventsIterator != events.end()) {
			next = true;
			sEventsValue ev = eventsIterator->second;
			if (ev.isNew && (ev.seriesId > 0)) {
				seriesId = ev.seriesId;
				episodeId = ev.episodeId;
				eventsIterator->second.isNew = false;
				eventsIterator++;
				break;
			}
			eventsIterator++;
			next = false;
		}
	} else {
		while (recIterator != recordings.end()) {
			next = true;
			sEventsValue ev = recIterator->second;
			if (ev.isNew && (ev.seriesId > 0)) {
				seriesId = ev.seriesId;
				episodeId = ev.episodeId;
				recIterator->second.isNew = false;
				recIterator++;
				break;
			}
			recIterator++;
			next = false;
		}

	}
	return next;
}

bool cScrapManager::GetNextMovie(bool isRec, int &movieId) {
	bool next = false;
	if (!isRec) {
		while (eventsIterator != events.end()) {
			next = true;
			sEventsValue ev = eventsIterator->second;
			if (ev.isNew && (ev.movieId > 0)) {
				movieId = ev.movieId;
				eventsIterator->second.isNew = false;
				eventsIterator++;
				break;
			}
			eventsIterator++;
			next = false;
		}
	} else {
		while (recIterator != recordings.end()) {
			next = true;
			sEventsValue ev = recIterator->second;
			if (ev.isNew && (ev.movieId > 0)) {
				movieId = ev.movieId;
				recIterator->second.isNew = false;
				recIterator++;
				break;
			}
			recIterator++;
			next = false;
		}		
	}
	return next;
}

// get first series (also init series iterator)
int cScrapManager::GetSeriesFirst(cTVDBSeries* &seriesval) {
    seriesval = NULL;
    seriesIterator = series.begin();
    if (seriesIterator == series.end())
        return 0; // no series availabe
    seriesval = seriesIterator->second; // return current series
    return 1; 
}

// get next series from iterator
int cScrapManager::GetSeriesNext(cTVDBSeries* &seriesval) {
    seriesval=NULL;
    if (seriesIterator != series.end())
        seriesIterator++;
    if (seriesIterator == series.end())
        return 0; // no series availabe
    seriesval = seriesIterator->second; // return current series
    return 1; 
}

// get first movie (also init series iterator)
int cScrapManager::GetMoviesFirst(cMovieDbMovie* &movieval) {
    movieval = NULL;
    moviesIterator = movies.begin();
    if (moviesIterator == movies.end())
        return 0; // no movie availabe
    movieval = moviesIterator->second; // return current movie
    return 1; 
}

// get next movie from iterator
int cScrapManager::GetMoviesNext(cMovieDbMovie* &movieval) {
    movieval=NULL;
    if (moviesIterator != movies.end())
        moviesIterator++;
    if (moviesIterator == movies.end())
        return 0; // no movie availabe
    movieval = moviesIterator->second; // return current movie
    return 1; 
}

cTVDBSeries *cScrapManager::GetSeries(int seriesId) {
	map<int, cTVDBSeries*>::iterator hit = series.find(seriesId);
	if (hit == series.end())
		return NULL;
	return hit->second;
}

cMovieDbMovie *cScrapManager::GetMovie(int movieId) {
	map<int, cMovieDbMovie*>::iterator hit = movies.find(movieId);
	if (hit == movies.end())
		return NULL;
	return hit->second;	
}

cTVDBSeries *cScrapManager::AddSeries(cDbTable* tSeries) {
    cTVDBSeries *s = new cTVDBSeries();
    UpdateSeries(s,tSeries);
    s->updateimages = true; // have to search images for new series
    series.insert(pair<int, cTVDBSeries*>(tSeries->getIntValue("SERIESID"), s));
    return s;
}

// update series using values from current db row
void cScrapManager::UpdateSeries(cTVDBSeries *seriesval, cDbTable* tSeries) {
    seriesval->id = tSeries->getIntValue("SERIESID");
    seriesval->name = tSeries->getStrValue("SERIESNAME");
    seriesval->overview = tSeries->getStrValue("SERIESOVERVIEW");
    seriesval->firstAired = tSeries->getStrValue("SERIESFIRSTAIRED");
    seriesval->network = tSeries->getStrValue("SERIESNETWORK");
    string genre = replaceString(tSeries->getStrValue("SERIESGENRE"), "|", ", ");
    seriesval->genre = genre;
    seriesval->rating = tSeries->getFloatValue("SERIESRATING");
    seriesval->status = tSeries->getStrValue("SERIESSTATUS");
    seriesval->lastupdate = tSeries->getIntValue("SERIESLASTUPDATED"); // save when series was last updated on server
}

cMovieDbMovie *cScrapManager::AddMovie(cDbTable* tMovies) {
    cMovieDbMovie *m = new cMovieDbMovie();
    UpdateMovie(m,tMovies);
    m->updateimages = true; // have to search images for new movies
    movies.insert(pair<int, cMovieDbMovie*>(tMovies->getIntValue("MOVIEID"), m));
    return m;
}

// update movie using values from current db row
void cScrapManager::UpdateMovie(cMovieDbMovie *movieval, cDbTable* tMovies) {
    movieval->id = tMovies->getIntValue("MOVIEID");
    movieval->title = tMovies->getStrValue("TITLE");
    movieval->originalTitle = tMovies->getStrValue("ORIGINALTITLE");
    movieval->tagline = tMovies->getStrValue("TAGLINE");
    movieval->overview = tMovies->getStrValue("OVERVIEW");
    movieval->adult = tMovies->getIntValue("ISADULT");
    movieval->collectionName = tMovies->getStrValue("COLLECTIONNAME");
    movieval->budget = tMovies->getIntValue("BUDGET");
    movieval->revenue = tMovies->getIntValue("REVENUE");
    string genre = replaceString(tMovies->getStrValue("GENRES"), "|", ",");
    movieval->genres = genre;
    movieval->homepage = tMovies->getStrValue("HOMEPAGE");
    movieval->releaseDate = tMovies->getStrValue("RELEAASEDATE");
    movieval->runtime = tMovies->getIntValue("RUNTIME");
    movieval->popularity = tMovies->getFloatValue("POPULARITY");
    movieval->voteAverage = tMovies->getFloatValue("VOTEAVERAGE");
}

void cScrapManager::AddSeriesEpisode(cTVDBSeries *series, cDbTable* tEpisodes) {
    cTVDBEpisode *e = new cTVDBEpisode();
    UpdateSeriesEpisode(e,tEpisodes);
    series->InsertEpisode(e);
}

// update episode using values from current db row
void cScrapManager::UpdateSeriesEpisode(cTVDBEpisode *episode, cDbTable* tEpisodes) {
    episode->id = tEpisodes->getIntValue("EPISODEID");
    episode->name = tEpisodes->getStrValue("EPISODENAME");
    episode->number =  tEpisodes->getIntValue("EPISODENUMBER");
    episode->season = tEpisodes->getIntValue("SEASONNUMBER");
    episode->overview = tEpisodes->getStrValue("EPISODEOVERVIEW");
    episode->firstAired = tEpisodes->getStrValue("EPISODEFIRSTAIRED");
    string guestStars = replaceString(tEpisodes->getStrValue("EPISODEGUESTSTARS"), "|", ", ");
    episode->guestStars = guestStars;
    episode->rating = tEpisodes->getFloatValue("EPISODERATING");
    episode->lastupdate = tEpisodes->getIntValue("EPISODELASTUPDATED"); 
}

cTVDBActor *cScrapManager::AddSeriesActor(cTVDBSeries *series, cDbTable* tActors) {
    cTVDBActor *a = new cTVDBActor();
    UpdateSeriesActor(a,tActors);
    series->InsertActor(a);
    return a;
}

// update actor using values from current db row
void cScrapManager::UpdateSeriesActor(cTVDBActor *actor, cDbTable* tActors) {
    actor->id = tActors->getIntValue("ACTORID");
    actor->name = tActors->getStrValue("ACTORNAME");
    actor->role =  tActors->getStrValue("ACTORROLE");
}

cMovieActor * cScrapManager::AddMovieActor(cMovieDbMovie *movie, cDbTable* tActor, string role, bool noActorThumb) {
    cMovieActor *a = new cMovieActor();
    UpdateMovieActor(a, tActor, role);
    if (noActorThumb) {
      movie->InsertActorNoThumb(a);
    } else {
      movie->InsertActor(a);
    }    
    return a;
}

// update actor using values from current db row
void cScrapManager::UpdateMovieActor(cMovieActor *actor, cDbTable* tActor, string role) {
    actor->id = tActor->getIntValue("ACTORID");
    actor->name = tActor->getStrValue("ACTORNAME");
    actor->role =  role;
}

void cScrapManager::AddMovieMedia(cMovieDbMovie *movie, cDbTable* tMovieMedia, string path) {
	cMovieMedia *m = new cMovieMedia();
	m->mediaType = tMovieMedia->getIntValue("MEDIATYPE");
	m->width = tMovieMedia->getIntValue("MEDIAWIDTH");
	m->height = tMovieMedia->getIntValue("MEDIAHEIGHT");
    m->path = path;
    movie->InsertMedia(m);	
}

// try to find actor in global movie actor thumbs map
cMovieMedia *cScrapManager::GetMovieActorThumb(int actorId) {
    map<int, cMovieMedia*>::iterator hit = movieActorsThumbs.find(actorId);
    if (hit == movieActorsThumbs.end())
        return NULL;    
    return hit->second;
}

// insert movie actor thumb
cMovieMedia *cScrapManager::AddMovieActorThumb(int actorId, int imgWidth, int imgHeight, string path, bool needrefresh) {
    cMovieMedia *m = new cMovieMedia();
    m->width = imgWidth;
    m->height = imgHeight;
    m->path = path;
    m->mediaType = mmActorThumb;
    m->needrefresh = needrefresh;
    m->mediavalid = m->mediavalid || !needrefresh; // all media which do not need a refresh should have a image file (if was valid, is stay valid)
    movieActorsThumbs.insert(pair<int, cMovieMedia*>(actorId, m));
    return m;
}

// get first movie actor (also init series iterator)
int cScrapManager::GetMovieActorThumbFirst(int &actorId, cMovieMedia* &movieActorThumb) {
    movieActorThumb = NULL;
    actorId = 0;
    movieActorsThumbsIterator = movieActorsThumbs.begin();
    if (movieActorsThumbsIterator == movieActorsThumbs.end())
        return 0; // no movie availabe
    movieActorThumb = movieActorsThumbsIterator->second; // return current movie
    actorId = movieActorsThumbsIterator->first; 
    return 1; 
}

// get next movie actor from iterator
int cScrapManager::GetMovieActorThumbNext(int &actorId, cMovieMedia* &movieActorThumb) {
    movieActorThumb=NULL;
    actorId = 0;
    if (movieActorsThumbsIterator != movieActorsThumbs.end())
        movieActorsThumbsIterator++;
    if (movieActorsThumbsIterator == movieActorsThumbs.end())
        return 0; // no movie availabe
    movieActorThumb = movieActorsThumbsIterator->second; // return current movie
    actorId = movieActorsThumbsIterator->first; 
    return 1; 
}

bool cScrapManager::AddRecording(int recStart, string recPath, int seriesId, int episodeId, int movieId) {
	sRecordingsKey k;
	k.recStart = recStart;
	k.recPath = recPath;
	//check if recording already exists
	map<sRecordingsKey, sEventsValue>::iterator hit = recordings.find(k);
	if (hit != recordings.end()) {
		sEventsValue v = hit->second;
		if ((v.seriesId == seriesId) && (v.episodeId == episodeId) && (v.movieId == movieId))
			return false;
		else
			recordings.erase(hit);
	}
	sEventsValue v;
	v.seriesId = seriesId;
	v.episodeId = episodeId;
	v.movieId = movieId;
	v.isNew = true;
	recordings.insert(pair<sRecordingsKey, sEventsValue>(k, v));	
	return true;
}

bool cScrapManager::RecordingExists(int recStart, string recPath) {
	sRecordingsKey k;
	k.recStart = recStart;
	k.recPath = recPath;
	map<sRecordingsKey, sEventsValue>::iterator hit = recordings.find(k);
	if (hit != recordings.end())
		return true;
	return false;	
}

bool cScrapManager::SeriesInUse(int seriesId) {
	map<int, cTVDBSeries*>::iterator hit = series.find(seriesId);
	if (hit != series.end())
		return true;
	return false;
}

bool cScrapManager::MovieInUse(int movieId) {
	map<int, cMovieDbMovie*>::iterator hit = movies.find(movieId);
	if (hit != movies.end())
		return true;
	return false;
}

void cScrapManager::DumpSeries(void) 
{
	int numSeries = 0;
	map<sEventsKey, sEventsValue>::iterator it;

	for (it = events.begin(); it != events.end(); it++) 
   {
		sEventsKey key = it->first;
		sEventsValue ev = it->second;

		if (ev.seriesId > 0) 
      {
			const cEvent* event = NULL;
			const cChannel* c = NULL;
         const cSchedules* schedules = NULL;
         tChannelID cID = tChannelID::FromString(key.channelId.c_str());

#if defined (APIVERSNUM) && (APIVERSNUM >= 20301)
         LOCK_CHANNELS_READ;
         LOCK_SCHEDULES_READ;

         c = Channels->GetByChannelID(cID);
         schedules = Schedules;
#else
         cSchedulesLock schedulesLock(true);
         schedules = (cSchedules*)cSchedules::Schedules(schedulesLock);
         c = Channels.GetByChannelID(cID);
#endif
         
			if (schedules) 
         {
				const cSchedule *schedule = schedules->GetSchedule(cID);
            
				if (schedule)
					event = schedule->GetEvent(key.eventId);
			}
         
			if (event)
				tell(0, "series (tvdbID %d, episodeId %d), Event (EventID %d): %s, %s: %s (%s)", ev.seriesId, 
                 ev.episodeId, 
                 key.eventId, 
                 *event->GetDateString(), 
                 *event->GetTimeString(), 
                 event->Title(),
                 c?(c->Name()) : "unknown channel");
         else
				tell(0, "series (tvdbID %d, episodeId %d), Event (EventID %d): No VDR Event found", 
                 ev.seriesId, ev.episodeId, key.eventId);

			numSeries++;
		}
	}

	tell(0, "Keeping %d series in memory", numSeries);
}

void cScrapManager::DumpMovies(void) 
{
	int numMovies = 0;
	map<sEventsKey, sEventsValue>::iterator it;

	for (it = events.begin(); it != events.end(); it++) 
   {
		sEventsKey key = it->first;
		sEventsValue ev = it->second;

		if (ev.movieId > 0) 
      {
			const cEvent *event = NULL;
			const cChannel *c = NULL;
         const cSchedules* schedules = NULL;
         tChannelID cID = tChannelID::FromString(key.channelId.c_str());

#if defined (APIVERSNUM) && (APIVERSNUM >= 20301)
         LOCK_CHANNELS_READ;
         LOCK_SCHEDULES_READ;

         c = Channels->GetByChannelID(cID);
         schedules = Schedules;
#else
         cSchedulesLock schedulesLock(true);
         schedules = (cSchedules*)cSchedules::Schedules(schedulesLock);
         c = Channels.GetByChannelID(cID);
#endif
         
			if (schedules) 
         {
				const cSchedule *schedule = schedules->GetSchedule(cID);

				if (schedule)
					event = schedule->GetEvent(key.eventId);
			}

			if (event) 
         {
				tell(0, "movie (moviedbId %d), Event (EventID %d): %s, %s: %s (%s)", ev.movieId, 
																					 key.eventId, 
																					 *event->GetDateString(), 
																					 *event->GetTimeString(), 
																					 event->Title(),
																					 c?(c->Name()):"unknown channel");
			} 
         else 
         {
				tell(0, "movie (moviedbId %d), Event (EventID %d): No VDR Event found", ev.movieId, key.eventId);
			}

			numMovies++;
		}
	}

	tell(0, "Keeping %d movies in memory", numMovies);
}

void cScrapManager::DumpRecordings(void) {
	tell(0, "%ld recordings in memory:", recordings.size());
	for (map<sRecordingsKey, sEventsValue>::iterator it = recordings.begin(); it != recordings.end(); it++) {
		sRecordingsKey key = it->first;
		sEventsValue val = it->second;
		if (val.seriesId > 0) {
			tell(0, "series (tvdbID %d, episodeId %d): %s", val.seriesId, val.episodeId, key.recPath.c_str());
		} else if (val.movieId) {
			tell(0, "movie (moviedbID %d): %s", val.movieId, key.recPath.c_str());
		} else {
			tell(0, "unknown recording: %s", key.recPath.c_str());			
		}
	}
}

bool cScrapManager::GetEventType(ScraperGetEventType *call) {
   tell(4, "scraper2vdr plugin service call GetEventType called");
	sEventsValue v;
   memset(&v, 0, sizeof(sEventsValue));
	if (call->event) {
		sEventsKey k;
		k.eventId = call->event->EventID();
		k.channelId = *(call->event->ChannelID().ToString());
		map<sEventsKey, sEventsValue>::iterator hit = events.find(k);
		if (hit == events.end()) {
         tell(4, "no event found for eventID %d, channelID %s", k.eventId, k.channelId.c_str());
			return false;
		}
      tell(4, "event found for eventID %d, channelID %s", k.eventId, k.channelId.c_str());
		v = hit->second;
	} else if (call->recording) {
		sRecordingsKey k;
		k.recStart = call->recording->Start();
        k.recPath = getRecPath(call->recording);
		map<sRecordingsKey, sEventsValue>::iterator hit = recordings.find(k);
		if (hit == recordings.end()) {
         tell(4, "no recording found for recStart %d, recPath %s", k.recStart, k.recPath.c_str());
			return false;
		}
		v = hit->second;
      tell(4, "recording found for recStart %d, recPath %s", k.recStart, k.recPath.c_str());
	}
	if (v.seriesId > 0) {
		call->type = tSeries;
      tell(4, "series detected, seriesId %d, episodeId %d", v.seriesId, v.episodeId);
	} else if (v.movieId > 0) {
		call->type = tMovie;
      tell(4, "movie detected, movieId %d", v.movieId);
	} else {
      tell(4, "unvalid entry");
		call->type = tNone;
		return false;			
	}
	call->seriesId = v.seriesId;
	call->episodeId = v.episodeId;
	call->movieId = v.movieId;
	return true;
}

bool cScrapManager::GetSeries(cSeries *s) {
    map<int, cTVDBSeries*>::iterator hit = series.find(s->seriesId);
    if (hit == series.end())
        return false;
    cTVDBSeries *sStored = hit->second;
    s->name = sStored->name;
	s->overview = sStored->overview;
    s->firstAired = sStored->firstAired;
    s->network = sStored->network;
    s->genre = sStored->genre;
    s->rating = sStored->rating;
    s->status = sStored->status;
    //Episode
    if (s->episodeId > 0) {
    	sStored->GetEpisode(s->episodeId, &s->episode);
	    sStored->GetSeasonPoster(s->episodeId, &s->seasonPoster);    
    }
    //Media
    sStored->GetPosters(&s->posters);
    sStored->GetBanners(&s->banners);
    sStored->GetFanart(&s->fanarts);
    //Actors
    sStored->GetActors(&s->actors);
    return true;
}

bool cScrapManager::GetMovie(cMovie *m) {
    map<int, cMovieDbMovie*>::iterator hit = movies.find(m->movieId);
    if (hit == movies.end())
        return false;
    cMovieDbMovie *mStored = hit->second;
    m->title = mStored->title;
    m->originalTitle = mStored->originalTitle;
    m->tagline = mStored->tagline;    
    m->overview = mStored->overview;
    m->adult = mStored->adult;
    m->collectionName = mStored->collectionName;
    m->budget = mStored->budget;
    m->revenue = mStored->revenue;
    m->genres = mStored->genres;
    m->homepage = mStored->homepage;
    m->releaseDate = mStored->releaseDate;
    m->runtime = mStored->runtime;
    m->popularity = mStored->popularity;
    m->voteAverage = mStored->voteAverage;
    //Media
	mStored->GetMedia(mmPoster, &m->poster);
	mStored->GetMedia(mmFanart, &m->fanart);
	mStored->GetMedia(mmCollectionPoster, &m->collectionPoster);
	mStored->GetMedia(mmCollectionFanart, &m->collectionFanart);
	//Actors
    mStored->GetActors(&m->actors);
    return true;
}

bool cScrapManager::GetPosterBanner(ScraperGetPosterBanner *call) {
    sEventsValue v;
    v.episodeId = 0;
    if (call->event) {
        sEventsKey k;
        k.eventId = call->event->EventID();
        k.channelId = *(call->event->ChannelID().ToString());
        map<sEventsKey, sEventsValue>::iterator hit = events.find(k);
        if (hit == events.end())
            return false;
        v = hit->second;
    }
	if (v.seriesId > 0) {
		call->type = tSeries;
		map<int, cTVDBSeries*>::iterator hitSeries = series.find(v.seriesId);
		if (hitSeries == series.end())
			return false;
		cTVDBSeries *s = hitSeries->second;
		bool found = s->GetRandomBanner(&call->banner);
		if (v.episodeId > 0) {
			s->GetSeasonPoster(v.episodeId, &call->poster);
		}
		return found;
	} else if (v.movieId > 0) {
		call->type = tMovie;
		map<int, cMovieDbMovie*>::iterator hitMovies = movies.find(v.movieId);
		if (hitMovies == movies.end())
			return false;
		cMovieDbMovie *m = hitMovies->second;
		return m->GetMedia(mmPoster, &call->poster);
	} else {
		call->type = tNone;
	}
	return false;			
}

bool cScrapManager::GetPosterBannerV2(ScraperGetPosterBannerV2 *call) {
    sEventsValue v;
    v.episodeId = 0;
    if (call->event) {
        sEventsKey k;
        k.eventId = call->event->EventID();
        k.channelId = *(call->event->ChannelID().ToString());
        map<sEventsKey, sEventsValue>::iterator hit = events.find(k);
        if (hit == events.end())
            return false;
        v = hit->second;
    } else if (call->recording) {
        sRecordingsKey k;
        k.recStart = call->recording->Start();
        k.recPath = getRecPath(call->recording);
        map<sRecordingsKey, sEventsValue>::iterator hit = recordings.find(k);
        if (hit == recordings.end()) {
            return false;
        }
        v = hit->second;
    }
	if (v.seriesId > 0) {
		call->type = tSeries;
		map<int, cTVDBSeries*>::iterator hitSeries = series.find(v.seriesId);
		if (hitSeries == series.end())
			return false;
		cTVDBSeries *s = hitSeries->second;
		bool found = s->GetRandomBanner(&call->banner);
		if (v.episodeId > 0) {
			s->GetSeasonPoster(v.episodeId, &call->poster);
		}
		return found;
	} else if (v.movieId > 0) {
		call->type = tMovie;
		map<int, cMovieDbMovie*>::iterator hitMovies = movies.find(v.movieId);
		if (hitMovies == movies.end())
			return false;
		cMovieDbMovie *m = hitMovies->second;
		return m->GetMedia(mmPoster, &call->poster);
	} else {
		call->type = tNone;
	}
	return false;			
}

bool cScrapManager::GetPoster(ScraperGetPoster *call) {
    sEventsValue v;
    if (call->event) {
    	sEventsKey k;
        k.eventId = call->event->EventID();
        k.channelId = *(call->event->ChannelID().ToString());
        map<sEventsKey, sEventsValue>::iterator hit = events.find(k);
        if (hit == events.end())
            return false;
        v = hit->second;
    } else if (call->recording) {
		sRecordingsKey k;
		k.recStart = call->recording->Start();
        k.recPath = getRecPath(call->recording);
		map<sRecordingsKey, sEventsValue>::iterator hit = recordings.find(k);
        if (hit == recordings.end())
            return false;
        v = hit->second;
    }
    if (v.seriesId > 0) {
        map<int, cTVDBSeries*>::iterator hitSeries = series.find(v.seriesId);
        if (hitSeries == series.end())
            return false;
        cTVDBSeries *s = hitSeries->second;
        return s->GetPoster(&call->poster);
    } else if (v.movieId > 0) {
        map<int, cMovieDbMovie*>::iterator hitMovies = movies.find(v.movieId);
        if (hitMovies == movies.end())
            return false;
        cMovieDbMovie *m = hitMovies->second;
        return m->GetMedia(mmPoster, &call->poster);
    }

    return true;
}

bool cScrapManager::GetPosterThumb(ScraperGetPosterThumb *call) {
    sEventsValue v;
    if (call->event) {
	    sEventsKey k;
        k.eventId = call->event->EventID();
        k.channelId = *(call->event->ChannelID().ToString());
        map<sEventsKey, sEventsValue>::iterator hit = events.find(k);
        if (hit == events.end())
            return false;
        v = hit->second;
    } else if (call->recording) {
		sRecordingsKey k;
		k.recStart = call->recording->Start();
        k.recPath = getRecPath(call->recording);
		map<sRecordingsKey, sEventsValue>::iterator hit = recordings.find(k);
        if (hit == recordings.end())
            return false;
        v = hit->second;
    }
    if (v.seriesId > 0) {
        map<int, cTVDBSeries*>::iterator hitSeries = series.find(v.seriesId);
        if (hitSeries == series.end())
            return false;
        cTVDBSeries *s = hitSeries->second;
        return s->GetPosterThumb(&call->poster);
    } else if (v.movieId > 0) {
        map<int, cMovieDbMovie*>::iterator hitMovies = movies.find(v.movieId);
        if (hitMovies == movies.end())
            return false;
        cMovieDbMovie *m = hitMovies->second;
        return m->GetMedia(mmPosterThumb, &call->poster);
    }
    return false;
}
