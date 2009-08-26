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

#include "penge-magic-list-view.h"
#include "penge-people-model.h"

G_DEFINE_TYPE (PengePeoplePane, penge_people_pane, NBTK_TYPE_TABLE)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), PENGE_TYPE_PEOPLE_PANE, PengePeoplePanePrivate))

typedef struct _PengePeoplePanePrivate PengePeoplePanePrivate;

struct _PengePeoplePanePrivate {
  MojitoClient *client;
  MojitoClientView *view;
  ClutterModel *model;
  ClutterActor *list_view;
};

#define MAX_ITEMS 32

#define TILE_WIDTH 170.0f
#define TILE_HEIGHT 115.0f

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

  if (priv->model)
  {
    g_object_unref (priv->model);
    priv->model = NULL;
  }

  G_OBJECT_CLASS (penge_people_pane_parent_class)->dispose (object);
}

static void
penge_people_pane_finalize (GObject *object)
{
  G_OBJECT_CLASS (penge_people_pane_parent_class)->finalize (object);
}

#if 0
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
#endif

static void
_client_open_view_cb (MojitoClient     *client,
                      MojitoClientView *view,
                      gpointer          userdata)
{
  PengePeoplePanePrivate *priv = GET_PRIVATE (userdata);

  mojito_client_view_start (view);
  priv->model = penge_people_model_new (view);
  penge_magic_list_view_set_model (PENGE_MAGIC_LIST_VIEW (priv->list_view),
                                   priv->model);
  penge_magic_list_view_set_item_type (PENGE_MAGIC_LIST_VIEW (priv->list_view),
                                       PENGE_TYPE_PEOPLE_TILE);
  penge_magic_list_view_add_attribute (PENGE_MAGIC_LIST_VIEW (priv->list_view),
                                       "item",
                                       0);
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

  /* Create the client and request the services list */
  priv->client = mojito_client_new ();
  mojito_client_get_services (priv->client, _client_get_services_cb, self);

  priv->list_view = penge_magic_list_view_new ();

  penge_magic_container_set_minimum_child_size (PENGE_MAGIC_CONTAINER (priv->list_view),
                                                TILE_WIDTH,
                                                TILE_HEIGHT);

  nbtk_table_add_actor_with_properties (NBTK_TABLE (self),
                                        priv->list_view,
                                        0, 0,
                                        "x-expand", TRUE,
                                        "y-expand", TRUE,
                                        "x-fill", TRUE,
                                        "y-fill", TRUE,
                                        NULL);

}


