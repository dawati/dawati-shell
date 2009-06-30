
#include <nbtk/nbtk.h>
#include "mpl-clutter-theme.h"

void
mpl_clutter_theme_ensure_loaded (void)
{
  static gboolean _theme_loaded = FALSE;

  if (!_theme_loaded)
    {
      g_debug ("Loading '%s'", PKGDATADIR "/theme/theme.css");
      nbtk_style_load_from_file (nbtk_style_get_default (),
                                 PKGDATADIR "/theme/theme.css", NULL);
      _theme_loaded = TRUE;    
    }
}

