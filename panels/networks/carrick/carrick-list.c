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

G_DEFINE_TYPE (CarrickList, carrick_list, GTK_TYPE_SCROLLED_WINDOW)

#define LIST_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), CARRICK_TYPE_LIST, CarrickListPrivate))

typedef struct _CarrickListPrivate CarrickListPrivate;

struct _CarrickListPrivate
{
  GtkWidget *box;
  GtkAdjustment *adjustment;
  GtkWidget *drag_window;
  GtkWidget *fallback;

  guint drag_position;
  guint drop_position;

  int scroll_speed;
  guint scroll_timeout_id;

  CarrickIconFactory *icon_factory;
  CarrickNotificationManager *notes;
};

enum
{
  PROP_0,
  PROP_ICON_FACTORY,
  PROP_NOTIFICATIONS,
};

static void
carrick_list_get_property (GObject *object, guint property_id,
                              GValue *value, GParamSpec *pspec)
{
  CarrickListPrivate *priv = LIST_PRIVATE (object);

  switch (property_id)
  {
  case PROP_ICON_FACTORY:
    g_value_set_object (value, priv->icon_factory);
    break;
  case PROP_NOTIFICATIONS:
    g_value_set_object (value, priv->notes);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
carrick_list_set_property (GObject *object, guint property_id,
                              const GValue *value, GParamSpec *pspec)
{
  CarrickListPrivate *priv = LIST_PRIVATE (object);

  switch (property_id)
  {
  case PROP_ICON_FACTORY:
    priv->icon_factory = CARRICK_ICON_FACTORY (g_value_get_object (value));
    break;
  case PROP_NOTIFICATIONS:
    priv->notes = CARRICK_NOTIFICATION_MANAGER (g_value_get_object (value));
    break;
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
  gtk_container_child_get (GTK_CONTAINER (priv->box),
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
  if (widget == GTK_WIDGET (priv->box))
  {
    /* dropped on "empty" space on list */
    priv->drop_position = -1;
  } else {
    /* dropped on a list item */
    gtk_container_child_get (GTK_CONTAINER (priv->box),
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
  GList *children;
  gboolean pos_changed;

  children = gtk_container_get_children (GTK_CONTAINER (priv->box));

  /* destroy the popup window */
  g_object_ref (widget);
  gtk_container_remove (GTK_CONTAINER (priv->drag_window), widget);
  gtk_widget_destroy (priv->drag_window);
  priv->drag_window = NULL;

  /* insert the widget into the list */
  gtk_box_pack_start (GTK_BOX (priv->box), widget,
                      FALSE, FALSE,  0);
  gtk_box_reorder_child (GTK_BOX (priv->box),
                         widget,
                         priv->drop_position);
  g_object_unref (widget);

  gtk_widget_set_state (widget, GTK_STATE_NORMAL);

  if (priv->drop_position == -1)
  {
    pos_changed = priv->drag_position != g_list_length (children);
  }
  else
  {
    pos_changed = priv->drop_position != priv->drag_position;
  }

  if (pos_changed && CARRICK_IS_SERVICE_ITEM (widget))
  {
    GtkWidget *other_widget;
    CmService *service, * other_service;

    service = carrick_service_item_get_service
      (CARRICK_SERVICE_ITEM (widget));

    /* TODO: should ensure favorite status for one or both services ? */
    /* TODO: should do both move_before() and move_after() if possible ? */
    if (priv->drop_position == 0)
    {
      other_widget = g_list_nth_data (children, 1);
      if (CARRICK_IS_SERVICE_ITEM (other_widget))
      {
        other_service = carrick_service_item_get_service
          (CARRICK_SERVICE_ITEM (other_widget));
        cm_service_move_before (service, other_service);
      }
    }
    else
    {
      if (priv->drop_position == -1)
      {
        /* dropped below last child */
        other_widget = g_list_last (children)->data;
      }
      else
      {
        other_widget = g_list_nth_data (children,
                                        priv->drop_position - 1);
      }

      if (CARRICK_IS_SERVICE_ITEM (other_widget))
      {
        other_service = carrick_service_item_get_service
          (CARRICK_SERVICE_ITEM (other_widget));
        cm_service_move_after (service, other_service);
      }
    }
  }

  g_list_free (children);
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
  CarrickListPrivate *priv = LIST_PRIVATE (list);

  gtk_container_foreach (GTK_CONTAINER (priv->box),
                         (GtkCallback)_list_collapse_inactive_items,
                         item);
}

typedef struct find_data
{
  CmService *service;
  GtkWidget *widget;
} find_data;

void
_list_contains_child (GtkWidget *item,
                      find_data *data)
{
  CarrickServiceItem *service_item = CARRICK_SERVICE_ITEM (item);

  if (cm_service_is_same (data->service,
                          carrick_service_item_get_service (service_item)))
  {
    data->widget = item;
  }
}

GtkWidget *
carrick_list_find_service_item (CarrickList *list,
                                CmService   *service)
{
  CarrickListPrivate *priv = LIST_PRIVATE (list);
  find_data *data;
  GtkWidget *ret;

  data = g_slice_new0 (find_data);
  data->service = service;
  data->widget = NULL;

  gtk_container_foreach (GTK_CONTAINER (priv->box),
                         (GtkCallback)_list_contains_child,
                         data);
  ret = data->widget;

  g_slice_free (find_data, data);

  return ret;
}

static void
_set_item_inactive (GtkWidget *widget)
{
  if (CARRICK_IS_SERVICE_ITEM (widget))
  {
    carrick_service_item_set_active (CARRICK_SERVICE_ITEM (widget),
                                     FALSE);
  }
}


void
carrick_list_set_all_inactive (CarrickList *list)
{
  CarrickListPrivate *priv = LIST_PRIVATE (list);

  gtk_container_foreach (GTK_CONTAINER (priv->box),
                         (GtkCallback)_set_item_inactive,
                         NULL);

}

void
carrick_list_set_icon_factory (CarrickList *list, 
                               CarrickIconFactory *icon_factory)
{
  CarrickListPrivate *priv;

  g_return_if_fail (CARRICK_IS_LIST (list));

  priv = LIST_PRIVATE (list);

  priv->icon_factory = icon_factory;
}

CarrickIconFactory*
carrick_list_get_icon_factory (CarrickList *list)
{
  CarrickListPrivate *priv;

  g_return_val_if_fail (CARRICK_IS_LIST (list), NULL);

  priv = LIST_PRIVATE (list);

  return priv->icon_factory;
}

void
carrick_list_set_notification_manager (CarrickList *list, 
                                       CarrickNotificationManager *notification_manager)
{
  CarrickListPrivate *priv;

  g_return_if_fail (CARRICK_IS_LIST (list));

  priv = LIST_PRIVATE (list);

  priv->notes = notification_manager;
}

CarrickNotificationManager*
carrick_list_get_notification_manager (CarrickList *list)
{
  CarrickListPrivate *priv;

  g_return_val_if_fail (CARRICK_IS_LIST (list), NULL);

  priv = LIST_PRIVATE (list);

  return priv->notes;
}


GList*
carrick_list_get_children (CarrickList *list)
{
  CarrickListPrivate *priv = LIST_PRIVATE (list);

  return gtk_container_get_children (GTK_CONTAINER (priv->box));
}

void
carrick_list_sort_list (CarrickList *list)
{
  CarrickListPrivate *priv = LIST_PRIVATE (list);
  GList *items = gtk_container_get_children (GTK_CONTAINER (priv->box));
  GList *l;

  for (l = items; l; l = l->next)
  {
    gtk_box_reorder_child (GTK_BOX (priv->box),
                           GTK_WIDGET (l->data),
                           carrick_service_item_get_order (l->data));
  }

  g_list_free (items);
}


#define FAST_SCROLL_BUFFER 15
#define SCROLL_BUFFER 60

static gboolean
carrick_list_scroll (CarrickList *list)
{
  CarrickListPrivate *priv = LIST_PRIVATE (list);
  gdouble val, page_size;
  gboolean at_end;

  val = gtk_adjustment_get_value (priv->adjustment);
  page_size = gtk_adjustment_get_page_size (priv->adjustment);

  if (priv->scroll_speed < 0)
  {
    at_end = val <= gtk_adjustment_get_lower (priv->adjustment);
  }
  else
  {
    at_end = val + page_size >= gtk_adjustment_get_upper (priv->adjustment);
  }


  if (!priv->drag_window ||
      priv->scroll_speed == 0 ||
      at_end)
  {
    priv->scroll_timeout_id = 0;
    return FALSE;
  }

  gtk_adjustment_set_value (priv->adjustment,
                            val + priv->scroll_speed);
  return TRUE;
}

static void
carrick_list_drag_motion (GtkWidget      *widget,
                          GdkDragContext *context,
                          gint            x,
                          gint            y,
                          guint           time,
                          CarrickList    *list)
{
  CarrickListPrivate *priv = LIST_PRIVATE (list);
  int list_x, list_y;
  int new_speed;

  gtk_widget_translate_coordinates (widget, GTK_WIDGET (list),
                                    x, y,
                                    &list_x, &list_y);
  if (gtk_adjustment_get_value (priv->adjustment) >
      gtk_adjustment_get_lower (priv->adjustment) &&
      list_y < FAST_SCROLL_BUFFER)
  {
    new_speed = -12;
  }
  else if (gtk_adjustment_get_value (priv->adjustment) >
           gtk_adjustment_get_lower (priv->adjustment) &&
           list_y < SCROLL_BUFFER)
  {
    new_speed = -5;
  }
  else if (gtk_adjustment_get_value (priv->adjustment) <
           gtk_adjustment_get_upper (priv->adjustment) &&
           GTK_WIDGET (list)->allocation.height - list_y < FAST_SCROLL_BUFFER)
  {
    new_speed = 12;
  }
  else if (gtk_adjustment_get_value (priv->adjustment) <
           gtk_adjustment_get_upper (priv->adjustment) &&
           GTK_WIDGET (list)->allocation.height - list_y < SCROLL_BUFFER)
  {
    new_speed = 5;
  }
  else
  {
    new_speed = 0;
  }

  if (new_speed != priv->scroll_speed)
  {
    priv->scroll_speed = new_speed;
    if (priv->scroll_speed != 0)
    {
      if (priv->scroll_timeout_id > 0)
      {
        g_source_remove (priv->scroll_timeout_id);
      }
      priv->scroll_timeout_id = g_timeout_add
        (40, (GSourceFunc)carrick_list_scroll, list);
    }
  }
}

void
carrick_list_add_item (CarrickList *list,
                       CmService *service)
{
  CarrickListPrivate *priv;
  GtkWidget *widget;

  g_return_if_fail (CARRICK_IS_LIST (list));
  g_return_if_fail (CM_IS_SERVICE (service));

  priv = LIST_PRIVATE (list);
  widget = carrick_service_item_new (priv->icon_factory,
                                     priv->notes,
                                     service);
  gtk_widget_show (widget);

  /* define a drag source */
  /*
  carrick_service_item_set_draggable (CARRICK_SERVICE_ITEM (widget), TRUE);
  gtk_drag_source_set (widget,
                       GDK_BUTTON1_MASK,
                       carrick_targets,
                       G_N_ELEMENTS (carrick_targets),
                       GDK_ACTION_MOVE);
  */
  g_signal_connect (widget,
                    "drag-begin",
                    G_CALLBACK (carrick_list_drag_begin),
                    list);
  g_signal_connect (widget,
                    "drag-motion",
                    G_CALLBACK (carrick_list_drag_motion),
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

  gtk_box_pack_start (GTK_BOX (priv->box),
                      widget,
                      FALSE,
                      FALSE,
                      2);

  gtk_widget_hide (priv->fallback);
  gtk_widget_show (priv->box);
}

void
carrick_list_set_fallback (CarrickList *list,
                           const gchar *fallback)
{
  CarrickListPrivate *priv = LIST_PRIVATE (list);

  gtk_label_set_text (GTK_LABEL (priv->fallback), fallback);
}

static GObject *
carrick_list_constructor (GType                  gtype,
                          guint                  n_properties,
                          GObjectConstructParam *properties)
{
  GObject *obj;
  GObjectClass *parent_class;
  CarrickListPrivate *priv;
  GtkWidget *viewport, *box;

  parent_class = G_OBJECT_CLASS (carrick_list_parent_class);
  obj = parent_class->constructor (gtype, n_properties, properties);

  priv = LIST_PRIVATE (obj);

  priv->adjustment = gtk_scrolled_window_get_vadjustment
    (GTK_SCROLLED_WINDOW (obj));
  viewport = gtk_viewport_new (gtk_scrolled_window_get_hadjustment
                               (GTK_SCROLLED_WINDOW (obj)),
                               gtk_scrolled_window_get_vadjustment
                               (GTK_SCROLLED_WINDOW (obj)));
  gtk_viewport_set_shadow_type (GTK_VIEWPORT (viewport),
                                GTK_SHADOW_NONE);
  gtk_widget_show (viewport);
  gtk_container_add (GTK_CONTAINER (obj), viewport);


  box = gtk_vbox_new (FALSE, 0);
  gtk_widget_show (box);
  gtk_container_add (GTK_CONTAINER (viewport),
                     box);

  priv->fallback = gtk_label_new ("");
  gtk_label_set_line_wrap (GTK_LABEL (priv->fallback),
                           TRUE);
  gtk_widget_set_size_request (priv->fallback,
                               550,
                               -1);
  gtk_widget_show (priv->fallback);
  gtk_container_add (GTK_CONTAINER (box),
                     priv->fallback);

  priv->box = gtk_vbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (box),
                     priv->box);

  /* add a drag target for dropping below any real
     items in the list*/
  gtk_drag_dest_set (GTK_WIDGET (priv->box),
                     GTK_DEST_DEFAULT_ALL,
                     carrick_targets,
                     G_N_ELEMENTS (carrick_targets),
                     GDK_ACTION_MOVE);
  g_signal_connect (priv->box,
                    "drag-drop",
                    G_CALLBACK (carrick_list_drag_drop),
                    obj);

  return obj;
}

static void
carrick_list_class_init (CarrickListClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GParamSpec *pspec;

  g_type_class_add_private (klass, sizeof (CarrickListPrivate));

  object_class->constructor = carrick_list_constructor;
  object_class->get_property = carrick_list_get_property;
  object_class->set_property = carrick_list_set_property;
  object_class->dispose = carrick_list_dispose;
  object_class->finalize = carrick_list_finalize;

  pspec = g_param_spec_object ("icon-factory",
                               "icon-factory",
                               "CarrickIconFactory object",
                               CARRICK_TYPE_ICON_FACTORY,
                               G_PARAM_READWRITE | G_PARAM_CONSTRUCT);
  g_object_class_install_property (object_class,
                                   PROP_ICON_FACTORY,
                                   pspec);

  pspec = g_param_spec_object ("notification-manager",
                               "CarrickNotificationManager",
                               "Notification manager to use",
                               CARRICK_TYPE_NOTIFICATION_MANAGER,
                               G_PARAM_READWRITE | G_PARAM_CONSTRUCT);
  g_object_class_install_property (object_class,
                                   PROP_NOTIFICATIONS,
                                   pspec);
}

static void
carrick_list_init (CarrickList *self)
{
  CarrickListPrivate *priv = LIST_PRIVATE (self);
  priv->fallback = NULL;

  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (self),
                                  GTK_POLICY_NEVER,
                                  GTK_POLICY_AUTOMATIC);
}

GtkWidget*
carrick_list_new (CarrickIconFactory *icon_factory,
                  CarrickNotificationManager *notifications)
{
  return g_object_new (CARRICK_TYPE_LIST,
                       "icon-factory", icon_factory,
                       "notification-manager", notifications,
                       NULL);
}
