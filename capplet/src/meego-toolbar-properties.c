/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */

/*
 * Copyright (c) 2010 Intel Corp.
 *
 * Author: Tomas Frydrych <tf@linux.intel.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 */

/*
 * Toolbar applet for gnome-control-center
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>

#include <glib/gi18n.h>

#include "mtp-bin.h"

static void
stage_size_allocation_changed_cb (ClutterActor           *stage,
                                  ClutterActorBox        *allocation,
                                  ClutterAllocationFlags  flags,
                                  ClutterActor           *box)
{
  gfloat width  = allocation->x2 - allocation->x1;
  gfloat height = allocation->y2 - allocation->y1;

  clutter_actor_set_size (box, width, height);
}

int
main (int argc, char **argv)
{
  ClutterActor   *stage;
  ClutterActor   *bin;
  ClutterColor    clr = {0xff, 0xff, 0xff, 0xff};
  GError         *error = NULL;

#ifdef ENABLE_NLS
  bindtextdomain (GETTEXT_PACKAGE, PLUGIN_LOCALEDIR);
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
  textdomain (GETTEXT_PACKAGE);
#endif

  clutter_init (&argc, &argv);

  mx_style_load_from_file (mx_style_get_default (),
                           THEMEDIR "/meego-toolbar-properties.css",
                           &error);

  if (error)
    {
      g_warning ("Could not load stylesheet" THEMEDIR
                 "/meego-toolbar-properties.css: %s", error->message);
      g_clear_error (&error);
    }

  bin = mtp_bin_new ();

  stage = clutter_stage_get_default ();

  clutter_actor_set_size (stage, 1024.0, 400.0);

  g_signal_connect (stage, "allocation-changed",
                    G_CALLBACK (stage_size_allocation_changed_cb),
                    bin);

  clutter_stage_set_color (CLUTTER_STAGE (stage), &clr);
  clutter_container_add_actor (CLUTTER_CONTAINER (stage), bin);

  mtp_bin_load_contents ((MtpBin *)bin);

  clutter_actor_show (stage);
  clutter_main ();

  return 0;
}
