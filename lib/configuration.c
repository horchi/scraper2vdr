/*
 * configuration.c
 *
 * See the README file for copyright information and how to reach the author.
 *
 */

#include "configuration.h"

time_t cSystemNotification::lastWatchdogAt = time(0);
const char* cSystemNotification::pidfile = "";

//***************************************************************************
// Class cSystemNotification
//***************************************************************************

cSystemNotification::cSystemNotification() 
   : cThread("SystemNotification thread", yes)
{ 
   interval = 0; 
}

//***************************************************************************
// System Watchdog Check / notify
//***************************************************************************

void cSystemNotification::check()
{
   if (interval && lastWatchdogAt <= time(0) - interval)
   {
      // notify all 'n' seconds
      
      notify(evKeepalive);
      lastWatchdogAt = time(0);
   }
}

//***************************************************************************
// Start / Stop notification thread
//***************************************************************************

int cSystemNotification::startNotifyThread(int timeout)
{
   threadTimeout = timeout;
   Start();
   return success;
}

int cSystemNotification::stopNotifyThread()
{
   waitCondition.Broadcast(); 
   Cancel(1);
   return success;
}

//***************************************************************************
// action
//***************************************************************************

void cSystemNotification::action()
{
   cMyMutex mutex;
   time_t timeoutAt = time(0) + threadTimeout;

   mutex.Lock();

   while (time(0) < timeoutAt && Running())
   {
      waitCondition.TimedWait(mutex, interval*1000);
      // tell(0, "loop ...");
      check();
   }

   if (time(0) >= timeoutAt)
      tell(0, "Warning: Ending notification thread, timeout reached!");
}

//***************************************************************************
// Notify System Deamon :o :(
//***************************************************************************

int cSystemNotification::notify(int event, const char* format, ...)
{
#ifdef USESYSD
   char* message;
   char* tmp;
   va_list ap;

   if (isEmpty(format))
      format = "";

   va_start(ap, format);
   vasprintf(&tmp, format, ap);

   switch (event)
   {
      case evStatus:    asprintf(&message, "%s", tmp);               break;
      case evStopping:  asprintf(&message, "STOPPING=1\n%s", tmp);   break;
      case evReady:     asprintf(&message, "READY=1\nSTATUS=Ready\nMAINPID=%d\n%s", getpid(), tmp);  break;
      case evKeepalive: asprintf(&message, "WATCHDOG=1\n%s", tmp);   break;
   }

   tell(event == evKeepalive ? 2 : 1, "Calling sd_notify(%s)", message);
   sd_notify(0, message);

   free(tmp);
   free(message);
#endif

   // handle pidfile at evReady / evStopping

   if (!isEmpty(pidfile))
   {
      if (event == evReady)      
      {
         FILE* f = fopen(pidfile, "w");
         
         if (f)
         {
            pid_t pid = getpid();
            tell(1, "Creating pidfile '%s'; pid %d", pidfile, pid);
            fprintf(f, "%d\n", pid);
            fclose(f);
         }
         else
            tell(0, "Error: Can't create pid file '%s' error was (%d) '%s'", 
                 pidfile, errno, strerror(errno));
      }
      else if (event == evStopping)
      {
         if (fileExists(pidfile))
         {
            tell(1, "Removing pidfile '%s'", pidfile);
            removeFile(pidfile);
         }
      }
   }

   return done;
}

//***************************************************************************
// 
//***************************************************************************

int cSystemNotification::getWatchdogState(int minInterval)
{
   interval = 0;

#ifdef USESYSD

#  ifdef SYSDWDIFO
   uint64_t us = 0;
   int sec;

   if (sd_watchdog_enabled(0, &us) > 0 && us > 0)
   {
      sec = us / tmeUsecondsPerSecond;

      if (sec < minInterval*2)
      {
         tell(0, "Warning: Systemd watchdog configured to (%ds) but min %ds are required,"
              "  ignoring watchdog", sec, minInterval * 2);
         return no;
      }

      interval = sec / 2;

      tell(0, "Info: Systemd watchdog request interval"
           " of at least(%ds), using (%ds) now!", sec, interval);

      return yes;
   }

   tell(0, "Info: Systemd watchdog not configured, don't sending notifications!");

#  else
   interval = defaultInterval;
   return yes;
#  endif

#endif

   tell(0, "Info: Systemd support not enabled, don't sending notifications!");

   return no;
}
