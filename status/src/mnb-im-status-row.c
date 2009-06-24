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
#include <glib/gi18n.h>
#include <stdlib.h>

#include <libmissioncontrol/mission-control.h>

#include "mnb-im-status-row.h"
#include "mnb-web-status-entry.h"

#define MNB_IM_STATUS_ROW_GET_PRIVATE(obj)       (G_TYPE_INSTANCE_GET_PRIVATE ((obj), MNB_TYPE_IM_STATUS_ROW, MnbIMStatusRowPrivate))

#define ICON_SIZE       48.0
#define H_PADDING       9.0

struct _MnbIMStatusRowPrivate
{
  ClutterActor *icon;
  ClutterActor *entry;

  gchar *account_name;
  gchar *display_name;

  gchar *no_icon_file;

  gchar *last_status_text;

  NbtkPadding padding;

  guint in_hover  : 1;
  guint is_online : 1;

  gfloat icon_separator_x;

  McAccount *account;

  TpConnectionPresenceType presence;
  gchar *status;
};

enum
{
  PROP_0,

  PROP_ACCOUNT_NAME
};

G_DEFINE_TYPE (MnbIMStatusRow, mnb_im_status_row, NBTK_TYPE_WIDGET);

static void
mnb_im_status_row_get_preferred_width (ClutterActor *actor,
                                    gfloat        for_height,
                                    gfloat       *min_width_p,
                                    gfloat       *natural_width_p)
{
  MnbIMStatusRowPrivate *priv = MNB_IM_STATUS_ROW (actor)->priv;
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
mnb_im_status_row_get_preferred_height (ClutterActor *actor,
                                     gfloat        for_width,
                                     gfloat       *min_height_p,
                                     gfloat       *natural_height_p)
{
  MnbIMStatusRowPrivate *priv = MNB_IM_STATUS_ROW (actor)->priv;

  if (min_height_p)
    *min_height_p = priv->padding.top + ICON_SIZE + priv->padding.bottom;

  if (natural_height_p)
    *natural_height_p = priv->padding.top + ICON_SIZE + priv->padding.bottom;
}

static void
mnb_im_status_row_allocate (ClutterActor          *actor,
                         const ClutterActorBox *box,
                         ClutterAllocationFlags flags)
{
  MnbIMStatusRowPrivate *priv = MNB_IM_STATUS_ROW (actor)->priv;
  ClutterActorClass *parent_class;
  gfloat available_width, available_height;
  gfloat min_width, min_height;
  gfloat natural_width, natural_height;
  gfloat text_width, text_height;
  NbtkPadding border = { 0, };
  ClutterActorBox child_box = { 0, };

  parent_class = CLUTTER_ACTOR_CLASS (mnb_im_status_row_parent_class);
  parent_class->allocate (actor, box, flags);

//  nbtk_widget_get_border (NBTK_WIDGET (actor), &border);

  available_width  = box->x2 - box->x1
                   - priv->padding.left - priv->padding.right
                   - border.left - border.right;
  available_height = box->y2 - box->y1
                   - priv->padding.top - priv->padding.bottom
                   - border.top - border.right;

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
  child_box.x1 = border.left + priv->padding.left;
  child_box.y1 = border.top  + priv->padding.top;
  child_box.x2 = child_box.x1 + ICON_SIZE;
  child_box.y2 = child_box.y1 + ICON_SIZE;
  clutter_actor_allocate (priv->icon, &child_box, flags);

  /* separator */
  priv->icon_separator_x = child_box.x2 + H_PADDING;

  /* text */
  text_width = available_width
             - ICON_SIZE
             - H_PADDING;
  clutter_actor_get_preferred_height (priv->entry, text_width,
                                      NULL,
                                      &text_height);

  child_box.x1 = border.left + priv->padding.left
               + ICON_SIZE
               + H_PADDING;
  child_box.y1 = (int) (border.top + priv->padding.top
               + ((ICON_SIZE - text_height) / 2));
  child_box.x2 = child_box.x1 + text_width;
  child_box.y2 = child_box.y1 + text_height;
  clutter_actor_allocate (priv->entry, &child_box, flags);
}

static void
mnb_im_status_row_paint (ClutterActor *actor)
{
  MnbIMStatusRowPrivate *priv = MNB_IM_STATUS_ROW (actor)->priv;

  CLUTTER_ACTOR_CLASS (mnb_im_status_row_parent_class)->paint (actor);

  if (priv->icon)
    clutter_actor_paint (priv->icon);

  if (priv->entry)
    clutter_actor_paint (priv->entry);
}

static void
mnb_im_status_row_pick (ClutterActor       *actor,
                     const ClutterColor *pick_color)
{
  MnbIMStatusRowPrivate *priv = MNB_IM_STATUS_ROW (actor)->priv;

  CLUTTER_ACTOR_CLASS (mnb_im_status_row_parent_class)->pick (actor, pick_color);

  if (priv->entry)
    clutter_actor_paint (priv->entry);
}

static void
mnb_im_status_row_map (ClutterActor *actor)
{
  MnbIMStatusRowPrivate *priv = MNB_IM_STATUS_ROW (actor)->priv;

  CLUTTER_ACTOR_CLASS (mnb_im_status_row_parent_class)->map (actor);

  if (priv->icon)
    clutter_actor_map (priv->icon);

  if (priv->entry)
    clutter_actor_map (priv->entry);
}

static void
mnb_im_status_row_unmap (ClutterActor *actor)
{
  MnbIMStatusRowPrivate *priv = MNB_IM_STATUS_ROW (actor)->priv;

  CLUTTER_ACTOR_CLASS (mnb_im_status_row_parent_class)->unmap (actor);

  if (priv->icon)
    clutter_actor_unmap (priv->icon);

  if (priv->entry)
    clutter_actor_unmap (priv->entry);
}

static gboolean
mnb_im_status_row_button_release (ClutterActor *actor,
                               ClutterButtonEvent *event)
{
  if (event->button == 1)
    {
      MnbIMStatusRowPrivate *priv = MNB_IM_STATUS_ROW (actor)->priv;

      if (!mnb_web_status_entry_get_is_active (MNB_WEB_STATUS_ENTRY (priv->entry)))
        {
          mnb_web_status_entry_set_is_active (MNB_WEB_STATUS_ENTRY (priv->entry), TRUE);
          return TRUE;
        }
    }

  return FALSE;
}

static gboolean
mnb_im_status_row_enter (ClutterActor *actor,
                      ClutterCrossingEvent *event)
{
  MnbIMStatusRowPrivate *priv = MNB_IM_STATUS_ROW (actor)->priv;

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
mnb_im_status_row_leave (ClutterActor *actor,
                        ClutterCrossingEvent *event)
{
  MnbIMStatusRowPrivate *priv = MNB_IM_STATUS_ROW (actor)->priv;

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
mnb_im_status_row_style_changed (NbtkWidget *widget)
{
  MnbIMStatusRowPrivate *priv = MNB_IM_STATUS_ROW (widget)->priv;
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

#if 0
static void
on_mojito_update_status (MojitoClientService *service,
                         gboolean             success,
                         const GError        *error,
                         gpointer             user_data)
{
  MnbIMStatusRow *row = user_data;
  MnbIMStatusRowPrivate *priv = row->priv;

  if (!success)
    {
      g_warning ("Unable to update the status: %s", error->message);

      mnb_web_status_entry_set_status_text (MNB_WEB_STATUS_ENTRY (priv->entry),
                                        priv->last_status_text,
                                        NULL);
    }
}
#endif

static void
on_status_entry_changed (MnbWebStatusEntry *entry,
                         const gchar    *new_status_text,
                         MnbIMStatusRow   *row)
{
  MnbIMStatusRowPrivate *priv = row->priv;

  if (priv->account == NULL)
    return;

  /* save the last status */
  g_free (priv->last_status_text);
  priv->last_status_text =
    g_strdup (mnb_web_status_entry_get_status_text (MNB_WEB_STATUS_ENTRY (priv->entry)));

#if 0
  mojito_client_service_update_status (priv->service,
                                       on_mojito_update_status,
                                       new_status_text,
                                       row);
#endif
}

static void
mnb_im_status_row_finalize (GObject *gobject)
{
  MnbIMStatusRowPrivate *priv = MNB_IM_STATUS_ROW (gobject)->priv;

  if (priv->account)
    {
      g_object_unref (priv->account);
      priv->account = NULL;
    }

  g_free (priv->account_name);
  g_free (priv->display_name);
  g_free (priv->no_icon_file);
  g_free (priv->last_status_text);

  clutter_actor_destroy (priv->icon);
  clutter_actor_destroy (priv->entry);

  G_OBJECT_CLASS (mnb_im_status_row_parent_class)->finalize (gobject);
}

static void
mnb_im_status_row_set_property (GObject      *gobject,
                               guint         prop_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
  MnbIMStatusRowPrivate *priv = MNB_IM_STATUS_ROW (gobject)->priv;

  switch (prop_id)
    {
    case PROP_ACCOUNT_NAME:
      g_free (priv->account_name);
      priv->account_name = g_value_dup_string (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
      break;
    }
}

static void
mnb_im_status_row_get_property (GObject    *gobject,
                               guint       prop_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
  MnbIMStatusRowPrivate *priv = MNB_IM_STATUS_ROW (gobject)->priv;

  switch (prop_id)
    {
    case PROP_ACCOUNT_NAME:
      g_value_set_string (value, priv->account_name);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
      break;
    }
}

static void
mnb_im_status_row_constructed (GObject *gobject)
{
  MnbIMStatusRow *row = MNB_IM_STATUS_ROW (gobject);
  MnbIMStatusRowPrivate *priv = row->priv;

  g_assert (priv->account_name != NULL);

  priv->account = mc_account_lookup (priv->account_name);
  priv->display_name = g_strdup (mc_account_get_display_name (priv->account));

  priv->entry = CLUTTER_ACTOR (mnb_web_status_entry_new (priv->display_name));
  clutter_actor_set_parent (CLUTTER_ACTOR (priv->entry),
                            CLUTTER_ACTOR (row));
  clutter_actor_set_reactive (CLUTTER_ACTOR (priv->entry), TRUE);
  clutter_actor_set_opacity (CLUTTER_ACTOR (priv->entry), 128);
  g_signal_connect (priv->entry, "status-changed",
                    G_CALLBACK (on_status_entry_changed),
                    row);

  priv->is_online = FALSE;

  if (G_OBJECT_CLASS (mnb_im_status_row_parent_class)->constructed)
    G_OBJECT_CLASS (mnb_im_status_row_parent_class)->constructed (gobject);
}

static void
mnb_im_status_row_class_init (MnbIMStatusRowClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  ClutterActorClass *actor_class = CLUTTER_ACTOR_CLASS (klass);
  GParamSpec *pspec;

  g_type_class_add_private (klass, sizeof (MnbIMStatusRowPrivate));

  gobject_class->constructed = mnb_im_status_row_constructed;
  gobject_class->set_property = mnb_im_status_row_set_property;
  gobject_class->get_property = mnb_im_status_row_get_property;
  gobject_class->finalize = mnb_im_status_row_finalize;

  actor_class->get_preferred_width = mnb_im_status_row_get_preferred_width;
  actor_class->get_preferred_height = mnb_im_status_row_get_preferred_height;
  actor_class->allocate = mnb_im_status_row_allocate;
  actor_class->paint = mnb_im_status_row_paint;
  actor_class->pick = mnb_im_status_row_pick;
  actor_class->map = mnb_im_status_row_map;
  actor_class->unmap = mnb_im_status_row_unmap;
  actor_class->enter_event = mnb_im_status_row_enter;
  actor_class->leave_event = mnb_im_status_row_leave;
  actor_class->button_release_event = mnb_im_status_row_button_release;

  pspec = g_param_spec_string ("account-name",
                               "Account Name",
                               "The unique name of the MissionControl account",
                               NULL,
                               G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
  g_object_class_install_property (gobject_class, PROP_ACCOUNT_NAME, pspec);
}

static void
mnb_im_status_row_init (MnbIMStatusRow *self)
{
  MnbIMStatusRowPrivate *priv;
  GError *error;

  self->priv = priv = MNB_IM_STATUS_ROW_GET_PRIVATE (self);

  g_signal_connect (self, "style-changed",
                    G_CALLBACK (mnb_im_status_row_style_changed), NULL);


  clutter_actor_set_reactive (CLUTTER_ACTOR (self), FALSE);

  priv->no_icon_file = g_build_filename (DATADIR,
                                         "mutter-moblin",
                                         "theme",
                                         "status",
                                         "no_image_icon.png",
                                         NULL);

  error = NULL;
  priv->icon = clutter_texture_new_from_file (priv->no_icon_file, &error);
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
  clutter_actor_set_size (priv->icon, ICON_SIZE, ICON_SIZE);
  clutter_actor_set_parent (priv->icon, CLUTTER_ACTOR (self));
}

NbtkWidget *
mnb_im_status_row_new (const gchar *account_name)
{
  g_return_val_if_fail (account_name != NULL, NULL);

  return g_object_new (MNB_TYPE_IM_STATUS_ROW,
                       "account-name", account_name,
                       NULL);
}

void
mnb_im_status_row_set_online (MnbIMStatusRow *row,
                              gboolean        is_online)
{
  MnbIMStatusRowPrivate *priv;

  g_return_if_fail (MNB_IS_IM_STATUS_ROW (row));

  priv = row->priv;

  if (priv->is_online != is_online)
    {
      priv->is_online = is_online;

      clutter_actor_set_opacity (priv->icon, priv->is_online ? 0xff : 0x88);
      clutter_actor_set_opacity (priv->entry, priv->is_online ? 0xff : 0x88);
      clutter_actor_set_reactive (CLUTTER_ACTOR (row),
                                  priv->is_online);
    }
}

void
mnb_im_status_row_set_status (MnbIMStatusRow           *row,
                              TpConnectionPresenceType  presence,
                              const gchar              *status)
{
  MnbIMStatusRowPrivate *priv;

  g_return_if_fail (MNB_IS_IM_STATUS_ROW (row));

  priv = row->priv;

  priv->presence = presence;

  if (status == NULL || *status == '\0')
    {
      switch (priv->presence)
        {
        case TP_CONNECTION_PRESENCE_TYPE_OFFLINE:
        case TP_CONNECTION_PRESENCE_TYPE_UNSET:
          status = _("Offline");
          break;

        case TP_CONNECTION_PRESENCE_TYPE_AVAILABLE:
          status = _("Available");
          break;

        case TP_CONNECTION_PRESENCE_TYPE_AWAY:
          status = _("Away");
          break;

        case TP_CONNECTION_PRESENCE_TYPE_EXTENDED_AWAY:
          status = _("Away");
          break;

        case TP_CONNECTION_PRESENCE_TYPE_HIDDEN:
          status = _("Hidden");
          break;

        case TP_CONNECTION_PRESENCE_TYPE_BUSY:
          status = _("Do Not Disturb");
          break;

        default:
          status = NULL;
          break;
        }
    }

  g_free (priv->status);
  priv->status = g_strdup (status);

  if (priv->entry == NULL)
    return;

  if (priv->status != NULL)
    mnb_web_status_entry_set_status_text (MNB_WEB_STATUS_ENTRY (priv->entry),
                                          priv->status,
                                          NULL);
}
