#include "plgconfig.h"
#include "setup.h"

using namespace std;

cScraper2VdrSetup::cScraper2VdrSetup(cUpdate* update) {
    this->update = update;
    tmpConfig = scraper2VdrConfig;
    Setup();
}

cScraper2VdrSetup::~cScraper2VdrSetup() {
}

void cScraper2VdrSetup::Setup(void) {
    char* buf;
    int currentItem = Current();
    Clear();

    asprintf(&buf, "-------------------- %s ----------------------------------", tr("General"));
    Add(new cOsdItem(buf));
    free(buf);
    cList<cOsdItem>::Last()->SetSelectable(false);
    Add(new cMenuEditBoolItem(tr("Show Main Menu Entry"), &tmpConfig.mainMenuEntry));
    Add(new cMenuEditIntItem(tr("Thumbnail Height"), &tmpConfig.thumbHeight));
    Add(new cMenuEditBoolItem(tr("Fixed Poster Size (resize,stretch,crop)"), &tmpConfig.useFixPosterSize));
    Add(new cMenuEditIntItem(tr("Poster Width (default 500)"), &tmpConfig.fixPosterWidth));
    Add(new cMenuEditIntItem(tr("Poster Height (default 735)"), &tmpConfig.fixPosterHeight));
    Add(new cMenuEditIntItem(tr("Season Poster Width (default 400)"), &tmpConfig.fixSeasonPosterWidth));
    Add(new cMenuEditIntItem(tr("Season Poster Height (default 578)"), &tmpConfig.fixSeasonPosterHeight));
    Add(new cMenuEditIntItem(tr("Max. Poster Distortion (in %)"), &tmpConfig.maxPosterDistortion));

    asprintf(&buf, "--------------------- %s ---------------------------------", tr("MySQL"));   
    Add(new cOsdItem(buf));
    free(buf);
    cList<cOsdItem>::Last()->SetSelectable(false);
    Add(new cMenuEditStrItem(tr("Host"), tmpConfig.dbHost, sizeof(tmpConfig.dbHost), tr(FileNameChars)));
    Add(new cMenuEditIntItem(tr("Port"), &tmpConfig.dbPort, 1, 99999));
    Add(new cMenuEditStrItem(tr("Database Name"), tmpConfig.dbName, sizeof(tmpConfig.dbName), tr(FileNameChars)));
    Add(new cMenuEditStrItem(tr("User"), tmpConfig.dbUser, sizeof(tmpConfig.dbUser), tr(FileNameChars)));
    Add(new cMenuEditStrItem(tr("Password"), tmpConfig.dbPass, sizeof(tmpConfig.dbPass), tr(FileNameChars)));

    asprintf(&buf, "--------------------- %s ---------------------------------", tr("Technical Stuff"));
    Add(new cOsdItem(buf));
    free(buf);
    cList<cOsdItem>::Last()->SetSelectable(false);
    Add(new cMenuEditIntItem(tr("Log level"), &tmpConfig.loglevel, 0, 4));
    
    asprintf(&buf, "--------------------- %s ---------------------------------", tr("Actions"));   
    Add(new cOsdItem(buf));
    free(buf);
    cList<cOsdItem>::Last()->SetSelectable(false);
    Add(new cOsdItem(tr("Update Scraper Information from Database")));
    Add(new cOsdItem(tr("Update Scraper Recordings Information from Database")));
    Add(new cOsdItem(tr("Scan for new recordings in video directory")));
    Add(new cOsdItem(tr("Scan for new or updated scrapinfo files")));
    // Add(new cOsdItem(tr("Cleanup Recordings in Database")));
    Add(new cOsdItem(tr("Reload all values (Series, Movies and Images)")));

    SetCurrent(Get(currentItem));
    Display();
}

eOSState cScraper2VdrSetup::ProcessKey(eKeys Key) {
    eOSState state = cMenuSetupPage::ProcessKey(Key);
    switch (state) 
    {
        case osContinue:
        {
            if (NORMALKEY(Key) == kUp || NORMALKEY(Key) == kDown) {
                cOsdItem* item = Get(Current());

                if (item)
                    item->ProcessKey(kNone);
            }    
            break;
        }
        default:
        {
            if ((NORMALKEY(Key) == kOk) && (Current()>=18) && (Current()<=23)) {
                // handle ok for action entries
                Store();
                if (Current() == 18) {
                    Skins.Message(mtInfo, tr("Updating Scraper EPG Information from Database"));
                    update->ForceUpdate();
                } else if (Current() == 19) {
                    Skins.Message(mtInfo, tr("Updating Scraper Recordings Information from Database"));
                    update->ForceRecordingUpdate();
                } else if (Current() == 20) {
                    Skins.Message(mtInfo, tr("Scanning for new recordings in video directory"));
                    update->ForceVideoDirUpdate();
                } else if (Current() == 21 ) {
                    Skins.Message(mtInfo, tr("Scanning for new or updated scrapinfo files"));
                    update->ForceScrapInfoUpdate();
//                 } else if (Current() == 22) {
//                     Skins.Message(mtInfo, tr("Cleaning up Recordings in Database"));
//                     update->TriggerCleanRecordingsDB();
                } else if (Current() == 22) {
                    Skins.Message(mtInfo, tr("Loading Series, Movies and Images from Database"));
                    update->ForceFullUpdate();
                }
                
                return osEnd; // close setup
            }            
            break;
        }    
    }
   
    return state;
}

void cScraper2VdrSetup::Store(void) {
    if (tmpConfig.hasDbLoginChanged(&scraper2VdrConfig)) {
       update->ForceReconnect();
    }
    scraper2VdrConfig = tmpConfig;

    SetupStore("mainMenuEntry", tmpConfig.mainMenuEntry);
    SetupStore("DbHost", tmpConfig.dbHost);
    SetupStore("DbPort", tmpConfig.dbPort);
    SetupStore("DbName", tmpConfig.dbName);
    SetupStore("DbUser", tmpConfig.dbUser);
    SetupStore("DbPass", tmpConfig.dbPass);
    SetupStore("LogLevel", tmpConfig.loglevel);
    SetupStore("thumbHeight", tmpConfig.thumbHeight);
    SetupStore("useFixPosterSize", tmpConfig.useFixPosterSize);
    SetupStore("fixPosterWidth", tmpConfig.fixPosterWidth);
    SetupStore("fixPosterHeight", tmpConfig.fixPosterHeight);
    SetupStore("fixSeasonPosterWidth", tmpConfig.fixSeasonPosterWidth);
    SetupStore("fixSeasonPosterHeight", tmpConfig.fixSeasonPosterHeight);
    SetupStore("maxPosterDistortion", tmpConfig.maxPosterDistortion);
}
