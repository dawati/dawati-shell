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

#include <telepathy-glib/account-manager.h>
#include <telepathy-glib/contact.h>

#include <glib.h>
#include <dbus/dbus-glib.h>

static void
am_ready_cb (GObject      *source_object,
             GAsyncResult *result,
             gpointer      userdata)

{
  TpAccountManager *account_manager = TP_ACCOUNT_MANAGER (source_object);
  AnerleyFeed *feed;
  ClutterActor *stage;
  ClutterActor *scroll_view;
  ClutterActor *icon_view;
  ClutterModel *model;
  GError *error = NULL;

  if (!tp_account_manager_prepare_finish (account_manager, result, &error))
  {
    g_warning ("Failed to make account manager ready: %s", error->message);
    g_error_free (error);
    return;
  }

  feed = ANERLEY_FEED (anerley_aggregate_tp_feed_new ());
  model = CLUTTER_MODEL (anerley_feed_model_new (feed));
  stage = clutter_stage_get_default ();
  icon_view = anerley_tile_view_new (ANERLEY_FEED_MODEL (model));

  scroll_view = mx_scroll_view_new ();
  clutter_container_add_actor (CLUTTER_CONTAINER (stage),
                               CLUTTER_ACTOR (scroll_view));
  clutter_container_add_actor (CLUTTER_CONTAINER (scroll_view),
                               CLUTTER_ACTOR (icon_view));
  clutter_actor_set_size (CLUTTER_ACTOR (scroll_view), 640, 480);
  clutter_actor_show_all (stage);
}

int
main (int    argc,
      char **argv)
{
  TpAccountManager *account_manager;
  gchar *path;
  MxStyle *style;
  GError *error = NULL;

  clutter_init (&argc, &argv);

  path = g_build_filename (PKG_DATA_DIR,
                           "style.css",
                           NULL);

  style = mx_style_get_default ();

  if (!mx_style_load_from_file (style,
                                  path,
                                  &error))
  {
    g_warning (G_STRLOC ": Error opening style: %s",
               error->message);
    g_clear_error (&error);
  }

  g_free (path);

  account_manager = tp_account_manager_dup ();

  tp_account_manager_prepare_async (account_manager, NULL,
                                   am_ready_cb, NULL);

  clutter_main ();

  g_object_unref (account_manager);

  return 0;
}

