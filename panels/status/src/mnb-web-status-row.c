/*
 * Copyright (C) 2008 - 2009 Intel Corporation.
 *
 * Author: Emmanuele Bassi <ebassi@linux.intel.com>
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <glib.h>
#include <stdlib.h>

#include <mojito-client/mojito-client.h>

#include "penge-magic-texture.h"

#include "mnb-web-status-row.h"
#include "mnb-web-status-entry.h"

#define MNB_WEB_STATUS_ROW_GET_PRIVATE(obj)       (G_TYPE_INSTANCE_GET_PRIVATE ((obj), MNB_TYPE_WEB_STATUS_ROW, MnbWebStatusRowPrivate))

#define ICON_SIZE       64.0
#define H_PADDING       9.0

struct _MnbWebStatusRowPrivate
{
  ClutterActor *avatar_bin;
  ClutterActor *icon;
  ClutterActor *entry;

  gchar *service_name;
  gchar *display_name;

  gchar *no_icon_file;

  gchar *last_status_text;
  GTimeVal last_status_time;

  NbtkPadding padding;

  guint in_hover  : 1;
  guint is_online : 1;

  gfloat icon_separator_x;

  MojitoClient *client;
  MojitoClientView *view;
  MojitoClientService *service;

  gulong update_id;
};

enum
{
  PROP_0,

  PROP_SERVICE_NAME
};

G_DEFINE_TYPE (MnbWebStatusRow, mnb_web_status_row, NBTK_TYPE_WIDGET);

static gchar *
get_mojito_service_name (const gchar *service_name)
{
  GKeyFile *key_file = g_key_file_new ();
  GError *error = NULL;
  gchar *service_file, *path, *display_name;

  service_file = g_strconcat (service_name, ".keys", NULL);
  path = g_build_filename (PREFIX, "share", "mojito", "services",
                           service_file,
                           NULL);

  g_free (service_file);

  g_key_file_load_from_file (key_file, path, 0, &error);
  if (error)
    {
      g_warning ("Unable to load keys file for service '%s' (path: %s): %s",
                 service_name,
                 path,
                 error->message);
      g_error_free (error);
      g_free (path);
      g_key_file_free (key_file);

      return NULL;
    }

  display_name = g_key_file_get_string (key_file,
                                        "MojitoService",
                                        "Name",
                                        &error);
  if (error)
    {
      g_warning ("Unable to get the Name key from the file for "
                 "service '%s' (path: %s): %s",
                 service_name,
                 path,
                 error->message);
      g_error_free (error);
      g_free (path);
      g_key_file_free (key_file);

      return NULL;
    }

  g_free (path);
  g_key_file_free (key_file);

  return display_name;
}

static void
mnb_web_status_row_get_preferred_width (ClutterActor *actor,
                                    gfloat        for_height,
                                    gfloat       *min_width_p,
                                    gfloat       *natural_width_p)
{
  MnbWebStatusRowPrivate *priv = MNB_WEB_STATUS_ROW (actor)->priv;
  gfloat min_width, natural_width;

  clutter_actor_get_preferred_width (priv->entry, for_height,
                                     &min_width,
                                     &natural_width);

  if (min_width_p)
    *min_width_p = priv->padding.left + ICON_SIZE + priv->padding.right;

  if (natural_width_p)
    *natural_width_p = priv->padding.left
                     + ICON_SIZE + H_PADDING + natural_width
                     + priv->padding.right;
}

static void
mnb_web_status_row_get_preferred_height (ClutterActor *actor,
                                     gfloat        for_width,
                                     gfloat       *min_height_p,
                                     gfloat       *natural_height_p)
{
  MnbWebStatusRowPrivate *priv = MNB_WEB_STATUS_ROW (actor)->priv;

  if (min_height_p)
    *min_height_p = priv->padding.top + ICON_SIZE + priv->padding.bottom;

  if (natural_height_p)
    *natural_height_p = priv->padding.top + ICON_SIZE + priv->padding.bottom;
}

static void
mnb_web_status_row_allocate (ClutterActor          *actor,
                         const ClutterActorBox *box,
                         ClutterAllocationFlags flags)
{
  MnbWebStatusRowPrivate *priv = MNB_WEB_STATUS_ROW (actor)->priv;
  ClutterActorClass *parent_class;
  gfloat available_width, available_height;
  gfloat min_width, min_height;
  gfloat natural_width, natural_height;
  gfloat text_width, text_height;
  ClutterActorBox child_box = { 0, };

  parent_class = CLUTTER_ACTOR_CLASS (mnb_web_status_row_parent_class);
  parent_class->allocate (actor, box, flags);

//  nbtk_widget_get_border (NBTK_WIDGET (actor), &border);

  available_width  = box->x2 - box->x1
                   - priv->padding.left - priv->padding.right;
  available_height = box->y2 - box->y1
                   - priv->padding.top - priv->padding.bottom;

  clutter_actor_get_preferred_size (priv->entry,
                                    &min_width, &min_height,
                                    &natural_width, &natural_height);

  /* layout
   *
   * +--------------------------------------------------------+
   * | +---+ | +-----------------------------------+--------+ |
   * | | X | | |xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx... | xxxxxx | |
   * | +---+ | +-----------------------------------+--------+ |
   * +--------------------------------------------------------+
   *
   *         +-------------- MnbWebStatusEntry ----------------+
   *   icon  |  text                               | button |
   */

  /* icon */
  child_box.x1 = priv->padding.left;
  child_box.y1 = priv->padding.top;
  child_box.x2 = child_box.x1 + ICON_SIZE;
  child_box.y2 = child_box.y1 + ICON_SIZE;
  clutter_actor_allocate (priv->avatar_bin, &child_box, flags);

  /* separator */
  priv->icon_separator_x = child_box.x2 + H_PADDING;

  /* text */
  text_width = available_width
             - ICON_SIZE
             - H_PADDING;
  clutter_actor_get_preferred_height (priv->entry, text_width,
                                      NULL,
                                      &text_height);

  child_box.x1 = priv->padding.left
               + ICON_SIZE
               + H_PADDING;
  child_box.y1 = (int) (priv->padding.top
               + ((ICON_SIZE - text_height) / 2));
  child_box.x2 = child_box.x1 + text_width;
  child_box.y2 = child_box.y1 + text_height;
  clutter_actor_allocate (priv->entry, &child_box, flags);
}

static void
mnb_web_status_row_paint (ClutterActor *actor)
{
  MnbWebStatusRowPrivate *priv = MNB_WEB_STATUS_ROW (actor)->priv;

  CLUTTER_ACTOR_CLASS (mnb_web_status_row_parent_class)->paint (actor);

  clutter_actor_paint (priv->avatar_bin);
  clutter_actor_paint (priv->entry);
}

static void
mnb_web_status_row_pick (ClutterActor       *actor,
                     const ClutterColor *pick_color)
{
  MnbWebStatusRowPrivate *priv = MNB_WEB_STATUS_ROW (actor)->priv;

  CLUTTER_ACTOR_CLASS (mnb_web_status_row_parent_class)->pick (actor, pick_color);

  /* we only want to propagate the pick if the entry is active */
  if (mnb_web_status_entry_get_is_active (MNB_WEB_STATUS_ENTRY (priv->entry)))
    clutter_actor_paint (priv->entry);
}

static void
mnb_web_status_row_map (ClutterActor *actor)
{
  MnbWebStatusRowPrivate *priv = MNB_WEB_STATUS_ROW (actor)->priv;

  CLUTTER_ACTOR_CLASS (mnb_web_status_row_parent_class)->map (actor);

  clutter_actor_map (priv->avatar_bin);
  clutter_actor_map (priv->entry);
}

static void
mnb_web_status_row_unmap (ClutterActor *actor)
{
  MnbWebStatusRowPrivate *priv = MNB_WEB_STATUS_ROW (actor)->priv;

  CLUTTER_ACTOR_CLASS (mnb_web_status_row_parent_class)->unmap (actor);

  clutter_actor_unmap (priv->avatar_bin);
  clutter_actor_unmap (priv->entry);
}

static gboolean
mnb_web_status_row_button_release (ClutterActor       *actor,
                                   ClutterButtonEvent *event)
{
  if (event->button == 1)
    {
      MnbWebStatusRowPrivate *priv = MNB_WEB_STATUS_ROW (actor)->priv;

      if (!mnb_web_status_entry_get_is_active (MNB_WEB_STATUS_ENTRY (priv->entry)))
        {
          mnb_web_status_entry_set_is_active (MNB_WEB_STATUS_ENTRY (priv->entry), TRUE);
          return FALSE;
        }
    }

  return FALSE;
}

static gboolean
mnb_web_status_row_enter (ClutterActor *actor,
                          ClutterCrossingEvent *event)
{
  MnbWebStatusRowPrivate *priv = MNB_WEB_STATUS_ROW (actor)->priv;

  if (!mnb_web_status_entry_get_is_active (MNB_WEB_STATUS_ENTRY (priv->entry)))
    {
      mnb_web_status_entry_set_in_hover (MNB_WEB_STATUS_ENTRY (priv->entry), TRUE);

      if (priv->is_online)
        mnb_web_status_entry_show_button (MNB_WEB_STATUS_ENTRY (priv->entry), TRUE);
    }

  priv->in_hover = TRUE;

  return TRUE;
}

static gboolean
mnb_web_status_row_leave (ClutterActor *actor,
                          ClutterCrossingEvent *event)
{
  MnbWebStatusRowPrivate *priv = MNB_WEB_STATUS_ROW (actor)->priv;

  if (!mnb_web_status_entry_get_is_active (MNB_WEB_STATUS_ENTRY (priv->entry)))
    {
      mnb_web_status_entry_set_in_hover (MNB_WEB_STATUS_ENTRY (priv->entry), FALSE);

      if (priv->is_online)
        mnb_web_status_entry_show_button (MNB_WEB_STATUS_ENTRY (priv->entry), FALSE);
    }

  priv->in_hover = FALSE;

  return TRUE;
}

static void
mnb_web_status_row_style_changed (NbtkWidget *widget)
{
  MnbWebStatusRowPrivate *priv = MNB_WEB_STATUS_ROW (widget)->priv;
  NbtkPadding *padding = NULL;

  nbtk_stylable_get (NBTK_STYLABLE (widget),
                     "padding", &padding,
                     NULL);

  if (padding)
    {
      priv->padding = *padding;

      g_boxed_free (NBTK_TYPE_PADDING, padding);
    }

  g_signal_emit_by_name (priv->entry, "style-changed");
}

static void
on_mojito_update_status (MojitoClientService *service,
                         const GError        *error,
                         gpointer             user_data)
{
  MnbWebStatusRow *row = user_data;
  MnbWebStatusRowPrivate *priv = row->priv;

  if (error)
    {
      g_warning ("Unable to update the status: %s", error->message);

      mnb_web_status_entry_set_status_text (MNB_WEB_STATUS_ENTRY (priv->entry),
                                            priv->last_status_text,
                                            &priv->last_status_time);
    }
}

static void
on_mojito_status_updated (MojitoClientService *service,
                          gboolean             success,
                          MnbWebStatusRow     *row)
{
  MnbWebStatusRowPrivate *priv = row->priv;

  if (!success)
    {
      mnb_web_status_entry_set_status_text (MNB_WEB_STATUS_ENTRY (priv->entry),
                                            priv->last_status_text,
                                            &priv->last_status_time);
    }
}

static void
on_status_entry_changed (MnbWebStatusEntry *entry,
                         const gchar    *new_status_text,
                         MnbWebStatusRow   *row)
{
  MnbWebStatusRowPrivate *priv = row->priv;
  const gchar *status_text;
  GTimeVal status_time;

  if (priv->service == NULL)
    return;

  mnb_web_status_entry_get_status_text (MNB_WEB_STATUS_ENTRY (priv->entry),
                                        &status_time);

  /* save the last status */
  g_free (priv->last_status_text);
  priv->last_status_text = g_strdup (status_text);
  priv->last_status_time = status_time;

  mojito_client_service_update_status (priv->service,
                                       on_mojito_update_status,
                                       new_status_text,
                                       row);
}

static void
on_mojito_avatar_retrieved (MojitoClientService *service,
                            const gchar         *avatar_path,
                            gpointer             user_data)
{
  MnbWebStatusRow *row = user_data;
  MnbWebStatusRowPrivate *priv = row->priv;
  GError *internal_error;

  internal_error = NULL;
  clutter_texture_set_load_async (CLUTTER_TEXTURE (priv->icon), TRUE);
  clutter_texture_set_from_file (CLUTTER_TEXTURE (priv->icon),
                                 avatar_path,
                                 &internal_error);
  if (internal_error)
    {
      g_warning ("Unable to set icon '%s' on '%s': %s",
                 priv->service_name,
                 avatar_path,
                 internal_error->message);

      clutter_texture_set_load_async (CLUTTER_TEXTURE (priv->icon), FALSE);
      clutter_texture_set_from_file (CLUTTER_TEXTURE (priv->icon),
                                     priv->no_icon_file,
                                     NULL);

      g_error_free (internal_error);
    }
}

static void
on_mojito_user_changed (MojitoClientService *service,
                        MnbWebStatusRow     *row G_GNUC_UNUSED)
{
  mojito_client_service_request_avatar (service);
}

static void
on_mojito_view_item_added (MojitoClientView *view,
                           MojitoItem       *item,
                           MnbWebStatusRow     *row)
{
  MnbWebStatusRowPrivate *priv = row->priv;
  const gchar *status_text;

  if (item == NULL || item->props == NULL)
    return;

  status_text = g_hash_table_lookup (item->props, "content");
  if (status_text != NULL && *status_text != '\0')
    mnb_web_status_entry_set_status_text (MNB_WEB_STATUS_ENTRY (priv->entry),
                                      status_text,
                                      &(item->date));
}

static void
on_mojito_view_open (MojitoClient     *client,
                     MojitoClientView *view,
                     gpointer          user_data)
{
  MnbWebStatusRow *row = user_data;
  MnbWebStatusRowPrivate *priv = row->priv;

  if (G_LIKELY (view != NULL))
    {
      priv->view = g_object_ref (view);

      if (row->priv->service != NULL)
        mojito_client_service_request_avatar (priv->service);
      else
        {
          priv->service = mojito_client_get_service (priv->client, priv->service_name);
          g_signal_connect (priv->service,
                            "user-changed", G_CALLBACK (on_mojito_user_changed),
                            row);
          g_signal_connect (priv->service,
                            "avatar-retrieved", G_CALLBACK (on_mojito_avatar_retrieved),
                            row);
          g_signal_connect (priv->service,
                            "status-updated", G_CALLBACK (on_mojito_status_updated),
                            row);

          mojito_client_service_request_avatar (priv->service);
        }


      /* now that we have a service and can update the status message
       * we need to enable the StatusEntry
       */
      clutter_actor_set_reactive (CLUTTER_ACTOR (row), priv->is_online);
      clutter_actor_set_opacity (priv->entry, priv->is_online ? 255 : 128);
      clutter_actor_set_opacity (priv->icon, priv->is_online ? 255 : 128);

      g_signal_connect (view, "item-added",
                        G_CALLBACK (on_mojito_view_item_added),
                        row);

      /* start the view to retrieve the last item */
      mojito_client_view_start (view);
    }
}

static gboolean
do_update_timeout (gpointer data)
{
  MnbWebStatusRow *row = data;
  MnbWebStatusRowPrivate *priv = row->priv;

  if (!row->priv->is_online)
    return TRUE;

  if (row->priv->view != NULL)
    mojito_client_view_refresh (row->priv->view);
  else
    {
      gchar *service_name;

      /* for the View we need a parametrized service name */
      service_name = g_strdup_printf ("%s:own=1", priv->service_name);
      mojito_client_open_view_for_service (priv->client,
                                           service_name, 1,
                                           on_mojito_view_open,
                                           row);
      g_free (service_name);

      return TRUE;
    }

  /* we only call GetPersonaIcon if we have a view open */
  if (row->priv->service != NULL)
    mojito_client_service_request_avatar (row->priv->service);

  return TRUE;
}

static void
on_mojito_online_changed (MojitoClient *client,
                          gboolean      is_online,
                          MnbWebStatusRow *row)
{
  MnbWebStatusRowPrivate *priv = row->priv;

  priv->is_online = is_online;

  clutter_actor_set_reactive (CLUTTER_ACTOR (row), priv->is_online);

  if (priv->is_online)
    {
      clutter_actor_set_opacity (priv->entry, 255);
      clutter_actor_set_opacity (priv->icon, 255);

      if (priv->view != NULL)
        mojito_client_view_refresh (row->priv->view);
      else
        {
          gchar *service_name;

          /* for the View we need a parametrized service name */
          service_name = g_strdup_printf ("%s:own=1", priv->service_name);
          mojito_client_open_view_for_service (priv->client,
                                               service_name, 1,
                                               on_mojito_view_open,
                                               row);
          g_free (service_name);
        }
    }
  else
    {
      clutter_actor_set_opacity (priv->entry, 128);
      clutter_actor_set_opacity (priv->icon, 128);
    }
}

static void
on_mojito_is_online (MojitoClient *client,
                     gboolean      is_online,
                     gpointer      data)
{
  MnbWebStatusRow *row = data;
  MnbWebStatusRowPrivate *priv = row->priv;

  priv->is_online = is_online;

  if (priv->is_online);
    {
      gchar *service_name;

      /* for the View we need a parametrized service name */
      service_name = g_strdup_printf ("%s:own=1", priv->service_name);
      mojito_client_open_view_for_service (priv->client,
                                           service_name, 1,
                                           on_mojito_view_open,
                                           row);
      g_free (service_name);
    }
}

static void
mnb_web_status_row_finalize (GObject *gobject)
{
  MnbWebStatusRowPrivate *priv = MNB_WEB_STATUS_ROW (gobject)->priv;

  if (priv->update_id)
    {
      g_source_remove (priv->update_id);
      priv->update_id = 0;
    }

  if (priv->view)
    g_object_unref (priv->view);

  if (priv->service)
    g_object_unref (priv->service);

  if (priv->client)
    g_object_unref (priv->client);

  g_free (priv->service_name);

  g_free (priv->no_icon_file);

  g_free (priv->last_status_text);

  clutter_actor_destroy (priv->avatar_bin);
  clutter_actor_destroy (priv->entry);

  G_OBJECT_CLASS (mnb_web_status_row_parent_class)->finalize (gobject);
}

static void
mnb_web_status_row_set_property (GObject      *gobject,
                               guint         prop_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
  MnbWebStatusRowPrivate *priv = MNB_WEB_STATUS_ROW (gobject)->priv;

  switch (prop_id)
    {
    case PROP_SERVICE_NAME:
      g_free (priv->service_name);
      priv->service_name = g_value_dup_string (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
      break;
    }
}

static void
mnb_web_status_row_get_property (GObject    *gobject,
                               guint       prop_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
  MnbWebStatusRowPrivate *priv = MNB_WEB_STATUS_ROW (gobject)->priv;

  switch (prop_id)
    {
    case PROP_SERVICE_NAME:
      g_value_set_string (value, priv->service_name);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
      break;
    }
}

static void
mnb_web_status_row_constructed (GObject *gobject)
{
  MnbWebStatusRow *row = MNB_WEB_STATUS_ROW (gobject);
  MnbWebStatusRowPrivate *priv = row->priv;

  g_assert (priv->service_name != NULL);
  g_assert (priv->client != NULL);

  priv->display_name = get_mojito_service_name (priv->service_name);

  priv->entry = CLUTTER_ACTOR (mnb_web_status_entry_new (priv->display_name));
  clutter_actor_set_parent (CLUTTER_ACTOR (priv->entry),
                            CLUTTER_ACTOR (row));
  clutter_actor_set_opacity (CLUTTER_ACTOR (priv->entry), 128);
  g_signal_connect (priv->entry, "status-changed",
                    G_CALLBACK (on_status_entry_changed),
                    row);

  /* we check if we're online first */
  priv->is_online = FALSE;
  mojito_client_is_online (priv->client, on_mojito_is_online, row);

  priv->update_id = g_timeout_add_seconds (5 * 60, do_update_timeout, row);

  if (G_OBJECT_CLASS (mnb_web_status_row_parent_class)->constructed)
    G_OBJECT_CLASS (mnb_web_status_row_parent_class)->constructed (gobject);
}

static void
mnb_web_status_row_class_init (MnbWebStatusRowClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  ClutterActorClass *actor_class = CLUTTER_ACTOR_CLASS (klass);
  GParamSpec *pspec;

  g_type_class_add_private (klass, sizeof (MnbWebStatusRowPrivate));

  gobject_class->constructed = mnb_web_status_row_constructed;
  gobject_class->set_property = mnb_web_status_row_set_property;
  gobject_class->get_property = mnb_web_status_row_get_property;
  gobject_class->finalize = mnb_web_status_row_finalize;

  actor_class->get_preferred_width = mnb_web_status_row_get_preferred_width;
  actor_class->get_preferred_height = mnb_web_status_row_get_preferred_height;
  actor_class->allocate = mnb_web_status_row_allocate;
  actor_class->paint = mnb_web_status_row_paint;
  actor_class->pick = mnb_web_status_row_pick;
  actor_class->map = mnb_web_status_row_map;
  actor_class->unmap = mnb_web_status_row_unmap;
  actor_class->enter_event = mnb_web_status_row_enter;
  actor_class->leave_event = mnb_web_status_row_leave;
  actor_class->button_release_event = mnb_web_status_row_button_release;

  pspec = g_param_spec_string ("service-name",
                               "Service Name",
                               "The name of the Mojito service",
                               NULL,
                               G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
  g_object_class_install_property (gobject_class, PROP_SERVICE_NAME, pspec);
}

static void
mnb_web_status_row_init (MnbWebStatusRow *self)
{
  MnbWebStatusRowPrivate *priv;
  GError *error;

  self->priv = priv = MNB_WEB_STATUS_ROW_GET_PRIVATE (self);

  g_signal_connect (self, "style-changed",
                    G_CALLBACK (mnb_web_status_row_style_changed), NULL);


  clutter_actor_set_reactive (CLUTTER_ACTOR (self), FALSE);

  priv->avatar_bin = CLUTTER_ACTOR (nbtk_bin_new ());
  nbtk_widget_set_style_class_name (NBTK_WIDGET (priv->avatar_bin),
                                    "MnbStatusAvatar");
  clutter_actor_set_parent (priv->avatar_bin, CLUTTER_ACTOR (self));

  priv->no_icon_file = g_build_filename (THEMEDIR,
                                         "no_image_icon.png",
                                         NULL);

  error = NULL;
  priv->icon = g_object_new (PENGE_TYPE_MAGIC_TEXTURE, NULL);
  clutter_texture_set_from_file (CLUTTER_TEXTURE (priv->icon),
                                 priv->no_icon_file,
                                 &error);
  if (G_UNLIKELY (priv->icon == NULL))
    {
      const ClutterColor color = { 204, 204, 0, 255 };

      priv->icon = clutter_rectangle_new_with_color (&color);

      if (error)
        {
          g_warning ("Unable to load '%s': %s",
                     priv->no_icon_file,
                     error->message);
          g_error_free (error);
        }
    }

  clutter_actor_set_opacity (priv->icon, 128);
  nbtk_bin_set_child (NBTK_BIN (priv->avatar_bin), priv->icon);

  priv->client = mojito_client_new ();
  g_signal_connect (priv->client, "online-changed",
                    G_CALLBACK (on_mojito_online_changed),
                    self);
}

NbtkWidget *
mnb_web_status_row_new (const gchar *service_name)
{
  g_return_val_if_fail (service_name != NULL, NULL);

  return g_object_new (MNB_TYPE_WEB_STATUS_ROW,
                       "service-name", service_name,
                       NULL);
}

void
mnb_web_status_row_force_update (MnbWebStatusRow *row)
{
  MnbWebStatusRowPrivate *priv;

  g_return_if_fail (MNB_IS_WEB_STATUS_ROW (row));

  priv = row->priv;

  if (priv->view)
    mojito_client_view_refresh (priv->view);
}
