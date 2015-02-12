
#include "plgconfig.h"
#include "setup.h"

cScraper2VdrSetup::cScraper2VdrSetup(cUpdate* update) {
    this->update = update;
    tmpConfig = scraper2VdrConfig;

    strn0cpy(host, tmpConfig.dbHost, sizeof(host));
    strn0cpy(dbname, tmpConfig.dbName, sizeof(dbname));
    strn0cpy(user, tmpConfig.dbUser, sizeof(user));
    strn0cpy(password, tmpConfig.dbPass, sizeof(password));

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
    Add(new cMenuEditBoolItem(tr("Fast Mode"), &tmpConfig.fastmode));
<<<<<<< HEAD
=======
    Add(new cMenuEditIntItem(tr("Thumbnail height"), &tmpConfig.thumbHeight));
    Add(new cMenuEditStrItem(tr("MySQL Host"), host, sizeof(host), tr(FileNameChars)));
    Add(new cMenuEditIntItem(tr("MySQL Port"), &tmpConfig.mysqlPort, 1, 99999));
    Add(new cMenuEditStrItem(tr("MySQL Database Name"), dbname, sizeof(dbname), tr(FileNameChars)));
    Add(new cMenuEditStrItem(tr("MySQL User"), user, sizeof(user), tr(FileNameChars)));
    Add(new cMenuEditStrItem(tr("MySQL Password"), password, sizeof(password), tr(FileNameChars)));
>>>>>>> aaad56f9cd289fc59883a4580b99a1fd337f80b7

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
    Add(new cOsdItem(tr("Cleanup Recordings in Database")));
    Add(new cOsdItem(tr("Reload all values (Series, Movies and Images)")));

    SetCurrent(Get(currentItem));
    Display();
}

eOSState cScraper2VdrSetup::ProcessKey(eKeys Key) {
    // bool hadSubMenu = HasSubMenu();   
    eOSState state = cMenuSetupPage::ProcessKey(Key);
    if (Key == kOk) {
<<<<<<< HEAD
       strn0cpy(tmpConfig.dbHost, host, sizeof(host));
       strn0cpy(tmpConfig.dbName, dbname, sizeof(dbname));
       strn0cpy(tmpConfig.dbUser, user, sizeof(user));
       strn0cpy(tmpConfig.dbPass, password, sizeof(password));

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
=======
        tmpConfig.mysqlHost = host;
        tmpConfig.mysqlDBName = dbname;
        tmpConfig.mysqlDBUser = user;
        tmpConfig.mysqlDBPass = password;
        Store();
        if (Current() == 9) {
            Skins.Message(mtInfo, tr("Updating Scraper EPG Information from Database"));
            update->ForceUpdate();
        } else if (Current() == 10) {
            Skins.Message(mtInfo, tr("Updating Scraper Recordings Information from Database"));
            update->ForceRecordingUpdate();
        } else if (Current() == 11) {
            Skins.Message(mtInfo, tr("Scanning for new recordings in video directory"));
            update->ForceVideoDirUpdate();
        } else if (Current() == 12 ) {
            Skins.Message(mtInfo, tr("Scanning for new or updated scrapinfo files"));
            update->ForceScrapInfoUpdate();
        } else if (Current() == 13) {
            Skins.Message(mtInfo, tr("Cleaning up Recordings in Database"));
            update->TriggerCleanRecordingsDB();
        } else if (Current() == 14) {
            Skins.Message(mtInfo, tr("Loading Series, Movies and Images from Database"));
            update->ForceFullUpdate();
        }
        return osEnd;
>>>>>>> aaad56f9cd289fc59883a4580b99a1fd337f80b7
    }
    return state;
}

void cScraper2VdrSetup::Store(void) {
    scraper2VdrConfig = tmpConfig;
    SetupStore("mainMenuEntry", tmpConfig.mainMenuEntry);
    SetupStore("DbHost", tmpConfig.dbHost);
    SetupStore("DbPort", tmpConfig.dbPort);
    SetupStore("DbName", tmpConfig.dbName);
    SetupStore("DbUser", tmpConfig.dbUser);
    SetupStore("DbPass", tmpConfig.dbPass);
    SetupStore("fastmode", tmpConfig.fastmode);
<<<<<<< HEAD
    SetupStore("LogLevel", tmpConfig.loglevel);
=======
    SetupStore("thumbHeight", tmpConfig.thumbHeight);

    EPG2VDRConfig.loglevel = tmpConfig.debug ? 2 : 1;
>>>>>>> aaad56f9cd289fc59883a4580b99a1fd337f80b7
}
