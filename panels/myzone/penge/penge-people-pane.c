/*
 * Copyright (C) 2008 - 2009 Intel Corporation.
 *
 * Author: Rob Bradford <rob@linux.intel.com>
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
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */


#include <mojito-client/mojito-client.h>
#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <gio/gdesktopappinfo.h>

#include "penge-utils.h"

#include "penge-people-tile.h"
#include "penge-people-pane.h"

G_DEFINE_TYPE (PengePeoplePane, penge_people_pane, NBTK_TYPE_TABLE)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), PENGE_TYPE_PEOPLE_PANE, PengePeoplePanePrivate))

typedef struct _PengePeoplePanePrivate PengePeoplePanePrivate;

struct _PengePeoplePanePrivate {
  MojitoClient *client;
  MojitoClientView *view;
  GHashTable *uuid_to_actor;

  ClutterActor *no_content_tile;
};

#define NUMBER_COLS 2
#define MAX_ITEMS 8

#define TILE_WIDTH 170
#define TILE_HEIGHT 115

#define COL_SPACING 6
#define ROW_SPACING 6

static void
penge_people_pane_dispose (GObject *object)
{
  PengePeoplePanePrivate *priv = GET_PRIVATE (object);

  if (priv->client)
  {
    g_object_unref (priv->client);
    priv->client = NULL;
  }

  if (priv->view)
  {
    g_object_unref (priv->view);
    priv->view = NULL;
  }

  if (priv->uuid_to_actor)
  {
    g_hash_table_unref (priv->uuid_to_actor);
    priv->uuid_to_actor = NULL;
  }

  G_OBJECT_CLASS (penge_people_pane_parent_class)->dispose (object);
}

static void
penge_people_pane_finalize (GObject *object)
{
  G_OBJECT_CLASS (penge_people_pane_parent_class)->finalize (object);
}

static ClutterActor *
penge_people_pane_fabricate_actor (PengePeoplePane *pane,
                                   MojitoItem      *item)
{
  ClutterActor *actor;

  actor = g_object_new (PENGE_TYPE_PEOPLE_TILE,
                        "item", item,
                        NULL);
  clutter_actor_set_size (actor, TILE_WIDTH, TILE_HEIGHT);
  
  return actor;
}

#define ICON_SIZE 48

static gboolean
_enter_event_cb (ClutterActor *actor,
                 ClutterEvent *event,
                 gpointer      userdata)
{
  nbtk_widget_set_style_pseudo_class (NBTK_WIDGET (actor),
                                      "hover");

  return FALSE;
}

static gboolean
_leave_event_cb (ClutterActor *actor,
                 ClutterEvent *event,
                 gpointer      userdata)
{
  nbtk_widget_set_style_pseudo_class (NBTK_WIDGET (actor),
                                      NULL);

  return FALSE;
}

static gboolean
_no_content_tile_button_press_event_cb (ClutterActor *actor,
                                        ClutterEvent *event,
                                        gpointer      userdata)
{
  GAppInfo *app_info = (GAppInfo *)userdata;

  if (penge_utils_launch_by_command_line (actor,
                                          g_app_info_get_executable (app_info)))
    penge_utils_signal_activated (actor);
  else
    g_warning (G_STRLOC ": Unable to launch web services settings");

  return TRUE;
}

static ClutterActor *
_make_no_content_tile (void)
{
  ClutterActor *tile;
  NbtkWidget *label;
  ClutterActor *tex;
  GtkIconTheme *icon_theme;
  GtkIconInfo *icon_info;
  GAppInfo *app_info;
  GError *error = NULL;
  GIcon *icon;
  ClutterActor *tmp_text;

  tile = (ClutterActor *)nbtk_table_new ();
  clutter_actor_set_size (tile,
                          TILE_WIDTH * 2 + COL_SPACING,
                          TILE_HEIGHT);
  nbtk_widget_set_style_class_name ((NbtkWidget *)tile, "PengeNoContentTile");


  label = nbtk_label_new (_("Your friends' feeds and web services will appear here. " \
                            "Activate your accounts now!"));
  clutter_actor_set_name ((ClutterActor *)label, "penge-no-content-main-message");

  tmp_text = nbtk_label_get_clutter_text (NBTK_LABEL (label));
  clutter_text_set_line_wrap (CLUTTER_TEXT (tmp_text), TRUE);
  clutter_text_set_line_wrap_mode (CLUTTER_TEXT (tmp_text),
                                   PANGO_WRAP_WORD_CHAR);
  clutter_text_set_ellipsize (CLUTTER_TEXT (tmp_text),
                              PANGO_ELLIPSIZE_NONE);

  nbtk_table_add_actor_with_properties (NBTK_TABLE (tile),
                                        (ClutterActor *)label,
                                        0,
                                        0,
                                        "x-expand",
                                        TRUE,
                                        "x-fill",
                                        TRUE,
                                        "y-expand",
                                        TRUE,
                                        "y-fill",
                                        TRUE,
                                        "col-span",
                                        2,
                                        NULL);

  app_info = (GAppInfo *)g_desktop_app_info_new ("bisho.desktop");

  if (app_info)
  {
    icon_theme = gtk_icon_theme_get_default ();

    icon = g_app_info_get_icon (app_info);
    icon_info = gtk_icon_theme_lookup_by_gicon (icon_theme,
                                                icon,
                                                ICON_SIZE,
                                                GTK_ICON_LOOKUP_GENERIC_FALLBACK);

    tex = clutter_texture_new_from_file (gtk_icon_info_get_filename (icon_info),
                                         &error);

    if (!tex)
    {
      g_warning (G_STRLOC ": Error opening icon: %s",
                 error->message);
      g_clear_error (&error);
    } else {
      clutter_actor_set_size (tex, ICON_SIZE, ICON_SIZE);
      nbtk_table_add_actor_with_properties (NBTK_TABLE (tile),
                                            tex,
                                            1,
                                            0,
                                            "x-expand",
                                            FALSE,
                                            "x-fill",
                                            FALSE,
                                            "y-fill",
                                            FALSE,
                                            "y-expand",
                                            TRUE,
                                            NULL);
    }

    label = nbtk_label_new (g_app_info_get_name (app_info));
    clutter_actor_set_name ((ClutterActor *)label, "penge-no-content-other-message");
    nbtk_table_add_actor_with_properties (NBTK_TABLE (tile),
                                          (ClutterActor *)label,
                                          1,
                                          1,
                                          "x-expand",
                                          TRUE,
                                          "x-fill",
                                          TRUE,
                                          "y-expand",
                                          TRUE,
                                          "y-fill",
                                          FALSE,
                                          NULL);
  }

  g_signal_connect (tile,
                    "button-press-event",
                    (GCallback)_no_content_tile_button_press_event_cb,
                    app_info);

  g_signal_connect (tile,
                    "enter-event",
                    (GCallback)_enter_event_cb,
                    NULL);
  g_signal_connect (tile,
                    "leave-event",
                    (GCallback)_leave_event_cb,
                    NULL);

  clutter_actor_set_reactive (tile, TRUE);

  return tile;
}


/*
 * This function iterates over the list of items that are apparently in the
 * view and packs them into the table.
 *
 * If an item that is not seen before comes in then a new actor of the
 * appropriate type is fabricated in the factory.
 */
static void
penge_people_pane_update (PengePeoplePane *pane)
{
  PengePeoplePanePrivate *priv = GET_PRIVATE (pane);

  GList *items = NULL, *l;
  MojitoItem *item;
  gint count = 0;
  ClutterActor *actor;
  GList *existing_actors = NULL;

  /* In case opening the view failed */
  if (priv->view)
  {
    items = mojito_client_view_get_sorted_items (priv->view);
  }

  if (items)
  {
    /* Hide the nothing configured tile */
    if (priv->no_content_tile)
    {
      clutter_container_remove_actor (CLUTTER_CONTAINER (pane), priv->no_content_tile);
      priv->no_content_tile = NULL;
    }
  }

  if (items == NULL)
  {
    existing_actors = clutter_container_get_children (CLUTTER_CONTAINER (pane));

    if (g_list_length (existing_actors) == 0)
    {
      /* Add the nothing configured tile */
      if (!priv->no_content_tile)
      {
        priv->no_content_tile = _make_no_content_tile ();
        nbtk_table_add_actor_with_properties (NBTK_TABLE (pane),
                                              priv->no_content_tile,
                                              0,
                                              0,
                                              "col-span",
                                              2,
                                              "y-expand",
                                              FALSE,
                                              "x-expand",
                                              TRUE,
                                              NULL);
        clutter_actor_show_all (priv->no_content_tile);
      }
    }

    g_list_free (existing_actors);
    return;
  }


  for (l = items; l; l = g_list_delete_link (l, l))
  {
    item = (MojitoItem *)l->data;
    actor = g_hash_table_lookup (priv->uuid_to_actor, item->uuid);

    if (!actor)
    {
      actor = penge_people_pane_fabricate_actor (pane, item);
      nbtk_table_add_actor (NBTK_TABLE (pane),
                            actor,
                            count / NUMBER_COLS,
                            count % NUMBER_COLS);
      clutter_container_child_set (CLUTTER_CONTAINER (pane),
                                   actor,
                                   "y-expand",
                                   FALSE,
                                   "x-expand",
                                   FALSE,
                                   NULL);

      g_hash_table_insert (priv->uuid_to_actor,
                           g_strdup (item->uuid),
                           g_object_ref (actor));
    } else {
      clutter_container_child_set (CLUTTER_CONTAINER (pane),
                                   actor,
                                   "row",
                                   count / NUMBER_COLS,
                                   "col",
                                   count % NUMBER_COLS,
                                   NULL);
    }
    count++;
  }
}

static void
_client_view_item_added_cb (MojitoClientView *view,
                         MojitoItem       *item,
                         gpointer          userdata)
{
  penge_people_pane_update (userdata);
}

/* When we've been told something has been removed from the view then we
 * must remove it from our local hash of uuid to actor and then destroy the
 * actor. This should remove it from it's container, etc.
 *
 * We must then update the pane to ensure that we relayout the table.
 */
static void
_client_view_item_removed_cb (MojitoClientView *view,
                              MojitoItem       *item,
                              gpointer          userdata)
{
  PengePeoplePane *pane = PENGE_PEOPLE_PANE (userdata);
  PengePeoplePanePrivate *priv = GET_PRIVATE (pane);
  ClutterActor *actor;
 
  actor = g_hash_table_lookup (priv->uuid_to_actor,
                               item->uuid);

  g_hash_table_remove (priv->uuid_to_actor, item->uuid);
  clutter_container_remove_actor (CLUTTER_CONTAINER (pane), actor);
  penge_people_pane_update (pane);
}


static void
_client_open_view_cb (MojitoClient     *client,
                      MojitoClientView *view,
                      gpointer          userdata)
{
  PengePeoplePane *pane = PENGE_PEOPLE_PANE (userdata);
  PengePeoplePanePrivate *priv = GET_PRIVATE (userdata);

  if (!view)
  {
    /* This will cause the not configured message to come up */
    penge_people_pane_update (pane);
    return;
  }

  /* Save out the view */
  priv->view = view;

  /* and start it ... */
  mojito_client_view_start (priv->view);

  g_signal_connect (priv->view, 
                    "item-added",
                    (GCallback)_client_view_item_added_cb,
                    pane);
/*
  g_signal_connect (priv->view, 
                    "item-changed",
                    _client_view_changed_cb,
                    object);
*/
  g_signal_connect (priv->view, 
                    "item-removed",
                    (GCallback)_client_view_item_removed_cb,
                    pane);
  
  penge_people_pane_update (pane);
}

static void
_client_get_services_cb (MojitoClient *client,
                         const GList  *services,
                         gpointer      userdata)
{
  GList *filtered_services = NULL;
  GList *l;

  for (l = (GList *)services; l; l = l->next)
  {
    if (g_str_equal (l->data, "twitter") ||
        g_str_equal (l->data, "flickr") ||
        g_str_equal (l->data, "myspace") ||
        g_str_equal (l->data, "lastfm"))
    {
      filtered_services = g_list_append (filtered_services, l->data);
    }
  }

  mojito_client_open_view (client, 
                           filtered_services, 
                           MAX_ITEMS, 
                           _client_open_view_cb, 
                           userdata);

  g_list_free (filtered_services);
}

static void
penge_people_pane_class_init (PengePeoplePaneClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (PengePeoplePanePrivate));

  object_class->dispose = penge_people_pane_dispose;
  object_class->finalize = penge_people_pane_finalize;
}

static void
penge_people_pane_init (PengePeoplePane *self)
{
  PengePeoplePanePrivate *priv = GET_PRIVATE (self);

  priv->uuid_to_actor = g_hash_table_new_full (g_str_hash,
                                               g_str_equal,
                                               g_free,
                                               g_object_unref);

  nbtk_table_set_row_spacing (NBTK_TABLE (self), ROW_SPACING);
  nbtk_table_set_col_spacing (NBTK_TABLE (self), COL_SPACING);

  /* Create the client and request the services list */
  priv->client = mojito_client_new ();
  mojito_client_get_services (priv->client, _client_get_services_cb, self);

  clutter_actor_set_width ((ClutterActor *)self, TILE_WIDTH * 2 + COL_SPACING);
}


