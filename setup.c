
#include "lib/config.h"

#include "setup.h"

extern cScraper2VdrConfig config;

cScraper2VdrSetup::cScraper2VdrSetup(cUpdate *update) {
    this->update = update;
    tmpConfig = config;
    strn0cpy(host, tmpConfig.mysqlHost.c_str(), sizeof(host));
    strn0cpy(dbname, tmpConfig.mysqlDBName.c_str(), sizeof(dbname));
    strn0cpy(user, tmpConfig.mysqlDBUser.c_str(), sizeof(user));
    strn0cpy(password, tmpConfig.mysqlDBPass.c_str(), sizeof(password));
    Setup();
}

cScraper2VdrSetup::~cScraper2VdrSetup() {
}


void cScraper2VdrSetup::Setup(void) {
    int currentItem = Current();
    Clear();
    
    Add(new cMenuEditBoolItem(tr("Show Main Menu Entry"), &tmpConfig.mainMenuEntry));
    Add(new cMenuEditBoolItem(tr("Debug Mode"), &tmpConfig.debug));
    Add(new cMenuEditBoolItem(tr("Fast Mode"), &tmpConfig.fastmode));
    Add(new cMenuEditStrItem(tr("MySQL Host"), host, sizeof(host), tr(FileNameChars)));
    Add(new cMenuEditIntItem(tr("MySQL Port"), &tmpConfig.mysqlPort, 1, 99999));
    Add(new cMenuEditStrItem(tr("MySQL Database Name"), dbname, sizeof(dbname), tr(FileNameChars)));
    Add(new cMenuEditStrItem(tr("MySQL User"), user, sizeof(user), tr(FileNameChars)));
    Add(new cMenuEditStrItem(tr("MySQL Password"), password, sizeof(password), tr(FileNameChars)));

    Add(new cOsdItem(tr("Update Scraper Information from Database")));
    Add(new cOsdItem(tr("Update Scraper Recordings Information from Database")));
    Add(new cOsdItem(tr("Scan for new recordings in video directory")));
    Add(new cOsdItem(tr("Scan for new or updated scrapinfo files")));
    Add(new cOsdItem(tr("Cleanup Recordings in Database")));

    SetCurrent(Get(currentItem));
    Display();
}

eOSState cScraper2VdrSetup::ProcessKey(eKeys Key) {
    // bool hadSubMenu = HasSubMenu();   
    eOSState state = cMenuSetupPage::ProcessKey(Key);
    if (Key == kOk) {
        tmpConfig.mysqlHost = host;
        tmpConfig.mysqlDBName = dbname;
        tmpConfig.mysqlDBUser = user;
        tmpConfig.mysqlDBPass = password;
        Store();
        if (Current() == 8) {
            Skins.Message(mtInfo, tr("Updating Scraper EPG Information from Database"));
            update->ForceUpdate();
        } else if (Current() == 9) {
            Skins.Message(mtInfo, tr("Updating Scraper Recordings Information from Database"));
            update->ForceRecordingUpdate();
        } else if (Current() == 10) {
            Skins.Message(mtInfo, tr("Scanning for new recordings in video directory"));
            update->ForceVideoDirUpdate();
        } else if (Current() == 11 ) {
            Skins.Message(mtInfo, tr("Scanning for new or updated scrapinfo files"));
            update->ForceScrapInfoUpdate();
        } else if (Current() == 12) {
            Skins.Message(mtInfo, tr("Cleaning up Recordings in Database"));
            update->TriggerCleanRecordingsDB();
        }
        return osEnd;
    }
    return state;
}

void cScraper2VdrSetup::Store(void) {
    config = tmpConfig;
    SetupStore("mainMenuEntry", tmpConfig.mainMenuEntry);
    SetupStore("mysqlHost", tmpConfig.mysqlHost.c_str());
    SetupStore("mysqlPort", tmpConfig.mysqlPort);
    SetupStore("mysqlDBName", tmpConfig.mysqlDBName.c_str());
    SetupStore("mysqlDBUser", tmpConfig.mysqlDBUser.c_str());
    SetupStore("mysqlDBPass", tmpConfig.mysqlDBPass.c_str());
    SetupStore("debug", tmpConfig.debug);
    SetupStore("fastmode", tmpConfig.fastmode);

    EPG2VDRConfig.loglevel = tmpConfig.debug ? 2 : 1;
}
