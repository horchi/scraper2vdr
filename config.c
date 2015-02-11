#include "lib/common.h"
#include "config.h"

cScraper2VdrConfig::cScraper2VdrConfig() {
    mainMenuEntry = 1;
    headless = false;
    uuid = "";
    imgDirSet = false;
    mysqlHost = "localhost";
    mysqlPort = 3306;
    mysqlDBName = "epg2vdr";
    mysqlDBUser = "epg2vdr";
    mysqlDBPass = "epg";
    recScrapInfoName = "scrapinfo";
    debug = 0;
    fastmode = 1;
    thumbHeight = 200;
}

cScraper2VdrConfig::~cScraper2VdrConfig() {
}

void cScraper2VdrConfig::SetUuid(cPlugin *plug) {
    if (uuid.size() == 0) {
        uuid = getUniqueId();
        plug->SetupStore("uuid", uuid.c_str());
    }
    tell(0, "epgd uuid: %s", uuid.c_str());
}

void cScraper2VdrConfig::SetImageDir(cString dir) {
    imageDir = *dir;
    imgDirSet = true;
}

void cScraper2VdrConfig::SetDefaultImageDir(void) {
    if (!imgDirSet) {
        imageDir = cPlugin::CacheDirectory(PLUGIN_NAME_I18N);
    }
    tell (0, "using image directory %s", imageDir.c_str());
}

void cScraper2VdrConfig::SetMode(string mode) {
    if (!mode.compare("headless"))
        headless = true;
}

bool cScraper2VdrConfig::SetupParse(const char *Name, const char *Value) {
    if      (strcmp(Name, "uuid") == 0)               uuid = Value;
    else if (strcmp(Name, "mainMenuEntry") == 0)      mainMenuEntry = atoi(Value);
    else if (strcmp(Name, "mysqlHost") == 0)          mysqlHost = Value;
    else if (strcmp(Name, "mysqlPort") == 0)          mysqlPort = atoi(Value);
    else if (strcmp(Name, "mysqlDBName") == 0)        mysqlDBName = Value;
    else if (strcmp(Name, "mysqlDBUser") == 0)        mysqlDBUser = Value;
    else if (strcmp(Name, "mysqlDBPass") == 0)        mysqlDBPass = Value;
    else if (strcmp(Name, "debug") == 0)              debug = atoi(Value);
    else if (strcmp(Name, "fastmode") == 0)           fastmode = atoi(Value);
    else if (strcmp(Name, "thumbHeight") == 0)        thumbHeight = atoi(Value);
    else
        return false;
    return true;
}
