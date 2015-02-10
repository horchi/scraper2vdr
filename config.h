#ifndef __SCRAPER2VDR_CONFIG_H
#define __SCRAPER2VDR_CONFIG_H

#include <string>
#include <vdr/plugin.h>

using namespace std;

class cScraper2VdrConfig {
    private:
    public:
        cScraper2VdrConfig();
        ~cScraper2VdrConfig();
        bool SetupParse(const char *Name, const char *Value);
        void SetUuid(cPlugin *plug);
        void SetImageDir(cString dir);
        void SetDefaultImageDir(void);
        void SetMode(string mode);
        int mainMenuEntry;
        bool headless;
        string uuid;
        bool imgDirSet;
        string imageDir;
        string mysqlHost;
        int mysqlPort;
        string mysqlDBName;
        string mysqlDBUser;
        string mysqlDBPass;
        string recScrapInfoName;
        int debug;
	int fastmode;
};
#endif //__SCRAPER2VDR_CONFIG_H
