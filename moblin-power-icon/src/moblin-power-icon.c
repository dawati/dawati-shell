
/*
 * Copyright (C) 2009-2010, Intel Corporation.
 *
 * Author: Robert Staudinger <robert.staudinger@intel.com>
 * 
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU Lesser General Public License,
 * version 2.1, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
 * more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */

#include <stdlib.h>
#include <glib/gi18n.h>
#include <clutter/clutter.h>
#include <moblin-panel/mpl-panel-common.h>
#include <moblin-panel/mpl-panel-windowless.h>
#include "config.h"

int
main (int    argc,
      char **argv)
{
  MplPanelClient *client;

  bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
  textdomain (GETTEXT_PACKAGE);

  clutter_init (&argc, &argv);

  client = mpl_panel_windowless_new (MPL_PANEL_POWER,
                                     _("battery"),
                                     PKGTHEMEDIR "/power-applet.css",
                                     "unknown",
                                     TRUE);

  clutter_main ();
  return EXIT_SUCCESS;
}

