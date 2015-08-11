/*
 * epgservice.h
 *
 * See the README file for copyright information and how to reach the author.
 *
 */

#ifndef __EPGSERVICE_H
#define __EPGSERVICE_H

#include "db.h"

#define EPG_PLUGIN_SEM_KEY 0x3db00001

//***************************************************************************
// Globals
//***************************************************************************

enum FieldFilter
{
   ffAll         = 0xFFFF,
   ffEpgd        = 1,
   ffEpgHttpd    = 2,
   ffEpg2Vdr     = 4,
   ffScraper2Vdr = 8,

   ffCount = 5
};

struct FieldFilterDef
{
   int filter;
   const char* name;
};

const char* toName(FieldFilter f);
int toFieldFilter(const char* name);

enum SearchFields
{
   sfTitle              = 1,
   sfShorttext          = 2,
   sfDescription        = 4,
   sfEpisode            = 8,
   sfEpisodePart        = 16, 

   sfExistStart         = sfEpisodePart,
   sfShorttextIfExit    = 32,
   sfEpisodeIfExit      = 64,
   sfEpisodePartIfExit  = 128
};

enum SearchMode
{
   smExact = 1,
   smRegexp,
   smLike,
   smContained
};

enum TimerNamingMode
{
   tnmDefault   = 0,     // naming would done by VDR
   
   // naming of following modes handled by recording.py an can 'configured' there 

   tnmAuto        = 1,   // autodetect if 'constabel', 'serie' or 'normal movie'
   tnmConstabel   = 2,   // naming in constabel series style with season, number, ..
   tnmSerie       = 3,   // series style, like Title/Subtitle
   tnmCategorized = 4,   // sorted in sub folders which are auto-named by category
   tnmUser        = 5    // user defined mode 'to implement in recording.py'

};

enum TimerState
{
   tsPending  = 'P',
   tsRunning  = 'R',
   tsFinished = 'F',
   tsDeleted  = 'D',
   tsError    = 'E'
};

enum TimerDoneState
{
   tdsTimerRequested     = 'Q',  // timer requested by epgd/webif
   tdsTimerCreated       = 'C',  // timer created by VDR
   tdsTimerCreateFailed  = 'f',  // create of timer failed by VDR

   tdsRecordingDone      = 'R',  // Recording finished successfull
   tdsRecordingFailed    = 'F',  // Recording failed

   tdsTimerDeleted       = 'D',  // timer deleted by user
   tdsTimerRejected      = 'J'   // timer rejected due to user action or timer conflict
};

enum UserMask
{
   umNone            = 0x0,
   umAll             = 0xFFFFFFFF,

   umConfig          = 0x1,
   umConfigEdit      = 0x2,
   umConfigUsers     = 0x4,

   umFree3           = 0x8,

   umTimer           = 0x10,
   umTimerEdit       = 0x20,
   umSearchTimer     = 0x40,
   umSearchTimerEdit = 0x80,

   umFree5           = 0x100,

   umFsk             = 0x200,

   umFree6           = 0x400,
   umFree7           = 0x800,

   umRecordings      = 0x1000,
   umRecordingsEdit  = 0x2000
};

int hasUserMask(unsigned int rights, UserMask mask);

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
         usTarget      = 'T',
         usMergeSpare  = 'S'
      };

      // get lists for SQL 'in' statements

      static const char* getDeletable()      { return "'A','L','P','R','I'"; }     // epg plugins
      static const char* getNeeded()         { return "'A','L','P','C','D','R'"; } // epg2vdr
      static const char* getVisible()        { return "'A','L','P'"; }             // epghttpd
      
      // checks 

      static int isNeeded(char c)            { return strchr("ALPCDR", c) != 0; }  // epgd2vdr
      static int isRemove(char c)            { return strchr("CDR", c) != 0; }     // epgd2vdr

};

typedef cUpdateState Us;

#endif // __EPGSERVICE_H
