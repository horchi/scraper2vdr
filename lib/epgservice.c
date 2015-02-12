/*
 * epgservice.c
 *
 * See the README file for copyright information and how to reach the author.
 *
 */

#include "epgservice.h"

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
