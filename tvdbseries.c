#define __STL_CONFIG_H
#include "lib/common.h"
#include "tvdbseries.h"

using namespace std;

cTVDBSeries::cTVDBSeries(void) {
    id = 0;
    name = "";
    overview = "";
    firstAired = "";
    network = "";
    genre = "";
    rating = 0.0;
    status = "";
    posterThumb = NULL;
    lastupdate = 0;
    lastepisodeupdate=0;
    lastscraped=0;
    updatecontent = true; // force update for new series
    updateimages = false; // no default update of images for new series
}

cTVDBSeries::~cTVDBSeries() {
    for (map<int, cTVDBActor*>::iterator it = actors.begin(); it != actors.end(); it++) {
        cTVDBActor *a = (cTVDBActor*)it->second;
        delete a;
    }
    for (map<int, cTVDBEpisode*>::iterator it = episodes.begin(); it != episodes.end(); it++) {
        cTVDBEpisode *e = (cTVDBEpisode*)it->second;
        delete e;
    }
    for (vector<cTVDBMedia*>::iterator it = posters.begin(); it != posters.end(); it++) {
        cTVDBMedia *p = *it;
        delete p;
    }
    for (vector<cTVDBMedia*>::iterator it = banners.begin(); it != banners.end(); it++) {
        cTVDBMedia *b = *it;
        delete b;
    }
    for (vector<cTVDBMedia*>::iterator it = fanart.begin(); it != fanart.end(); it++) {
        cTVDBMedia *f = *it;
        delete f;
    }
    for (map<int, cTVDBMedia*>::iterator it = seasonPosters.begin(); it != seasonPosters.end(); it++) {
        cTVDBMedia *s = (cTVDBMedia*)it->second;
        delete s;
    }
    for (map<int, cTVDBMedia*>::iterator it = seasonPosterThumbs.begin(); it != seasonPosterThumbs.end(); it++) {
        cTVDBMedia *s = (cTVDBMedia*)it->second;
        delete s;
    }
    if (posterThumb)
        delete posterThumb;
}

void cTVDBSeries::InsertEpisode(cTVDBEpisode *episode) {
    map<int, cTVDBEpisode*>::iterator hit = episodes.find(episode->id);
    if (hit != episodes.end())
        delete episode;
    else
        episodes.insert(pair<int, cTVDBEpisode*>(episode->id, episode));
}

cTVDBEpisode *cTVDBSeries::GetEpisode(int episodeId) {
    map<int, cTVDBEpisode*>::iterator hit = episodes.find(episodeId);
    if (hit == episodes.end())
        return NULL;
    return hit->second;
}  

void cTVDBSeries::InsertEpisodeImage(int episodeId, int width, int height, string path) {
    map<int, cTVDBEpisode*>::iterator hit = episodes.find(episodeId);
    if (hit != episodes.end()) {
        cTVDBEpisode *e = hit->second;
        cTVDBMedia *m = new cTVDBMedia();
        m->width = width;
        m->height = height;
        m->path = path;
        m->mediaType = msEpisodePic;
        m->mediavalid = true; // force true for old functions
        e->episodeImage = m;
    }    
}

// insert/update episode image
void cTVDBSeries::SetEpisodeImage(int episodeId, int imgWidth, int imgHeight, string path, bool needrefresh) {
    map<int, cTVDBEpisode*>::iterator hit = episodes.find(episodeId);
    if (hit != episodes.end()) {
        cTVDBEpisode *e = hit->second;
        cTVDBMedia *m;
        if (e->episodeImage) {
            m = e->episodeImage;
        } else {
            m = new cTVDBMedia();
            e->episodeImage = m;
        }    
        m->width = imgWidth;
        m->height = imgHeight;
        m->path = path;
        m->mediaType = msEpisodePic;
        m->needrefresh = needrefresh;
        m->mediavalid = m->mediavalid || !needrefresh; // all media which do not need a refresh should have a image file (if was valid, is stay valid)
    }
}

// get first episode (also init episode iterator)
int cTVDBSeries::GetEpisodeFirst(cTVDBEpisode* &episode) {
    episode = NULL;  
    episodesIterator = episodes.begin();
    if (episodesIterator == episodes.end())
        return 0; // no episodes availabe
    episode = episodesIterator->second; // return current episode
    return 1; 
}

// get next episode from iterator
int cTVDBSeries::GetEpisodeNext(cTVDBEpisode* &episode) {
    episode = NULL;
    if (episodesIterator != episodes.end())
        episodesIterator++;
    if (episodesIterator == episodes.end())
        return 0; // no next availabe
    episode = episodesIterator->second; // return current episode
    return 1; 
}

void cTVDBSeries::InsertActor(cTVDBActor *actor) {
    actors.insert(pair<int, cTVDBActor*>(actor->id, actor));    
}

cTVDBActor *cTVDBSeries::GetActor(int actorId) {
    map<int, cTVDBActor*>::iterator hit = actors.find(actorId);
    if (hit == actors.end())
        return NULL;
    return hit->second;
}

void cTVDBSeries::SetActorThumb(int actorId, int imgWidth, int imgHeight, string path) {
    map<int, cTVDBActor*>::iterator hit = actors.find(actorId);
    if (hit != actors.end()) {
        cTVDBActor *a = hit->second;
        cTVDBMedia *m = new cTVDBMedia();
        m->width = imgWidth;
        m->height = imgHeight;
        m->path = path;
        m->mediaType = msActorThumb;
        m->needrefresh = false;
        m->mediavalid = true; // force true for old functions
        a->actorThumb = m;
    }    
}

// insert/update actor thumb
void cTVDBSeries::SetActorThumb(int actorId, int imgWidth, int imgHeight, string path, bool needrefresh) {
    map<int, cTVDBActor*>::iterator hit = actors.find(actorId);
    if (hit != actors.end()) {
        cTVDBActor *a = hit->second;
        cTVDBMedia *m;
        if (a->actorThumb) {
            m = a->actorThumb;
        } else {
            m = new cTVDBMedia();
            a->actorThumb = m;
        }    
        m->width = imgWidth;
        m->height = imgHeight;
        m->path = path;
        m->mediaType = msActorThumb;
        m->needrefresh = needrefresh;
        m->mediavalid = m->mediavalid || !needrefresh; // all media which do not need a refresh should have a image file (if was valid, is stay valid)
    }
}

// get first actor (also init actor iterator)
int cTVDBSeries::GetActorFirst(cTVDBActor* &actor) {
    actor = NULL;  
    actorsIterator = actors.begin();
    if (actorsIterator == actors.end())
        return 0; // no actor availabe
    actor = actorsIterator->second; // return current actor
    return 1; 
}

// get next actor from iterator
int cTVDBSeries::GetActorNext(cTVDBActor* &actor){
    actor = NULL;
    if (actorsIterator != actors.end())
        actorsIterator++;
    if (actorsIterator == actors.end())
        return 0; // no next availabe
    actor = actorsIterator->second; // return current actor
    return 1; 
}

void cTVDBSeries::InsertMedia(int mediaType, int imgWidth, int imgHeight, string path, int season) {
    InsertMedia(mediaType, imgWidth, imgHeight, path, season, false);
}

// insert media object
void cTVDBSeries::InsertMedia(int mediaType, int imgWidth, int imgHeight, string path, int season, bool needrefresh) {
    cTVDBMedia *media = new cTVDBMedia();
    media->width = imgWidth;
    media->height = imgHeight;
    media->path = path;
    media->mediaType = mediaType;
    media->needrefresh = needrefresh;
    media->mediavalid = !needrefresh; // all media which do not need a refresh should have a image file
    switch (mediaType) {
        case msPoster1:
        case msPoster2:
        case msPoster3:
            posters.push_back(media);
            break;
        case msFanart1:
        case msFanart2:
        case msFanart3:
            fanart.push_back(media);
            break;
        case msBanner1:
        case msBanner2:
        case msBanner3:
            banners.push_back(media);
            break;
        case msSeasonPoster:
            seasonPosters.insert(pair<int, cTVDBMedia*>(season, media));
            break;
        case msPosterThumb:
            posterThumb = media;
            break;
        case msSeasonPosterThumb:
            seasonPosterThumbs.insert(pair<int, cTVDBMedia*>(season, media));
            break;
        default:
        break;
    }   
}

// try to find media of given type
cTVDBMedia *cTVDBSeries::GetMedia(int mediaType, int season) {
    switch (mediaType) {
        case msPoster1:
        case msPoster2:
        case msPoster3:
            for (vector<cTVDBMedia*>::iterator it = posters.begin(); it != posters.end(); it++) {
                cTVDBMedia *mStored = *it;
                if (mStored->mediaType == mediaType)
                    return mStored;
            }    
            break;
        case msFanart1:
        case msFanart2:
        case msFanart3:
            for (vector<cTVDBMedia*>::iterator it = fanart.begin(); it != fanart.end(); it++) {
                cTVDBMedia *mStored = *it;
                if (mStored->mediaType == mediaType)
                    return mStored;
            }    
            break;
        case msBanner1:
        case msBanner2:
        case msBanner3:
            for (vector<cTVDBMedia*>::iterator it = banners.begin(); it != banners.end(); it++) {
                cTVDBMedia *mStored = *it;
                if (mStored->mediaType == mediaType)
                    return mStored;
            }    
            break;
        case msSeasonPoster:
            {
            map<int, cTVDBMedia*>::iterator hit = seasonPosters.find(season);
            if (hit != seasonPosters.end())
                return hit->second;
            }
            break;
        case msPosterThumb:
            return posterThumb;
            break;
        case msSeasonPosterThumb:
            {
            map<int, cTVDBMedia*>::iterator hit = seasonPosterThumbs.find(season);
            if (hit != seasonPosterThumbs.end())
                return hit->second;
            }
            break;
        default:
        break;
    }   
    return NULL; // not found
}

// get first season poster (also init season poster iterator)
int cTVDBSeries::GetSeasonPosterFirst(int &season, cTVDBMedia* &media) {
    media = NULL;
    season = 0;
    seasonPostersIterator = seasonPosters.begin();
    if (seasonPostersIterator == seasonPosters.end())
        return 0; // no season poster availabe
    media = seasonPostersIterator->second; // return current season poster
    season = seasonPostersIterator->first; // return current season number 
    return 1; 
}

// get season poster from iterator
int cTVDBSeries::GetSeasonPosterNext(int &season, cTVDBMedia* &media) {
    media=NULL;
    season = 0;
    if (seasonPostersIterator != seasonPosters.end())
        seasonPostersIterator++;
    if (seasonPostersIterator == seasonPosters.end())
        return 0; // no next availabe
    media = seasonPostersIterator->second; // return current season poster
    season = seasonPostersIterator->first; // return current season number 
    return 1; 
}

// delete season poster and thumb
void cTVDBSeries::DeleteSeasonPoster(int season) {
    map<int, cTVDBMedia*>::iterator hit = seasonPosters.find(season);
    if (hit != seasonPosters.end()) {
        cTVDBMedia *s = (cTVDBMedia*)hit->second;
        delete s;
        seasonPosters.erase(season);
    }
    hit = seasonPosterThumbs.find(season);
    if (hit != seasonPosterThumbs.end()) {
        cTVDBMedia *s = (cTVDBMedia*)hit->second;
        delete s;
        seasonPosterThumbs.erase(season);
    }
}

void cTVDBSeries::GetEpisode(int episodeId, cEpisode *e) {
    map<int, cTVDBEpisode*>::iterator hit = episodes.find(episodeId);
    if (hit == episodes.end())
        return;
    cTVDBEpisode *eStored = hit->second;
    e->number = eStored->number;
    e->season = eStored->season;
    e->name = eStored->name;
    e->firstAired = eStored->firstAired;
    e->guestStars = eStored->guestStars;
    e->overview = eStored->overview;
    e->rating = eStored->rating;
    if (eStored->episodeImage) {
        if (eStored->episodeImage->mediavalid) {
            // only use valid images
            e->episodeImage.path = eStored->episodeImage->path;
            e->episodeImage.width = eStored->episodeImage->width;
            e->episodeImage.height = eStored->episodeImage->height;
        }    
    }
}

void cTVDBSeries::GetPosters(vector<cTvMedia> *p) {
    for (vector<cTVDBMedia*>::iterator it = posters.begin(); it != posters.end(); it++) {
        cTVDBMedia *mStored = *it;
        if (mStored->mediavalid) {
            // only use valid images
            cTvMedia m;
            m.path = mStored->path;        
            m.width = mStored->width;
            m.height = mStored->height;
            p->push_back(m);
        }    
    }
}

bool cTVDBSeries::GetPoster(cTvMedia *p) {
    if (posters.size() > 0) {
        if (posters[0]->mediavalid) {
            // only use valid images
            p->path = posters[0]->path;
            p->width = posters[0]->width;
            p->height = posters[0]->height;
            return true;
        }    
    }
    return false;
}

bool cTVDBSeries::GetPosterThumb(cTvMedia *p) {
    if (posterThumb) {
        if (posterThumb->mediavalid) {
            // only use valid images
            p->path = posterThumb->path;
            p->width = posterThumb->width;
            p->height = posterThumb->height;
            return true;
        }    
    }
    return false;
}

void cTVDBSeries::GetBanners(vector<cTvMedia> *b) {
    for (vector<cTVDBMedia*>::iterator it = banners.begin(); it != banners.end(); it++) {
        cTVDBMedia *bStored = *it;
        if (bStored->mediavalid) {
            // only use valid images
            cTvMedia m;
            m.path = bStored->path;        
            m.width = bStored->width;
            m.height = bStored->height;
            b->push_back(m);
        }    
    }
}

bool cTVDBSeries::GetRandomBanner(cTvMedia *b) {
    int numBanners = banners.size();
    if (numBanners == 0)
        return false;
    srand((unsigned)time(NULL));
    int banner = rand()%numBanners;
    cTVDBMedia *bStored = banners[banner];
    if (bStored->mediavalid) {
        // only use valid images
        b->path = bStored->path;
        b->width = bStored->width;
        b->height = bStored->height;
        return true;
    }
    return false;    
}

void cTVDBSeries::GetFanart(vector<cTvMedia> *f) {
    for (vector<cTVDBMedia*>::iterator it = fanart.begin(); it != fanart.end(); it++) {
        cTVDBMedia *fStored = *it;
        if (fStored->mediavalid) {
            // only use valid images
            cTvMedia m;
            m.path = fStored->path;        
            m.width = fStored->width;
            m.height = fStored->height;
            f->push_back(m);
        }    
    }
}

void cTVDBSeries::GetSeasonPoster(int episodeId, cTvMedia *sp) {
    map<int, cTVDBEpisode*>::iterator hit = episodes.find(episodeId);
    if (hit == episodes.end())
        return;
    cTVDBEpisode *e = hit->second;
    map<int, cTVDBMedia*>::iterator hit2 = seasonPosters.find(e->season);
    if (hit2 == seasonPosters.end())
        return;
    cTVDBMedia *spStored = hit2->second;
    if (spStored->mediavalid) {
        // only use valid images
        sp->width = spStored->width;
        sp->height = spStored->height;
        sp->path = spStored->path;
    }    
}

void cTVDBSeries::GetActors(vector<cActor> *a) {
    for (map<int, cTVDBActor*>::iterator it = actors.begin(); it != actors.end(); it++) {
        cTVDBActor *aStored = it->second;
        cActor act;
        act.name = aStored->name;
        act.role = aStored->role;
        if (aStored->actorThumb) {
            if (aStored->actorThumb->mediavalid) {
                // only use valid images
                act.actorThumb.width = aStored->actorThumb->width;
                act.actorThumb.height = aStored->actorThumb->height;
                act.actorThumb.path = aStored->actorThumb->path;
                act.actorThumb.path = aStored->actorThumb->path;
            }    
        }
        a->push_back(act);
    }
}

void cTVDBSeries::Dump(void) {
    tell(0, "--------------------------- Series Info ----------------------------------");
    tell(0, "series %s, ID: %d", name.c_str(), id);
    tell(0, "Overview: %s", overview.c_str());
    tell(0, "FirstAired: %s", firstAired.c_str());
    tell(0, "Network: %s", network.c_str());
    tell(0, "Status: %s", status.c_str());
    tell(0, "Genre: %s", genre.c_str());
    tell(0, "Rating: %f", rating);
    tell(0, "--------------------------- Media ----------------------------------");
    for (vector<cTVDBMedia*>::iterator it = posters.begin(); it != posters.end(); it++) {
        cTVDBMedia *m = *it;
        tell(0, "Poster %d, Path: %s", m->mediaType, m->path.c_str());
        tell(0, "width %d, height %d, needrefresh %d, valid %d", m->width, m->height, m->needrefresh, m->mediavalid);
    }
    for (vector<cTVDBMedia*>::iterator it = banners.begin(); it != banners.end(); it++) {
        cTVDBMedia *m = *it;
        tell(0, "Banner %d, Path: %s", m->mediaType, m->path.c_str());
        tell(0, "width %d, height %d, needrefresh %d, valid %d", m->width, m->height, m->needrefresh, m->mediavalid);
    }
    for (vector<cTVDBMedia*>::iterator it = fanart.begin(); it != fanart.end(); it++) {
        cTVDBMedia *m = *it;
        tell(0, "Fanart %d, Path: %s", m->mediaType, m->path.c_str());
        tell(0, "width %d, height %d, needrefresh %d, valid %d", m->width, m->height, m->needrefresh, m->mediavalid);
    }
    tell(0, "--------------------------- Episodes ----------------------------------");
    for (map<int, cTVDBEpisode*>::iterator it = episodes.begin(); it != episodes.end(); it++) {
        cTVDBEpisode *e = it->second;
        tell(0, "Episode %d, Name: %s", e->id, e->name.c_str());
        if (e->episodeImage) {
            tell(0, "Episode Image: %d x %d, Path: %s, needrefresh %d, valid %d", e->episodeImage->width, e->episodeImage->height, e->episodeImage->path.c_str(), e->episodeImage->needrefresh, e->episodeImage->mediavalid);
        }
    }
    tell(0, "--------------------------- Season Posters ----------------------------------");
    for (map<int, cTVDBMedia*>::iterator it = seasonPosters.begin(); it != seasonPosters.end(); it++) {
        int season = it->first;
        cTVDBMedia *m = it->second;
        tell(0, "Season %d, %d x %d, Path: %s, needrefresh %d, valid %d", season, m->width, m->height, m->path.c_str(), m->needrefresh, m->mediavalid);
    }
    tell(0, "--------------------------- Actors ----------------------------------");
    for (map<int, cTVDBActor*>::iterator it = actors.begin(); it != actors.end(); it++) {
        cTVDBActor *a = it->second;
        tell(0, "Actor %d, Name: %s, Role %s", a->id, a->name.c_str(), a->role.c_str());
        if (a->actorThumb) {
            tell(0, "Thumb: %d x %d, Path: %s, needrefresh %d, valid %d", a->actorThumb->width, a->actorThumb->height, a->actorThumb->path.c_str(), a->actorThumb->needrefresh, a->actorThumb->mediavalid);
        }
    }
    if (posterThumb) {
        tell(0, "posterThumb path %s, width %d, height %d, needrefresh %d, valid %d", posterThumb->path.c_str(), posterThumb->width, posterThumb->height, posterThumb->needrefresh, posterThumb->mediavalid);
    }
}