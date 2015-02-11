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
    lastscraped=0;
    updateimages = false; // no default update of images for new movies
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

// insert media object (create media object also)
void cMovieDbMovie::InsertMedia(int mediaType, int imgWidth, int imgHeight, string path, bool needrefresh) {
    cMovieMedia *media = new cMovieMedia();
    media->width = imgWidth;
    media->height = imgHeight;
    media->path = path;
    media->mediaType = mediaType;
    media->needrefresh = needrefresh;
    media->mediavalid = !needrefresh; // all media which do not need a refresh should have a image file
    medias.insert(pair<int, cMovieMedia*>(media->mediaType, media));
}

// try to find media of given type
cMovieMedia *cMovieDbMovie::GetMediaObj(int mediatype) {
    map<int, cMovieMedia*>::iterator hit = medias.find(mediatype);
    if (hit != medias.end())
        return hit->second;
    return NULL;
}

void cMovieDbMovie::InsertActor(cMovieActor *actor) {
    cMovieMedia *m = new cMovieMedia();
    m->mediavalid = true; // force true for old functions
    actor->actorThumb = m;
    actors.insert(pair<int, cMovieActor*>(actor->id, actor));
}

// Insert actor to map, but do not generate empty actor thumb (get assigned later)
void cMovieDbMovie::InsertActorNoThumb(cMovieActor *actor) {
    actors.insert(pair<int, cMovieActor*>(actor->id, actor));
}

// try to find actor with given ID
cMovieActor *cMovieDbMovie::GetActor(int actorId) {
    map<int, cMovieActor*>::iterator hit = actors.find(actorId);
    if (hit == actors.end())
        return NULL;
    return hit->second;
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
    if (hit != medias.end()) {
        cMovieMedia *pStored = hit->second;
        if (pStored->mediavalid) {
            p->path = pStored->path;
            p->width = pStored->width;
            p->height = pStored->height;
            return true;
        }    
    }
    return false;
}
    
void cMovieDbMovie::GetActors(vector<cActor> *a) {
    for (map<int, cMovieActor*>::iterator it = actors.begin(); it != actors.end(); it++) {
        cMovieActor *aStored = it->second;
        cActor act;
        act.name = aStored->name;
        act.role = aStored->role;
        if (aStored->actorThumb) {
            if (aStored->actorThumb->mediavalid) {
                // only use valid images
                act.actorThumb.width = aStored->actorThumb->width;
                act.actorThumb.height = aStored->actorThumb->height;
                act.actorThumb.path = aStored->actorThumb->path;
            }    
        } else {
            // check if we have a external actorThumb
            if (aStored->actorThumbExternal) {
                if (aStored->actorThumbExternal->mediavalid) {
                    // only use valid images
                    act.actorThumb.width = aStored->actorThumbExternal->width;
                    act.actorThumb.height = aStored->actorThumbExternal->height;
                    act.actorThumb.path = aStored->actorThumbExternal->path;
                }    
            }    
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