#ifndef __TVSCRAPER_MOVIEDBMOVIE_H
#define __TVSCRAPER_MOVIEDBMOVIE_H

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <utility>
#include <algorithm>
#include "services.h"

using namespace std;

enum mediaMovies {
     mmPoster,
     mmFanart,
     mmCollectionPoster,
     mmCollectionFanart,
     mmActorThumb,
     mmPosterThumb,
};

// --- cMovieMedia -------------------------------------------------------------
class cMovieMedia {
public:
    cMovieMedia(void) {
        path = "";
        mediaType = mmPoster;
        width = 0;
        height = 0;
        needrefresh = false;
        mediavalid = false;
    };
    ~cMovieMedia(void) {
    };
    string path;
    int mediaType;
    int width;
    int height;
    bool needrefresh; // if is true we need to load new image data from server for this image
    bool mediavalid; // if is true there should be a file available for this media
};

// --- cMovieActor -------------------------------------------------------------
class cMovieActor {
public:
    cMovieActor(void) {
        id = 0;
        name = "";
        role = "";
        actorThumb = NULL;
        actorThumbExternal = NULL;
    };
    ~cMovieActor(void) {
        if (actorThumb)
            delete actorThumb;
    };
    int id;
    string name;
    string role;
    cMovieMedia *actorThumb;
    cMovieMedia *actorThumbExternal; // pointer to global movie actors map
};

// --- cMovieDbMovie -------------------------------------------------------------

class cMovieDbMovie {
private:
    map<int, cMovieActor*> actors;
    map<int, cMovieMedia*> medias;
public:
    cMovieDbMovie(void);
    virtual ~cMovieDbMovie(void);
    int id;
    string title;
    string originalTitle;
    string tagline;    
    string overview;
    bool adult;
    int collectionID;
    string collectionName;
    int budget;
    int revenue;
    string genres;
    string homepage;
    string imdbid;
    string releaseDate;
    int runtime;
    float popularity;
    float voteAverage;
    long lastscraped; // Time when newest event/recording of this movie get scraped from epgd (check for new episodes)
    bool updateimages; // True if there are images inside this movie which need to get downloaded
    void InsertActor(cMovieActor *actor);
    void InsertActorNoThumb(cMovieActor *actor); // Insert actor to map, but do not generate empty actor thumb (get assigned later)
    cMovieActor *GetActor(int actorId); // try to find actor with given ID
    void InsertMedia(cMovieMedia *media);
    void InsertMedia(int mediaType, int imgWidth, int imgHeight, string path, bool needrefresh); // insert media object (create media object also)
    cMovieMedia *GetMediaObj(int mediatype); // try to find media of given type
    vector<int> GetActorIDs(void);
    void SetActorThumbSize(int actorId, int imgWidth, int imgHeight);
    void SetActorPath(int actorId, string path);
    //Getter for Serivice Calls
    bool GetMedia(mediaMovies mediatype, cTvMedia *p);
    void GetActors(vector<cActor> *a);
    void Dump();
};


#endif //__TVSCRAPER_TVDBSERIES_H
