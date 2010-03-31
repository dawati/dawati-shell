/*
 * Moblin-Panel-Web: The web panel for Moblin
 * 
 * Chromium browser APIs wrapper
 */

#include <time.h>
#include <algorithm>
#include <string>
#include <bitset>

#define NVALGRIND

#include <glib.h>
#include <gio/gio.h>
#include <stdio.h>
#include "chrome/browser/browser_process.h"
#include "chrome/browser/thumbnail_store.h"
#include "chrome/browser/history/thumbnail_database.h"
#include "chrome-profile-provider.h"
#include "chrome/browser/dom_ui/dom_ui_thumbnail_source.h"
#include "chrome/browser/sessions/session_types.h"
#include "base/singleton.h"

//#define DEBUG_CHROMIUM_API

using namespace history;

static ChromeProfileProvider* g_chrome_profile_provider = NULL;

ChromeProfileProvider* ChromeProfileProvider::GetInstance()
{
  if(!g_chrome_profile_provider) {
    g_chrome_profile_provider = new ChromeProfileProvider;
  }
  return g_chrome_profile_provider;
}

ChromeProfileProvider::ChromeProfileProvider() :
  profile_(NULL),
  history_service_(NULL),
  session_service_(NULL),
  autocompletion_controller_(NULL),
  session_context_(NULL),
  session_callback_(NULL),
  session_expback_(NULL),
  favorite_context_(NULL),
  favorite_callback_(NULL),
  favorite_expback_(NULL),
  autocompletion_context_(NULL),
  autocompletion_callback_(NULL),
  autocompletion_expback_(NULL)
{
}

ChromeProfileProvider::~ChromeProfileProvider() {
  if (autocompletion_controller_)
    delete autocompletion_controller_;
}

bool
ChromeProfileProvider::Initialize(const char* config_dir_name)
{
  if (profile_)
    return false;

  // TODO: improve the performance
  // Sync the chrome default profile
  gchar* chrome_profile_path = g_build_filename (g_get_home_dir(),
                                                 ".config",
                                                 config_dir_name,
                                                 "Default",
                                                 NULL);

  gchar* panel_profile_path = g_build_filename (g_get_home_dir(),
                                                ".config",
                                                "web-panel",
                                                NULL);
  
  g_mkdir_with_parents (panel_profile_path, 0755);
  
  std::string cmd_line = "rsync -av " + std::string(chrome_profile_path) + " " + std::string(panel_profile_path) + " -a";

  //printf("command line: %s\n", cmd_line.c_str());
  
  g_spawn_command_line_sync (cmd_line.c_str(), NULL, NULL, NULL, NULL);
  
  user_data_dir_ = FilePath(panel_profile_path);

  g_free(chrome_profile_path);
  g_free(panel_profile_path);

  profile_ = Profile::CreateProfile(user_data_dir_.AppendASCII("Default"));
  if (!profile_)
    return false;

  history_service_ = profile_->GetHistoryService(Profile::EXPLICIT_ACCESS);
  session_service_ = profile_->GetSessionService();

  if (!autocompletion_controller_) {
    autocompletion_controller_ = new AutocompleteController(profile_);
    registrar_.Add(this, NotificationType::AUTOCOMPLETE_CONTROLLER_RESULT_UPDATED,
                   NotificationService::AllSources());
  }
  else
    autocompletion_controller_->SetProfile(profile_);

  return true;
}

void
ChromeProfileProvider::Uninitialize()
{
  if (profile_){
    // delete profile will destroy the services
    delete profile_;
  }
  profile_ = NULL;
}

void
ChromeProfileProvider::GetFavoritePages(void* context, 
                                        FavoriteCallBack* callback,
                                        ExceptionCallback* expback)
{
  const int result_count = 8;

  favorite_context_ = context;
  favorite_callback_ = callback;
  favorite_expback_ = expback;

  // Send query to history service
  history_service_->QuerySegmentUsageSince(&consumer_,
                                           base::Time::Now() - base::TimeDelta::FromDays(90),
                                           result_count,
                                           NewCallback(this,
                                                       &ChromeProfileProvider::
                                                       OnSegmentUsageAvailable));
}

void
ChromeProfileProvider::OnSegmentUsageAvailable(CancelableRequestProvider::Handle handle,
                                               std::vector<PageUsageData*>* page_data)
{
  if (page_data->size() == 0) {
    if (favorite_context_ && favorite_expback_)
      favorite_expback_ (favorite_context_, 0);
    return;
  }

  for (size_t i = 0; i < page_data->size(); i++)
    {
      PageUsageData page = *(*page_data)[i];

      thumbnail_urls_.push_back(page.GetURL().spec().c_str());

      if (history_service_->GetPageThumbnail(page.GetURL(),
                                             &consumer_,
                                             NewCallback(static_cast<ChromeProfileProvider*>(this),
                                                         &ChromeProfileProvider::OnThumbnailDataAvailable)))
        {
          //nest run message loop to make sure thumbnail data is ready before callback
          bool old_state = MessageLoop::current()->NestableTasksAllowed();
          MessageLoop::current()->SetNestableTasksAllowed(true);
          MessageLoop::current()->Run();
          MessageLoop::current()->SetNestableTasksAllowed(old_state);
        }
      if (favorite_context_ && favorite_callback_)
        {
          favorite_callback_ (favorite_context_,
                              page.GetURL().spec().c_str(),
                              UTF16ToUTF8(page.GetTitle()).c_str());
        }

#ifdef DEBUG_CHROMIUM_API
      g_debug ("Fav: %s, %s",
               page.GetURL().spec().c_str(),
               UTF16ToUTF8(page.GetTitle()).c_str());
#endif
    }
}

void
ChromeProfileProvider::StartAutoComplete(const char* keyword,
                                         void* context,
                                         AutoCompletionCallBack* callback,
                                         ExceptionCallback* expback)
{
  autocompletion_context_ = context;
  autocompletion_callback_ = callback;
  autocompletion_expback_ = expback;

  if (keyword && autocompletion_controller_)
    {
      // Fire ac event to UI thread
      autocompletion_controller_->Start(UTF8ToWide(keyword),
                         std::wstring(),  // desired_tld
                         false,            // prevent_inline_autocomplete
                         false,           // prefer_keyword
                         false);          // synchronous_only
    }
}

void
ChromeProfileProvider::StopAutoComplete(void)
{
  autocompletion_controller_->Stop(false);
}


void 
ChromeProfileProvider::OnThumbnailDataAvailable(HistoryService::Handle handle,
                                                scoped_refptr<RefCountedBytes> jpeg_data)
{
  if (jpeg_data.get()) 
    {
      SaveThumbnail(thumbnail_urls_.back(), 
                    jpeg_data->front(), 
                    jpeg_data->size());
      thumbnail_urls_.pop_back();
    }
  MessageLoop::current()->Quit();
}

void
ChromeProfileProvider::SaveThumbnail(const char* url, 
                                     const unsigned char* data,
                                     size_t len)
{
  gchar *thumbnail_path;  
  gchar *thumbnail_filename;
  gchar *csum;  

  csum = g_compute_checksum_for_string (G_CHECKSUM_MD5, url, -1);  

  // Cheat the mutter-moblin. Pretend as a png file here.
  thumbnail_filename = g_strconcat (csum, ".png", NULL);
  thumbnail_path = g_build_filename (g_get_home_dir (),  
                                     ".thumbnails",  
                                     "large",  
                                     thumbnail_filename,  
                                     NULL);  
  g_free (csum);  

  // Todo, if file exists and mod time is in 24h, just reuse 
  // the old image.
  FILE* fp = fopen(thumbnail_path, "w");
  if (fp)
    {
      fwrite(data, len, 1, fp);
      fclose(fp);
    }

  g_free (thumbnail_filename);  
  g_free (thumbnail_path);  

  return; 
}

void 
ChromeProfileProvider::Observe(NotificationType type,
                               const NotificationSource& source,
                               const NotificationDetails& details)
{
  // should check if controller is done to avoid duplicate results
  if (autocompletion_controller_->done() &&
      type == NotificationType::AUTOCOMPLETE_CONTROLLER_RESULT_UPDATED)
    {
      AutocompleteResult result;
      result.CopyFrom(*(Details<const AutocompleteResult>(details).ptr()));

      if (result.size() == 0)
        if (autocompletion_context_ && autocompletion_expback_)
          {
            autocompletion_expback_(autocompletion_context_, 0);
            return;
          }

      for (size_t i = 0; i < result.size(); i++)
        {
          AutocompleteMatch match = result.match_at(i);
          if (autocompletion_context_ && autocompletion_callback_)
            {
              char contents[match.contents.size() * 2 + 1];
              char description[match.description.size() * 2 + 1];

              sprintf(contents, "%ls", match.contents.c_str());
              sprintf(description, "%ls", match.description.c_str());

              autocompletion_callback_(autocompletion_context_,
                                       (int)match.type, 
                                       match.destination_url.spec().c_str(), 
                                       contents, 
                                       description);
            }

#ifdef DEBUG_CHROMIUM_API
          g_debug ("AC: type=%d, url=%s, fill=%ls, contents=%ls, desc=%ls", 
                   match.type, 
                   match.destination_url.spec().c_str(), 
                   match.fill_into_edit.c_str(), 
                   match.contents.c_str(),
                   match.description.c_str());
#endif
        }
    }
}

// Identifier for commands written to file.
static const SessionCommand::id_type kCommandSetTabWindow = 0;
// kCommandSetWindowBounds is no longer used (it's superseded by
// kCommandSetWindowBounds2). I leave it here to document what it was.
// static const SessionCommand::id_type kCommandSetWindowBounds = 1;
static const SessionCommand::id_type kCommandSetTabIndexInWindow = 2;
static const SessionCommand::id_type kCommandTabClosed = 3;
static const SessionCommand::id_type kCommandWindowClosed = 4;
static const SessionCommand::id_type
  kCommandTabNavigationPathPrunedFromBack = 5;
static const SessionCommand::id_type kCommandUpdateTabNavigation = 6;
static const SessionCommand::id_type kCommandSetSelectedNavigationIndex = 7;
static const SessionCommand::id_type kCommandSetSelectedTabInIndex = 8;
static const SessionCommand::id_type kCommandSetWindowType = 9;
static const SessionCommand::id_type kCommandSetWindowBounds2 = 10;
static const SessionCommand::id_type
  kCommandTabNavigationPathPrunedFromFront = 11;
static const SessionCommand::id_type kCommandSetPinnedState = 12;
static const SessionCommand::id_type kCommandSetAppExtensionID = 13;

struct TabEntry {
  SessionID::id_type tab_id;
  int                navigation_index;
  std::string        url_spec;
  string16           title;
};


void 
ChromeProfileProvider::OnGotPreviousSession(SessionService::Handle handle,
                                            std::vector<SessionWindow*>* windows)
{
  for (std::vector<SessionWindow*>::iterator w = windows->begin();
       w != windows->end(); w++)
    {
      if ((*w)->type == Browser::TYPE_NORMAL)
        {
          for (std::vector<SessionTab*>::iterator t = (*w)->tabs.begin(); 
               t != (*w)->tabs.end(); t++)
            {
              TabNavigation* tab_nav = &(*t)->navigations.at((*t)->current_navigation_index);

              thumbnail_urls_.push_back(tab_nav->url().spec().c_str());
              if (history_service_->GetPageThumbnail(tab_nav->url(),
                                                     &consumer_,
                                                     NewCallback(static_cast
                                                                 <ChromeProfileProvider*>(this),
                                                                 &ChromeProfileProvider::
                                                                 OnThumbnailDataAvailable)))
                {
                  bool old_state = MessageLoop::current()->NestableTasksAllowed();
                  MessageLoop::current()->SetNestableTasksAllowed(true);
                  MessageLoop::current()->Run();
                  MessageLoop::current()->SetNestableTasksAllowed(old_state);
                }
              if (session_context_ && session_callback_)
                {
                  session_callback_(session_context_, 
                                    (*t)->tab_id.id(), 
                                    (*t)->window_id.id(),
                                    tab_nav->url().spec().c_str(),
                                    UTF16ToUTF8(tab_nav->title()).c_str());
                }
            }
        }
    }
}

void
ChromeProfileProvider::GetSessions(void* context, 
                                   SessionCallBack* callback,
                                   ExceptionCallback* expback)
{
  session_context_ = context;
  session_callback_ = callback;
  session_expback_ = expback;

#ifdef USE_SESSION_SERVICE
  session_service_->GetLastSession(&consumer_,
                                   NewCallback(this,
                                               &ChromeProfileProvider::
                                               OnGotPreviousSession));

#else
  SessionFileReader session_reader(user_data_dir_.
                                   AppendASCII("Default/Current Session"));

  std::vector<SessionCommand*> commands;
  session_reader.Read(BaseSessionService::SESSION_RESTORE, &commands);
  // Fixme: 256 tabs should be big enough here
  std::bitset<256> bitmap(0);
  std::vector<TabEntry*> tabs;

  std::map<int, TabEntry*> tabs_map;
  
  for (std::vector<SessionCommand*>::iterator i = commands.begin();
       i < commands.end(); i++) 
    {      
      if ((*i))
        {
          if ((*i)->id() == kCommandUpdateTabNavigation)
            {
              scoped_ptr<Pickle> pickle((*i)->PayloadAsPickle());
              if (!pickle.get())
                continue;

              void* iterator = NULL;
              TabEntry *tab_entry = new TabEntry();

              if (!pickle->ReadInt(&iterator, &tab_entry->tab_id) ||
                  !pickle->ReadInt(&iterator, &tab_entry->navigation_index) ||
                  !pickle->ReadString(&iterator, &tab_entry->url_spec) ||
                  !pickle->ReadString16(&iterator, &tab_entry->title))
                {
                  continue;
                }
              tabs_map[tab_entry->tab_id] = tab_entry;
            }
          else if ((*i)->id() == kCommandTabClosed) // for closed tabs
            {
              scoped_ptr<Pickle> pickle((*i)->PayloadAsPickle());
              if (!pickle.get())
                continue;

              struct ClosedPayload 
              {
                int id;
                int64 time;
              };

              ClosedPayload payload;
              
              if(!(*i)->GetPayload(&payload, sizeof(payload)))
                continue;
              
              
              std::map<int, TabEntry*>::iterator it = tabs_map.find(payload.id);
              
              if (it != tabs_map.end())
                tabs_map.erase(it);
            }
        }
    }
  
  for (std::map<int, TabEntry*>::iterator i = tabs_map.begin();
       i != tabs_map.end(); i++)
    {
      tabs.push_back((*i).second);
    }

  if (tabs.size() == 0)
    {
      if (session_context_ && session_expback_)
        session_expback_(session_context_, 0);
      return;
    }

  // Second reverse scan
  for (std::vector<TabEntry*>::iterator i = tabs.begin();
       i < tabs.end(); i++) 
    {
      // Dont rely on this way - the session may or may not have thubmail available.
      thumbnail_urls_.push_back((*i)->url_spec.c_str());
      if (history_service_->GetPageThumbnail(GURL((*i)->url_spec),
                                             &consumer_,
                                             NewCallback(static_cast
                                                         <ChromeProfileProvider*>(this),
                                                         &ChromeProfileProvider::
                                                         OnThumbnailDataAvailable)))
        {
          bool old_state = MessageLoop::current()->NestableTasksAllowed();
          MessageLoop::current()->SetNestableTasksAllowed(true);
          MessageLoop::current()->Run();
          MessageLoop::current()->SetNestableTasksAllowed(old_state);
        }
      if (session_context_ && session_callback_)
        {
          session_callback_(session_context_,
                            (*i)->tab_id, 
                            (*i)->navigation_index,
                            (*i)->url_spec.c_str(),
                            UTF16ToUTF8((*i)->title).c_str());
        }

#ifdef DEBUG_CHROMIUM_API
      g_debug ("tab_id=%d, navigation_index=0x%x, url=%s, title=%s", 
               (*i)->tab_id, 
               (*i)->navigation_index, 
               (*i)->url_spec.c_str(), 
               UTF16ToUTF8((*i)->title).c_str());
#endif

      delete *i;
    }
#endif
}
