#ifndef __SCRAPER2VDR_SETUP_H
#define __SCRAPER2VDR_SETUP_H

#include <vdr/menuitems.h>
#include "update.h"
#include "config.h"

class cScraper2VdrSetup : public cMenuSetupPage {
    public:
        cScraper2VdrSetup(cUpdate *update);
        virtual ~cScraper2VdrSetup();       
    private:
        cUpdate *update;
        cScraper2VdrConfig tmpConfig;
        char host[256];
        char dbname[256];
        char user[256];
        char password[256];
        void Setup(void);
    protected:
        virtual eOSState ProcessKey(eKeys Key);
        virtual void Store(void);

};
#endif //__SCRAPER2VDR_SETUP_H