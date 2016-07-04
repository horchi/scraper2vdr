#ifndef __SCRAPER2VDR_H
#define __SCRAPER2VDR_H

#include <getopt.h>

#include <vdr/plugin.h>

#include "lib/common.h"

#include "setup.h"
#include "scrapmanager.h"
#include "update.h"
#include "services.h"
#include "HISTORY.h"

//***************************************************************************
// Constants
//***************************************************************************

static const char *DESCRIPTION    = "'scraper2vdr' plugin";
static const char *MAINMENUENTRY  = "Scraper2Vdr";

//***************************************************************************
// cPluginScraper2vdr
//***************************************************************************

class cPluginScraper2vdr : public cPlugin {
private:
    int initExitDbConnection(MysqlInitExitAction action);
    cScrapManager *scrapManager;
    cUpdate *update;
public:
    cPluginScraper2vdr(void);
    virtual ~cPluginScraper2vdr();
    virtual const char *Version(void)      { return VERSION; }
    virtual const char *Description(void)  { return DESCRIPTION; }
    virtual const char *CommandLineHelp(void);
    virtual bool ProcessArgs(int argc, char *argv[]);
    virtual bool Initialize(void);
    virtual bool Start(void);
    virtual void Stop(void);
    virtual void Housekeeping(void);
    virtual void MainThreadHook(void);
    virtual cString Active(void);
    virtual time_t WakeupTime(void);
    virtual const char *MainMenuEntry(void) { return scraper2VdrConfig.mainMenuEntry ? MAINMENUENTRY : NULL; }
    virtual cOsdObject *MainMenuAction(void);
    virtual cMenuSetupPage *SetupMenu(void);
    virtual bool SetupParse(const char *Name, const char *Value);
    virtual bool Service(const char *Id, void *Data = NULL);
    virtual const char **SVDRPHelpPages(void);
    virtual cString SVDRPCommand(const char *Command, const char *Option, int &ReplyCode);
};

//***************************************************************************
#endif // __SCRAPER2VDR_H
