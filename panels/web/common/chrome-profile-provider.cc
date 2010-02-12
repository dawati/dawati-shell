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
#include <stdio.h>
#include "chrome/browser/browser_process.h"
#include "chrome/browser/thumbnail_store.h"
#include "chrome/browser/history/thumbnail_database.h"
#include "chrome-profile-provider.h"
#include "chrome/browser/dom_ui/dom_ui_thumbnail_source.h"
#include "chrome/browser/sessions/session_types.h"
#include "base/singleton.h"

using namespace history;

int 
ChromeProfileProvider::Initialize(service_type type)
{
  // Get Chrome default profile
  PathService::Get(chrome::DIR_USER_DATA, &user_data_dir_);

#ifndef USE_SESSION_SERVICE
  if (!(type & PROVIDER_SESSION))
#endif
    {
      profile_ = Profile::CreateProfile(user_data_dir_.AppendASCII("Default.backup"));
      if (!profile_) 
        return (-1);
    }

  if (type & PROVIDER_FAVORITE
#if USE_SESSION_SERVICE || GET_SESSION_THUMB
      || type & PROVIDER_SESSION
#endif
     ) 
    {
      // Get Chrome history service
      history_service_ = profile_->GetHistoryService(Profile::EXPLICIT_ACCESS);
    }

#ifdef USE_SESSION_SERVICE
  if (type & PROVIDER_SESSION) 
    {
      // Get Chrome history service
      session_service_ = profile_->GetSessionService();
    }
#endif


  if (type & PROVIDER_AUTOCOMP) {
    // AUTOCOMPLETE_CONTROLLER_DEFAULT_MATCH_UPDATED
    registrar_.Add(this, NotificationType::AUTOCOMPLETE_CONTROLLER_RESULT_UPDATED,
                   NotificationService::AllSources());

    // Create the autocompelet controller
    autocompletion_controller_ = new AutocompleteController(profile_);
  }

  return 0;
}

void 
ChromeProfileProvider::GetFavoritePages(void* context, 
                                        FavoriteCallBack* callback)
{
  const int result_count = 8;
  CancelableRequestConsumer cancelable_consumer;

  favorite_context_ = context;
  favorite_callback_ = callback;

  // Send query to history service
  history_service_->QuerySegmentUsageSince(&cancelable_consumer,
                                   base::Time::Now() - base::TimeDelta::FromDays(90),
                                   result_count,
                                   NewCallback(this,
                                               &ChromeProfileProvider::
                                               OnSegmentUsageAvailable));


  // Trigger the main loop, will be exited from OnSegmentUsageAvailable
  MessageLoop::current()->Run();
}

void 
ChromeProfileProvider::GetAutoCompleteData(const char* keyword, 
                                           void* context, 
                                           AutoCompletionCallBack* callback)
{
  autocompletion_context_ = context;
  autocompletion_callback_ = callback;

  if (keyword && autocompletion_controller_)
    {
      // Fire ac event to UI thread
      autocompletion_controller_->Start(UTF8ToWide(keyword),
                         std::wstring(),  // desired_tld
                         true,            // prevent_inline_autocomplete
                         false,           // prefer_keyword
                         false);          // synchronous_only

      MessageLoop::current()->Run();
    }
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
ChromeProfileProvider::OnSegmentUsageAvailable(CancelableRequestProvider::Handle handle,
                                               std::vector<PageUsageData*>* page_data)
{
  CancelableRequestConsumer cancelable_consumer;

  Reset();

  for (size_t i = 0; i < page_data->size(); i++)
    {
      PageUsageData page = *(*page_data)[i];

      thumbnail_urls_.push_back(page.GetURL().spec().c_str());

      if (history_service_->GetPageThumbnail(page.GetURL(),
                                 &cancelable_consumer,
                                 NewCallback(static_cast<ChromeProfileProvider*>(this),
                                             &ChromeProfileProvider::OnThumbnailDataAvailable)))
        {
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

      result_counter_++;

#ifdef DEBUG_CHROMIUM_API
      g_debug ("Fav: %s, %s", 
               page.GetURL().spec().c_str(), 
               UTF16ToUTF8(page.GetTitle()).c_str());
#endif
    }
  MessageLoop::current()->Quit();
}

void 
ChromeProfileProvider::Observe(NotificationType type,
                               const NotificationSource& source,
                               const NotificationDetails& details)
{
  if (autocompletion_controller_->done() && 
      type == NotificationType::AUTOCOMPLETE_CONTROLLER_RESULT_UPDATED)
    {
      AutocompleteResult result;
      result.CopyFrom(*(Details<const AutocompleteResult>(details).ptr()));
      MessageLoop::current()->Quit();

      Reset();

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

          result_counter_++;

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

extern const SessionCommand::id_type kCommandUpdateTabNavigation = 6;

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
  CancelableRequestConsumer cancelable_consumer;

  Reset();

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
                                                     &cancelable_consumer,
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

              result_counter_++;
            }
        }
    }
  MessageLoop::current()->Quit();
}

void 
ChromeProfileProvider::GetSessions(void* context, 
                                   SessionCallBack* callback) 
{
  CancelableRequestConsumer cancelable_consumer;
  session_context_ = context;
  session_callback_ = callback;

#ifdef USE_SESSION_SERVICE
  session_service_->GetLastSession(&cancelable_consumer,
                                   NewCallback(this, 
                                               &ChromeProfileProvider::
                                               OnGotPreviousSession));

  MessageLoop::current()->Run();

#else
  SessionFileReader session_reader(user_data_dir_.
                                   AppendASCII("Default.backup/Current Session"));

  std::vector<SessionCommand*> commands;
  session_reader.Read(BaseSessionService::SESSION_RESTORE, &commands);
  // Fixme: 256 tabs should be big enough here
  std::bitset<256> bitmap(0);
  std::vector<TabEntry*> tabs;

  Reset();

  // First reverse scan - mask
  for (std::vector<SessionCommand*>::reverse_iterator i = commands.rbegin();
       i < commands.rend(); i++) 
    {
      if ((*i) && (*i)->id() == kCommandUpdateTabNavigation) 
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

          if (!bitmap.test(tab_entry->tab_id)) 
            {
              bitmap.set(tab_entry->tab_id);
              tabs.push_back(tab_entry);
            }
        }
    }


  // Second reverse scan
  for (std::vector<TabEntry*>::reverse_iterator i = tabs.rbegin();
       i < tabs.rend(); i++) 
    {
      // Dont rely on this way - the session may or may not have thubmail available.
#if GET_SESSION_THUMB
      thumbnail_urls_.push_back((*i)->url_spec.c_str());
      if (history_service_->GetPageThumbnail(GURL((*i)->url_spec),
                                             &cancelable_consumer,
                                             NewCallback(static_cast
                                                         <ChromeProfileProvider*>(this),
                                                         &ChromeProfileProvider::
                                                         OnThumbnailDataAvailable)))
        {
          MessageLoop::current()->Run();
        }
#endif
      if (session_context_ && session_callback_)
        {
          session_callback_(session_context_,
                            (*i)->tab_id, 
                            (*i)->navigation_index,
                            (*i)->url_spec.c_str(),
                            UTF16ToUTF8((*i)->title).c_str());
        }

      result_counter_++;

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
