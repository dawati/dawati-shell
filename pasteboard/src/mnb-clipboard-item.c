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
#include "mnb-pasteboard-marshal.h"

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

G_DEFINE_TYPE (MnbClipboardItem, mnb_clipboard_item, NBTK_TYPE_TABLE);

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

  g_free (item->filter_contents);

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

  actor_class->enter_event = mnb_clipboard_item_enter;
  actor_class->leave_event = mnb_clipboard_item_leave;

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
                  mnb_pasteboard_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  item_signals[ACTION_CLICKED] =
    g_signal_new (g_intern_static_string ("action-clicked"),
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (MnbClipboardItemClass, action_clicked),
                  NULL, NULL,
                  mnb_pasteboard_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);
}

static void
mnb_clipboard_item_init (MnbClipboardItem *self)
{
  NbtkTable *table = NBTK_TABLE (self);
  ClutterActor *text;
  ClutterTexture *texture;
  NbtkTextureCache *texture_cache;
  gchar *remove_icon_path;

  clutter_actor_set_width (CLUTTER_ACTOR (self), 600);
  clutter_actor_set_reactive (CLUTTER_ACTOR (self), TRUE);

  nbtk_table_set_row_spacing (table, 2);
  nbtk_table_set_col_spacing (table, 6);

  self->contents = CLUTTER_ACTOR (nbtk_label_new (""));
  text = nbtk_label_get_clutter_text (NBTK_LABEL (self->contents));
  clutter_text_set_line_wrap (CLUTTER_TEXT (text), TRUE);
  clutter_text_set_ellipsize (CLUTTER_TEXT (text), PANGO_ELLIPSIZE_NONE);
  nbtk_table_add_actor_with_properties (table, self->contents,
                                        0, 0,
                                        "x-expand", TRUE,
                                        "y-expand", TRUE,
                                        "x-fill", TRUE,
                                        "y-fill", TRUE,
                                        "x-align", 0.0,
                                        "y-align", 0.0,
                                        NULL);

  self->remove_button = CLUTTER_ACTOR (nbtk_button_new ());
  nbtk_widget_set_style_class_name (NBTK_WIDGET (self->remove_button),
                                    "MnbClipboardItemDeleteButton");
  clutter_actor_set_reactive (self->remove_button, TRUE);
  clutter_actor_hide (self->remove_button);
  g_signal_connect (self->remove_button, "clicked",
                    G_CALLBACK (on_remove_clicked),
                    self);
  nbtk_table_add_actor_with_properties (table, self->remove_button,
                                        0, 1,
                                        "x-expand", FALSE,
                                        "y-expand", TRUE,
                                        "x-fill", FALSE,
                                        "y-fill", TRUE,
                                        "x-align", 0.0,
                                        "y-align", 0.0,
                                        "row-span", 2,
                                        NULL);

  remove_icon_path = g_build_filename (THEMEDIR,
                                       "pasteboard-item-delete-hover.png",
                                       NULL);

  texture_cache = nbtk_texture_cache_get_default ();
  texture = nbtk_texture_cache_get_texture (texture_cache,
                                            remove_icon_path,
                                            FALSE);
  if (texture != NULL)
    nbtk_bin_set_child (NBTK_BIN (self->remove_button),
                        CLUTTER_ACTOR (texture));

  g_free (remove_icon_path);

  self->action_button = CLUTTER_ACTOR (nbtk_button_new ());
  nbtk_button_set_label (NBTK_BUTTON (self->action_button), _("Copy"));
  nbtk_widget_set_style_class_name (NBTK_WIDGET (self->action_button),
                                    "MnbClipboardItemCopyButton");
  clutter_actor_set_reactive (self->action_button, TRUE);
  g_signal_connect (self->action_button, "clicked",
                    G_CALLBACK (on_action_clicked),
                    self);
  nbtk_table_add_actor_with_properties (table, self->action_button,
                                        1, 0,
                                        "x-expand", FALSE,
                                        "y-expand", FALSE,
                                        "x-fill", FALSE,
                                        "y-fill", FALSE,
                                        "x-align", 0.0,
                                        "y-align", 0.5,
                                        NULL);
}

G_CONST_RETURN gchar *
mnb_clipboard_item_get_contents (MnbClipboardItem *item)
{
  g_return_val_if_fail (MNB_IS_CLIPBOARD_ITEM (item), NULL);

  return nbtk_label_get_text (NBTK_LABEL (item->contents));
}

G_CONST_RETURN gchar *
mnb_clipboard_item_get_filter_contents (MnbClipboardItem *item)
{
  g_return_val_if_fail (MNB_IS_CLIPBOARD_ITEM (item), NULL);

  if (item->filter_contents == NULL)
    {
      const gchar *contents;

      contents = nbtk_label_get_text (NBTK_LABEL (item->contents));
      item->filter_contents = g_utf8_strdown (contents, -1);
    }

  return item->filter_contents;
}

gint64
mnb_clipboard_item_get_serial (MnbClipboardItem *item)
{
  g_return_val_if_fail (MNB_IS_CLIPBOARD_ITEM (item), 0);

  return item->serial;
}

void
mnb_clipboard_item_show_action (MnbClipboardItem *item)
{
  g_return_if_fail (MNB_IS_CLIPBOARD_ITEM (item));

  clutter_actor_show (item->action_button);
}

void
mnb_clipboard_item_hide_action (MnbClipboardItem *item)
{
  g_return_if_fail (MNB_IS_CLIPBOARD_ITEM (item));

  clutter_actor_hide (item->action_button);
}
