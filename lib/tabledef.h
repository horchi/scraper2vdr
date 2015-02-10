/*
 * tabledef.h
 *
 * See the README file for copyright information and how to reach the author.
 *
 */

#ifndef __TABLEDEF_H
#define __TABLEDEF_H

#include "db.h"

//***************************************************************************
// cEpgdState
//***************************************************************************

class cEpgdState
{
   public:

      enum State
      {
         esUnknown = na,

         esInit,
         esStandby,
         esStopped,

         // handler pause on this states!

         esBusy,
         esBusyEvents = esBusy,
         esBusyMatch,
         esBusyScraping,

         // handler don't pause on this states!

         esBusyImages,

         esCount
      };

      static const char* toName(State s);
      static State toState(const char* name);
      static int isValid(State s) { return s > esUnknown && s < esCount; }

      static const char* states[];
};

typedef cEpgdState Es;

//***************************************************************************
// cUpdateState
//***************************************************************************

class cUpdateState
{
   public:

      enum State
      {
         // add to VDRs EPG
         
         usActive      = 'A',
         usLink        = 'L',
         usPassthrough = 'P',
         
         // remove from VDRs EPG
         
         usChanged     = 'C',
         usDelete      = 'D', 
         usRemove      = 'R',
         
         // don't care for VDRs EPG
         
         usInactive    = 'I',
         usTarget      = 'T'
      };

      // get lists for SQL 'in' statements

      static const char* getDeletable()      { return "'A','L','P','R','I'"; }
      static const char* getNeeded()         { return "'A','L','P','C','D','R'"; }
      static const char* getVisible()        { return "'A','L','P'"; }

      // checks fpr c++ code

      static int isNeeded(char c)            { return strchr("ALPCDR", c) != 0; }
      static int isRemove(char c)            { return strchr("CDR", c) != 0; }
      
};

typedef cUpdateState Us;

//***************************************************************************
// class cTableFileRef
//***************************************************************************

class cTableFileRefs : public cDbTable
{
   public:

      cTableFileRefs(cDbConnection* aConnection)
         : cDbTable(aConnection, fields, indices) { }

      virtual const char* TableName()    { return "fileref"; }
      
      enum FieldIndex
      {
         fiName,
         fiSource,

         fiInsSp,
         fiUpdSp,

         fiExternalId,
         fiFileRef,
         fiTag,

         fiCount
      };

      static FieldDef fields[];
      static IndexDef indices[];
};

//***************************************************************************
// class cTableImageRef
//***************************************************************************

class cTableImageRefs : public cDbTable
{
   public:

      cTableImageRefs(cDbConnection* aConnection)
         : cDbTable(aConnection, fields, indices) { }

      virtual const char* TableName()    { return "imagerefs"; }

      enum FieldIndex
      {
         fiEventId,
         fiLfn,

         fiInsSp,
         fiUpdSp,
         fiSource,
         fiFileRef,

         fiImgName,

         fiCount
      };

      static FieldDef fields[];
      static IndexDef indices[];
};

//***************************************************************************
// class cTableImage
//***************************************************************************

class cTableImages : public cDbTable
{
   public:

      cTableImages(cDbConnection* aConnection)
         : cDbTable(aConnection, fields) { }

      virtual const char* TableName()    { return "images"; }

      enum FieldIndex
      {
         fiImgName,

         fiInsSp,
         fiUpdSp,
         fiImage,

         fiCount
      };

      static FieldDef fields[];
};

//***************************************************************************
// class cTableEvent
//***************************************************************************

class cTableEvents : public cDbTable
{
   public:

      cTableEvents(cDbConnection* aConnection)
         : cDbTable(aConnection, fields, indices) { }

      virtual const char* TableName()    { return "events"; }

      enum FieldIndex
      {
         fiEventId,
         fiChannelId,

         fiMasterId,
         fiUseId,

         fiSource,
         fiFileRef,
         fiInsSp,
         fiUpdSp,
         fiUpdFlg,           // update flag
         fiDelFlg,           // deletion flag

         fiTableId,
         fiVersion,
         fiTitle,
         fiCompTitle,        // compressed (without whitespace and special characters)
         fiShortText,
         fiCompShortText,    // compressed (without whitespace and special characters)
         fiLongDescription,
         fiStartTime,
         fiDuration,
         fiParentalRating,
         fiVps,
         fiDescription,      // view field, not stored!

         fiShortDescription,
         fiActor,
         fiAudio,
         fiCategory,
         fiCountry,
         fiDirector,
         fiFlags,
         fiGenre,
         fiInfo,
         fiMusic,
         fiProducer,
         fiScreenplay,
         fiShortreview,
         fiTipp,
         fiTopic,
         fiYear,
         fiRating,
         fiMovieid,
         fiModerator,
         fiOther,
         fiGuest,
         fiCamera,

         fiExtEpNum,
         fiImageCount,

         fiScrSeriesId,
         fiScrSeriesEpisode,
         fiScrMovieId,
         fiScrSp,

         fiCount
      };

      static FieldDef* toField(const char* name);      
      static FieldDef fields[];
      static IndexDef indices[];
};

//***************************************************************************
// class cTableComponent
//***************************************************************************

class cTableComponents : public cDbTable
{
   public:

      cTableComponents(cDbConnection* aConnection)
         : cDbTable(aConnection, fields) { }

      virtual const char* TableName()    { return "components"; }

      enum FieldIndex
      {
         fiEventId,
         fiChannelId,
         fiStream,
         fiType,
         fiLang,
         fiDescription,

         fiInsSp,
         fiUpdSp
      };

      static FieldDef fields[];
};

//***************************************************************************
// class cTableEpisode
//***************************************************************************

class cTableEpisodes : public cDbTable
{
   public:

      cTableEpisodes(cDbConnection* aConnection)
         : cDbTable(aConnection, fields, indices) { }

      virtual const char* TableName()    { return "episodes"; }


      enum FieldIndex
      {
         // primary key 

         fiCompName,      // compressed name (without whitespace and special characters)
         fiCompPartName,  //      "      "       "
         fiLang,          // "de", "en", ...

         fiInsSp,
         fiUpdSp,
         fiLink,

         // episode data 

         fiShortName,
         fiEpisodeName,   // episode name (fielname without path and suffix)

         // part data

         fiPartName,      // part name
         fiSeason,
         fiPart,
         fiParts,
         fiNumber,

         fiExtraCol1,
         fiExtraCol2,
         fiExtraCol3,
         fiComment,

         fiCount
      };

      static FieldDef fields[];
      static IndexDef indices[];
};

//***************************************************************************
// class cTableChannelMap
//***************************************************************************

class cTableChannelMap : public cDbTable
{
   public:

      cTableChannelMap(cDbConnection* aConnection)
         : cDbTable(aConnection, fields, indices) { }

      virtual const char* TableName()    { return "channelmap"; }

      enum FieldIndex
      {
         fiExternalId,   // 
         fiChannelId,    // 
         fiSource,

         fiChannelName,

         fiVps,
         fiMerge,
         fiMergeSp,

         fiInsSp,
         fiUpdSp,
         fiUpdFlg,

         fiCount
      };

      static FieldDef fields[];
      static IndexDef indices[];
};

//***************************************************************************
// class cTableVdr
//***************************************************************************

class cTableVdrs : public cDbTable
{
   public:

      cTableVdrs(cDbConnection* aConnection)
         : cDbTable(aConnection, fields, indices) { }

      virtual const char* TableName()    { return "vdrs"; }

      enum FieldIndex
      {
         fiUuid,

         fiInsSp,
         fiUpdSp,

         fiName,
         fiVersion,
         fiDbApi,
         fiLastUpdate,
         fiNextUpdate,
         fiState,
         fiMaster,
         fiIp,

         fiCount
      };

      static FieldDef fields[];
      static IndexDef indices[];
};

//***************************************************************************
// class cTableParameters
//***************************************************************************

class cTableParameters : public cDbTable
{
   public:

      cTableParameters(cDbConnection* aConnection)
         : cDbTable(aConnection, fields) { }

      virtual const char* TableName()    { return "parameters"; }

      enum FieldIndex
      {
         fiOwner,
         fiName,

         fiInsSp,
         fiUpdSp,

         fiValue,

         fiCount
      };

      static FieldDef fields[];
};

//***************************************************************************
// cTableAnalyse
//***************************************************************************

class cTableAnalyse : public cDbTable
{
   public:

      cTableAnalyse(cDbConnection* aConnection)
         : cDbTable(aConnection, fields, indices) { }

      virtual const char* TableName()    { return "analyse"; }

      enum FieldIndex
      {
         fiChannelId,   
         fiVdrMasterId, 
         fiVdrEventId,  

         fiVdrStartTime,
         fiVdrDuration, 
         fiVdrTitle,    
         fiVdrShortText,
         
         fiExtMasterId, 
         fiExtEventId,  
         fiExtStartTime,
         fiExtDuration, 
         fiExtTitle,    
         fiExtShortText,
         fiExtEpisode,
         fiExtMerge,
         fiExiImages,

         fiLvMin,
         fiRank,

         fiCount
      };

      static FieldDef fields[];
      static IndexDef indices[];
};

//***************************************************************************
// cTableSnapshot
//***************************************************************************

class cTableSnapshot : public cDbTable
{
   public:

      cTableSnapshot(cDbConnection* aConnection)
         : cDbTable(aConnection, fields, indices) { }

      virtual const char* TableName()    { return "snapshot"; }

      enum FieldIndex
      {
         fiChannelId,    
         fiSource,
         fiVdrMasterId,  
         fiEventId,      
         fiUseId,        
         fiStartTime,    
         fiDuration,     
         fiTitle,        
         fiCompTitle,    
         fiShortText,
         fiCompShortText,
         fiUpdsp,
         fiEpisode,
         fiMerge,
         fiImages,
         
         fiCount
      };

      static FieldDef fields[];
      static IndexDef indices[];
};

//***************************************************************************
// class cTableTimers
//***************************************************************************

class cTableTimers : public cDbTable
{
   public:

      cTableTimers(cDbConnection* aConnection)
         : cDbTable(aConnection, fields) { }

      virtual const char* TableName()    { return "timers"; }

      enum FieldIndex
      {
         fiEventId,
         fiChannelId,
         fiVdrUuid,
         
         fiInsSp,
         fiUpdSp,
         
         fiState,
         fiStartTime,
         fiEndTime
      };

      static FieldDef fields[];
};

//***************************************************************************
// class cTableSeries
//***************************************************************************

class cTableSeries : public cDbTable
{
   public:

      cTableSeries(cDbConnection* aConnection)
         : cDbTable(aConnection, fields, indices) { }

      virtual const char* TableName()    { return "series"; }

      enum FieldIndex
      {
         fiSeriesId,

         fiSeriesName,
         fiSeriesLastScraped,
         fiSeriesLastUpdated,
         fiSeriesOverview,
         fiSeriesFirstAired,
         fiSeriesNetwork,
         fiSeriesIMDBId,
         fiSeriesGenre,
         fiSeriesRating,
         fiSeriesStatus,

         fiCount
      };

      static FieldDef* toField(const char* name);      
      static FieldDef fields[];
      static IndexDef indices[];
};

//***************************************************************************
// class cTableSeriesEpisode
//***************************************************************************

class cTableSeriesEpisode : public cDbTable
{
   public:

      cTableSeriesEpisode(cDbConnection* aConnection)
         : cDbTable(aConnection, fields, indices) { }

      virtual const char* TableName()    { return "series_episode"; }

      enum FieldIndex
      {
         fiEpisodeId,

         fiEpisodeNumber,
         fiSeasonNumber,
         fiEpisodeName,
         fiEpisodeOverview,
         fiEpisodeFirstAired,
         fiEpisodeGuestStars,
         fiEpisodeRating,
         fiEpisodeLastUpdated,
         fiSeriesId,   

         fiCount
      };

      static FieldDef* toField(const char* name);      
      static FieldDef fields[];
      static IndexDef indices[];
};

//***************************************************************************
// class cTableSeriesMedia
//***************************************************************************

class cTableSeriesMedia : public cDbTable
{
   public:

      cTableSeriesMedia(cDbConnection* aConnection)
         : cDbTable(aConnection, fields, indices) { }

      virtual const char* TableName()    { return "series_media"; }

      enum FieldIndex
      {
         fiSeriesId,
         fiSeasonNumber,
         fiEpisodeId,
         fiActorId,
         fiMediaType,

         fiMediaUrl,
         fiMediaWidth,
         fiMediaHeight,
         fiMediaRating,
         fiMediaContent,

         fiCount
      };

      static FieldDef* toField(const char* name);      
      static FieldDef fields[];
      static IndexDef indices[];
};

//***************************************************************************
// class cTableSeriesActor
//***************************************************************************

class cTableSeriesActor : public cDbTable
{
   public:

      cTableSeriesActor(cDbConnection* aConnection)
         : cDbTable(aConnection, fields, indices) { }

      virtual const char* TableName()    { return "series_actor"; }

      enum FieldIndex
      {
         fiActorId,

         fiActorName,
         fiActorRole,
         fiSortOrder,

         fiCount
      };

      static FieldDef* toField(const char* name);      
      static FieldDef fields[];
      static IndexDef indices[];
};

//***************************************************************************
// class cTableMovies
//***************************************************************************

class cTableMovies : public cDbTable
{
   public:

      cTableMovies(cDbConnection* aConnection)
         : cDbTable(aConnection, fields, indices) { }

      virtual const char* TableName()    { return "movie"; }

      enum FieldIndex
      {
         fiMovieId,

         fiTitle,
         fiOriginalTitle,
         fiTagline,
         fiOverview,
         fiIsAdult,
         fiCollectionId,
         fiCollectionName,
         fiBudget,
         fiRevenue,
         fiGenres,
         fiHomepage,
         fiReleaaseDate,
         fiRuntime,
         fiPopularity,
         fiVoteAverage,

         fiCount
      };

      static FieldDef* toField(const char* name);      
      static FieldDef fields[];
      static IndexDef indices[];
};

//***************************************************************************
// class cTableMovieActor
//***************************************************************************

class cTableMovieActor : public cDbTable
{
   public:

      cTableMovieActor(cDbConnection* aConnection)
         : cDbTable(aConnection, fields, indices) { }

      virtual const char* TableName()    { return "movie_actor"; }

      enum FieldIndex
      {
         fiActorId,

         fiActorName,

         fiCount
      };

      static FieldDef* toField(const char* name);      
      static FieldDef fields[];
      static IndexDef indices[];
};

//***************************************************************************
// class cTableMovieActors
//***************************************************************************

class cTableMovieActors : public cDbTable
{
   public:

      cTableMovieActors(cDbConnection* aConnection)
         : cDbTable(aConnection, fields, indices) { }

      virtual const char* TableName()    { return "movie_actors"; }

      enum FieldIndex
      {
         fiMovieId,
         fiActorId,

         fiRole,

         fiCount
      };

      static FieldDef* toField(const char* name);      
      static FieldDef fields[];
      static IndexDef indices[];
};

//***************************************************************************
// class cTableMovieMedia
//***************************************************************************

class cTableMovieMedia : public cDbTable
{
   public:

      cTableMovieMedia(cDbConnection* aConnection)
         : cDbTable(aConnection, fields, indices) { }

      virtual const char* TableName()    { return "movie_media"; }

      enum FieldIndex
      {
         fiMovieId,
         fiActorId,
         fiMediaType,

         fiMediaUrl,
         fiMediaWidth,
         fiMediaHeight,
         fiMediaContent,

         fiCount
      };

      static FieldDef* toField(const char* name);      
      static FieldDef fields[];
      static IndexDef indices[];
};

//***************************************************************************
// class cTableRecordings
//***************************************************************************

class cTableRecordings : public cDbTable
{
   public:

      cTableRecordings(cDbConnection* aConnection)
         : cDbTable(aConnection, fields, indices) { }

      virtual const char* TableName()    { return "recordings"; }

      enum FieldIndex
      {
         fiUuid,
         fiRecPath,
         fiRecStart,

         fiEventId,
         fiChannelId,
         fiScrapInfoMovieId,
         fiScrapInfoSeriesId,
         fiScrapInfoEpisodeId,
         fiScrapNew,
         fiRecTitle,
         fiRecSubTitle,
         fiRecDuration,
         fiMovieId,
         fiSeriesId,
         fiEpisodeId,

         fiCount
      };

      static FieldDef* toField(const char* name);      
      static FieldDef fields[];
      static IndexDef indices[];
};

#endif //__TABLEDEF_H
