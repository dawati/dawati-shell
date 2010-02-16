
/*
 * Copyright © 2010 Intel Corp.
 *
 * Authors: Rob Bradford <rob@linux.intel.com> (dalston-power-applet.c)
 *          Rob Staudinger <robert.staudinger@intel.com>
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
 */

#include <stdbool.h>
#include <locale.h>
#include <stdlib.h>
#include <glib/gi18n.h>
#include <clutter/clutter.h>
#include <clutter-gtk/clutter-gtk.h>
#include <libnotify/notify.h>
#include "mpd-application.h"
#include "config.h"

int
main (int    argc,
      char **argv)
{
  MpdApplication *application;

  setlocale (LC_ALL, "");
  bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
  textdomain (GETTEXT_PACKAGE);

  gtk_clutter_init (&argc, &argv);
  notify_init (_("Moblin Power Icon"));

  application = mpd_application_new ();

  clutter_main ();

  g_object_unref (application);

  return EXIT_SUCCESS;
}

