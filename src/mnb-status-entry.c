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

#include <penge/penge-utils.h>
#include "mnb-status-entry.h"
#include "marshal.h"

#define H_PADDING               (6.0)
#define CANCEL_ICON_SIZE        (22)

#define MNB_STATUS_ENTRY_GET_PRIVATE(obj)       (G_TYPE_INSTANCE_GET_PRIVATE ((obj), MNB_TYPE_STATUS_ENTRY, MnbStatusEntryPrivate))

struct _MnbStatusEntryPrivate
{
  ClutterActor *status_entry;
  ClutterActor *service_label;
  ClutterActor *cancel_icon;
  ClutterActor *button;

  gchar *service_name;
  gchar *display_name;
  gchar *status_text;
  gchar *old_status_text;
  gchar *status_time;

  ClutterUnit separator_x;

  NbtkPadding padding;

  guint in_hover  : 1;
  guint is_active : 1;
};

enum
{
  PROP_0,

  PROP_SERVICE_NAME
};

enum
{
  STATUS_CHANGED,
  UPDATE_CANCELLED,

  LAST_SIGNAL
};

static guint entry_signals[LAST_SIGNAL] = { 0, };

G_DEFINE_TYPE (MnbStatusEntry, mnb_status_entry, NBTK_TYPE_WIDGET);

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
on_cancel_clicked (NbtkButton     *button,
                   MnbStatusEntry *entry)
{
  MnbStatusEntryPrivate *priv = entry->priv;
  ClutterActor *text;

  text = nbtk_entry_get_clutter_text (NBTK_ENTRY (priv->status_entry));

  nbtk_button_set_label (NBTK_BUTTON (priv->button), _("Edit"));

  clutter_actor_set_reactive (text, FALSE);

  clutter_text_set_text (CLUTTER_TEXT (text), priv->old_status_text);
  clutter_text_set_editable (CLUTTER_TEXT (text), FALSE);
  clutter_text_set_activatable (CLUTTER_TEXT (text), FALSE);

  clutter_actor_show (priv->service_label);
  clutter_actor_hide (priv->cancel_icon);

  nbtk_widget_set_style_pseudo_class (NBTK_WIDGET (entry), "hover");

  g_free (priv->old_status_text);
  priv->old_status_text = NULL;

  priv->is_active = FALSE;

  g_signal_emit (entry, entry_signals[UPDATE_CANCELLED], 0);
}

static void
on_button_clicked (NbtkButton     *button,
                   MnbStatusEntry *entry)
{
  mnb_status_entry_set_is_active (entry,
                                  entry->priv->is_active == TRUE ? FALSE
                                                                 : TRUE);
}

static void
mnb_status_entry_get_preferred_width (ClutterActor *actor,
                                      ClutterUnit   for_height,
                                      ClutterUnit  *min_width_p,
                                      ClutterUnit  *natural_width_p)
{
  MnbStatusEntryPrivate *priv = MNB_STATUS_ENTRY (actor)->priv;
  ClutterUnit min_width, natural_width;

  clutter_actor_get_preferred_width (priv->status_entry, for_height,
                                     &min_width,
                                     &natural_width);

  if (min_width_p)
    *min_width_p = priv->padding.left + min_width + priv->padding.right;

  if (natural_width_p)
    *natural_width_p = priv->padding.left
                     + natural_width
                     + priv->padding.right;
}

static void
mnb_status_entry_get_preferred_height (ClutterActor *actor,
                                       ClutterUnit   for_width,
                                       ClutterUnit  *min_height_p,
                                       ClutterUnit  *natural_height_p)
{
  MnbStatusEntryPrivate *priv = MNB_STATUS_ENTRY (actor)->priv;
  ClutterUnit min_height, natural_height;

  clutter_actor_get_preferred_height (priv->status_entry, for_width,
                                      &min_height,
                                      &natural_height);

  if (min_height_p)
    *min_height_p = priv->padding.top + min_height + priv->padding.bottom;

  if (natural_height_p)
    *natural_height_p = priv->padding.top
                      + natural_height
                      + priv->padding.bottom;
}

static void
mnb_status_entry_allocate (ClutterActor          *actor,
                           const ClutterActorBox *box,
                           gboolean               origin_changed)
{
  MnbStatusEntryPrivate *priv = MNB_STATUS_ENTRY (actor)->priv;
  ClutterActorClass *parent_class;
  ClutterUnit available_width, available_height;
  ClutterUnit min_width, min_height;
  ClutterUnit natural_width, natural_height;
  ClutterUnit button_width, button_height;
  ClutterUnit service_width, service_height;
  ClutterUnit icon_width, icon_height;
  ClutterUnit text_width, text_height;
  ClutterActorBox child_box = { 0, };

  parent_class = CLUTTER_ACTOR_CLASS (mnb_status_entry_parent_class);
  parent_class->allocate (actor, box, origin_changed);

  available_width  = (int) (box->x2 - box->x1
                   - priv->padding.left
                   - priv->padding.right);
  available_height = (int) (box->y2 - box->y1
                   - priv->padding.top
                   - priv->padding.bottom);

  clutter_actor_get_preferred_size (priv->button,
                                    &min_width, &min_height,
                                    &natural_width, &natural_height);

  button_width  = CLAMP (natural_width, min_width, available_width);
  button_height = CLAMP (natural_height, min_height, available_height);

  /* layout
   *
   * +----------------------------------------------------+
   * | +---------------------+-------------+---+--------+ |
   * | |xxxxxxxxxxxxxxxxx... |xxxxxxxxxxxxx| X | xxxxxx | |
   * | +---------------------+-------------+---+--------+ |
   * +----------------------------------------------------+
   *
   *    status               | service     |   | button
   */

  icon_height = CANCEL_ICON_SIZE;
  if (CLUTTER_ACTOR_IS_MAPPED (priv->cancel_icon))
    clutter_actor_get_preferred_width (priv->cancel_icon,
                                       icon_height,
                                       NULL,
                                       &icon_width);
  else
    icon_width = 0;

  if (CLUTTER_ACTOR_IS_MAPPED (priv->service_label))
    {
      clutter_actor_get_preferred_width (priv->service_label,
                                         available_height,
                                         NULL,
                                         &service_width);
      clutter_actor_get_preferred_height (priv->service_label,
                                          service_width,
                                          NULL,
                                          &service_height);
    }
  else
    service_width = (2 * H_PADDING);

  /* status entry */
  text_width = (int) (available_width
             - priv->padding.right
             - button_width
             - service_width
             - icon_width
             - (6 * H_PADDING));

  clutter_actor_get_preferred_height (priv->status_entry, text_width,
                                      NULL,
                                      &text_height);

  child_box.x1 = (int) priv->padding.left;
  child_box.y1 = (int) priv->padding.top;
  child_box.x2 = (int) (child_box.x1 + text_width);
  child_box.y2 = (int) (child_box.y1 + text_height);
  clutter_actor_allocate (priv->status_entry, &child_box, origin_changed);

  /* service label */
  if (CLUTTER_ACTOR_IS_MAPPED (priv->service_label))
    {
      child_box.x1 = (int) (available_width
                   - priv->padding.right
                   - button_width
                   - H_PADDING
                   - icon_width
                   - H_PADDING
                   - service_width);
      child_box.y1 = (int) (priv->padding.top
                   + ((available_height - service_height) / 2));
      child_box.x2 = (int) (child_box.x1 + service_width);
      child_box.y2 = (int) (child_box.y1 + service_height);
      clutter_actor_allocate (priv->service_label, &child_box, origin_changed);
    }

  /* cancel icon */
  if (CLUTTER_ACTOR_IS_MAPPED (priv->cancel_icon))
    {
      child_box.x1 = (int) (available_width
                   - priv->padding.right
                   - button_width
                   - H_PADDING
                   - icon_width);
      child_box.y1 = (int) (priv->padding.top
                   + ((available_height - icon_height) / 2));
      child_box.x2 = (int) (child_box.x1 + icon_width);
      child_box.y2 = (int) (child_box.y1 + icon_height);
      clutter_actor_allocate (priv->cancel_icon, &child_box, origin_changed);
    }

  /* separator */
  priv->separator_x = available_width
                    - priv->padding.right
                    - button_width
                    - (H_PADDING - 1);

  /* button */
  child_box.x1 = (int) (priv->separator_x + (2 * H_PADDING)
               + (((available_width - priv->separator_x) - button_width) / 2));
  child_box.y1 = (int) (priv->padding.top
               + ((available_height - button_height) / 2));
  child_box.x2 = (int) (child_box.x1 + button_width);
  child_box.y2 = (int) (child_box.y1 + button_height);
  clutter_actor_allocate (priv->button, &child_box, origin_changed);
}

static void
mnb_status_entry_paint (ClutterActor *actor)
{
  MnbStatusEntryPrivate *priv = MNB_STATUS_ENTRY (actor)->priv;

  CLUTTER_ACTOR_CLASS (mnb_status_entry_parent_class)->paint (actor);

  if (priv->status_entry && CLUTTER_ACTOR_IS_MAPPED (priv->status_entry))
    clutter_actor_paint (priv->status_entry);

  if (priv->service_label && CLUTTER_ACTOR_IS_MAPPED (priv->service_label))
    clutter_actor_paint (priv->service_label);

  if (priv->cancel_icon && CLUTTER_ACTOR_IS_MAPPED (priv->cancel_icon))
    clutter_actor_paint (priv->cancel_icon);

  if (priv->button &&
      CLUTTER_ACTOR_IS_MAPPED (priv->button) &&
      priv->in_hover &&
      priv->separator_x != 0)
    {
      ClutterActorBox alloc = { 0, };
      gfloat x_pos, start_y, end_y;

      clutter_actor_get_allocation_box (actor, &alloc);
      x_pos = CLUTTER_UNITS_TO_FLOAT (priv->separator_x);
      start_y = CLUTTER_UNITS_TO_FLOAT (priv->padding.top);
      end_y = CLUTTER_UNITS_TO_FLOAT (alloc.y2 - priv->padding.bottom - 8);

      cogl_set_source_color4ub (204, 204, 204, 255);
      cogl_path_move_to (x_pos, start_y);
      cogl_path_line_to (x_pos, end_y);
      cogl_path_stroke ();
    }

  if (priv->button && CLUTTER_ACTOR_IS_MAPPED (priv->button))
    clutter_actor_paint (priv->button);
}

static void
mnb_status_entry_pick (ClutterActor       *actor,
                       const ClutterColor *pick_color)
{
  MnbStatusEntryPrivate *priv = MNB_STATUS_ENTRY (actor)->priv;

  CLUTTER_ACTOR_CLASS (mnb_status_entry_parent_class)->pick (actor,
                                                             pick_color);

  if (priv->status_entry && clutter_actor_should_pick_paint (priv->status_entry))
    clutter_actor_paint (priv->status_entry);

  if (priv->service_label && clutter_actor_should_pick_paint (priv->service_label))
    clutter_actor_paint (priv->service_label);

  if (priv->cancel_icon && clutter_actor_should_pick_paint (priv->cancel_icon))
    clutter_actor_paint (priv->cancel_icon);

  if (priv->button && clutter_actor_should_pick_paint (priv->button))
    clutter_actor_paint (priv->button);
}

static void
mnb_status_entry_map (ClutterActor *actor)
{
  MnbStatusEntryPrivate *priv = MNB_STATUS_ENTRY (actor)->priv;

  CLUTTER_ACTOR_CLASS (mnb_status_entry_parent_class)->map (actor);

  if (priv->status_entry)
    clutter_actor_map (priv->status_entry);

  if (priv->service_label)
    clutter_actor_map (priv->service_label);

  if (priv->cancel_icon)
    clutter_actor_map (priv->cancel_icon);

  if (priv->button)
    clutter_actor_map (priv->button);
}

static void
mnb_status_entry_unmap (ClutterActor *actor)
{
  MnbStatusEntryPrivate *priv = MNB_STATUS_ENTRY (actor)->priv;

  CLUTTER_ACTOR_CLASS (mnb_status_entry_parent_class)->unmap (actor);

  if (priv->status_entry)
    clutter_actor_unmap (priv->status_entry);

  if (priv->service_label)
    clutter_actor_unmap (priv->service_label);

  if (priv->cancel_icon)
    clutter_actor_unmap (priv->cancel_icon);

  if (priv->button)
    clutter_actor_unmap (priv->button);
}

static gboolean
mnb_status_entry_button_release (ClutterActor *actor,
                                 ClutterButtonEvent *event)
{
  MnbStatusEntryPrivate *priv = MNB_STATUS_ENTRY (actor)->priv;

  if (event->button == 1)
    {
      if (!priv->is_active)
        {
          mnb_status_entry_set_is_active (MNB_STATUS_ENTRY (actor), TRUE);

          return TRUE;
        }
    }

  return FALSE;
}

static void
mnb_status_entry_style_changed (NbtkWidget *widget)
{
  MnbStatusEntryPrivate *priv = MNB_STATUS_ENTRY (widget)->priv;
  NbtkPadding *padding = NULL;

  nbtk_stylable_get (NBTK_STYLABLE (widget),
                     "padding", &padding,
                     NULL);

  if (padding)
    {
      priv->padding = *padding;

      g_boxed_free (NBTK_TYPE_PADDING, padding);
    }

  g_signal_emit_by_name (priv->status_entry, "style-changed");
  g_signal_emit_by_name (priv->service_label, "style-changed");
  g_signal_emit_by_name (priv->button, "style-changed");

  /* chain up */
  NBTK_WIDGET_CLASS (mnb_status_entry_parent_class)->style_changed (widget);
}

static void
mnb_status_entry_finalize (GObject *gobject)
{
  MnbStatusEntryPrivate *priv = MNB_STATUS_ENTRY (gobject)->priv;

  g_free (priv->service_name);
  g_free (priv->display_name);
  g_free (priv->status_text);
  g_free (priv->status_time);
  g_free (priv->old_status_text);

  clutter_actor_destroy (priv->cancel_icon);
  clutter_actor_destroy (priv->service_label);
  clutter_actor_destroy (priv->status_entry);
  clutter_actor_destroy (priv->button);

  G_OBJECT_CLASS (mnb_status_entry_parent_class)->finalize (gobject);
}

static void
mnb_status_entry_set_property (GObject      *gobject,
                               guint         prop_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
  MnbStatusEntryPrivate *priv = MNB_STATUS_ENTRY (gobject)->priv;

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
mnb_status_entry_get_property (GObject    *gobject,
                               guint       prop_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
  MnbStatusEntryPrivate *priv = MNB_STATUS_ENTRY (gobject)->priv;

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
mnb_status_entry_constructed (GObject *gobject)
{
  MnbStatusEntry *entry = MNB_STATUS_ENTRY (gobject);
  MnbStatusEntryPrivate *priv = entry->priv;
  gchar *str;

  g_assert (priv->service_name != NULL);

  priv->display_name = get_mojito_service_name (priv->service_name);
  if (priv->display_name != NULL)
    {
      /* Translators: %s is the user visible name of the service */
      str = g_strdup_printf (_("Enter your %s status here..."),
                             priv->display_name);
    }
  else
    str = g_strdup (_("Enter your current status here..."));

  nbtk_entry_set_text (NBTK_ENTRY (priv->status_entry), str);

  g_free (str);

  if (G_OBJECT_CLASS (mnb_status_entry_parent_class)->constructed)
    G_OBJECT_CLASS (mnb_status_entry_parent_class)->constructed (gobject);
}

static void
mnb_status_entry_class_init (MnbStatusEntryClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  ClutterActorClass *actor_class = CLUTTER_ACTOR_CLASS (klass);
  NbtkWidgetClass *widget_class = NBTK_WIDGET_CLASS (klass);
  GParamSpec *pspec;

  g_type_class_add_private (klass, sizeof (MnbStatusEntryPrivate));

  gobject_class->constructed = mnb_status_entry_constructed;
  gobject_class->set_property = mnb_status_entry_set_property;
  gobject_class->get_property = mnb_status_entry_get_property;
  gobject_class->finalize = mnb_status_entry_finalize;

  actor_class->get_preferred_width = mnb_status_entry_get_preferred_width;
  actor_class->get_preferred_height = mnb_status_entry_get_preferred_height;
  actor_class->allocate = mnb_status_entry_allocate;
  actor_class->paint = mnb_status_entry_paint;
  actor_class->pick = mnb_status_entry_pick;
  actor_class->button_release_event = mnb_status_entry_button_release;
  actor_class->map = mnb_status_entry_map;
  actor_class->unmap = mnb_status_entry_unmap;

  widget_class->style_changed = mnb_status_entry_style_changed;

  pspec = g_param_spec_string ("service-name",
                               "Service Name",
                               "The name of the Mojito service",
                               NULL,
                               G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
  g_object_class_install_property (gobject_class, PROP_SERVICE_NAME, pspec);

  entry_signals[STATUS_CHANGED] =
    g_signal_new ("status-changed",
                  G_TYPE_FROM_CLASS (gobject_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (MnbStatusEntryClass, status_changed),
                  NULL, NULL,
                  moblin_netbook_marshal_VOID__STRING,
                  G_TYPE_NONE, 1,
                  G_TYPE_STRING);
  entry_signals[UPDATE_CANCELLED] =
    g_signal_new ("update-cancelled",
                  G_TYPE_FROM_CLASS (gobject_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (MnbStatusEntryClass, update_cancelled),
                  NULL, NULL,
                  moblin_netbook_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);
}

static void
mnb_status_entry_init (MnbStatusEntry *self)
{
  MnbStatusEntryPrivate *priv;
  ClutterActor *text;

  self->priv = priv = MNB_STATUS_ENTRY_GET_PRIVATE (self);

  priv->is_active = FALSE;

  priv->status_entry =
    CLUTTER_ACTOR (nbtk_entry_new (_("Enter your current status here...")));
  nbtk_widget_set_style_class_name (NBTK_WIDGET (priv->status_entry),
                                    "MnbStatusEntryText");
  clutter_actor_set_parent (priv->status_entry, CLUTTER_ACTOR (self));
  text = nbtk_entry_get_clutter_text (NBTK_ENTRY (priv->status_entry));
  clutter_text_set_editable (CLUTTER_TEXT (text), FALSE);
  clutter_text_set_single_line_mode (CLUTTER_TEXT (text), TRUE);
  clutter_text_set_use_markup (CLUTTER_TEXT (text), TRUE);
  clutter_text_set_ellipsize (CLUTTER_TEXT (text), PANGO_ELLIPSIZE_END);
  clutter_actor_set_reactive (CLUTTER_ACTOR (text), FALSE);

  priv->service_label =
    CLUTTER_ACTOR (nbtk_label_new (""));
  nbtk_widget_set_style_class_name (NBTK_WIDGET (priv->service_label),
                                    "MnbStatusEntrySubText");
  clutter_actor_set_parent (priv->service_label, CLUTTER_ACTOR (self));
  text = nbtk_label_get_clutter_text (NBTK_LABEL (priv->service_label));
  clutter_text_set_editable (CLUTTER_TEXT (text), FALSE);
  clutter_text_set_single_line_mode (CLUTTER_TEXT (text), TRUE);
  clutter_text_set_use_markup (CLUTTER_TEXT (text), FALSE);

  {
    ClutterActor *cancel_icon = NULL;
    ClutterColor cancel_icon_color = { 255, 255, 255, 0 };

    cancel_icon = clutter_rectangle_new ();
    clutter_rectangle_set_color (CLUTTER_RECTANGLE (cancel_icon),
                                 &cancel_icon_color);
    clutter_actor_set_size (cancel_icon,
                            CANCEL_ICON_SIZE,
                            CANCEL_ICON_SIZE);

    priv->cancel_icon = CLUTTER_ACTOR (nbtk_button_new ());
    nbtk_widget_set_style_class_name (NBTK_WIDGET (priv->cancel_icon),
                                      "MnbStatusEntryCancel");
    clutter_container_add_actor (CLUTTER_CONTAINER (priv->cancel_icon),
                                 cancel_icon);

    clutter_actor_hide (priv->cancel_icon);
    clutter_actor_set_reactive (priv->cancel_icon, TRUE);
    clutter_actor_set_parent (priv->cancel_icon, CLUTTER_ACTOR (self));
    g_signal_connect (priv->cancel_icon, "clicked",
                      G_CALLBACK (on_cancel_clicked),
                      self);
  }

  priv->button = CLUTTER_ACTOR (nbtk_button_new_with_label (_("Edit")));
  nbtk_widget_set_style_class_name (NBTK_WIDGET (priv->button),
                                    "MnbStatusEntryButton");
  clutter_actor_hide (priv->button);
  clutter_actor_set_reactive (priv->button, TRUE);
  clutter_actor_set_parent (priv->button, CLUTTER_ACTOR (self));
  g_signal_connect (priv->button, "clicked",
                    G_CALLBACK (on_button_clicked),
                    self);
}

NbtkWidget *
mnb_status_entry_new (const gchar *service_name)
{
  g_return_val_if_fail (service_name != NULL, NULL);

  return g_object_new (MNB_TYPE_STATUS_ENTRY,
                       "service-name", service_name,
                       NULL);
}

void
mnb_status_entry_show_button (MnbStatusEntry *entry,
                              gboolean        show)
{
  g_return_if_fail (MNB_IS_STATUS_ENTRY (entry));

  if (show)
    clutter_actor_show (entry->priv->button);
  else
    clutter_actor_hide (entry->priv->button);
}

gboolean
mnb_status_entry_get_is_active (MnbStatusEntry *entry)
{
  g_return_val_if_fail (MNB_IS_STATUS_ENTRY (entry), FALSE);

  return entry->priv->is_active;
}

void
mnb_status_entry_set_is_active (MnbStatusEntry *entry,
                                gboolean        is_active)
{
  MnbStatusEntryPrivate *priv;
  ClutterActor *text;

  g_return_if_fail (MNB_IS_STATUS_ENTRY (entry));

  priv = entry->priv;

  if (priv->is_active == is_active)
    return;

  priv->is_active = is_active;

  text = nbtk_entry_get_clutter_text (NBTK_ENTRY (priv->status_entry));

  if (priv->is_active)
    {
      nbtk_button_set_label (NBTK_BUTTON (priv->button), _("Post"));

      g_free (priv->old_status_text);
      priv->old_status_text =
        g_strdup (clutter_text_get_text (CLUTTER_TEXT (text)));

      clutter_actor_set_reactive (text, TRUE);

      clutter_text_set_editable (CLUTTER_TEXT (text), TRUE);
      clutter_text_set_activatable (CLUTTER_TEXT (text), TRUE);
      clutter_text_set_text (CLUTTER_TEXT (text), "");

      clutter_actor_hide (priv->service_label);
      clutter_actor_show (priv->cancel_icon);

      clutter_actor_grab_key_focus (priv->status_entry);

      nbtk_widget_set_style_pseudo_class (NBTK_WIDGET (entry), "active");
    }
  else
    {
      nbtk_button_set_label (NBTK_BUTTON (priv->button), _("Edit"));

      clutter_actor_set_reactive (text, FALSE);

      clutter_text_set_editable (CLUTTER_TEXT (text), FALSE);
      clutter_text_set_activatable (CLUTTER_TEXT (text), FALSE);

      clutter_actor_show (priv->service_label);
      clutter_actor_hide (priv->cancel_icon);

      nbtk_widget_set_style_pseudo_class (NBTK_WIDGET (entry), "hover");

      g_free (priv->old_status_text);
      priv->old_status_text = NULL;

      g_signal_emit (entry, entry_signals[STATUS_CHANGED], 0,
                     clutter_text_get_text (CLUTTER_TEXT (text)));
    }

  clutter_actor_queue_relayout (CLUTTER_ACTOR (entry));
}

gboolean
mnb_status_entry_get_in_hover (MnbStatusEntry *entry)
{
  g_return_val_if_fail (MNB_IS_STATUS_ENTRY (entry), FALSE);

  return entry->priv->in_hover;
}

void
mnb_status_entry_set_in_hover (MnbStatusEntry *entry,
                               gboolean        in_hover)
{
  g_return_if_fail (MNB_IS_STATUS_ENTRY (entry));

  if (entry->priv->in_hover != in_hover)
    {
      entry->priv->in_hover = in_hover;

      if (entry->priv->in_hover)
        nbtk_widget_set_style_pseudo_class (NBTK_WIDGET (entry), "hover");
      else
        {
          if (entry->priv->is_active)
            nbtk_widget_set_style_pseudo_class (NBTK_WIDGET (entry), "active");
          else
            nbtk_widget_set_style_pseudo_class (NBTK_WIDGET (entry), NULL);
        }
    }
}

void
mnb_status_entry_set_status_text (MnbStatusEntry *entry,
                                  const gchar    *status_text,
                                  GTimeVal       *status_time)
{
  MnbStatusEntryPrivate *priv;
  ClutterActor *text;
  gchar *service_line;

  g_return_if_fail (MNB_IS_STATUS_ENTRY (entry));
  g_return_if_fail (status_text != NULL);

  priv = entry->priv;

  g_free (priv->status_text);
  g_free (priv->status_time);

  priv->status_text = g_strdup (status_text);

  if (status_time)
    priv->status_time = penge_utils_format_time (status_time);

  text = nbtk_entry_get_clutter_text (NBTK_ENTRY (priv->status_entry));
  clutter_text_set_markup (CLUTTER_TEXT (text), priv->status_text);

  service_line = g_strdup_printf ("%s - %s",
                                  priv->status_time,
                                  priv->display_name);

  text = nbtk_label_get_clutter_text (NBTK_LABEL (priv->service_label));
  clutter_text_set_markup (CLUTTER_TEXT (text), service_line);
  g_free (service_line);

  clutter_actor_queue_relayout (CLUTTER_ACTOR (entry));
}

G_CONST_RETURN gchar *
mnb_status_entry_get_status_text (MnbStatusEntry *entry)
{
  g_return_val_if_fail (MNB_IS_STATUS_ENTRY (entry), NULL);

  return entry->priv->status_text;
}
