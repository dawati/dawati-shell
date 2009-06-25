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
#include <string.h>
#include <glib/gi18n.h>

#include <penge/penge-utils.h>

#include "mnb-clipboard-item.h"

#include "marshal.h"

enum
{
  PROP_0,

  PROP_CONTENTS,
  PROP_MTIME,
  PROP_SERIAL
};

enum
{
  REMOVE_CLICKED,
  ACTION_CLICKED,

  LAST_SIGNAL
};

G_DEFINE_TYPE (MnbClipboardItem, mnb_clipboard_item, NBTK_TYPE_WIDGET);

static guint item_signals[LAST_SIGNAL] = { 0, };

static void
on_remove_clicked (NbtkButton *button,
                   MnbClipboardItem *self)
{
  g_signal_emit (self, item_signals[REMOVE_CLICKED], 0);
}

static void
on_action_clicked (NbtkButton *button,
                   MnbClipboardItem *self)
{
  g_signal_emit (self, item_signals[ACTION_CLICKED], 0);
}

static void
mnb_clipboard_item_allocate (ClutterActor *actor,
                             const ClutterActorBox *box,
                             ClutterAllocationFlags flags)
{
  MnbClipboardItem *self = MNB_CLIPBOARD_ITEM (actor);
  ClutterActorClass *klass;
  gfloat remove_width, remove_height;
  gfloat action_width, action_height;
  gfloat child_width, child_height;
  gfloat time_width, time_height;
  ClutterActorBox action_box = { 0, };
  ClutterActorBox remove_box = { 0, };
  ClutterActorBox time_box = { 0, };
  NbtkPadding padding = { 0, };

  klass = CLUTTER_ACTOR_CLASS (mnb_clipboard_item_parent_class);
  klass->allocate (actor, box, flags);

  nbtk_widget_get_padding (NBTK_WIDGET (self), &padding);

  /*
   * +------------------------------------------+-+
   * | XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX...    | |
   * |                                          | |
   * | +-------+                                |x|
   * | | xxxxx |               xxxxxxxxxxxxxxxx | |
   * | +-------+                                | |
   * +------------------------------------------+-+
   */

  clutter_actor_get_preferred_size (self->action_button,
                                    NULL, NULL,
                                    &action_width, &action_height);

  clutter_actor_get_preferred_size (self->remove_button,
                                    NULL, NULL,
                                    &remove_width, &remove_height);

  clutter_actor_get_preferred_size (self->time_label,
                                    NULL, NULL,
                                    &time_width, &time_height);

  if (self->contents)
    {
      gfloat natural_width, natural_height;
      gfloat min_width, min_height;
      gfloat available_width, available_height;
      ClutterRequestMode request;
      ClutterActorBox allocation = { 0, };

      available_width  = box->x2 - box->x1
                       - remove_width
                       - padding.left
                       - padding.right;
      available_height = box->y2 - box->y1
                       - padding.top
                       - padding.bottom;

      if (available_width < 0)
        available_width = 0;

      if (available_height < 0)
        available_height = 0;

      request = CLUTTER_REQUEST_HEIGHT_FOR_WIDTH;
      g_object_get (G_OBJECT (self->contents), "request-mode", &request, NULL);

      if (request == CLUTTER_REQUEST_HEIGHT_FOR_WIDTH)
        {
          clutter_actor_get_preferred_width (self->contents, available_height,
                                             &min_width,
                                             &natural_width);

          child_width = CLAMP (natural_width, min_width, available_width);

          clutter_actor_get_preferred_height (self->contents, child_width,
                                              &min_height,
                                              &natural_height);

          child_height = CLAMP (natural_height, min_height, available_height);
        }
      else if (request == CLUTTER_REQUEST_WIDTH_FOR_HEIGHT)
        {
          clutter_actor_get_preferred_height (self->contents, available_width,
                                              &min_height,
                                              &natural_height);

          child_height = CLAMP (natural_height, min_height, available_height);

          clutter_actor_get_preferred_width (self->contents, child_height,
                                             &min_width,
                                             &natural_width);

          child_width = CLAMP (natural_width, min_width, available_width);
        }

      allocation.x1 = padding.left;
      allocation.y1 = padding.top;
      allocation.x2 = allocation.x1 + child_width;
      allocation.y2 = allocation.y1 + child_height;

      clutter_actor_allocate (self->contents, &allocation, flags);
    }

  action_box.x1 = padding.left;
  action_box.y1 = padding.top + child_height + 6.0;
  action_box.x2 = action_box.x1 + action_width;
  action_box.y2 = action_box.y1 + action_height;

  clutter_actor_allocate (self->action_button, &action_box, flags);

  time_box.x1 = (box->x2 - box->x1)
              - padding.right
              - remove_width
              - (6.0 * 3)
              - time_width;
  time_box.y1 = padding.top + child_height + 6.0;
  time_box.x2 = (box->x2 - box->x1)
              - padding.right
              - remove_width
              - (6.0 * 2);
  time_box.y2 = time_box.y1 + time_height;

  clutter_actor_allocate (self->time_label, &time_box, flags);

  remove_box.x1 = (box->x2 - box->x1)
                - padding.right
                - remove_width
                - 6.0;
  remove_box.y1 = padding.top;
  remove_box.x2 = (box->x2 - box->x1)
                - padding.right;
  remove_box.y2 = (box->y2 - box->y1)
                - padding.bottom;

  clutter_actor_allocate (self->remove_button, &remove_box, flags);
}

static void
mnb_clipboard_item_get_preferred_width (ClutterActor *actor,
                                        gfloat        for_height,
                                        gfloat       *min_width_p,
                                        gfloat       *natural_width_p)
{
  MnbClipboardItem *item = MNB_CLIPBOARD_ITEM (actor);
  gfloat button_min, button_natural;
  gfloat contents_min, contents_natural;
  NbtkPadding padding = { 0, };

  nbtk_widget_get_padding (NBTK_WIDGET (actor), &padding);

  if (item->contents)
    clutter_actor_get_preferred_width (item->contents, for_height,
                                       &contents_min,
                                       &contents_natural);
  else
    contents_min = contents_natural = 0;

  clutter_actor_get_preferred_width (item->remove_button, for_height,
                                     &button_min,
                                     &button_natural);

  if (min_width_p)
    *min_width_p = padding.left
                 + contents_min + 6 + button_min
                 + padding.right;

  if (natural_width_p)
    *natural_width_p = padding.left
                     + contents_natural + 6 + button_natural
                     + padding.right;
}

static void
mnb_clipboard_item_get_preferred_height (ClutterActor *actor,
                                         gfloat        for_width,
                                         gfloat       *min_height_p,
                                         gfloat       *natural_height_p)
{
  MnbClipboardItem *item = MNB_CLIPBOARD_ITEM (actor);
  gfloat button_min, button_natural;
  gfloat contents_min, contents_natural;
  NbtkPadding padding = { 0, };

  nbtk_widget_get_padding (NBTK_WIDGET (actor), &padding);

  if (item->contents)
    clutter_actor_get_preferred_height (item->contents, for_width,
                                        &contents_min,
                                        &contents_natural);
  else
    contents_min = contents_natural = 0;

  clutter_actor_get_preferred_height (item->action_button, for_width,
                                      &button_min,
                                      &button_natural);

  if (min_height_p)
    *min_height_p = padding.top
                  + contents_min + 6 + button_min
                  + padding.bottom;

  if (natural_height_p)
    *natural_height_p = padding.top
                      + contents_natural + 6 + button_natural
                      + padding.bottom;
}

static void
mnb_clipboard_item_paint (ClutterActor *actor)
{
  MnbClipboardItem *item = MNB_CLIPBOARD_ITEM (actor);

  CLUTTER_ACTOR_CLASS (mnb_clipboard_item_parent_class)->paint (actor);

  if (CLUTTER_ACTOR_IS_MAPPED (item->contents))
    clutter_actor_paint (item->contents);

  if (CLUTTER_ACTOR_IS_MAPPED (item->time_label))
    clutter_actor_paint (item->time_label);

  if (CLUTTER_ACTOR_IS_MAPPED (item->remove_button))
    clutter_actor_paint (item->remove_button);

  if (CLUTTER_ACTOR_IS_MAPPED (item->action_button))
    clutter_actor_paint (item->action_button);
}

static void
mnb_clipboard_item_pick (ClutterActor *actor,
                         const ClutterColor *pick_color)
{
  MnbClipboardItem *item = MNB_CLIPBOARD_ITEM (actor);

  CLUTTER_ACTOR_CLASS (mnb_clipboard_item_parent_class)->pick (actor, pick_color);

  /* these are all we care about */
  if (CLUTTER_ACTOR_IS_MAPPED (item->remove_button))
    clutter_actor_paint (item->remove_button);

  clutter_actor_paint (item->action_button);
}

static gboolean
mnb_clipboard_item_enter (ClutterActor *actor,
                          ClutterCrossingEvent *event)
{
  MnbClipboardItem *item = MNB_CLIPBOARD_ITEM (actor);

  nbtk_widget_set_style_pseudo_class (NBTK_WIDGET (actor), "hover");

  clutter_actor_show (item->remove_button);

  return TRUE;
}

static gboolean
mnb_clipboard_item_leave (ClutterActor *actor,
                          ClutterCrossingEvent *event)
{
  MnbClipboardItem *item = MNB_CLIPBOARD_ITEM (actor);

  nbtk_widget_set_style_pseudo_class (NBTK_WIDGET (actor), "hover");

  clutter_actor_hide (item->remove_button);

  return TRUE;
}

static void
mnb_clipboard_item_map (ClutterActor *actor)
{
  MnbClipboardItem *item = MNB_CLIPBOARD_ITEM (actor);

  CLUTTER_ACTOR_CLASS (mnb_clipboard_item_parent_class)->map (actor);

  clutter_actor_map (item->contents);
  clutter_actor_map (item->time_label);
  clutter_actor_map (item->remove_button);
  clutter_actor_map (item->action_button);
}

static void
mnb_clipboard_item_unmap (ClutterActor *actor)
{
  MnbClipboardItem *item = MNB_CLIPBOARD_ITEM (actor);

  CLUTTER_ACTOR_CLASS (mnb_clipboard_item_parent_class)->unmap (actor);

  clutter_actor_unmap (item->contents);
  clutter_actor_unmap (item->time_label);
  clutter_actor_unmap (item->remove_button);
  clutter_actor_unmap (item->action_button);
}

static void
mnb_clipboard_item_set_property (GObject      *gobject,
                                 guint         prop_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
  MnbClipboardItem *self = MNB_CLIPBOARD_ITEM (gobject);

  switch (prop_id)
    {
    case PROP_CONTENTS:
      nbtk_label_set_text (NBTK_LABEL (self->contents),
                           g_value_get_string (value));
      break;

    case PROP_MTIME:
      {
        gint64 mtime = g_value_get_int64 (value);
        GTimeVal tv = { mtime, 0 };
        gchar *time = penge_utils_format_time (&tv);

        nbtk_label_set_text (NBTK_LABEL (self->time_label), time);

        g_free (time);
      }
      break;

    case PROP_SERIAL:
      self->serial = g_value_get_int64 (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
      break;
    }
}

static void
mnb_clipboard_item_finalize (GObject *gobject)
{
  MnbClipboardItem *item = MNB_CLIPBOARD_ITEM (gobject);

  clutter_actor_destroy (item->contents);
  clutter_actor_destroy (item->time_label);
  clutter_actor_destroy (item->remove_button);
  clutter_actor_destroy (item->action_button);

  G_OBJECT_CLASS (mnb_clipboard_item_parent_class)->finalize (gobject);
}

static void
mnb_clipboard_item_class_init (MnbClipboardItemClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  ClutterActorClass *actor_class = CLUTTER_ACTOR_CLASS (klass);
  GParamSpec *pspec;

  gobject_class->set_property = mnb_clipboard_item_set_property;
  gobject_class->finalize = mnb_clipboard_item_finalize;

  actor_class->get_preferred_width = mnb_clipboard_item_get_preferred_width;
  actor_class->get_preferred_height = mnb_clipboard_item_get_preferred_height;
  actor_class->allocate = mnb_clipboard_item_allocate;
  actor_class->paint = mnb_clipboard_item_paint;
  actor_class->pick = mnb_clipboard_item_pick;
  actor_class->enter_event = mnb_clipboard_item_enter;
  actor_class->leave_event = mnb_clipboard_item_leave;
  actor_class->map = mnb_clipboard_item_map;
  actor_class->unmap = mnb_clipboard_item_unmap;

  pspec = g_param_spec_string ("contents",
                               "Contents",
                               "Contents of the item",
                               NULL,
                               G_PARAM_WRITABLE |
                               G_PARAM_CONSTRUCT_ONLY);
  g_object_class_install_property (gobject_class, PROP_CONTENTS, pspec);

  pspec = g_param_spec_int64 ("mtime",
                              "Modification Time",
                              "Timestamp of the contents",
                              0, G_MAXINT64, 0,
                              G_PARAM_WRITABLE);
  g_object_class_install_property (gobject_class, PROP_MTIME, pspec);

  pspec = g_param_spec_int64 ("serial",
                              "Serial",
                              "Serial number of the item",
                              0, G_MAXINT64, 0,
                              G_PARAM_WRITABLE |
                              G_PARAM_CONSTRUCT_ONLY);
  g_object_class_install_property (gobject_class, PROP_SERIAL, pspec);

  item_signals[REMOVE_CLICKED] =
    g_signal_new (g_intern_static_string ("remove-clicked"),
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (MnbClipboardItemClass, remove_clicked),
                  NULL, NULL,
                  moblin_netbook_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  item_signals[ACTION_CLICKED] =
    g_signal_new (g_intern_static_string ("action-clicked"),
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (MnbClipboardItemClass, action_clicked),
                  NULL, NULL,
                  moblin_netbook_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);
}

static void
mnb_clipboard_item_init (MnbClipboardItem *self)
{
  ClutterActor *text;
  ClutterTexture *texture;
  NbtkTextureCache *texture_cache;
  gchar *remove_icon_path;

  clutter_actor_set_reactive (CLUTTER_ACTOR (self), TRUE);

  self->contents = CLUTTER_ACTOR (nbtk_label_new (""));
  text = nbtk_label_get_clutter_text (NBTK_LABEL (self->contents));
  clutter_text_set_line_wrap (CLUTTER_TEXT (text), TRUE);
  clutter_text_set_ellipsize (CLUTTER_TEXT (text), PANGO_ELLIPSIZE_NONE);
  clutter_actor_set_parent (self->contents, CLUTTER_ACTOR (self));

  self->remove_button = CLUTTER_ACTOR (nbtk_button_new ());
  clutter_actor_set_parent (self->remove_button, CLUTTER_ACTOR (self));
  nbtk_widget_set_style_class_name (NBTK_WIDGET (self->remove_button),
                                    "MnbClipboardItemDeleteButton");
  clutter_actor_set_reactive (self->remove_button, TRUE);
  clutter_actor_hide (self->remove_button);
  g_signal_connect (self->remove_button, "clicked",
                    G_CALLBACK (on_remove_clicked),
                    self);

  remove_icon_path = g_build_filename (PKGDATADIR,
                                       "theme",
                                       "pasteboard-item-delete-hover.png",
                                       NULL);

  texture_cache = nbtk_texture_cache_get_default ();
  texture = CLUTTER_TEXTURE (nbtk_texture_cache_get_texture (texture_cache,
                                                             remove_icon_path,
                                                             TRUE));
  nbtk_bin_set_child (NBTK_BIN (self->remove_button),
                      CLUTTER_ACTOR (texture));

  g_free (remove_icon_path);

  self->action_button = CLUTTER_ACTOR (nbtk_button_new ());
  nbtk_button_set_label (NBTK_BUTTON (self->action_button), _("Copy"));
  nbtk_widget_set_style_class_name (NBTK_WIDGET (self->action_button),
                                    "MnbClipboardItemCopyButton");
  clutter_actor_set_parent (self->action_button, CLUTTER_ACTOR (self));
  clutter_actor_set_reactive (self->action_button, TRUE);
  g_signal_connect (self->action_button, "clicked",
                    G_CALLBACK (on_action_clicked),
                    self);

  self->time_label = CLUTTER_ACTOR (nbtk_label_new (""));
  clutter_actor_set_parent (self->time_label, CLUTTER_ACTOR (self));
}

G_CONST_RETURN gchar *
mnb_clipboard_item_get_contents (MnbClipboardItem *item)
{
  g_return_val_if_fail (MNB_IS_CLIPBOARD_ITEM (item), NULL);

  return nbtk_label_get_text (NBTK_LABEL (item->contents));
}

gint64
mnb_clipboard_item_get_serial (MnbClipboardItem *item)
{
  g_return_val_if_fail (MNB_IS_CLIPBOARD_ITEM (item), 0);

  return item->serial;
}
