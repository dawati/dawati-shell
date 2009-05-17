/*
 * Anerley - people feeds and widgets
 * Copyright (C) 2009, Intel Corporation.
 *
 * Authors: Rob Bradford <rob@linux.intel.com>
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


#include <anerley/anerley-tp-feed.h>
#include <anerley/anerley-item.h>
#include <anerley/anerley-simple-grid-view.h>
#include <anerley/anerley-tile.h>
#include <anerley/anerley-aggregate-tp-feed.h>

#include <clutter/clutter.h>
#include <libmissioncontrol/mission-control.h>
#include <telepathy-glib/contact.h>

#include <glib.h>
#include <dbus/dbus-glib.h>

int
main (int    argc,
      char **argv)
{
  MissionControl *mc;
  AnerleyTpFeed *feed;
  DBusGConnection *conn;
  ClutterActor *stage;
  NbtkWidget *scroll_view;
  NbtkWidget *icon_view;
  ClutterModel *model;
  gchar *path;
  NbtkStyle *style;
  GError *error = NULL;


  clutter_init (&argc, &argv);

  path = g_build_filename (PKG_DATA_DIR,
                           "style.css",
                           NULL);

  style = nbtk_style_get_default ();

  if (!nbtk_style_load_from_file (style,
                                  path,
                                  &error))
  {
    g_warning (G_STRLOC ": Error opening style: %s",
               error->message);
    g_clear_error (&error);
  }

  g_free (path);

  conn = dbus_g_bus_get (DBUS_BUS_SESSION, NULL);
  mc = mission_control_new (conn);
  feed = anerley_aggregate_tp_feed_new (mc);
  model = anerley_feed_model_new (feed);
  stage = clutter_stage_get_default ();
  icon_view = anerley_tile_view_new (model);

  scroll_view = nbtk_scroll_view_new ();
  clutter_container_add_actor (CLUTTER_CONTAINER (stage),
                               scroll_view);
  clutter_container_add_actor (CLUTTER_CONTAINER (scroll_view),
                               icon_view);
  clutter_actor_set_size (scroll_view, 640, 480);
  clutter_actor_show_all (stage);

  clutter_main ();

  return 0;
}

