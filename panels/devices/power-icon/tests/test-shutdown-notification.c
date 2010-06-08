
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

#include <stdlib.h>
#include <clutter/clutter.h>
#include <libnotify/notify.h>
#include "mpd-shutdown-notification.h"

static void
_note_shutdown_cb (NotifyNotification *note,
                   void               *data)
{
  g_debug ("shutdown");
}

int
main (int     argc,
      char  **argv)
{
  NotifyNotification *note;

  clutter_init (&argc, &argv);
  notify_init (argv[0]);

  note = mpd_shutdown_notification_new ();
  g_signal_connect (note, "shutdown",
                    G_CALLBACK (_note_shutdown_cb), NULL);

  mpd_shutdown_notification_run (MPD_SHUTDOWN_NOTIFICATION (note));

  clutter_main ();

  g_object_unref (note);

  return EXIT_SUCCESS;
}

