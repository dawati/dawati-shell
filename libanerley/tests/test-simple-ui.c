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
#include <anerley/anerley-feed-model.h>
#include <anerley/anerley-tile-view.h>

#include <clutter/clutter.h>
#include <telepathy-glib/contact.h>

#include <glib.h>
#include <dbus/dbus-glib.h>

static void
_entry_text_changed_cb (ClutterText  *text,
                        ClutterModel *model)
{
  const gchar *str;
  str = clutter_text_get_text (text);
  g_debug ("foo: %s", str);

  anerley_feed_model_set_filter_text ((AnerleyFeedModel *)model, str);
}

int
main (int    argc,
      char **argv)
{
  AnerleyFeed *feed;
  ClutterActor *stage;
  NbtkWidget *scroll_view;
  NbtkWidget *icon_view;
  ClutterModel *model;
  gchar *path;
  NbtkStyle *style;
  GError *error = NULL;
  NbtkWidget *table, *entry;
  ClutterActor *tmp;


  g_thread_init (NULL);

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

  feed = anerley_aggregate_tp_feed_new ();
  model = anerley_feed_model_new (feed);
  stage = clutter_stage_get_default ();
  icon_view = anerley_tile_view_new (ANERLEY_FEED_MODEL (model));

  table = nbtk_table_new ();
  entry = nbtk_entry_new (NULL);
  tmp = nbtk_entry_get_clutter_text (NBTK_ENTRY (entry));
  g_signal_connect (tmp,
                    "text-changed",
                    G_CALLBACK (_entry_text_changed_cb),
                    model);

  nbtk_table_add_actor_with_properties (NBTK_TABLE (table),
                                        CLUTTER_ACTOR (entry),
                                        0,
                                        0,
                                        "x-fill",
                                        TRUE,
                                        "x-expand",
                                        TRUE,
                                        "y-expand",
                                        FALSE,
                                        NULL);

  scroll_view = nbtk_scroll_view_new ();
  clutter_container_add_actor (CLUTTER_CONTAINER (stage),
                               CLUTTER_ACTOR (table));
  clutter_container_add_actor (CLUTTER_CONTAINER (scroll_view),
                               CLUTTER_ACTOR (icon_view));
  clutter_actor_show_all (stage);
  nbtk_table_add_actor_with_properties (NBTK_TABLE (table),
                                        CLUTTER_ACTOR (scroll_view),
                                        1,
                                        0,
                                        "x-fill",
                                        TRUE,
                                        "x-expand",
                                        TRUE,
                                        "y-expand",
                                        TRUE,
                                        "y-fill",
                                        TRUE,
                                        NULL);

  clutter_actor_set_size (CLUTTER_ACTOR (table), 640, 480);
  clutter_main ();

  return 0;
}

