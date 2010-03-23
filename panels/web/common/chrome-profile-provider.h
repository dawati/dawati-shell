/*
 * Moblin-Panel-Web: The web panel for Moblin
 * 
 * Chromium browser APIs wrapper
 */

#include <time.h>
#include <algorithm>
#include <string>

#include "app/app_paths.h"
#include "base/basictypes.h"
#include "base/file_path.h"
#include "base/file_util.h"
#include "base/i18n/icu_util.h"
#include "base/message_loop.h"
#include "base/path_service.h"
#include "base/scoped_vector.h"
#include "base/string_util.h"
#include "chrome/browser/pref_service.h"
#include "chrome/common/pref_names.h"
#include "chrome/common/notification_service.h"
#include "chrome/common/notification_registrar.h"
#include "chrome/common/chrome_paths.h"
#include "chrome/browser/browser_prefs.h"
#include "chrome/browser/profile.h"
#include "chrome/browser/autocomplete/autocomplete.h"
#include "chrome/browser/history/history.h"
#include "chrome/browser/history/history_types.h"
#include "chrome/browser/history/page_usage_data.h"
#include "chrome/browser/sessions/session_service.h"
#include "chrome/browser/sessions/session_command.h"
#include "chrome/browser/sessions/session_backend.h"

typedef void SessionCallBack        (void*       session_ctx, 
                                     int         tab_id, 
                                     int         navigation_index, 
                                     const char* url, 
                                     const char* title);

typedef void FavoriteCallBack       (void*       favorite_ctx,
                                     const char* url, 
                                     const char* title);

typedef void AutoCompletionCallBack (void*       ac_ctx, 
                                     int         type, 
                                     const char* url, 
                                     const char* content, 
                                     const char* description);

#define PROVIDER_AUTOCOMP 1
#define PROVIDER_SESSION  2
#define PROVIDER_FAVORITE 4

class ChromeProfileProvider : public NotificationObserver {
public:
    typedef unsigned char service_type;

    ChromeProfileProvider() : 
        profile_(NULL), 
        history_service_(NULL),
        session_service_(NULL),
        autocompletion_controller_(NULL),
        session_context_(NULL),
        session_callback_(NULL),
        favorite_context_(NULL),
        favorite_callback_(NULL),
        autocompletion_context_(NULL),
        autocompletion_callback_(NULL),
        result_counter_(0) { }

    ~ChromeProfileProvider() { 
      if (autocompletion_controller_)
        delete autocompletion_controller_;
    }

    Profile* GetProfile()    { return profile_;    }

    int Initialize           (service_type            type);

    void GetFavoritePages    (void*                   context, 
                              FavoriteCallBack*       callback);

    void GetAutoCompleteData (const char*             keyword, 
                              void*                   context, 
                              AutoCompletionCallBack* callback);

    void GetSessions         (void*                   context, 
                              SessionCallBack*        callback);

    int ResultCount()        { return result_counter_; }

    void Reset()             {
      result_counter_ = 0;
      thumbnail_urls_.clear();
    }


private:
    void OnSegmentUsageAvailable (CancelableRequestProvider::Handle handle,
                                  std::vector<PageUsageData*>*      page_data);

    void OnThumbnailDataAvailable(HistoryService::Handle            handle,
                                  scoped_refptr<RefCountedBytes>    jpeg_data);

    virtual void Observe         (NotificationType                  type,
                                  const NotificationSource&         source,
                                  const NotificationDetails&        details);

    void SaveThumbnail           (const char*                       url, 
                                  const unsigned char* data,
                                  size_t len);

    void OnGotPreviousSession    (SessionService::Handle            handle,
                                  std::vector<SessionWindow*>*      windows);

    DISALLOW_EVIL_CONSTRUCTORS(ChromeProfileProvider);

private:
    Profile*                 profile_;
    FilePath                 user_data_dir_;
    HistoryService*          history_service_;
    SessionService*          session_service_;
    AutocompleteController*  autocompletion_controller_;
    NotificationRegistrar    registrar_;

    void*                    session_context_;
    SessionCallBack*         session_callback_;
    void*                    favorite_context_;
    FavoriteCallBack*        favorite_callback_;
    void*                    autocompletion_context_;
    AutoCompletionCallBack*  autocompletion_callback_;

    // Create the loop where it use this wrapper
    //MessageLoopForUI        main_message_loop_;
    int                      result_counter_;
    std::vector<const char*> thumbnail_urls_;
};

