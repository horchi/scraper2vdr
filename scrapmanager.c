#define __STL_v_H
#include <vdr/recording.h>
#include "tools.h"
#include "config.h"
#include "scrapmanager.h"

using namespace std;

extern cScraper2VdrConfig config;

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

cTVDBSeries *cScrapManager::AddSeries(cTableSeries* tSeries) {
	cTVDBSeries *s = new cTVDBSeries();
	s->id = tSeries->getIntValue(cTableSeries::fiSeriesId);
    s->name = tSeries->getStrValue(cTableSeries::fiSeriesName);
    s->overview = tSeries->getStrValue(cTableSeries::fiSeriesOverview);
    s->firstAired = tSeries->getStrValue(cTableSeries::fiSeriesFirstAired);
    s->network = tSeries->getStrValue(cTableSeries::fiSeriesNetwork);
    string genre = replaceString(tSeries->getStrValue(cTableSeries::fiSeriesGenre), "|", ", ");
    s->genre = genre;
    s->rating = tSeries->getFloatValue(cTableSeries::fiSeriesRating);
    s->status = tSeries->getStrValue(cTableSeries::fiSeriesStatus);
    series.insert(pair<int, cTVDBSeries*>(tSeries->getIntValue(cTableSeries::fiSeriesId), s));
    return s;
}

cMovieDbMovie *cScrapManager::AddMovie(cTableMovies* tMovies) {
	cMovieDbMovie *m = new cMovieDbMovie();
	m->id = tMovies->getIntValue(cTableMovies::fiMovieId);
	m->title = tMovies->getStrValue(cTableMovies::fiTitle);
    m->originalTitle = tMovies->getStrValue(cTableMovies::fiOriginalTitle);
    m->tagline = tMovies->getStrValue(cTableMovies::fiTagline);
    m->overview = tMovies->getStrValue(cTableMovies::fiOverview);
    m->adult = tMovies->getIntValue(cTableMovies::fiIsAdult);
    m->collectionName = tMovies->getStrValue(cTableMovies::fiCollectionName);
    m->budget = tMovies->getIntValue(cTableMovies::fiBudget);
    m->revenue = tMovies->getIntValue(cTableMovies::fiRevenue);
    string genre = replaceString(tMovies->getStrValue(cTableMovies::fiGenres), "|", ",");
    m->genres = genre;
    m->homepage = tMovies->getStrValue(cTableMovies::fiHomepage);
    m->releaseDate = tMovies->getStrValue(cTableMovies::fiReleaaseDate);
    m->runtime = tMovies->getIntValue(cTableMovies::fiRuntime);
    m->popularity = tMovies->getFloatValue(cTableMovies::fiPopularity);
    m->voteAverage = tMovies->getFloatValue(cTableMovies::fiVoteAverage);
    movies.insert(pair<int, cMovieDbMovie*>(tMovies->getIntValue(cTableMovies::fiMovieId), m));
    return m;
}

void cScrapManager::AddSeriesEpisode(cTVDBSeries *series, cTableSeriesEpisode* tEpisodes) {
	cTVDBEpisode *e = new cTVDBEpisode();
	e->id = tEpisodes->getIntValue(cTableSeriesEpisode::fiEpisodeId);
	e->name = tEpisodes->getStrValue(cTableSeriesEpisode::fiEpisodeName);
    e->number =  tEpisodes->getIntValue(cTableSeriesEpisode::fiEpisodeNumber);
    e->season = tEpisodes->getIntValue(cTableSeriesEpisode::fiSeasonNumber);
    e->overview = tEpisodes->getStrValue(cTableSeriesEpisode::fiEpisodeOverview);
    e->firstAired = tEpisodes->getStrValue(cTableSeriesEpisode::fiEpisodeFirstAired);
    string guestStars = replaceString(tEpisodes->getStrValue(cTableSeriesEpisode::fiEpisodeGuestStars), "|", ", ");
    e->guestStars = guestStars;
    e->rating = tEpisodes->getFloatValue(cTableSeriesEpisode::fiEpisodeRating);
    series->InsertEpisode(e);
}

void cScrapManager::AddSeriesActor(cTVDBSeries *series, cTableSeriesActor* tActors) {
	cTVDBActor *a = new cTVDBActor();
	a->id = tActors->getIntValue(cTableSeriesActor::fiActorId);
	a->name = tActors->getStrValue(cTableSeriesActor::fiActorName);
    a->role =  tActors->getStrValue(cTableSeriesActor::fiActorRole);
    series->InsertActor(a);
}

void cScrapManager::AddMovieMedia(cMovieDbMovie *movie, cTableMovieMedia* tMovieMedia, string path) {
	cMovieMedia *m = new cMovieMedia();
	m->mediaType = tMovieMedia->getIntValue(cTableMovieMedia::fiMediaType);
	m->width = tMovieMedia->getIntValue(cTableMovieMedia::fiMediaWidth);
	m->height = tMovieMedia->getIntValue(cTableMovieMedia::fiMediaHeight);
    m->path = path;
    movie->InsertMedia(m);	
}


void cScrapManager::AddMovieActor(cMovieDbMovie *movie, cTableMovieActor* tActor, string role) {
	cMovieActor *a = new cMovieActor();
	a->id = tActor->getIntValue(cTableMovieActor::fiActorId);
	a->name = tActor->getStrValue(cTableMovieActor::fiActorName);
    a->role =  role;
    movie->InsertActor(a);
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

void cScrapManager::DumpSeries(void) {
	int numSeries = 0;
	map<sEventsKey, sEventsValue>::iterator it;
	for (it = events.begin(); it != events.end(); it++) {
		sEventsKey key = it->first;
		sEventsValue ev = it->second;
		if (ev.seriesId > 0) {
			const cEvent *event = NULL;
			const cChannel *c = NULL;
			cSchedulesLock lock;
			cSchedules *s = (cSchedules *)cSchedules::Schedules(lock);
			if (s) {
				tChannelID cID = tChannelID::FromString(key.channelId.c_str());
				c = Channels.GetByChannelID(cID);
				const cSchedule *schedule = s->GetSchedule(cID);
				if (schedule) {
					event = schedule->GetEvent(key.eventId);
				}
			}
			if (event) {
				tell(0, "series (tvdbID %d, episodeId %d), Event (EventID %d): %s, %s: %s (%s)", ev.seriesId, 
																							     ev.episodeId, 
																							     key.eventId, 
																							     *event->GetDateString(), 
																							     *event->GetTimeString(), 
																							     event->Title(),
																							     c?(c->Name()):"unknown channel");
			} else {
				tell(0, "series (tvdbID %d, episodeId %d), Event (EventID %d): No VDR Event found", ev.seriesId, ev.episodeId, key.eventId);
			}
			numSeries++;
		}
	}
	tell(0, "Keeping %d series in memory", numSeries);
}

void cScrapManager::DumpMovies(void) {
	int numMovies = 0;
	map<sEventsKey, sEventsValue>::iterator it;
	for (it = events.begin(); it != events.end(); it++) {
		sEventsKey key = it->first;
		sEventsValue ev = it->second;
		if (ev.movieId > 0) {
			const cEvent *event = NULL;
			const cChannel *c = NULL;
			cSchedulesLock lock;
			cSchedules *s = (cSchedules *)cSchedules::Schedules(lock);
			if (s) {
				tChannelID cID = tChannelID::FromString(key.channelId.c_str());
				c = Channels.GetByChannelID(cID);
				const cSchedule *schedule = s->GetSchedule(cID);
				if (schedule) {
					event = schedule->GetEvent(key.eventId);
				}
			}
			if (event) {
				tell(0, "movie (moviedbId %d), Event (EventID %d): %s, %s: %s (%s)", ev.movieId, 
																					 key.eventId, 
																					 *event->GetDateString(), 
																					 *event->GetTimeString(), 
																					 event->Title(),
																					 c?(c->Name()):"unknown channel");
			} else {
				tell(0, "movie (moviedbId %d), Event (EventID %d): No VDR Event found", ev.movieId, key.eventId);
			}
			numMovies++;
		}
	}
	tell(0, "Keeping %d movies in memory", numMovies);
}

void cScrapManager::DumpRecordings(void) {
	tell(0, "%d recordings in memory:", (int)recordings.size());
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
	if (config.debug) {
		tell(0, "scraper2vdr plugin service call GetEventType called");
	}
	sEventsValue v;
   memset(&v, 0, sizeof(sEventsValue));
	if (call->event) {
		sEventsKey k;
		k.eventId = call->event->EventID();
		k.channelId = *(call->event->ChannelID().ToString());
		map<sEventsKey, sEventsValue>::iterator hit = events.find(k);
		if (hit == events.end()) {
			if (config.debug) {
				tell(0, "no event found for eventID %d, channelID %s", k.eventId, k.channelId.c_str());
			}
			return false;
		}
		if (config.debug)
			tell(0, "event found for eventID %d, channelID %s", k.eventId, k.channelId.c_str());
		v = hit->second;
	} else if (call->recording) {
		sRecordingsKey k;
		k.recStart = call->recording->Start();
        k.recPath = getRecPath(call->recording);
		map<sRecordingsKey, sEventsValue>::iterator hit = recordings.find(k);
		if (hit == recordings.end()) {
			if (config.debug) {
				tell(0, "no recording found for recStart %d, recPath %s", k.recStart, k.recPath.c_str());
			}
			return false;
		}
		v = hit->second;
		if (config.debug) {
			tell(0, "recording found for recStart %d, recPath %s", k.recStart, k.recPath.c_str());
		}
	}
	if (v.seriesId > 0) {
		call->type = tSeries;
		if (config.debug)
			tell(0, "series detected, seriesId %d, episodeId %d", v.seriesId, v.episodeId);
	} else if (v.movieId > 0) {
		call->type = tMovie;
		if (config.debug)
			tell(0, "movie detected, movieId %d", v.movieId);
	} else {
		if (config.debug)
			tell(0, "unvalid entry");
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
	sEventsKey k;
	k.eventId = call->event->EventID();
	k.channelId = *(call->event->ChannelID().ToString());
	map<sEventsKey, sEventsValue>::iterator hit = events.find(k);
	if (hit == events.end())
		return false;
	sEventsValue v = hit->second;
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
