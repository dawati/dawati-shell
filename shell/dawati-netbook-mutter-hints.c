#include "dawati-netbook-mutter-hints.h"
#include <meta/window.h>

#include <string.h>

typedef struct _MutterHints MutterHints;

typedef enum
{
  HINT_NEW_WORKSPACE = 0x1,
} HintFlags;


struct _MutterHints
{
  MnbThreeState new_workspace;

  gboolean      naked        : 1;
  gboolean      screen_sized : 1;
};

static void
parse_mutter_hints (const gchar *hints_str, MutterHints *hints, HintFlags flags)
{
  gchar **items = g_strsplit (hints_str, ":", 0);
  gchar **p = items;

  if (!p)
    return;

  while (*p && flags)
    {
      gchar **entry = g_strsplit (*p, "=", 0);

      if (entry)
        {
          gchar *key;
          gchar *value;

          key   = *entry;
          value = key ? *(entry+1) : NULL;

          /*
           * Only look for flags the caller is interested in; unset each flag
           * bit as we parse the relevant hint and quit once no more flags are
           * set.
           */
          if (key && value)
            {
              if ((flags & HINT_NEW_WORKSPACE) &&
                       !strcmp (key, "dawati-on-new-workspace"))
                {
                  flags &= ~HINT_NEW_WORKSPACE;

                  if (!strcmp (value, "yes"))
                    hints->new_workspace = MNB_STATE_YES;
                  else if (!strcmp (value, "no"))
                    hints->new_workspace = MNB_STATE_NO;
                }
              else
                g_debug (G_STRLOC ": unknown hint [%s=%s]", key, value);
            }

          g_strfreev (entry);
        }

      p++;
    }

  g_strfreev (items);
}

MnbThreeState
dawati_netbook_mutter_hints_on_new_workspace (MetaWindow *window)
{
  const gchar *hints_str = meta_window_get_mutter_hints (window);
  MutterHints  hints = {0};

  if (!hints_str)
    return MNB_STATE_UNSET;

  parse_mutter_hints (hints_str, &hints, HINT_NEW_WORKSPACE);

  return hints.new_workspace;
}
