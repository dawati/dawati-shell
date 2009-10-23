pref("mwb.complete_domains", true);
pref("mwb.search_in_automagic", true);
pref("mwb.search_engine.names", "Ask;Bing;Google;Twitter;Wikipedia;Yahoo!");
pref("mwb.search_engine.selected", "Yahoo!");
pref("mwb.search_engine.data.Ask.url",
     "http://www.ask.com/web?q=%s&search=search");
pref("mwb.search_engine.data.Bing.url",
     "http://www.bing.com/search?q=%s");
pref("mwb.search_engine.data.Google.url",
     "http://www.google.com/search?q=%s");
pref("mwb.search_engine.data.Twitter.url",
     "http://search.twitter.com/search?q=%s");
pref("mwb.search_engine.data.Wikipedia.url",
     "http://en.wikipedia.org/w/index.php?title=Special%3ASearch&search=%s&fulltext=Search");
pref("mwb.search_engine.data.Yahoo!.url",
     "http://search.yahoo.com/search?fr=ffasmoblin&type=AFFILIATE&p=%s");
// Default to using the locale from the OS
pref("intl.locale.matchOS", true);
// Make sure it uses the translated locale pref so that
// navigator.language will be set correctly for Javascript
pref("general.useragent.locale", "chrome://global/locale/intl.properties");
// Default to keeping the history for 1 month
pref("browser.history_expire_days_min", 30);
pref("browser.history_expire_days", 30);
// Disable the file picker dialog for downloads
pref("clutter_mozembed.disable_download_file_picker", true);
// Blank download directory means to use the XDG dir
pref("clutter_mozembed.download_directory", "");
// base url for the wifi geolocation network provider
pref("geo.wifi.uri", "https://www.google.com/loc/json");
// Turn off security nagging warnings (Moblin bug #5245)
pref("security.warn_entering_secure", false);
pref("security.warn_entering_secure.show_once", false);
pref("security.warn_entering_weak", false);
pref("security.warn_entering_weak.show_once", false);
pref("security.warn_leaving_secure", false);
pref("security.warn_leaving_secure.show_once", false);
pref("security.warn_submit_insecure", false);
pref("security.warn_submit_insecure.show_once", false);
pref("security.warn_viewing_mixed", false);
pref("security.warn_viewing_mixed.show_once", false);
// Set theme
pref("general.skins.selectedSkin", "moblin");
// Use XUL error pages
pref("browser.xul.error_pages.enabled", true);
