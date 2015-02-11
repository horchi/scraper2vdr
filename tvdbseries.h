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
        needrefresh = false;
        mediavalid = false;
    };
    ~cTVDBMedia(void) {
    };
    string path;
    int mediaType;
    int width;
    int height;
    bool needrefresh; // if is true we need to load new image data from server for this image
    bool mediavalid; // if is true there should be a file available for this media
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
        lastupdate = 0;
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
    long lastupdate; // Time when episode get updated on thetvdb-server
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
    map<int, cTVDBEpisode*>::iterator episodesIterator;
    map<int, cTVDBActor*>::iterator actorsIterator;
    map<int, cTVDBMedia*>::iterator seasonPostersIterator;
public:
    cTVDBSeries(void);
    int id;
    virtual ~cTVDBSeries(void);
    string name;
    string overview;
    string firstAired;
    string network;
    string genre;
    float rating;
    string status;
    long lastupdate; // Time when series/episodes get updated on thetvdb-server (refresh series data and images)
    long lastepisodeupdate; // Time when newest event/recording of this series get updated on thetvdb-server (refresh episodes and episode images)   
    long lastscraped; // Time when newest event/recording of one of the episodes of this series get scraped from epgd (check for new episodes)
    bool updatecontent; // True if content should be updated/loaded (true after lastupdate changed)
    bool updateimages; // True if there are images inside this series which need to get downloaded
    void InsertEpisode(cTVDBEpisode *episode);
    cTVDBEpisode *GetEpisode(int episodeId);
    void InsertEpisodeImage(int episodeId, int width, int height, string path);
    void SetEpisodeImage(int episodeId, int imgWidth, int imgHeight, string path, bool needrefresh); // insert/update episode image
    int GetEpisodeFirst(cTVDBEpisode* &episode); // get first episode (also init episode iterator)
    int GetEpisodeNext(cTVDBEpisode* &episode); // get next episode from iterator
    void InsertActor(cTVDBActor *actor);
    cTVDBActor *GetActor(int actorId); // try to find actor with given ID
    void SetActorThumb(int actorId, int imgWidth, int imgHeight, string path);
    void SetActorThumb(int actorId, int imgWidth, int imgHeight, string path, bool needrefresh); // insert/update actor thumb
    int GetActorFirst(cTVDBActor* &actor); // get first actor (also init actor iterator)
    int GetActorNext(cTVDBActor* &actor); // get next actor from iterator
    void InsertMedia(int mediaType, int imgWidth, int imgHeight, string path, int season = 0);
    void InsertMedia(int mediaType, int imgWidth, int imgHeight, string path, int season, bool needrefresh); // insert media object
    cTVDBMedia *GetMedia(int mediaType, int season); // try to find media of given type
    int GetSeasonPosterFirst(int &season, cTVDBMedia* &media); // get first season poster (also init season poster iterator)
    int GetSeasonPosterNext(int &season, cTVDBMedia* &media); // get season poster from iterator
    void DeleteSeasonPoster(int season); // delete season poster and thumb
    
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
