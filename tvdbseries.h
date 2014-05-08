#ifndef __TVSCRAPER_TVDBSERIES_H
#define __TVSCRAPER_TVDBSERIES_H

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <utility>
#include <algorithm>
#include "services.h"

using namespace std;

enum mediaSeries {
    msBanner1,
    msBanner2,
    msBanner3,
    msPoster1,
    msPoster2,
    msPoster3,
    msSeasonPoster,
    msFanart1,
    msFanart2,
    msFanart3,
    msEpisodePic,
    msActorThumb,
    msPosterThumb,
    msSeasonPosterThumb,
};

// --- cTVDBMedia -------------------------------------------------------------
class cTVDBMedia {
public:
    cTVDBMedia(void) {
        path = "";
        mediaType = msBanner1;
        width = 0;
        height = 0;
    };
    ~cTVDBMedia(void) {
    };
    string path;
    int mediaType;
    int width;
    int height;
};

// --- cTVDBEpisode -------------------------------------------------------------
class cTVDBEpisode {
public:
    cTVDBEpisode(void) {
        id = 0;
        number = 0;
        season = 0;
        name = "";
        firstAired = "";
        guestStars = "";
        overview = "";
        rating = 0.0;
        episodeImage = NULL;
    };
    ~cTVDBEpisode(void) {
        if (episodeImage)
            delete episodeImage;
    };
    int id;
    int number;
    int season;
    string name;
    string firstAired;
    string guestStars;
    string overview;
    float rating;
    cTVDBMedia *episodeImage;
};

// --- cTVDBActor -------------------------------------------------------------
class cTVDBActor {
public:
    cTVDBActor(void) {
        id = 0;
        name = "";
        role = "";
        thumbWidth = 0;
        thumbHeight = 0;
        actorThumb = NULL;
    };
    ~cTVDBActor(void) {
        if (actorThumb)
            delete actorThumb;
    };
    int id;
    string name;
    string role;
    int thumbWidth;
    int thumbHeight;
    cTVDBMedia *actorThumb;
};

// --- cTVDBSeries -------------------------------------------------------------

class cTVDBSeries {
private:
    map<int, cTVDBEpisode*> episodes;
    map<int, cTVDBActor*> actors;
    vector<cTVDBMedia*> posters;
    vector<cTVDBMedia*> banners;
    vector<cTVDBMedia*> fanart;
    map<int, cTVDBMedia*> seasonPosters;
    map<int, cTVDBMedia*> seasonPosterThumbs;
    cTVDBMedia *posterThumb;
public:
    cTVDBSeries(void);
    virtual ~cTVDBSeries(void);
    int id;
    string name;
    string overview;
    string firstAired;
    string network;
    string genre;
    float rating;
    string status;
    void InsertEpisode(cTVDBEpisode *episode);
    void InsertEpisodeImage(int episodeId, int width, int height, string path);
    void InsertActor(cTVDBActor *actor);
    void InsertActorThumb(int actorId, int imgWidth, int imgHeight, string path);
    void InsertMedia(int mediaType, int imgWidth, int imgHeight, string path, int season = 0);
    //Getter for Serivice Calls
    void GetEpisode(int episodeId, cEpisode *e);
    void GetPosters(vector<cTvMedia> *p);
    bool GetPoster(cTvMedia *p);
    bool GetPosterThumb(cTvMedia *p);
    void GetBanners(vector<cTvMedia> *b);
    bool GetRandomBanner(cTvMedia *b);
    void GetFanart(vector<cTvMedia> *f);
    void GetSeasonPoster(int episodeId, cTvMedia *sp);
    void GetActors(vector<cActor> *a);
    void Dump(void);
};


#endif //__TVSCRAPER_TVDBSERIES_H
