/*
 * tabledef.c
 *
 * See the README file for copyright information and how to reach the author.
 *
 */

#include "tabledef.h"

//***************************************************************************
// cEpgdState
//***************************************************************************

const char* cEpgdState::states[] =
{
   "init",
   "standby",
   "stopped",

   "busy (events)",
   "busy (match)",
   "busy (scraping)",
   "busy (images)",

   0
};

const char* cEpgdState::toName(cEpgdState::State s)
{
   if (!isValid(s))
      return "unknown";

   return states[s];
}

cEpgdState::State cEpgdState::toState(const char* name)
{
   for (int i = 0; i < esCount; i++)
      if (strcmp(states[i], name) == 0)
         return (State)i;

   return esUnknown;
}

//***************************************************************************
// Event Fields
//***************************************************************************
//***************************************************************************
// Fields
//***************************************************************************

cDbService::FieldDef cTableEvents::fields[] =
{
   // name               format     size  index               type

   // primary key

   { "eventid",          ffUInt,       0, fiEventId,          ftPrimary },
   { "channelid",        ffAscii,     50, fiChannelId,        ftPrimary },

   { "masterid",         ffUInt,       0, fiMasterId,         ftAutoinc },
   { "useid",            ffUInt,       0, fiUseId,            ftData },

   // meta                                                         

   { "source",           ffAscii,     10, fiSource,           ftMeta },
   { "fileref",          ffAscii,     50, fiFileRef,          ftMeta },
   { "inssp",            ffInt,       10, fiInsSp,            ftMeta },
   { "updsp",            ffInt,       10, fiUpdSp,            ftMeta },
   { "updflg",           ffAscii,      1, fiUpdFlg,           ftMeta },
   { "delflg",           ffAscii,      1, fiDelFlg,           ftMeta },
                                                                   
   // vdr event data                                               
                                                                   
   { "tableid",          ffInt,        2, fiTableId,          ftData },
   { "version",          ffInt,        3, fiVersion,          ftData },
   { "title",            ffAscii,    200, fiTitle,            ftData },
   { "comptitle",        ffAscii,    200, fiCompTitle,        ftData },
   { "shorttext",        ffAscii,    300, fiShortText,        ftData },
   { "compshorttext",    ffAscii,    300, fiCompShortText,    ftData },
   { "longdescription",  ffText,   25000, fiLongDescription,  ftData },
   { "starttime",        ffInt,       10, fiStartTime,        ftData },
   { "duration",         ffInt,        5, fiDuration,         ftData },
   { "parentalrating",   ffInt,        2, fiParentalRating,   ftData },
   { "vps",              ffInt,       10, fiVps,              ftData },

   { "description",      ffText,   50000, fiDescription,      ftCalc },
                                                                   
   // additional external data
                                                                   
   { "shortdescription", ffAscii,   3000, fiShortDescription, ftData },
   { "actor",            ffAscii,   3000, fiActor,            ftData },
   { "audio",            ffAscii,     50, fiAudio,            ftData },
   { "category",         ffAscii,     50, fiCategory,         ftData },
   { "country",          ffAscii,     50, fiCountry,          ftData },
   { "director",         ffAscii,    250, fiDirector,         ftData },
   { "flags",            ffAscii,    100, fiFlags,            ftData },
   { "genre",            ffAscii,    100, fiGenre,            ftData },
   { "info",             ffText,   10000, fiInfo,             ftData },
   { "music",            ffAscii,    250, fiMusic,            ftData },
   { "producer",         ffText,    1000, fiProducer,         ftData },
   { "screenplay",       ffAscii,    500, fiScreenplay,       ftData },
   { "shortreview",      ffAscii,    500, fiShortreview,      ftData },
   { "tipp",             ffAscii,    250, fiTipp,             ftData },
   { "topic",            ffAscii,    500, fiTopic,            ftData },
   { "year",             ffAscii,     10, fiYear,             ftData },
   { "rating",           ffAscii,    250, fiRating,           ftData },
   { "fsk",              ffAscii,      2, fiFsk,              ftData },
   { "movieid",          ffAscii,     20, fiMovieid,          ftData },
   { "moderator",        ffAscii,    250, fiModerator,        ftData },
   { "other",            ffText,    2000, fiOther,            ftData },
   { "guest",            ffText,    1000, fiGuest,            ftData },
   { "camera",           ffText,    1000, fiCamera,           ftData },

   { "extepnum",         ffInt,        4, fiExtEpNum,         ftData },
   { "imagecount",       ffInt,        2, fiImageCount,       ftData },

   // episodes (constable)

//   { "episodecompname",          ffAscii,    250, fiEpisode,          ftData },
//   { "episodecomppartname",      ffAscii,    250, fiEpisodePart,      ftData },
//   { "episodelang",      ffAscii,      3, fiEpisodeLang,      ftData },

   // tv scraper

   { "scrseriesid",      ffInt,       11, fiScrSeriesId,      ftData },
   { "scrseriesepisode", ffInt,       11, fiScrSeriesEpisode, ftData },
   { "scrmovieid",       ffInt,       11, fiScrMovieId,       ftData },
   { "scrsp",            ffInt,       11, fiScrSp,            ftData },

   { 0 }
};

cDbService::FieldDef* cTableEvents::toField(const char* name)
{
   for (int i = 0; i < fiCount; i++)
      if (strcmp(fields[i].name, name) == 0)
         return &fields[i];
   
   tell(0, "Request for unexpected field '%s', ignoring", name);

   return 0;
}

cDbService::IndexDef cTableEvents::indices[] =
{
   // index             fields  

   { "comptitle",               { fiCompTitle, na            }, 0 },
   { "source",                  { fiSource, na               }, 0 },
   { "FilerefSource",           { fiFileRef, fiSource,    na }, 0 },
   { "channelid",               { fiChannelId, na            }, 0 },
   { "useid",                   { fiUseId, na                }, 0 },
   { "useidchannelid",          { fiUseId, fiChannelId,   na }, 0 },
   { "updflgupdsp",             { fiUpdFlg, fiUpdSp,      na }, 0 },
   { "sourcechannelid",         { fiSource, fiChannelId,  na }, 0 },
   { "scrsp",                   { fiScrSp,  na               }, 0 },

   { 0 }
};

//***************************************************************************
// Components
//***************************************************************************

cDbService::FieldDef cTableComponents::fields[] =
{
   // name               format     size  index          type

   { "eventid",          ffUInt,        0, fiEventId,     ftPrimary },
   { "channelid",        ffAscii,      50, fiChannelId,   ftPrimary },
   { "stream",           ffInt,         3, fiStream,      ftPrimary },
   { "type",             ffInt,         3, fiType,        ftPrimary },
   { "lang",             ffAscii,       8, fiLang,        ftPrimary },
   { "description",      ffAscii,     100, fiDescription, ftPrimary },

   { "inssp",            ffInt,         0, fiInsSp,       ftMeta },
   { "updsp",            ffInt,         0, fiUpdSp,       ftMeta },

   { 0 }
};

//***************************************************************************
// File References
//***************************************************************************

cDbService::FieldDef cTableFileRefs::fields[] =
{
   // name               format     size  index          type

   { "name",             ffAscii,    100, fiName,        ftPrimary },
   { "source",           ffAscii,     10, fiSource,      ftPrimary },

   { "inssp",            ffInt,        0, fiInsSp,       ftMeta },
   { "updsp",            ffInt,        0, fiUpdSp,       ftMeta },

   { "extid",            ffAscii,     10, fiExternalId,  ftData },
   { "fileref",          ffAscii,    100, fiFileRef,     ftData },     // name + '-' + tag
   { "tag",              ffAscii,    100, fiTag,         ftData },

   { 0 }
};

cDbService::IndexDef cTableFileRefs::indices[] =
{
   // index              fields             

   { "SourceFileref",  { fiSource, fiFileRef, na }, 0 },
   { "Fileref",        { fiFileRef, na           }, 0 },

   { 0 }
};

//***************************************************************************
// Image Ref Fields
//***************************************************************************

cDbService::FieldDef cTableImageRefs::fields[] =
{
   // name               format     size index          type

   { "eventid",          ffUInt,       0, fiEventId,     ftPrimary },
   { "lfn",              ffInt,        0, fiLfn,         ftPrimary },

   { "inssp",            ffInt,        0, fiInsSp,       ftMeta   },
   { "updsp",            ffInt,        0, fiUpdSp,       ftMeta   },
   { "source",           ffAscii,     10, fiSource,      ftMeta },

   { "fileref",          ffAscii,    100, fiFileRef,     ftData },
   { "imagename",        ffAscii,    100, fiImgName,     ftData   },

   { 0 }
};

cDbService::IndexDef cTableImageRefs::indices[] =
{
   // index     fields     

   { "lfn",   { fiLfn, na     }, 0 },
   { "name",  { fiImgName, na }, 0 },

   { 0 }
};

//***************************************************************************
// Image Fields
//***************************************************************************

cDbService::FieldDef cTableImages::fields[] =
{
   // name               format     size index          type

   { "imagename",        ffAscii,    100, fiImgName,     ftPrimary },

   { "inssp",            ffInt,        0, fiInsSp,       ftMeta },
   { "updsp",            ffInt,        0, fiUpdSp,       ftMeta },
 
   { "image",            ffMlob,  200000, fiImage,       ftData },

   { 0 }
};

//***************************************************************************
// Series Episode Fields
//***************************************************************************

cDbService::FieldDef cTableEpisodes::fields[] =
{
   // name               format    size  index           type

   // primary key 

   { "compname",          ffAscii,   100, fiCompName,     ftPrimary },   // episode name compressed
   { "comppartname",      ffAscii,   200, fiCompPartName, ftPrimary },   // part name compressed
   { "lang",              ffAscii,    10, fiLang,         ftPrimary }, 

   { "inssp",             ffInt,       0, fiInsSp,        ftMeta },
   { "updsp",             ffInt,       0, fiUpdSp,        ftMeta },
   { "link",              ffInt,       0, fiLink,         ftData },

   // episode data 

   { "shortname",         ffAscii,   100, fiShortName,    ftData },
   { "episodename",       ffAscii,   100, fiEpisodeName,  ftData },   // episode name

   // part data

   { "partname",          ffAscii,   300, fiPartName,     ftData },   // part name
   { "season",            ffInt,       0, fiSeason,       ftData },
   { "part",              ffInt,       0, fiPart,         ftData },
   { "parts",             ffInt,       0, fiParts,        ftData },
   { "number",            ffInt,       0, fiNumber,       ftData },

   { "extracol1",         ffAscii,   250, fiExtraCol1,    ftData },
   { "extracol2",         ffAscii,   250, fiExtraCol2,    ftData },
   { "extracol3",         ffAscii,   250, fiExtraCol3,    ftData },

   { "comment",           ffAscii,   250, fiComment,      ftData },

   { 0 }
};

cDbService::IndexDef cTableEpisodes::indices[] =
{
   // index     fields

   { "updsp", { fiUpdSp, na }, 0 },

   { 0 }
};

//***************************************************************************
// Channel Map Fields
//***************************************************************************

cDbService::FieldDef cTableChannelMap::fields[] =
{
   // name               format     size index           type

   { "extid",            ffAscii,    10, fiExternalId,   ftPrimary },
   { "channelid",        ffAscii,    50, fiChannelId,    ftPrimary },
   { "source",           ffAscii,    20, fiSource,       ftPrimary },

   { "channelname",      ffAscii,   100, fiChannelName,  ftData },

   { "vps",              ffInt,       0, fiVps,          ftData },
   { "merge",            ffInt,       0, fiMerge,        ftData },
   { "mergesp",          ffInt,       0, fiMergeSp,      ftData },

   { "inssp",            ffInt,       0, fiInsSp,        ftMeta },
   { "updsp",            ffInt,       0, fiUpdSp,        ftMeta },
   { "updflg",           ffAscii,     1, fiUpdFlg,       ftMeta },

   { 0 }
};

cDbService::IndexDef cTableChannelMap::indices[] =
{
   // index                fields 

   { "sourceExtid",        { fiSource, fiExternalId, na }, 0 },
   { "source",             { fiSource, na               }, 0 },
   { "updflg",             { fiUpdFlg, na               }, 0 },
   { "sourcechannelid",    { fiSource, fiChannelId,  na }, 0 },
   { "mergesp",            { fiMergeSp, na              }, 0 },
   { "channelid",          { fiChannelId, na            }, 0 },

   { 0 }
};

//***************************************************************************
// VDRs Fields
//***************************************************************************

cDbService::FieldDef cTableVdrs::fields[] =
{
   // name               format     size index           type

   { "uuid",             ffAscii,    40, fiUuid,         ftPrimary },

   { "inssp",            ffInt,       0, fiInsSp,        ftMeta },
   { "updsp",            ffInt,       0, fiUpdSp,        ftMeta },

   { "name",             ffAscii,   100, fiName,         ftData },
   { "version",          ffAscii,   100, fiVersion,      ftData },
   { "dbapi",            ffUInt,      0, fiDbApi,        ftData },
   { "lastupd",          ffInt,       0, fiLastUpdate,   ftData },
   { "nextupd",          ffInt,       0, fiNextUpdate,   ftData },
   { "state",            ffAscii,    20, fiState,        ftData },
   { "master",           ffAscii,     1, fiMaster,       ftData },
   { "ip",               ffAscii,    20, fiIp,           ftData },

   { 0 }
};

cDbService::IndexDef cTableVdrs::indices[] =
{
   // index           fields 

   { "state",        { fiState, na              }, 0 },

   { 0 }
};

//***************************************************************************
// Parameter Fields
//***************************************************************************

cDbService::FieldDef cTableParameters::fields[] =
{
   // name               format     size index           type

   { "owner",            ffAscii,    40, fiOwner,        ftPrimary },
   { "name",             ffAscii,    40, fiName,         ftPrimary },

   { "inssp",            ffInt,       0, fiInsSp,        ftMeta },
   { "updsp",            ffInt,       0, fiUpdSp,        ftMeta },

   { "value",            ffAscii,   100, fiValue,        ftData },

   { 0 }
};

//***************************************************************************
// Analyse
//***************************************************************************

cDbService::FieldDef cTableAnalyse::fields[] =
{
   // name               format     size  index            type

   { "channelid",        ffAscii,     50, fiChannelId,     ftPrimary },
   { "vdr_masterid",     ffUInt,       0, fiVdrMasterId,   ftData },
   { "vdr_eventid",      ffUInt,       0, fiVdrEventId,    ftPrimary },

   { "vdr_starttime",    ffInt,       10, fiVdrStartTime,  ftData },
   { "vdr_duration",     ffInt,        5, fiVdrDuration,   ftData },
   { "vdr_title",        ffAscii,    200, fiVdrTitle,      ftData },
   { "vdr_shorttext",    ffAscii,    300, fiVdrShortText,  ftData },

   { "ext_masterid",     ffUInt,       0, fiExtMasterId,   ftData },
   { "ext_eventid",      ffUInt,       0, fiExtEventId,    ftData },
   { "ext_starttime",    ffInt,       10, fiExtStartTime,  ftData },
   { "ext_duration",     ffInt,        5, fiExtDuration,   ftData },
   { "ext_title",        ffAscii,    200, fiExtTitle,      ftData },
   { "ext_shorttext",    ffAscii,    300, fiExtShortText,  ftData },
   { "ext_episode",      ffAscii,      1, fiExtEpisode,    ftData },
   { "ext_merge",        ffInt,       11, fiExtMerge,      ftData },
   { "ext_images",       ffAscii,      1, fiExiImages,     ftData },

   { "lvmin",            ffInt,        3, fiLvMin,         ftData },
   { "rank",             ffInt,        5, fiRank,          ftData },

   { 0 }
};

cDbService::IndexDef cTableAnalyse::indices[] =
{
   // index            fields 

   { "vdr_masterid", { fiVdrMasterId, na }, 0 },

   { 0 }
};

//***************************************************************************
// Snapshot
//***************************************************************************

cDbService::FieldDef cTableSnapshot::fields[] =
{
   // name            format     size  index             type

   { "channelid",     ffAscii,     50, fiChannelId,      ftData },
   { "source",        ffAscii,     10, fiSource,         ftData },

   { "masterid",      ffUInt,       0, fiVdrMasterId,    ftData },
   { "eventid",       ffUInt,       0, fiEventId,        ftData },
   { "useid",         ffUInt,       0, fiUseId,          ftData },

   { "starttime",     ffInt,       10, fiStartTime,      ftData },
   { "duration",      ffInt,        5, fiDuration,       ftData },
   { "title",         ffAscii,    200, fiTitle,          ftData },
   { "comptitle",     ffAscii,    200, fiCompTitle,      ftData },
   { "shorttext",     ffAscii,    300, fiShortText,      ftData },
   { "compshorttext", ffAscii,    300, fiCompShortText,  ftData },

   { "updsp",         ffInt,       10, fiUpdsp,          ftData },

   { "episode",       ffAscii,      1, fiEpisode,        ftData },
   { "merge",         ffInt,        0, fiMerge,          ftData },
   { "images",        ffAscii,      1, fiImages,         ftData },

   { 0 }
};

cDbService::IndexDef cTableSnapshot::indices[] =
{
   // index            fields 

   { "channelid",       { fiChannelId, na }, 0 },
   { "starttimeSource", { fiStartTime, fiSource, na }, 0 },

   { 0 }
};

//***************************************************************************
// Timers
//***************************************************************************

cDbService::FieldDef cTableTimers::fields[] =
{
   // name               format     size  index          type

   { "eventid",          ffUInt,        0, fiEventId,     ftPrimary },
   { "channelid",        ffAscii,      50, fiChannelId,   ftPrimary },
   { "vdruuid",          ffAscii,      40, fiVdrUuid,     ftPrimary },

   { "inssp",            ffInt,         0, fiInsSp,       ftMeta },
   { "updsp",            ffInt,         0, fiUpdSp,       ftMeta },

   { "state",            ffAscii,       1, fiState,       ftData },       // { 'D'eleted, 'N'ew, 'A'ssigned, pease 'R'eassign }
   { "starttime",        ffInt,        10, fiStartTime,   ftData },
   { "endtime",          ffInt,        10, fiEndTime,     ftData },

   { 0 }
};

//***************************************************************************
// Series Fields
//***************************************************************************

cDbService::FieldDef cTableSeries::fields[] =
{
   // name                     format     size  index                  type

   // primary key

   { "series_id",              ffUInt,       0, fiSeriesId,            ftPrimary },

   // data

   { "series_name",            ffAscii,    200, fiSeriesName,          ftData },
   { "series_last_scraped",    ffUInt,       0, fiSeriesLastScraped,   ftData },
   { "series_last_updated",    ffUInt,       0, fiSeriesLastUpdated,   ftData },
   { "series_overview",        ffText,   10000, fiSeriesOverview,      ftData },
   { "series_firstaired",      ffAscii,     50, fiSeriesFirstAired,    ftData },
   { "series_network",         ffAscii,    100, fiSeriesNetwork,       ftData },
   { "series_imdb_id",         ffAscii,     20, fiSeriesIMDBId,        ftData },
   { "series_genre",           ffAscii,    100, fiSeriesGenre,         ftData },
   { "series_rating",          ffFloat,     31, fiSeriesRating,        ftData },
   { "series_status",          ffAscii,     50, fiSeriesStatus,        ftData },

   { 0 }
};

cDbService::FieldDef* cTableSeries::toField(const char* name)
{
   for (int i = 0; i < fiCount; i++)
      if (strcmp(fields[i].name, name) == 0)
         return &fields[i];
   
   tell(0, "Request for unexpected field '%s', ignoring", name);

   return 0;
}

cDbService::IndexDef cTableSeries::indices[] =
{
   // index               fields  

   { "seriesname",        { fiSeriesName, na }, 0 },

   { 0 }
};


//***************************************************************************
// SeriesEpisode Fields
//***************************************************************************

cDbService::FieldDef cTableSeriesEpisode::fields[] =
{
   // name                        format     size  index                  type

   // primary key

   { "episode_id",                  ffUInt,       0, fiEpisodeId,           ftPrimary },

   // data

   { "episode_number",              ffUInt,       0, fiEpisodeNumber,       ftData },
   { "season_number",               ffUInt,       0, fiSeasonNumber,        ftData },
   { "episode_name",                ffAscii,    300, fiEpisodeName,         ftData },
   { "episode_overview",            ffText,   10000, fiEpisodeOverview,     ftData },
   { "episode_firstaired",          ffAscii,     20, fiEpisodeFirstAired,   ftData },
   { "episode_gueststars",          ffAscii,   1000, fiEpisodeGuestStars,   ftData },
   { "episode_rating",              ffFloat,     31, fiEpisodeRating,       ftData },
   { "episode_last_updated",        ffUInt,       0, fiEpisodeLastUpdated,  ftData },
   { "series_id",                   ffUInt,       0, fiSeriesId,            ftData },
   
   { 0 }
};

cDbService::FieldDef* cTableSeriesEpisode::toField(const char* name)
{
   for (int i = 0; i < fiCount; i++)
      if (strcmp(fields[i].name, name) == 0)
         return &fields[i];
   
   tell(0, "Request for unexpected field '%s', ignoring", name);

   return 0;
}

cDbService::IndexDef cTableSeriesEpisode::indices[] =
{
   // index               fields  
   { "series_id",         { fiSeriesId, na }, 0 },

   { 0 }
};

//***************************************************************************
// SeriesMedia Fields
//***************************************************************************

cDbService::FieldDef cTableSeriesMedia::fields[] =
{
   // name                        format     size  index                    type

   // primary key

   { "series_id",                  ffUInt,       0, fiSeriesId,             ftPrimary },
   { "season_number",              ffUInt,       0, fiSeasonNumber,         ftPrimary },
   { "episode_id",                 ffUInt,       0, fiEpisodeId,            ftPrimary },
   { "actor_id",                   ffUInt,       0, fiActorId,              ftPrimary },
   { "media_type",                 ffUInt,       0, fiMediaType,            ftPrimary },

   // data

   { "media_url",                  ffAscii,    100, fiMediaUrl,             ftData },
   { "media_width",                ffUInt,       0, fiMediaWidth,           ftData },
   { "media_height",               ffUInt,       0, fiMediaHeight,          ftData },
   { "media_rating",               ffFloat,     31, fiMediaRating,          ftData },
   { "media_content",              ffMlob, 1000000, fiMediaContent,         ftData },
   
   { 0 }
};

cDbService::FieldDef* cTableSeriesMedia::toField(const char* name)
{
   for (int i = 0; i < fiCount; i++)
      if (strcmp(fields[i].name, name) == 0)
         return &fields[i];
   
   tell(0, "Request for unexpected field '%s', ignoring", name);

   return 0;
}

cDbService::IndexDef cTableSeriesMedia::indices[] =
{
   // index               fields  

   { "series_id",         { fiSeriesId, na }, 0 },
   { "season_number",     { fiSeasonNumber, na }, 0 },
   { "episode_id",        { fiEpisodeId, na }, 0 },
   { "actor_id",          { fiActorId, na }, 0 },

   { 0 }
};

//***************************************************************************
// SeriesActor Fields
//***************************************************************************

cDbService::FieldDef cTableSeriesActor::fields[] =
{
   // name                        format     size  index                    type

   // primary key

   { "actor_id",                  ffUInt,       0, fiActorId,              ftPrimary },

   // data

   { "actor_name",                ffAscii,    100, fiActorName,            ftData },
   { "actor_role",                ffAscii,    500, fiActorRole,            ftData },
   { "actor_sortorder",           ffUInt,       0, fiSortOrder,            ftData },
   
   { 0 }
};

cDbService::FieldDef* cTableSeriesActor::toField(const char* name)
{
   for (int i = 0; i < fiCount; i++)
      if (strcmp(fields[i].name, name) == 0)
         return &fields[i];
   
   tell(0, "Request for unexpected field '%s', ignoring", name);

   return 0;
}

cDbService::IndexDef cTableSeriesActor::indices[] =
{
   // index               fields  

   { 0 }
};

//***************************************************************************
// Movie Fields
//***************************************************************************

cDbService::FieldDef cTableMovies::fields[] =
{
   // name                        format     size  index                    type

   // primary key

   { "movie_id",                   ffUInt,       0, fiMovieId,              ftPrimary },

   // data

   { "movie_title",                ffAscii,    300, fiTitle,                ftData },
   { "movie_original_title",       ffAscii,    300, fiOriginalTitle,        ftData },
   { "movie_tagline",              ffAscii,   1000, fiTagline,              ftData },
   { "movie_overview",             ffText,    5000, fiOverview,             ftData },
   { "movie_adult",                ffUInt,       0, fiIsAdult,              ftData },
   { "movie_collection_id",        ffUInt,       0, fiCollectionId,         ftData },
   { "movie_collection_name",      ffAscii,    300, fiCollectionName,       ftData },
   { "movie_budget",               ffUInt,       0, fiBudget,               ftData },
   { "movie_revenue",              ffUInt,       0, fiRevenue,              ftData },
   { "movie_genres",               ffAscii,    500, fiGenres,               ftData },
   { "movie_homepage",             ffAscii,    300, fiHomepage,             ftData },
   { "movie_release_date",         ffAscii,     20, fiReleaaseDate,         ftData },
   { "movie_runtime",              ffUInt,       0, fiRuntime,              ftData },
   { "movie_popularity",           ffFloat,     31, fiPopularity,           ftData },
   { "movie_vote_average",         ffFloat,     31, fiVoteAverage,          ftData },
   
   { 0 }
};

cDbService::FieldDef* cTableMovies::toField(const char* name)
{
   for (int i = 0; i < fiCount; i++)
      if (strcmp(fields[i].name, name) == 0)
         return &fields[i];
   
   tell(0, "Request for unexpected field '%s', ignoring", name);

   return 0;
}

cDbService::IndexDef cTableMovies::indices[] =
{
   // index              fields  

   { "movie_id",         { fiMovieId, na }, 0 },
   { "movietitle",       { fiTitle, na }, 0 },

   { 0 }
};

//***************************************************************************
// MovieActor Fields
//***************************************************************************

cDbService::FieldDef cTableMovieActor::fields[] =
{
   // name                        format     size  index                    type

   // primary key

   { "actor_id",                   ffUInt,       0, fiActorId,              ftPrimary },

   // data

   { "actor_name",                 ffAscii,    300, fiActorName,            ftData },
   
   { 0 }
};

cDbService::FieldDef* cTableMovieActor::toField(const char* name)
{
   for (int i = 0; i < fiCount; i++)
      if (strcmp(fields[i].name, name) == 0)
         return &fields[i];
   
   tell(0, "Request for unexpected field '%s', ignoring", name);

   return 0;
}

cDbService::IndexDef cTableMovieActor::indices[] =
{
   // index               fields  

   { "actor_id",         { fiActorId, na }, 0 },
   
   { 0 }
};

//***************************************************************************
// MovieActors Fields
//***************************************************************************

cDbService::FieldDef cTableMovieActors::fields[] =
{
   // name                        format     size  index                    type

   // primary key

   { "movie_id",                   ffUInt,       0, fiMovieId,              ftPrimary },
   { "actor_id",                   ffUInt,       0, fiActorId,              ftPrimary },

   // data

   { "actor_role",                 ffAscii,    300, fiRole,                 ftData },
   
   { 0 }
};

cDbService::FieldDef* cTableMovieActors::toField(const char* name)
{
   for (int i = 0; i < fiCount; i++)
      if (strcmp(fields[i].name, name) == 0)
         return &fields[i];
   
   tell(0, "Request for unexpected field '%s', ignoring", name);

   return 0;
}

cDbService::IndexDef cTableMovieActors::indices[] =
{
   // index               fields  

   { "movie_id",         { fiMovieId, na }, 0 },
   { "actor_id",         { fiActorId, na }, 0 },
   
   { 0 }
};

//***************************************************************************
// cTableMovieMedia Fields
//***************************************************************************

cDbService::FieldDef cTableMovieMedia::fields[] =
{
   // name                        format     size  index                    type

   // primary key

   { "movie_id",                   ffUInt,       0, fiMovieId,              ftPrimary },
   { "actor_id",                   ffUInt,       0, fiActorId,              ftPrimary },
   { "media_type",                 ffUInt,       0, fiMediaType,            ftPrimary },

   // data

   { "media_url",                  ffAscii,    100, fiMediaUrl,             ftData },
   { "media_width",                ffUInt,       0, fiMediaWidth,           ftData },
   { "media_height",               ffUInt,       0, fiMediaHeight,          ftData },
   { "media_content",              ffMlob, 1000000, fiMediaContent,         ftData },
   
   { 0 }
};

cDbService::FieldDef* cTableMovieMedia::toField(const char* name)
{
   for (int i = 0; i < fiCount; i++)
      if (strcmp(fields[i].name, name) == 0)
         return &fields[i];
   
   tell(0, "Request for unexpected field '%s', ignoring", name);

   return 0;
}

cDbService::IndexDef cTableMovieMedia::indices[] =
{
   // index               fields  

   { "movie_id",          { fiMovieId, na }, 0 },
   { "actor_id",          { fiActorId, na }, 0 },

   { 0 }
};

//***************************************************************************
// cTableRecordings Fields
//***************************************************************************

cDbService::FieldDef cTableRecordings::fields[] =
{
   // name                        format     size  index                    type

   // primary key

   { "uuid",                       ffAscii,     40, fiUuid,                 ftPrimary },
   { "rec_path",                   ffAscii,    200, fiRecPath,              ftPrimary },
   { "rec_start",                  ffUInt,       0, fiRecStart,             ftPrimary },
   
   // data

   { "event_id",                   ffUInt,       0, fiEventId,              ftData },
   { "channel_id",                 ffAscii,     50, fiChannelId,            ftData },
   { "scrapinfo_movie_id",         ffUInt,       0, fiScrapInfoMovieId,     ftData },
   { "scrapinfo_series_id",        ffUInt,       0, fiScrapInfoSeriesId,    ftData },
   { "scrapinfo_episode_id",       ffUInt,       0, fiScrapInfoEpisodeId,   ftData },
   { "scrap_new",                  ffUInt,       0, fiScrapNew,             ftData },
   { "rec_title",                  ffAscii,    200, fiRecTitle,             ftData },
   { "rec_subtitle",               ffAscii,    500, fiRecSubTitle,          ftData },
   { "rec_duration",               ffUInt,       0, fiRecDuration,          ftData },
   { "movie_id",                   ffUInt,       0, fiMovieId,              ftData },
   { "series_id",                  ffUInt,       0, fiSeriesId,             ftData },
   { "episode_id",                 ffUInt,       0, fiEpisodeId,            ftData },
   
   { 0 }
};

cDbService::FieldDef* cTableRecordings::toField(const char* name)
{
   for (int i = 0; i < fiCount; i++)
      if (strcmp(fields[i].name, name) == 0)
         return &fields[i];
   
   tell(0, "Request for unexpected field '%s', ignoring", name);

   return 0;
}

cDbService::IndexDef cTableRecordings::indices[] =
{
   // index                   fields  
 
   { "uuid",                  { fiUuid, na     }, 0 },
   { "rec_path",              { fiRecPath, na  }, 0 },
   { "rec_start",             { fiRecStart, na }, 0 },
   { "scrap_new",             { fiScrapNew, na }, 0 },


   { 0 }
};
