
/*
 * Copyright (c) 2010 Intel Corp.
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
 */

#include <stdbool.h>
#include <stdlib.h>
#include <clutter/clutter.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <libnotify/notify.h>
#include "mpd-power-icon.h"

static MpdPowerIcon *_icon = NULL;

static bool
_test_percentage_cb (void *data)
{
  unsigned int percentage;

  percentage = GPOINTER_TO_UINT (data);

  mpd_power_icon_test_notification (_icon, percentage);

  return false;
}

int
main (int     argc,
      char  **argv)
{
  unsigned int   percentage;

  if (argc < 2)
  {
    g_warning ("Need percentage");
    return EXIT_FAILURE;
  }

  percentage = atoi (argv[1]);

  clutter_init (&argc, &argv);
  notify_init (_("Moblin Power Icon"));
  /* Needed for egg-idletime's X extensions. */
  gtk_init (&argc, &argv);

  _icon = mpd_power_icon_new ();

  g_idle_add ((GSourceFunc) _test_percentage_cb, GUINT_TO_POINTER (percentage));

  clutter_main ();

  g_object_unref (_icon);
  _icon = NULL;

  return EXIT_SUCCESS;
}

