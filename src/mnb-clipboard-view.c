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

#include <string.h>

#include <clutter/clutter.h>

#include <gtk/gtk.h>

#include <glib/gi18n.h>

#include "mnb-clipboard-view.h"
#include "mnb-clipboard-store.h"
#include "mnb-clipboard-item.h"

#define MNB_CLIPBOARD_VIEW_GET_PRIVATE(obj)     (G_TYPE_INSTANCE_GET_PRIVATE ((obj), MNB_TYPE_CLIPBOARD_VIEW, MnbClipboardViewPrivate))

#define ROW_SPACING     (2.0)

#define EMPTY_TEXT      _("You need to copy some text to use Pasteboard")

struct _MnbClipboardViewPrivate
{
  MnbClipboardStore *store;

  GSList *rows;

  guint add_id;
  guint remove_id;
};

enum
{
  PROP_0,

  PROP_STORE
};

G_DEFINE_TYPE (MnbClipboardView, mnb_clipboard_view, NBTK_TYPE_WIDGET);

static void
on_action_clicked (MnbClipboardItem *item,
                   MnbClipboardView *view)
{
  MnbClipboardViewPrivate *priv = view->priv;
  GtkClipboard *clipboard;
  const gchar *text;
  gint64 serial;
  GSList *l;

  l = view->priv->rows;
  if (item == l->data)
    return;

  text = mnb_clipboard_item_get_contents (item);
  if (text == NULL || *text == '\0')
    return;

  /* remove the item from the view */
  serial = mnb_clipboard_item_get_serial (item);
  mnb_clipboard_store_remove (priv->store, serial);

  /* this will add another item at the beginning of the view */
  clipboard = gtk_clipboard_get (GDK_SELECTION_CLIPBOARD);
  gtk_clipboard_set_text (clipboard, text, -1);
}

static void
on_remove_clicked (MnbClipboardItem *item,
                   MnbClipboardView *view)
{
  MnbClipboardViewPrivate *priv = view->priv;
  gint64 serial = mnb_clipboard_item_get_serial (item);

  mnb_clipboard_store_remove (priv->store, serial);
}

static void
on_store_item_removed (MnbClipboardStore *store,
                       gint64             serial,
                       MnbClipboardView  *view)
{
  MnbClipboardViewPrivate *priv = view->priv;
  MnbClipboardItem *item = NULL;
  gboolean item_found = FALSE;
  GSList *l;

  for (l = priv->rows; l != NULL; l = l->next)
    {
      item = l->data;

      if (mnb_clipboard_item_get_serial (item) == serial)
        {
          item_found = TRUE;
          break;
        }
    }

  if (!item_found)
    return;

  priv->rows = g_slist_remove (priv->rows, item);
  clutter_actor_destroy (CLUTTER_ACTOR (item));

  clutter_actor_queue_relayout (CLUTTER_ACTOR (view));
}

static void
on_store_item_added (MnbClipboardStore    *store,
                     MnbClipboardItemType  item_type,
                     MnbClipboardView     *view)
{
  ClutterActor *row = NULL;

  switch (item_type)
    {
    case MNB_CLIPBOARD_ITEM_TEXT:
      {
        gchar *text = NULL;
        gint64 mtime = 0, serial = 0;

        text = mnb_clipboard_store_get_last_text (store, &mtime, &serial);
        if (text != NULL)
          {
            row = g_object_new (MNB_TYPE_CLIPBOARD_ITEM,
                                "contents", text,
                                "serial", serial,
                                NULL);

            g_signal_connect (row, "remove-clicked",
                              G_CALLBACK (on_remove_clicked),
                              view);
            g_signal_connect (row, "action-clicked",
                              G_CALLBACK (on_action_clicked),
                              view);

            g_free (text);
          }
      }
      break;

    case MNB_CLIPBOARD_ITEM_URIS:
      break;

    case MNB_CLIPBOARD_ITEM_IMAGE:
      break;

    case MNB_CLIPBOARD_ITEM_INVALID:
      g_assert_not_reached ();
      break;
    }

  if (row == NULL)
    return;

  view->priv->rows = g_slist_prepend (view->priv->rows, row);
  clutter_actor_set_parent (row, CLUTTER_ACTOR (view));

  clutter_actor_queue_relayout (CLUTTER_ACTOR (view));
}

static void
mnb_clipboard_view_get_preferred_width (ClutterActor *actor,
                                        ClutterUnit   for_height,
                                        ClutterUnit  *min_width_p,
                                        ClutterUnit  *natural_width_p)
{
  MnbClipboardViewPrivate *priv = MNB_CLIPBOARD_VIEW (actor)->priv;
  ClutterUnit min_width, natural_width;
  GSList *l;

  if (priv->rows == NULL)
    {
      if (min_width_p)
        *min_width_p = 0;

      if (natural_width_p)
        *natural_width_p = 0;

      return;
    }

  min_width = natural_width = 0;
  for (l = priv->rows; l != NULL; l = l->next)
    {
      ClutterActor *row = l->data;
      ClutterUnit row_min, row_natural;

      row_min = row_natural = 0;
      clutter_actor_get_preferred_width (row, for_height,
                                         &row_min,
                                         &row_natural);

      min_width = MAX (min_width, row_min);
      natural_width = MAX (natural_width, row_natural);
    }

  if (min_width_p)
    *min_width_p = min_width;

  if (natural_width_p)
    *natural_width_p = natural_width;
}

static void
mnb_clipboard_view_get_preferred_height (ClutterActor *actor,
                                         ClutterUnit   for_width,
                                         ClutterUnit  *min_height_p,
                                         ClutterUnit  *natural_height_p)
{
  MnbClipboardViewPrivate *priv = MNB_CLIPBOARD_VIEW (actor)->priv;
  ClutterUnit min_height, natural_height;
  NbtkPadding padding = { 0, };
  GSList *l;

  nbtk_widget_get_padding (NBTK_WIDGET (actor), &padding);

  min_height = natural_height = padding.top + padding.bottom;

  for (l = priv->rows; l != NULL; l = l->next)
    {
      ClutterActor *child = l->data;
      ClutterUnit child_min, child_natural;

      clutter_actor_get_preferred_height (child, for_width,
                                          &child_min,
                                          &child_natural);

      min_height += (child_min + ROW_SPACING);
      natural_height += (child_natural + ROW_SPACING);
    }

  if (min_height_p)
    *min_height_p = min_height;

  if (natural_height_p)
    *natural_height_p = natural_height;
}

static void
mnb_clipboard_view_allocate (ClutterActor *actor,
                             const ClutterActorBox *box,
                             gboolean origin_changed)
{
  MnbClipboardViewPrivate *priv = MNB_CLIPBOARD_VIEW (actor)->priv;
  ClutterUnit available_width, available_height;
  NbtkPadding padding = { 0, };
  ClutterUnit y;
  GSList *l;

  CLUTTER_ACTOR_CLASS (mnb_clipboard_view_parent_class)->allocate (actor, box, origin_changed);

  nbtk_widget_get_padding (NBTK_WIDGET (actor), &padding);

  available_width  = box->x2 - box->x1
                   - padding.left
                   - padding.right;
  available_height = box->y2 - box->y1
                   - padding.top
                   - padding.bottom;

  y = box->y1 + padding.top;

  for (l = priv->rows; l != NULL; l = l->next)
    {
      ClutterActor *child = l->data;
      ClutterUnit child_height;
      ClutterActorBox child_box = { 0, };

      if (!CLUTTER_ACTOR_IS_VISIBLE (child))
        continue;

      clutter_actor_get_preferred_height (child, available_width,
                                          NULL,
                                          &child_height);

      child_box.x1 = box->x1 + padding.left;
      child_box.y1 = y;
      child_box.x2 = child_box.x1 + available_width;
      child_box.y2 = child_box.y1 + child_height;

      clutter_actor_allocate (child, &child_box, origin_changed);

      y += (child_height + ROW_SPACING);
    }
}

static void
mnb_clipboard_view_paint (ClutterActor *actor)
{
  MnbClipboardViewPrivate *priv = MNB_CLIPBOARD_VIEW (actor)->priv;
  GSList *l;
  gint i;

  CLUTTER_ACTOR_CLASS (mnb_clipboard_view_parent_class)->paint (actor);

#if 0
  if (priv->rows == NULL)
    {
      PangoLayout *layout;
      ClutterColor *text_color = NULL;
      CoglColor color = { 0, };
      gchar *font_family = NULL;
      gchar *font_str;
      gint font_size = 12;
      ClutterActorBox box = { 0, };
      PangoFontDescription *font_desc;
      PangoRectangle rect = { 0, };
      gint text_x;

      clutter_actor_get_allocation_box (actor, &box);

      nbtk_stylable_get (NBTK_STYLABLE (actor),
                         "color", &text_color,
                         "font-family", &font_family,
                         "font-size", &font_size,
                         NULL);

      layout = clutter_actor_create_pango_layout (actor, EMPTY_TEXT);

      if (font_family != NULL)
        {
          font_str = g_strdup_printf ("%s %dpx", font_family, font_size);

          g_free (font_family);
        }
      else
        font_str = g_strdup ("Liberation Sans 12px");

      font_desc = pango_font_description_from_string (font_str);
      pango_layout_set_font_description (layout, font_desc);
      pango_layout_set_width (layout, CLUTTER_UNITS_TO_PANGO_UNIT (box.x2 - box.x1));
      pango_layout_set_height (layout, CLUTTER_UNITS_TO_PANGO_UNIT (box.y2 - box.y1));
      pango_layout_set_wrap (layout, PANGO_WRAP_WORD);

      g_free (font_str);

      if (text_color != NULL)
        {
          cogl_color_set_from_4ub (&color,
                                   text_color->red,
                                   text_color->green,
                                   text_color->blue,
                                   text_color->alpha);
          clutter_color_free (text_color);
        }
      else
        cogl_color_set_from_4ub (&color, 0, 0, 0, 255);

      pango_layout_get_extents (layout, NULL, &rect);
      text_x = (int) ((box.x2 - box.x1) - (rect.width / 1024.0)) / 2.0;

      cogl_pango_render_layout (layout,
                                text_x, 0,
                                &color,
                                0);

      pango_font_description_free (font_desc);
      g_object_unref (layout);

      return;
    }
#endif

  for (l = priv->rows, i = 0; l != NULL; l = l->next, i++)
    {
      ClutterActor *child = l->data;

      if (!CLUTTER_ACTOR_IS_VISIBLE (child))
        {
          i--;
          continue;
        }

      if (i == 0)
        {
          ClutterActorBox box = { 0, };

          clutter_actor_get_allocation_box (child, &box);

          cogl_set_source_color4ub (0xef, 0xef, 0xef, 255);
          cogl_rectangle (box.x1 + 1, box.y1 + 1,
                          box.x2 - 1, box.y2 - 1);
        }

      clutter_actor_paint (child);

      if (i != 0)
        {
          ClutterActorBox box = { 0, };

          clutter_actor_get_allocation_box (child, &box);

          cogl_set_source_color4ub (0x1a, 0x1a, 0x1a, 255);
          cogl_path_move_to (box.x1, box.y1);
          cogl_path_line_to (box.x2, box.y1);
        }
    }
}

static void
mnb_clipboard_view_pick (ClutterActor *actor,
                         const ClutterColor *pick_color)
{
  MnbClipboardViewPrivate *priv = MNB_CLIPBOARD_VIEW (actor)->priv;
  GSList *l;

  CLUTTER_ACTOR_CLASS (mnb_clipboard_view_parent_class)->pick (actor, pick_color);

  for (l = priv->rows; l != NULL; l = l->next)
    clutter_actor_paint (l->data);
}

static void
mnb_clipboard_view_finalize (GObject *gobject)
{
  MnbClipboardViewPrivate *priv = MNB_CLIPBOARD_VIEW (gobject)->priv;

  g_signal_handler_disconnect (priv->store, priv->add_id);
  g_signal_handler_disconnect (priv->store, priv->remove_id);
  g_object_unref (priv->store);

  g_slist_foreach (priv->rows, (GFunc) clutter_actor_destroy, NULL);
  g_slist_free (priv->rows);

  G_OBJECT_CLASS (mnb_clipboard_view_parent_class)->finalize (gobject);
}

static void
mnb_clipboard_view_set_property (GObject      *gobject,
                                 guint         prop_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
  MnbClipboardViewPrivate *priv = MNB_CLIPBOARD_VIEW (gobject)->priv;

  switch (prop_id)
    {
    case PROP_STORE:
      if (priv->store != NULL)
        {
          if (priv->add_id != 0)
            {
              g_signal_handler_disconnect (priv->store, priv->add_id);
              priv->add_id = 0;
            }

          if (priv->remove_id != 0)
            {
              g_signal_handler_disconnect (priv->store, priv->remove_id);
              priv->remove_id = 0;
            }

          g_object_unref (priv->store);
        }

      if (g_value_get_object (value) != NULL)
        priv->store = g_value_dup_object (value);
      else
        priv->store = mnb_clipboard_store_new ();

      priv->add_id = g_signal_connect (priv->store, "item-added",
                                       G_CALLBACK (on_store_item_added),
                                       gobject);
      priv->remove_id = g_signal_connect (priv->store, "item-removed",
                                          G_CALLBACK (on_store_item_removed),
                                          gobject);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
      break;
    }
}

static void
mnb_clipboard_view_get_property (GObject    *gobject,
                                 guint       prop_id,
                                 GValue     *value,
                                 GParamSpec *pspec)
{
  MnbClipboardViewPrivate *priv = MNB_CLIPBOARD_VIEW (gobject)->priv;

  switch (prop_id)
    {
    case PROP_STORE:
      g_value_set_object (value, priv->store);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
      break;
    }
}

static void
mnb_clipboard_view_class_init (MnbClipboardViewClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  ClutterActorClass *actor_class = CLUTTER_ACTOR_CLASS (klass);
  GParamSpec *pspec;

  g_type_class_add_private (klass, sizeof (MnbClipboardViewPrivate));

  gobject_class->set_property = mnb_clipboard_view_set_property;
  gobject_class->get_property = mnb_clipboard_view_get_property;
  gobject_class->finalize = mnb_clipboard_view_finalize;

  actor_class->get_preferred_width = mnb_clipboard_view_get_preferred_width;
  actor_class->get_preferred_height = mnb_clipboard_view_get_preferred_height;
  actor_class->allocate = mnb_clipboard_view_allocate;
  actor_class->paint = mnb_clipboard_view_paint;
  actor_class->pick = mnb_clipboard_view_pick;

  pspec = g_param_spec_object ("store",
                               "Store",
                               "The MnbClipboardStore",
                               MNB_TYPE_CLIPBOARD_STORE,
                               G_PARAM_READWRITE |
                               G_PARAM_CONSTRUCT);
  g_object_class_install_property (gobject_class, PROP_STORE, pspec);
}

static void
mnb_clipboard_view_init (MnbClipboardView *view)
{
  view->priv = MNB_CLIPBOARD_VIEW_GET_PRIVATE (view);
}

NbtkWidget *
mnb_clipboard_view_new (MnbClipboardStore *store)
{
  g_return_val_if_fail (store == NULL || MNB_IS_CLIPBOARD_STORE (store), NULL);

  return g_object_new (MNB_TYPE_CLIPBOARD_VIEW,
                       "store", store,
                       NULL);
}

MnbClipboardStore *
mnb_clipboard_view_get_store (MnbClipboardView *view)
{
  g_return_val_if_fail (MNB_IS_CLIPBOARD_VIEW (view), NULL);

  return view->priv->store;
}

void
mnb_clipboard_view_filter (MnbClipboardView *view,
                           const gchar      *filter)
{
  MnbClipboardViewPrivate *priv;

  g_return_if_fail (MNB_IS_CLIPBOARD_VIEW (view));

  priv = view->priv;

  if (priv->rows == NULL)
    return;

  if (filter == NULL || *filter == '\0')
    g_slist_foreach (priv->rows, (GFunc) clutter_actor_show, NULL);
  else
    {
      gchar *needle;
      GSList *l;

      needle = g_utf8_strdown (filter, -1);

      for (l = priv->rows; l != NULL; l = l->next)
        {
          MnbClipboardItem *row = l->data;
          gchar *contents;

          contents = g_utf8_strdown (mnb_clipboard_item_get_contents (row), -1);
          if (strstr (contents, needle) == NULL)
            clutter_actor_hide (CLUTTER_ACTOR (row));
          else
            clutter_actor_show (CLUTTER_ACTOR (row));

          g_free (contents);
        }

      g_free (needle);
    }

  clutter_actor_queue_relayout (CLUTTER_ACTOR (view));
}
