#define __STL_CONFIG_H
#include "lib/common.h"
#include "moviedbmovie.h"

using namespace std;

cMovieDbMovie::cMovieDbMovie(void) {
    title = "";
    originalTitle = "";
    tagline = "";
    overview = "";
    adult = false;
    collectionID = 0;
    collectionName = "";
    budget = 0;
    revenue = 0;
    genres = "";
    homepage = "";
    imdbid = "";
    releaseDate = "";
    runtime = 0;
    popularity = 0.0;
    voteAverage = 0.0;
}

cMovieDbMovie::~cMovieDbMovie() {
    for (map<int, cMovieActor*>::iterator it = actors.begin(); it != actors.end(); it++) {
        cMovieActor *a = (cMovieActor*)it->second;
        delete a;
    }
    for (map<int, cMovieMedia*>::iterator it = medias.begin(); it != medias.end(); it++) {
        cMovieMedia *m = (cMovieMedia*)it->second;
        delete m;
    }
}

void cMovieDbMovie::InsertMedia(cMovieMedia *media) {
    medias.insert(pair<int, cMovieMedia*>(media->mediaType, media));
}

void cMovieDbMovie::InsertActor(cMovieActor *actor) {
    cMovieMedia *m = new cMovieMedia();
    actor->actorThumb = m;
    actors.insert(pair<int, cMovieActor*>(actor->id, actor));
}

vector<int> cMovieDbMovie::GetActorIDs(void) {
    vector<int> IDs;
    for (map<int, cMovieActor*>::iterator it = actors.begin(); it != actors.end(); it++) {
        cMovieActor *a = it->second;
        IDs.push_back(a->id);
    }
    return IDs;
}

void cMovieDbMovie::SetActorThumbSize(int actorId, int imgWidth, int imgHeight) {
    map<int, cMovieActor*>::iterator hit = actors.find(actorId);
    if (hit != actors.end()) {
        cMovieActor *a = hit->second;
        if (!a->actorThumb)
            return;
        cMovieMedia *thumb = a->actorThumb;
        thumb->width = imgWidth;
        thumb->height = imgHeight;
        thumb->mediaType = mmActorThumb;
    }
}

void cMovieDbMovie::SetActorPath(int actorId, string path) {
    map<int, cMovieActor*>::iterator hit = actors.find(actorId);
    if (hit != actors.end()) {
        cMovieActor *a = hit->second;
        if (!a->actorThumb)
            return;
        a->actorThumb->path = path;
    }
}

bool cMovieDbMovie::GetMedia(mediaMovies mediatype, cTvMedia *p) {
    map<int, cMovieMedia*>::iterator hit = medias.find(mediatype);
    if (hit == medias.end())
        return false;
    cMovieMedia *pStored = hit->second;
    p->path = pStored->path;
    p->width = pStored->width;
    p->height = pStored->height;
    return true;
}

void cMovieDbMovie::GetActors(vector<cActor> *a) {
    for (map<int, cMovieActor*>::iterator it = actors.begin(); it != actors.end(); it++) {
        cMovieActor *aStored = it->second;
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

void cMovieDbMovie::Dump(void) {
    tell(0, "--------------------------- Movie Info ----------------------------------");
    tell(0, "title %s, ID: %d", title.c_str(), id);
    tell(0, "Orig. Title: %s", originalTitle.c_str());
    tell(0, "Tagline: %s", tagline.c_str());
    tell(0, "Overview: %s", overview.c_str());
    tell(0, "Collection: %s", collectionName.c_str());
    tell(0, "Genre: %s", genres.c_str());
    tell(0, "Popularity: %f", popularity);
    tell(0, "--------------------------- Actors ----------------------------------");
    for (map<int, cMovieActor*>::iterator it = actors.begin(); it != actors.end(); it++) {
        cMovieActor *a = it->second;
        tell(0, "Actor %d, Name: %s, Role %s", a->id, a->name.c_str(), a->role.c_str());
        if (a->actorThumb) {
            tell(0, "thmbWidth %d, thmbHeight %d", a->actorThumb->width, a->actorThumb->height);
            tell(0, "Path %s", a->actorThumb->path.c_str());
        }
    }
    tell(0, "--------------------------- Media ----------------------------------");
    for (map<int, cMovieMedia*>::iterator it = medias.begin(); it != medias.end(); it++) {
        cMovieMedia *m = it->second;
        tell(0, "Media %d", m->mediaType);
        tell(0, "width %d, height %d", m->width, m->height);
        tell(0, "Path %s", m->path.c_str());
    }
}