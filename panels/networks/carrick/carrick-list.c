/*
 * Carrick - a connection panel for the Moblin Netbook
 * Copyright (C) 2009 Intel Corporation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License version
 * 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 *
 * Written by - Joshua Lock <josh@linux.intel.com>
 *
 */

#include "carrick-list.h"

#include <config.h>
#include <gtk/gtk.h>

#include "carrick-service-item.h"

#define CARRICK_DRAG_TARGET "CARRICK_DRAG_TARGET"

static const GtkTargetEntry carrick_targets[] = {
  { CARRICK_DRAG_TARGET, GTK_TARGET_SAME_APP, 0 },
};

G_DEFINE_TYPE (CarrickList, carrick_list, GTK_TYPE_VBOX)

#define LIST_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), CARRICK_TYPE_LIST, CarrickListPrivate))

typedef struct _CarrickListPrivate CarrickListPrivate;

struct _CarrickListPrivate
{
  GtkWidget *drag_window;
  guint drag_position;
  guint drop_position;
  GtkWidget *found;
};

static void
carrick_list_get_property (GObject *object, guint property_id,
                              GValue *value, GParamSpec *pspec)
{
  switch (property_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
carrick_list_set_property (GObject *object, guint property_id,
                              const GValue *value, GParamSpec *pspec)
{
  switch (property_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
carrick_list_dispose (GObject *object)
{
  G_OBJECT_CLASS (carrick_list_parent_class)->dispose (object);
}

static void
carrick_list_finalize (GObject *object)
{
  G_OBJECT_CLASS (carrick_list_parent_class)->finalize (object);
}

static void
carrick_list_drag_begin (GtkWidget      *widget,
                         GdkDragContext *context,
                         CarrickList    *list)
{
  CarrickListPrivate *priv = LIST_PRIVATE (list);
  gint x, y;

  /* save old place in list for drag-failures */
  gtk_container_child_get (GTK_CONTAINER (list),
                           widget,
                           "position", &priv->drag_position,
                           NULL);
  priv->drop_position = priv->drag_position;

  /* remove widget from list and setup dnd popup window */
  priv->drag_window = gtk_window_new (GTK_WINDOW_POPUP);
  gtk_widget_set_size_request (priv->drag_window,
                               widget->allocation.width,
                               widget->allocation.height);
  gtk_widget_get_pointer (widget, &x, &y);
  gtk_widget_reparent (widget, priv->drag_window);
  gtk_drag_set_icon_widget (context,
                            priv->drag_window,
                            x, y);

  gtk_widget_set_state (widget, GTK_STATE_SELECTED);
}

static gboolean
carrick_list_drag_drop (GtkWidget      *widget,
                        GdkDragContext *context,
                        gint            x,
                        gint            y,
                        guint           time,
                        CarrickList    *list)
{
  CarrickListPrivate *priv = LIST_PRIVATE (list);

  /* find drop position in list */
  if (widget == GTK_WIDGET (list))
  {
    /* dropped on "empty" space on list */
    priv->drop_position = -1;
  } else {
    /* dropped on a list item */
    gtk_container_child_get (GTK_CONTAINER (list),
                             widget,
                             "position", &priv->drop_position,
                             NULL);
  }

  gtk_drag_finish (context, TRUE, TRUE, time);
  return TRUE;
}

static void
carrick_list_drag_end (GtkWidget      *widget,
                       GdkDragContext *context,
                       CarrickList    *list)
{
  CarrickListPrivate *priv = LIST_PRIVATE (list);

  /* destroy the popup window */
  g_object_ref (widget);
  gtk_container_remove (GTK_CONTAINER (priv->drag_window), widget);
  gtk_widget_destroy (priv->drag_window);
  priv->drag_window = NULL;

  /* insert the widget into the list */
  gtk_box_pack_start (GTK_BOX (list), widget,
                      FALSE, FALSE,  0);
  gtk_box_reorder_child (GTK_BOX (list),
                         widget,
                         priv->drop_position);
  g_object_unref (widget);

  gtk_widget_set_state (widget, GTK_STATE_NORMAL);

  if (priv->drop_position != priv->drag_position) 
  {
    /* TODO: notify connman of the user input...
       First, cm_service_move_before/after() is not enabled on the
       connman side, I believe. 
       Second, both source and target need to be favorites for
       cm_service_move_after() to work. Should they be made favorites 
       because of this (at least in some cases)? */
  }
}

static void
_list_collapse_inactive_items (GtkWidget *item,
                               GtkWidget *active_item)
{
  if (item != active_item && CARRICK_IS_SERVICE_ITEM (item))
  {
    carrick_service_item_set_active (CARRICK_SERVICE_ITEM (item), FALSE);
  }
}

static void
_list_active_changed (GtkWidget *item,
                      GtkWidget *list)
{
  gtk_container_foreach (GTK_CONTAINER (list),
                         (GtkCallback)_list_collapse_inactive_items,
                         item);
}

void
_list_contains_child (GtkWidget *item,
                      gpointer   service)
{
  CarrickListPrivate *priv = LIST_PRIVATE (gtk_widget_get_parent (item));
  CarrickServiceItem *service_item = CARRICK_SERVICE_ITEM (item);
  CmService *serv = CM_SERVICE (service);

  if (cm_service_is_same (serv,
                          carrick_service_item_get_service (service_item)))
  {
    priv->found = GTK_WIDGET (service_item);
  }
}

GtkWidget *
carrick_list_find_service_item (CarrickList *list,
                                CmService   *service)
{
  CarrickListPrivate *priv = LIST_PRIVATE (list);
  GtkWidget *ret;
  gtk_container_foreach (GTK_CONTAINER (list),
                         _list_contains_child,
                         (gpointer) service);

  ret = priv->found;
  priv->found = NULL;

  return ret;
}

void
carrick_list_sort_list (CarrickList *list)
{
  GList *items = gtk_container_get_children (GTK_CONTAINER (list));
  GList *l;

  for (l = items; l; l = l->next)
  {
    gtk_box_reorder_child (GTK_BOX (list),
                           GTK_WIDGET (l->data),
                           carrick_service_item_get_order (l->data));
  }

  g_list_free (items);
}

void
carrick_list_add_item (CarrickList *list,
                       GtkWidget *widget)
{
  g_return_if_fail (CARRICK_IS_LIST (list));
  g_return_if_fail (GTK_IS_WIDGET (widget));

  /* define a drag source */
  gtk_drag_source_set (widget,
                       GDK_BUTTON1_MASK,
                       carrick_targets,
                       G_N_ELEMENTS (carrick_targets),
                       GDK_ACTION_MOVE);
  g_signal_connect (widget,
                    "drag-begin",
                    G_CALLBACK (carrick_list_drag_begin),
                    list);
  g_signal_connect (widget,
                    "drag-end",
                    G_CALLBACK (carrick_list_drag_end),
                    list);

  /* define a drag destination */
  gtk_drag_dest_set (widget,
                     GTK_DEST_DEFAULT_ALL,
                     carrick_targets,
                     G_N_ELEMENTS (carrick_targets),
                     GDK_ACTION_MOVE);
  g_signal_connect (widget,
                    "drag-drop",
                    G_CALLBACK (carrick_list_drag_drop),
                    list);

  /* listen to activate so we can deactivate other items in list */
  g_signal_connect (widget,
                    "activate",
                    G_CALLBACK (_list_active_changed),
                    list);

  gtk_box_pack_start (GTK_BOX (list),
                      widget,
                      FALSE,
                      FALSE,
                      0);
}


static void
carrick_list_class_init (CarrickListClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (CarrickListPrivate));

  object_class->get_property = carrick_list_get_property;
  object_class->set_property = carrick_list_set_property;
  object_class->dispose = carrick_list_dispose;
  object_class->finalize = carrick_list_finalize;
}

static void
carrick_list_init (CarrickList *self)
{
  CarrickListPrivate *priv = LIST_PRIVATE (self);

  /* add a drag target for dropping below any real
     items in the list*/
  gtk_drag_dest_set (GTK_WIDGET (self),
                     GTK_DEST_DEFAULT_ALL,
                     carrick_targets,
                     G_N_ELEMENTS (carrick_targets),
                     GDK_ACTION_MOVE);
  g_signal_connect (self,
                    "drag-drop",
                    G_CALLBACK (carrick_list_drag_drop),
                    self);


  priv->found = NULL;
}

GtkWidget*
carrick_list_new (void)
{
  return g_object_new (CARRICK_TYPE_LIST, NULL);
}
