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

void cTVDBSeries::InsertEpisodeImage(int episodeId, int width, int height, string path) {
    map<int, cTVDBEpisode*>::iterator hit = episodes.find(episodeId);
    if (hit != episodes.end()) {
        cTVDBEpisode *e = hit->second;
        cTVDBMedia *m = new cTVDBMedia();
        m->width = width;
        m->height = height;
        m->path = path;
        m->mediaType = msEpisodePic;
        e->episodeImage = m;
    }    
}

void cTVDBSeries::InsertActor(cTVDBActor *actor) {
    actors.insert(pair<int, cTVDBActor*>(actor->id, actor));    
}

void cTVDBSeries::InsertActorThumb(int actorId, int imgWidth, int imgHeight, string path) {
    map<int, cTVDBActor*>::iterator hit = actors.find(actorId);
    if (hit != actors.end()) {
        cTVDBActor *a = hit->second;
        cTVDBMedia *m = new cTVDBMedia();
        m->width = imgWidth;
        m->height = imgHeight;
        m->path = path;
        m->mediaType = msActorThumb;
        a->actorThumb = m;
    }
}

void cTVDBSeries::InsertMedia(int mediaType, int imgWidth, int imgHeight, string path, int season) {
    cTVDBMedia *media = new cTVDBMedia();
    media->width = imgWidth;
    media->height = imgHeight;
    media->path = path;
    media->mediaType = mediaType;
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
        e->episodeImage.path = eStored->episodeImage->path;
        e->episodeImage.width = eStored->episodeImage->width;
        e->episodeImage.height = eStored->episodeImage->height;
    }
}

void cTVDBSeries::GetPosters(vector<cTvMedia> *p) {
    for (vector<cTVDBMedia*>::iterator it = posters.begin(); it != posters.end(); it++) {
        cTVDBMedia *mStored = *it;
        cTvMedia m;
        m.path = mStored->path;        
        m.width = mStored->width;
        m.height = mStored->height;
        p->push_back(m);
    }
}

bool cTVDBSeries::GetPoster(cTvMedia *p) {
    if (posters.size() > 0) {
        p->path = posters[0]->path;
        p->width = posters[0]->width;
        p->height = posters[0]->height;
        return true;
    }
    return false;
}

bool cTVDBSeries::GetPosterThumb(cTvMedia *p) {
    if (posterThumb) {
        p->path = posterThumb->path;
        p->width = posterThumb->width;
        p->height = posterThumb->height;
        return true;
    }
    return false;
}

void cTVDBSeries::GetBanners(vector<cTvMedia> *b) {
    for (vector<cTVDBMedia*>::iterator it = banners.begin(); it != banners.end(); it++) {
        cTVDBMedia *bStored = *it;
        cTvMedia m;
        m.path = bStored->path;        
        m.width = bStored->width;
        m.height = bStored->height;
        b->push_back(m);
    }
}

bool cTVDBSeries::GetRandomBanner(cTvMedia *b) {
    int numBanners = banners.size();
    if (numBanners == 0)
        return false;
    srand((unsigned)time(NULL));
    int banner = rand()%numBanners;
    cTVDBMedia *bStored = banners[banner];
    b->path = bStored->path;
    b->width = bStored->width;
    b->height = bStored->height;
    return true;
}

void cTVDBSeries::GetFanart(vector<cTvMedia> *f) {
    for (vector<cTVDBMedia*>::iterator it = fanart.begin(); it != fanart.end(); it++) {
        cTVDBMedia *fStored = *it;
        cTvMedia m;
        m.path = fStored->path;        
        m.width = fStored->width;
        m.height = fStored->height;
        f->push_back(m);
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
    sp->width = spStored->width;
    sp->height = spStored->height;
    sp->path = spStored->path;
}

void cTVDBSeries::GetActors(vector<cActor> *a) {
    for (map<int, cTVDBActor*>::iterator it = actors.begin(); it != actors.end(); it++) {
        cTVDBActor *aStored = it->second;
        cActor act;
        act.name = aStored->name;
        act.role = aStored->role;
        if (aStored->actorThumb) {
            act.actorThumb.width = aStored->actorThumb->width;
            act.actorThumb.height = aStored->actorThumb->height;
            act.actorThumb.path = aStored->actorThumb->path;
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
        tell(0, "width %d, height %d", m->width, m->height);
    }
    for (vector<cTVDBMedia*>::iterator it = banners.begin(); it != banners.end(); it++) {
        cTVDBMedia *m = *it;
        tell(0, "Banner %d, Path: %s", m->mediaType, m->path.c_str());
        tell(0, "width %d, height %d", m->width, m->height);
    }
    for (vector<cTVDBMedia*>::iterator it = fanart.begin(); it != fanart.end(); it++) {
        cTVDBMedia *m = *it;
        tell(0, "Fanart %d, Path: %s", m->mediaType, m->path.c_str());
        tell(0, "width %d, height %d", m->width, m->height);
    }
    tell(0, "--------------------------- Episodes ----------------------------------");
    for (map<int, cTVDBEpisode*>::iterator it = episodes.begin(); it != episodes.end(); it++) {
        cTVDBEpisode *e = it->second;
        tell(0, "Episode %d, Name: %s", e->id, e->name.c_str());
        if (e->episodeImage) {
            tell(0, "Episode Image: %d x %d, Path: %s", e->episodeImage->width, e->episodeImage->height, e->episodeImage->path.c_str());
        }
    }
    tell(0, "--------------------------- Season Posters ----------------------------------");
    for (map<int, cTVDBMedia*>::iterator it = seasonPosters.begin(); it != seasonPosters.end(); it++) {
        int season = it->first;
        cTVDBMedia *m = it->second;
        tell(0, "Season %d, %d x %d, Path: %s", season, m->width, m->height, m->path.c_str());
    }
    tell(0, "--------------------------- Actors ----------------------------------");
    for (map<int, cTVDBActor*>::iterator it = actors.begin(); it != actors.end(); it++) {
        cTVDBActor *a = it->second;
        tell(0, "Actor %d, Name: %s, Role %s", a->id, a->name.c_str(), a->role.c_str());
        if (a->actorThumb) {
            tell(0, "Thumb: %d x %d, Path: %s", a->actorThumb->width, a->actorThumb->height, a->actorThumb->path.c_str());
        }
    }
    if (posterThumb) {
        tell(0, "posterThumb path %s, width %d, height %d", posterThumb->path.c_str(), posterThumb->width, posterThumb->height);
    }
}
