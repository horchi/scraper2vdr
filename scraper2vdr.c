/*
 * scraper2vdr.c: A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */

#include "lib/config.h"

#include "scraper2vdr.h"

const char* logPrefix = LOG_PREFIX;

#if defined (APIVERSNUM) && (APIVERSNUM < 10600)
# error VDR API versions < 1.6.0 are not supported !
#endif

//***************************************************************************
// Plugin Main Menu
//***************************************************************************

class cScraper2VdrPluginMenu : public cOsdMenu {
    public:
        cScraper2VdrPluginMenu(const char* title, cUpdate *update);
        virtual ~cScraper2VdrPluginMenu() { };
        virtual eOSState ProcessKey(eKeys key);
    protected:
        cUpdate *update;
};

cScraper2VdrPluginMenu::cScraper2VdrPluginMenu(const char* title, cUpdate *update) : cOsdMenu(title) {
    this->update = update;
    Clear();
    cOsdMenu::Add(new cOsdItem(tr("Update Scraper Information from Database")));
    cOsdMenu::Add(new cOsdItem(tr("Update Scraper Recordings Information from Database")));
    cOsdMenu::Add(new cOsdItem(tr("Scan for new recordings in video directory")));
    cOsdMenu::Add(new cOsdItem(tr("Scan for new or updated scrapinfo files")));
    cOsdMenu::Add(new cOsdItem(tr("Cleanup Recordings in Database")));
    SetHelp(0, 0, 0,0);
    Display();
}

//***************************************************************************
// Process Key
//***************************************************************************

eOSState cScraper2VdrPluginMenu::ProcessKey(eKeys key) {
   eOSState state = cOsdMenu::ProcessKey(key);

    if (state != osUnknown)
        return state;

    switch (key) {
        case kOk: {
            if (Current() == 0) {
                Skins.Message(mtInfo, tr("Updating Scraper EPG Information from Database"));
                update->ForceUpdate();
            } else if (Current() == 1) {
                Skins.Message(mtInfo, tr("Updating Scraper Recordings Information from Database"));
                update->ForceRecordingUpdate();
            } else if (Current() == 2) {
                Skins.Message(mtInfo, tr("Scanning for new recordings in video directory"));
                update->ForceVideoDirUpdate();
            } else if (Current() == 3) {
                Skins.Message(mtInfo, tr("Scanning for new or updated scrapinfo files"));
                update->ForceScrapInfoUpdate();
            } else if (Current() == 4) {
                Skins.Message(mtInfo, tr("Cleaning up Recordings in Database"));
                update->TriggerCleanRecordingsDB();
            }
            return osEnd;
        }

        default:
            break;
    }
    return state;
}

//***************************************************************************
// cPluginScraper2vdr
//***************************************************************************

cPluginScraper2vdr::cPluginScraper2vdr(void) {
    update = NULL;
    scrapManager = NULL;
    cDbConnection::init(EPG_PLUGIN_SEM_KEY);
}

cPluginScraper2vdr::~cPluginScraper2vdr() {
    delete update;
    delete scrapManager;
    cDbConnection::exit();
}

const char *cPluginScraper2vdr::CommandLineHelp(void) {
    return
      "  -i <IMAGEDIR>, --imagedir=<IMAGEDIR> Set directory where images are stored\n"
      "  -m <MODE>, --mode=<MODE> mode can be client or headless, see README\n";
}

bool cPluginScraper2vdr::ProcessArgs(int argc, char *argv[]) {
    static const struct option long_options[] = {
        { "imagedir", required_argument, NULL, 'i' },
        { "mode", required_argument, NULL, 'm' },
        { 0, 0, 0, 0 }
    };
    int c;
    while ((c = getopt_long(argc, argv, "i:m:", long_options, NULL)) != -1) {
        switch (c) {
            case 'i':
                config.SetImageDir(optarg);
                break;
            case 'm':
                config.SetMode(optarg);
                break;
            default:
                return false;
        }
    }
    return true;
}

bool cPluginScraper2vdr::Initialize(void) {
    config.SetUuid(this);
    config.SetDefaultImageDir();
    scrapManager = new cScrapManager();
    update = new cUpdate(scrapManager);
    return true;
}

bool cPluginScraper2vdr::Start(void) {
    update->Start();
    return true;
}

void cPluginScraper2vdr::Stop(void) {
    update->Stop();
}

void cPluginScraper2vdr::Housekeeping(void) {
}

void cPluginScraper2vdr::MainThreadHook(void) {
}

cString cPluginScraper2vdr::Active(void) {
    return NULL;
}

time_t cPluginScraper2vdr::WakeupTime(void) {
    return 0;
}

cOsdObject *cPluginScraper2vdr::MainMenuAction(void) {
    return new cScraper2VdrPluginMenu("Scraper2Vdr", update);
}

cMenuSetupPage *cPluginScraper2vdr::SetupMenu(void) {
    return new cScraper2VdrSetup(update);
}

bool cPluginScraper2vdr::SetupParse(const char *Name, const char *Value) {
    return config.SetupParse(Name, Value);
}

bool cPluginScraper2vdr::Service(const char *Id, void *Data) {
    if (Data == NULL)
        return false;
    if (strcmp(Id, "GetEventType") == 0) {
        ScraperGetEventType* call = (ScraperGetEventType*) Data;
        if (!call->event && !call->recording)
            return false;
        return scrapManager->GetEventType(call);
    }

    if (strcmp(Id, "GetSeries") == 0) {
        cSeries* call = (cSeries*) Data;
        if (call->seriesId == 0)
            return false;
        return scrapManager->GetSeries(call);
    }

    if (strcmp(Id, "GetMovie") == 0) {
        cMovie* call = (cMovie*) Data;
        if (call->movieId == 0)
            return false;
        return scrapManager->GetMovie(call);
    }

    if (strcmp(Id, "GetPosterBanner") == 0) {
        ScraperGetPosterBanner* call = (ScraperGetPosterBanner*) Data;
        if (!call->event) {
            return false;
        }
        return scrapManager->GetPosterBanner(call);
    }

    if (strcmp(Id, "GetPosterBannerV2") == 0) {
        ScraperGetPosterBannerV2* call = (ScraperGetPosterBannerV2*) Data;
        if (!call->event && !call->recording) {
            return false;
        }
        return scrapManager->GetPosterBannerV2(call);
    }

    if (strcmp(Id, "GetPoster") == 0) {
        ScraperGetPoster* call = (ScraperGetPoster*) Data;
        if (!call->event && !call->recording)
            return false;
        return scrapManager->GetPoster(call);
    }

    if (strcmp(Id, "GetPosterThumb") == 0) {
        ScraperGetPosterThumb* call = (ScraperGetPosterThumb*) Data;
        if (!call->event && !call->recording)
            return false;
        return scrapManager->GetPosterThumb(call);  
    }

    return false;
}

const char **cPluginScraper2vdr::SVDRPHelpPages(void) {
    static const char *HelpPages[] = {
      "UPDT\n"
      "    Load all series and movies for events from database.",
      "UPDR\n"
      "    Load recordings from database.",
      "SCVD\n"
      "    Trigger scan fornew recordings in video directory.",
      "SCSI\n"
      "    Trigger scan for scrapinfo files in video directory.",
      "CRDB\n"
      "    Trigger cleanup of recordings database.",
      "DSER\n"
      "    Dump series kept in memory.",
      "DMOV\n"
      "    Dump movies kept in memory.",
      "DREC\n"
      "    Dump recordings kept in memory.",
      0
    };
    return HelpPages;
}

cString cPluginScraper2vdr::SVDRPCommand(const char *Command, const char *Option, int &ReplyCode) {
    if (strcasecmp(Command, "UPDT") == 0) {
        update->ForceUpdate();
        return "SCRAPER2VDR full update from database forced.";
    } else if (strcasecmp(Command, "UPDR") == 0) {
        update->ForceRecordingUpdate();
        return "SCRAPER2VDR scanning of recordings in database triggered.";
    } else if (strcasecmp(Command, "SCVD") == 0) {
        update->ForceVideoDirUpdate();
        return "SCRAPER2VDR scan for new recordings in video dir triggered.";
    } else if (strcasecmp(Command, "SCSI") == 0) {
        update->ForceScrapInfoUpdate();
        return "SCRAPER2VDR scan for new or updated scrapinfo files triggered.";
    } else if (strcasecmp(Command, "CRDB") == 0) {
        update->TriggerCleanRecordingsDB();
        return "SCRAPER2VDR cleanup of recording DB triggered.";
    } else if (strcasecmp(Command, "DSER") == 0) {
        scrapManager->DumpSeries();
        return "SCRAPER2VDR dumping available series";
    } else if (strcasecmp(Command, "DMOV") == 0) {
        scrapManager->DumpMovies();
        return "SCRAPER2VDR dumping available movies";
    } else if (strcasecmp(Command, "DREC") == 0) {
        scrapManager->DumpRecordings();
        return "SCRAPER2VDR dumping available recordings";
    }
    return NULL;
}

VDRPLUGINCREATOR(cPluginScraper2vdr); // Don't touch this!
