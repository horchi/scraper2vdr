/*
 * -----------------------------------
 * scraper2vdr Plugin - Revision History
 * -----------------------------------
 *
 */

#define _VERSION     "1.0.9"
#define VERSION_DATE "04.01.2018"
#define DB_API       5

#ifdef GIT_REV
#  define VERSION _VERSION "-GIT" GIT_REV
#else
#  define VERSION _VERSION
#endif

/*
 * ------------------------------------

2018-01-04: version 1.0.9 (horchi)
   - added: service to query plugins environment (the resource path config)

2017-12-25: version 1.0.8 (horchi)
   - change: Updated DB_API

2017-12-22: version 1.0.7 (horchi)
   - change: Fixed handling of service calls

2017-12-21: version 1.0.6 (horchi)
   - change: gcc 7 porting

2017-06-11: version 1.0.5 (horchi)
   - change: Porting to VDR 2.3.7

2017-05-29: version 1.0.4 (horchi)
   - change: vdr 2.3.4 porting

2016-12-05: version 1.0.3 (horchi)
   - change: changed Makefile for make -jN (by 3po)

2016-10-05: version 1.0.2 (horchi)
   - bugfix: fixed timing of retry loop

2016-08-26: version 1.0.1 (horchi)
   - added: support of long eventids for tvsp (merged dev into master)

2016-07-04: version 1.0.0 (horchi)
   - change: merged http branch into master

2016-05-20: version 0.1.25 (horchi)
   - change: minor logging change

2016-05-20: version 0.1.24 (horchi)
   - added:  new column for textual rating and commentator
   - change: removed unused info column
   - change: to actual libhorchi

2016-05-10: version 0.1.23 (horchi)
   - change: Fixed group by statement (patch by ckone)

 Version 0.1.21 (horchi)
   - changed log level for field filter to 2

 Version 0.1.20 (horchi)
   - changed removed scrapinfo file support, using info.epg2vdr via epg2vdr plugin instead
           -> move your settings to info.epg2vdr like:
           SCRINFOMOVIEID = 22222
           SCRINFOSERIESID = 23454
           SCRINFOEPISODEID = 23424

 Version 0.1.19 (horchi)
   - changed more compatibility to graphicsmagick

 Version 0.1.18 (horchi)
   - changed fixed wrong fieldname

 Version 0.1.17 (horchi)
   - changed log messages

 Version 0.1.16 (horchi)
   - changed recording path (now without base path)
   - added service to register mysql_lib_init

 Version 0.1.15 (horchi)
   - started http branch

 Version 0.1.14 (horchi)
   - porting to vdr 2.3.1


 Version 0.1.13 (horchi)
   - fixed format string handling at call of deleteWhere
   - added format string to deleteWhere to avoid buggy format strings
   - update table recordinglist for use by epghttpd (maybe even for later replacement of recordings table)

-------------------------
------ old history ------
-------------------------

2014-03-02: Version 0.0.1

- Initial revision.

Version 0.1.0

- some performance optimizations
- fixed recordings with path size longer that 200 chars
- improved thread handling
- added more debugging possibilities
- fixed bug that scraped events are not displayed randomly

Version 0.1.1
- ...

Version 0.1.2
- update of db api
- prepare statements only once
- added statement statistic
- minor changes
- changed mail loop timing
- added path detection for libmysql
- fixed epgd state detection
- added cppcheck to Makefile

Version 0.1.3
- fixed a bug that series meta data is not loaded completely
- fixed crash during shutdown of plugin
- fixed escaping when deleting outdated recordings

Version 0.1.4
- added ScraperGetPosterBannerV2 Service

Version 0.1.5
- introduced new DB API

Version 0.1.6
- removed outdated field

2015-01-10
- first test version for new DB parser. Currently only series are supported, download of movie data is disabled
- new DB parser should be much faster (only 1/3 of SQL commands)
- with the new DB parser it's now possible to use epgd/scraper2vdr over slow tcp connections (like slow WLAN 802.11b),
because there is much less traffic (binary image data get only copied if image should get updated/loaded)

Version 0.1.7
- first test version for new DB parser with full series and movie support
- thumbnail size is now configurable in setup
  should be set to height which is used by skin to optimze performance of skin (no scaling of thumbs neccessarry)
- full update of series, movies and image files can be triggered from menu/setup
  this will load existing images also (thumbs get created with new size)
- prevent logging of unneccessarry messages (e.g. found nothing to update)
- DB API and LIB porting
- apply new SQL-DB settings without restart of vdr
- changed log-level values:
  0 - only SQL-Connection messages (connected/disconnected...) and errors
  1 - add status messages
  2 - add debug messages (missing images, sql commands...)

Version 0.1.8
- modified makefile to be compatible to launchpad
- add backward compatibility to epgd without recordings.scrsp field (but update to current version is recommended)
- init new setup parameters with values of scraper2vdr < 0.1.7 (Mysql...), so no user actions are necessary after update to scraper2vdr >= 0.1.7

Version 0.1.9
- fixed crash when reconnect to DB (introduced during add backward compatibility)

Version 0.1.10
- setup now react to ok/return keys like other plugins (do not close setup when press ok during edit string parameter)

Version 0.1.11
- add fixed poster size (enabled/defined in setup)
  if is enabled, all posters get stretched/scaled/croped to defined size
  because of that all thumbnails will have the same size also (using definend thumbnail height)
  this is a enhancement for skin, because now they can calculate with a fixed poster size and aspect ratio
  Warning, this functionality need more CPU power. But the CPU load should not reach problematic levels, because a sleep is used periodically.

Version 0.1.12
- fixed wrong poster thumbnail width in service calls (introduced in 0.1.11)
  this was only a visibility problem, the stored files are ok
- removed option for using new DB parser
- removed all code of old DB parser

 * ------------------------------------
 */
