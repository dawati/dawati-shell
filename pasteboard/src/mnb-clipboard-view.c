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

G_DEFINE_TYPE (MnbClipboardView, mnb_clipboard_view, NBTK_TYPE_BOX_LAYOUT);

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
  MnbClipboardItem *row = NULL;
  gboolean item_found = FALSE;
  GSList *l;

  for (l = priv->rows; l != NULL; l = l->next)
    {
      row = l->data;

      if (mnb_clipboard_item_get_serial (row) == serial)
        {
          item_found = TRUE;
          break;
        }
    }

  if (!item_found)
    return;

  priv->rows = g_slist_remove (priv->rows, row);
  clutter_container_remove_actor (CLUTTER_CONTAINER (view), CLUTTER_ACTOR (row));
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
  clutter_container_add_actor (CLUTTER_CONTAINER (view), CLUTTER_ACTOR (row));
}

/* we override the BoxLayout::paint completely because we need to:
 *
 *  - paint the background
 *  - paint the first row with a different background color
 *  - paint the children
 *
 * so: KEEP IN SYNC WITH NbtkBoxLayout::paint
 */
static void
mnb_clipboard_view_paint (ClutterActor *actor)
{
  NbtkAdjustment *h_adjustment, *v_adjustment;
  ClutterActorBox box_b;
  GList *children, *l;
  gdouble x, y;

  h_adjustment = v_adjustment = NULL;
  nbtk_scrollable_get_adjustments (NBTK_SCROLLABLE (actor),
                                   &h_adjustment,
                                   &v_adjustment);

  if (h_adjustment)
    x = nbtk_adjustment_get_value (h_adjustment);
  else
    x = 0;

  if (v_adjustment)
    y = nbtk_adjustment_get_value (v_adjustment);
  else
    y = 0;

  nbtk_widget_draw_background (NBTK_WIDGET (actor));

  clutter_actor_get_allocation_box (actor, &box_b);
  box_b.x2 = (box_b.x2 - box_b.x1) + x;
  box_b.x1 = x;
  box_b.y2 = (box_b.y2 - box_b.y1) + y;
  box_b.y1 = y;

  children = clutter_container_get_children (CLUTTER_CONTAINER (actor));
  children = g_list_reverse (children);
  for (l = children; l != NULL; l = l->next)
    {
      ClutterActor *child = l->data;
      ClutterActorBox child_b;

      if (!CLUTTER_ACTOR_IS_VISIBLE (child))
        continue;

      clutter_actor_get_allocation_box (child, &child_b);

      if ((child_b.x1 < box_b.x2) &&
          (child_b.x2 > box_b.x1) &&
          (child_b.y1 < box_b.y2) &&
          (child_b.y2 > box_b.y1))
        {
          /* draw a background on the first row, to mark it as the
           * current paste target
           */
          if (l == children)
            {
              cogl_set_source_color4ub (0xef, 0xef, 0xef, 255);
              cogl_rectangle (child_b.x1, child_b.y1,
                              child_b.x2, child_b.y2);
            }

          clutter_actor_paint (child);
        }
    }

  g_list_free (children);
}

static void
mnb_clipboard_view_finalize (GObject *gobject)
{
  MnbClipboardViewPrivate *priv = MNB_CLIPBOARD_VIEW (gobject)->priv;

  g_signal_handler_disconnect (priv->store, priv->add_id);
  g_signal_handler_disconnect (priv->store, priv->remove_id);
  g_object_unref (priv->store);

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

  actor_class->paint = mnb_clipboard_view_paint;

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

  nbtk_box_layout_set_vertical (NBTK_BOX_LAYOUT (view), TRUE);
  nbtk_box_layout_set_pack_start (NBTK_BOX_LAYOUT (view), TRUE);
  nbtk_box_layout_set_spacing (NBTK_BOX_LAYOUT (view), 2);
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
