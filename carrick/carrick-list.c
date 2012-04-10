/*
 * Carrick - a connection panel for the Dawati Netbook
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
#include <glib/gi18n.h>

#include "connman-service-bindings.h"
#include "connman-manager-bindings.h"

#include "carrick-service-item.h"
#include "carrick-ofono-agent.h"

G_DEFINE_TYPE (CarrickList, carrick_list, GTK_TYPE_SCROLLED_WINDOW)

#define LIST_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), CARRICK_TYPE_LIST, CarrickListPrivate))

struct _CarrickListPrivate
{
  GtkWidget     *box;
  GtkAdjustment *adjustment;
  GtkWidget     *drag_window;
  GtkWidget     *modem_box;
  GtkWidget     *modem_dummy;
  GtkWidget     *pin_box;
  GtkWidget     *puk_entry;
  GtkWidget     *pin_entry;
  GtkWidget     *pin_label;
  GtkWidget     *fallback;

  GtkWidget     *active_item;
  double         active_item_rel_pos;
  int            active_item_height;

  gboolean dragged_item_removed;
  guint drag_position;
  guint drop_position;

  int   scroll_speed;
  guint scroll_timeout_id;

  CarrickIconFactory *icon_factory;
  CarrickNotificationManager *notes;
  CarrickNetworkModel *model;

  gboolean offline_mode;
  gboolean have_daemon;
  gboolean wifi_enabled;
  gboolean ethernet_enabled;
  gboolean threeg_enabled;
  gboolean wimax_enabled;
  gboolean bluetooth_enabled;
  gboolean have_wifi;
  gboolean have_ethernet;
  gboolean have_threeg;
  gboolean have_wimax;
  gboolean have_bluetooth;

  CarrickOfonoAgent *ofono;
  int ofono_connmans;
  GHashTable *cell_services;
  guint pin_requests;

  GHashTable *updates_pending;
  guint delayed_update;
};

enum {
  PROP_0,
  PROP_ICON_FACTORY,
  PROP_NOTIFICATIONS,
  PROP_MODEL
};

static void carrick_list_set_model (CarrickList *list, CarrickNetworkModel *model);
static void carrick_list_add (CarrickList *list, GtkTreePath *path);
static void carrick_list_update_fallback (CarrickList *self);

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

    case PROP_MODEL:
      g_value_set_object (value, priv->model);
      break;

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
    case PROP_ICON_FACTORY:
      carrick_list_set_icon_factory (CARRICK_LIST (object),
                                     CARRICK_ICON_FACTORY (g_value_get_object (value)));
      break;

    case PROP_NOTIFICATIONS:
      carrick_list_set_notification_manager (CARRICK_LIST (object),
                                             CARRICK_NOTIFICATION_MANAGER (g_value_get_object (value)));
      break;

    case PROP_MODEL:
      carrick_list_set_model (CARRICK_LIST (object),
                              CARRICK_NETWORK_MODEL (g_value_get_object (value)));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
carrick_list_dispose (GObject *object)
{
  CarrickList *list = CARRICK_LIST (object);
  carrick_list_set_model (list, NULL);

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
  CarrickListPrivate *priv = list->priv;
  gint                x, y;
  GtkAllocation       alloc;

  gtk_widget_get_allocation (widget, &alloc);

  if (priv->drag_window != NULL)
    return;

  carrick_service_item_set_active (CARRICK_SERVICE_ITEM (widget), FALSE);

  /* save old place in list for drag-failures */
  gtk_container_child_get (GTK_CONTAINER (priv->box),
                           widget,
                           "position", &priv->drag_position,
                           NULL);
  priv->drop_position = priv->drag_position;

  /* remove widget from list and setup dnd popup window */
  priv->drag_window = gtk_window_new (GTK_WINDOW_POPUP);
  gtk_widget_set_size_request (priv->drag_window,
                               alloc.width,
                               alloc.height);
  gtk_widget_get_pointer (widget, &x, &y);
  gtk_widget_reparent (widget, priv->drag_window);
  gtk_drag_set_icon_widget (context,
                            priv->drag_window,
                            x, y);
}

static gboolean
carrick_list_drag_drop (GtkWidget      *widget,
                        GdkDragContext *context,
                        gint            x,
                        gint            y,
                        guint           time,
                        CarrickList    *list)
{
  CarrickListPrivate *priv = list->priv;

  gtk_container_child_get (GTK_CONTAINER (priv->box),
                           widget,
                           "position", &priv->drop_position,
                           NULL);

  gtk_drag_finish (context, TRUE, TRUE, time);
  return TRUE;
}

typedef struct move_data
{
  GtkWidget *item;
  GtkWidget *list;
  gint pos;
} move_data;

static void
move_notify_cb (DBusGProxy *proxy,
                GError     *error,
                gpointer    user_data)
{
  move_data *data = user_data;

  if (error)
    {
      g_debug ("Moving service failed: %s, resetting",
               error->message);
      g_error_free (error);
      gtk_box_reorder_child (GTK_BOX (data->list),
                             data->item,
                             data->pos);
    }

  g_slice_free (move_data, data);
}

static void
carrick_list_drag_end (GtkWidget      *widget,
                       GdkDragContext *context,
                       CarrickList    *list)
{
  CarrickListPrivate *priv = list->priv;

  if (gtk_widget_get_parent (widget) != priv->drag_window)
    return;

  /* destroy the popup window */
  g_object_ref (widget);
  gtk_container_remove (GTK_CONTAINER (priv->drag_window), widget);
  gtk_widget_destroy (priv->drag_window);
  priv->drag_window = NULL;

  /* If the item that was being dragged got removed from the model
   * during the drag, we should do nothing else. */
  if (priv->dragged_item_removed)
    {
      priv->dragged_item_removed = FALSE;
      return;
    }

  /* insert the widget into the list */
  gtk_box_pack_start (GTK_BOX (priv->box), widget,
                      FALSE, FALSE, 2);
  gtk_box_reorder_child (GTK_BOX (priv->box),
                         widget,
                         priv->drop_position);
  g_object_unref (widget);

  if (priv->drop_position != priv->drag_position)
    {
      GList       *children;
      GtkWidget   *other_widget;
      DBusGProxy  *service, * other_service;
      const gchar *path;
      move_data   *data = NULL;

      children = gtk_container_get_children (GTK_CONTAINER (priv->box));

      service = carrick_service_item_get_proxy (CARRICK_SERVICE_ITEM (widget));
      data = g_slice_new0 (move_data);
      data->list = GTK_WIDGET (list->priv->box);
      data->item = widget;
      data->pos = priv->drag_position;

      if (priv->drop_position == 0)
        {
          other_widget = g_list_nth_data (children, 0);
          other_service = carrick_service_item_get_proxy
                    (CARRICK_SERVICE_ITEM (other_widget));
          path = dbus_g_proxy_get_path (other_service);

          net_connman_Service_move_before_async (service,
                                                 path,
                                                 move_notify_cb,
                                                 data);
        }
      else
        {
          other_widget = g_list_nth_data (children,
                                          priv->drop_position - 1);

          other_service = carrick_service_item_get_proxy
                    (CARRICK_SERVICE_ITEM (other_widget));
          path = dbus_g_proxy_get_path (other_service);

          net_connman_Service_move_after_async (service,
                                                path,
                                                move_notify_cb,
                                                data);
        }

      g_list_free (children);
    }
}

static void
_list_collapse_inactive_items (GtkWidget *item,
                               GtkWidget *active_item)
{
  if (item != active_item)
    {
      carrick_service_item_set_active (CARRICK_SERVICE_ITEM (item), FALSE);
    }
}

static gboolean
_active_item_was_visible (CarrickList *list)
{
  CarrickListPrivate *priv = list->priv;
  GtkAllocation alloc;

  gtk_widget_get_allocation (priv->active_item, &alloc);
  if (priv->active_item_rel_pos + alloc.height < 0)
    return FALSE;

  gtk_widget_get_allocation (GTK_WIDGET (list), &alloc);
  if (priv->active_item_rel_pos > alloc.height)
    return FALSE;

  return TRUE;
}

/* TODO: its possible that it would make sense to set the adjustment
 * after the whole list allocation-cycle is done... there is a
 * potential for a race here */
static void
active_item_alloc_cb (GtkWidget     *item,
                      GdkRectangle  *allocation,
                      CarrickList   *list)
{
  CarrickListPrivate *priv = list->priv;
  GtkAllocation list_alloc;

  if (!gtk_widget_is_ancestor (item, GTK_WIDGET (list)))
    return;

  gtk_widget_get_allocation (GTK_WIDGET (list), &list_alloc);

  /* Active item moved or expanded: We can autoscroll to keep
   * it in the same spot on the screen, but we only want that if
   * the active item is/was visible */
  if (_active_item_was_visible (list) &&
      allocation->height >= priv->active_item_height)
    {
      int space_below, expand, move = 0;

      /* make sure expanding items are visible */
      if (allocation->height > priv->active_item_height)
        {
          expand = allocation->height - priv->active_item_height;
          space_below = list_alloc.height -
                        (priv->active_item_rel_pos + priv->active_item_height);
          move = MAX (0, expand - space_below);
        }

      /* scroll so that A) expanded item is visible and B)
       * the items position relative to screen stays the same */
      gtk_adjustment_set_value (priv->adjustment,
                                allocation->y - priv->active_item_rel_pos + move);
    }

  priv->active_item_height = allocation->height;
}

static void
_adjustment_value_changed_cb (GtkAdjustment *adj, CarrickList *list)
{
  CarrickListPrivate *priv = list->priv;
  GtkAllocation alloc;

  /* save the position of the active item relative to the adjustment.
   * This allows us to preserve that position in active_item_alloc_cb() */
  if (priv->active_item) {
    gtk_widget_get_allocation (priv->active_item, &alloc);
    priv->active_item_rel_pos = alloc.y -
                                gtk_adjustment_get_value (adj);
  } else {
    priv->active_item_rel_pos =  0;
  }
}

/* called when a ServiceItem signals it is active or when
 * the active ServiceItem is removed from list*/
static void
_set_active_item (CarrickList *list, GtkWidget *item)
{
  CarrickListPrivate *priv = list->priv;

  if (priv->active_item && priv->active_item != list->priv->modem_dummy)
    g_signal_handlers_disconnect_by_func (priv->active_item,
                                         active_item_alloc_cb,
                                         list);

  priv->active_item = item;
  if (priv->active_item && priv->active_item != list->priv->modem_dummy)
    {
      GtkAllocation alloc;

      gtk_widget_get_allocation (priv->active_item, &alloc);

      priv->active_item_rel_pos = alloc.y -
                                  gtk_adjustment_get_value (priv->adjustment);
      priv->active_item_height = alloc.height;
      g_signal_connect (priv->active_item, "size-allocate",
                        G_CALLBACK (active_item_alloc_cb), list);
    }
}


static void
_item_active_changed (GtkWidget  *item,
                      GParamSpec *pspec,
                      GtkWidget  *list)
{
  CarrickListPrivate *priv = LIST_PRIVATE (list);

  if (carrick_service_item_get_active (CARRICK_SERVICE_ITEM (item)))
    {
      gtk_container_foreach (GTK_CONTAINER (priv->box),
                             (GtkCallback) _list_collapse_inactive_items,
                             item);
      _set_active_item (CARRICK_LIST (list), item);
    }
}


static void
carrick_list_update_modem_service (CarrickList *list)
{
  gboolean show_dummy_modem = FALSE;

  if (!list->priv->modem_dummy)
    return;

  /* show the dummy if
   * A) we have more ofono Connection managers than cell services
   *    (meaning we can configure a new sim)
   * B) there is a pin query
   */
  show_dummy_modem = (list->priv->ofono_connmans > g_hash_table_size (list->priv->cell_services) ||
                      (list->priv->pin_requests > 0));
  gtk_widget_set_visible (list->priv->modem_box, show_dummy_modem);
}

static void
_notify_service_item_type_cb (GtkWidget  *item,
                              GParamSpec *pspec,
                              CarrickList  *list)
{
  CarrickListPrivate *priv = LIST_PRIVATE (list);
  const char *type;

  type = carrick_service_item_get_service_type (CARRICK_SERVICE_ITEM (item));
  if (g_strcmp0 (type, "cellular") == 0) {
    g_hash_table_insert (priv->cell_services, item, item);
    carrick_list_update_modem_service (list);
  } else {
    if (g_hash_table_remove (priv->cell_services, item))
      carrick_list_update_modem_service (list);
  }

}

typedef struct find_data
{
  GtkWidget  *widget;
  DBusGProxy *service;
} find_data;

static void
_list_contains_child (GtkWidget *item,
                      find_data *data)
{
  CarrickServiceItem *service_item = CARRICK_SERVICE_ITEM (item);
  DBusGProxy         *proxy = carrick_service_item_get_proxy (service_item);
  const gchar        *path;
  const gchar        *data_path = dbus_g_proxy_get_path (data->service);

  if (proxy)
    {
      path = dbus_g_proxy_get_path (proxy);

      if (g_str_equal (data_path, path))
        {
          data->widget = item;
        }
    }
}

static GtkWidget *
carrick_list_find_service_item (CarrickList *list,
                                DBusGProxy  *service)
{
  CarrickListPrivate *priv = list->priv;
  find_data          *data;
  GtkWidget          *ret;

  data = g_slice_new0 (find_data);
  data->service = service;
  data->widget = NULL;

  gtk_container_foreach (GTK_CONTAINER (priv->box),
                         (GtkCallback) _list_contains_child,
                         data);
  ret = data->widget;

  g_slice_free (find_data, data);

  return ret;
}

static void
_set_item_inactive (GtkWidget *widget)
{
  carrick_service_item_set_active (CARRICK_SERVICE_ITEM (widget), FALSE);
}


void
carrick_list_clear_state (CarrickList *list)
{
  CarrickListPrivate *priv = list->priv;

  gtk_container_foreach (GTK_CONTAINER (priv->box),
                         (GtkCallback) _set_item_inactive,
                         NULL);
  gtk_adjustment_set_value (priv->adjustment,
                            gtk_adjustment_get_lower (priv->adjustment));
}


static void
carrick_list_create_modem_service (CarrickList *list)
{
  CarrickListPrivate *priv = list->priv;

  if (priv->modem_dummy || !priv->icon_factory || !priv->notes)
    return;

  priv->modem_dummy = carrick_service_item_new_as_modem_proxy (priv->icon_factory,
                                                               priv->ofono,
                                                               priv->notes);
  g_signal_connect (priv->modem_dummy, "notify::active",
                    G_CALLBACK (_item_active_changed), list);
  gtk_box_pack_start (GTK_BOX (priv->modem_box), priv->modem_dummy,
                      FALSE, FALSE, 2);
  gtk_widget_show (priv->modem_dummy);

  carrick_list_update_modem_service (list);
}

void
carrick_list_set_icon_factory (CarrickList        *list,
                               CarrickIconFactory *icon_factory)
{
  CarrickListPrivate *priv;

  g_return_if_fail (CARRICK_IS_LIST (list));

  priv = list->priv;

  priv->icon_factory = icon_factory;

  carrick_list_create_modem_service (list);
}

CarrickIconFactory*
carrick_list_get_icon_factory (CarrickList *list)
{
  CarrickListPrivate *priv;

  g_return_val_if_fail (CARRICK_IS_LIST (list), NULL);

  priv = list->priv;

  return priv->icon_factory;
}

void
carrick_list_set_notification_manager (CarrickList                *list,
                                       CarrickNotificationManager *notification_manager)
{
  CarrickListPrivate *priv;

  g_return_if_fail (CARRICK_IS_LIST (list));

  priv = list->priv;

  priv->notes = notification_manager;

  carrick_list_create_modem_service (list);
}

CarrickNotificationManager*
carrick_list_get_notification_manager (CarrickList *list)
{
  CarrickListPrivate *priv;

  g_return_val_if_fail (CARRICK_IS_LIST (list), NULL);

  priv = list->priv;

  return priv->notes;
}

static void
_row_inserted_cb (GtkTreeModel *tree_model,
                  GtkTreePath  *path,
                  GtkTreeIter  *iter,
                  gpointer      user_data)
{
  CarrickList *self = user_data;

  /* New row added to model, draw widgetry */
  carrick_list_add (self, path);
}

static void
_find_and_remove (GtkWidget *item,
                  gpointer   user_data)
{
  g_return_if_fail (CARRICK_IS_SERVICE_ITEM (item));
  g_return_if_fail (item != NULL);
  g_return_if_fail (user_data != NULL);

  CarrickList *list = CARRICK_LIST (user_data);
  CarrickServiceItem *service_item = CARRICK_SERVICE_ITEM (item);
  GtkTreeRowReference *row = carrick_service_item_get_row_reference (service_item);

  if (gtk_tree_row_reference_valid (row) == FALSE)
    {
      if (list->priv->active_item == item)
        _set_active_item (list, NULL);

      if (g_hash_table_remove (list->priv->cell_services, item))
        carrick_list_update_modem_service (list);

      g_hash_table_remove (list->priv->updates_pending, item);

      gtk_widget_destroy (item);
    }
}

static CarrickServiceItem*
_get_item_from_drag_window(GtkWidget *drag_window)
{
  GList *children;
  CarrickServiceItem *item;

  children = gtk_container_get_children (GTK_CONTAINER (drag_window));
  item = CARRICK_SERVICE_ITEM (children->data);
  g_list_free (children);

  return item;
}

static void
_row_deleted_cb (GtkTreeModel *tree_model,
                 GtkTreePath  *path,
                 gpointer      user_data)
{
  CarrickList *self = user_data;
  CarrickListPrivate *priv = self->priv;
  GtkTreeIter iter;

  /* If the model is empty, delete all widgets and show some fallback content */
  if (gtk_tree_model_get_iter_first (tree_model, &iter) == FALSE)
    {
      g_hash_table_remove_all (self->priv->updates_pending);

      gtk_container_foreach (GTK_CONTAINER (priv->box),
                             (GtkCallback)gtk_widget_destroy,
                             NULL);
      carrick_list_update_fallback (self);
      gtk_widget_show (priv->fallback);
      _set_active_item (self, NULL);

      if (priv->drag_window)
        priv->dragged_item_removed = TRUE;

      return;
    }

  /* Then we check whether the dragged item got removed. */
  if (priv->drag_window && !priv->dragged_item_removed)
    {
      CarrickServiceItem *item = _get_item_from_drag_window (priv->drag_window);
      GtkTreeRowReference *row = carrick_service_item_get_row_reference (item);

      if (!gtk_tree_row_reference_valid (row))
        {
          priv->dragged_item_removed = TRUE;
          return;
        }
    }

  /* Row that got removed is not the one that is being dragged;
   * find widget with corresponding GtkTreePath and destroy */
  gtk_container_foreach (GTK_CONTAINER (priv->box),
                         _find_and_remove,
                         user_data);
}

static void
_rows_reordered_cb (GtkTreeModel *tree_model,
                    GtkTreePath  *path,
                    GtkTreeIter  *iter,
                    gpointer      user_data)
{
  CarrickList        *list = user_data;
  CarrickListPrivate *priv = list->priv;
  GtkWidget          *item = NULL;
  DBusGProxy         *proxy = NULL;
  guint               order, index;

  if (!iter)
    return;

  gtk_tree_model_get (tree_model, iter,
                      CARRICK_COLUMN_PROXY, &proxy,
                      CARRICK_COLUMN_INDEX, &index,
                      -1);

  /* Find widget for changed row, tell it to refresh
   * variables and update */
  item = carrick_list_find_service_item (list,
                                         proxy);

  /* Check the order and, where neccesarry, reorder */
  gtk_container_child_get (GTK_CONTAINER (priv->box),
                           item,
                           "position", &order,
                           NULL);
  if (order != index)
    gtk_box_reorder_child (GTK_BOX (priv->box),
                           item,
                           index);
}

static void
_update_service_item (CarrickServiceItem *item,
                      gpointer value,
                      gpointer userdata)
{
  carrick_service_item_update (item);
}

static gboolean
_delayed_update (CarrickList *list)
{
  g_hash_table_foreach_remove (list->priv->updates_pending,
                               (GHRFunc)_update_service_item,
                               NULL);
  list->priv->delayed_update = 0;
  return FALSE;
}

static void
carrick_list_add_pending_service_update (CarrickList *list,
                                         CarrickServiceItem *item)
{
  /* wait a short time before running carrick_service_item_update()
   * so subsequent changes don't trigger another call. */
  g_hash_table_insert (list->priv->updates_pending, item, item);
  if (list->priv->delayed_update == 0)
    list->priv->delayed_update = g_timeout_add (100,
                                                (GSourceFunc)_delayed_update,
                                                list);
}

static void
_row_changed_cb (GtkTreeModel *model,
                 GtkTreePath  *path,
                 GtkTreeIter  *iter,
                 gpointer      user_data)
{
  CarrickList        *list = user_data;
  CarrickListPrivate *priv = list->priv;
  GtkWidget          *item = NULL;
  DBusGProxy         *proxy = NULL;
  guint               order, index;

  gtk_tree_model_get (model, iter,
                      CARRICK_COLUMN_PROXY, &proxy,
                      CARRICK_COLUMN_INDEX, &index,
                      -1);

  /* Find widget for changed row, tell it to refresh
   * variables and update */
  item = carrick_list_find_service_item (list, proxy);
  if (item)
    {
      carrick_list_add_pending_service_update (list,
                                               CARRICK_SERVICE_ITEM (item));

      /* Check the order and, where neccesarry, reorder */
      gtk_container_child_get (GTK_CONTAINER (priv->box),
                               item,
                               "position", &order,
                               NULL);
      if (order != index)
        gtk_box_reorder_child (GTK_BOX (priv->box),
                               item,
                               index);

      return;
    }

  /* If we didn't find the widget on the list, it may be that it is
   * being dragged, so let's check this possibility as well. */
  if (priv->drag_window)
    {
      CarrickServiceItem *item = _get_item_from_drag_window (priv->drag_window);
      GtkTreeRowReference *row_reference = carrick_service_item_get_row_reference (item);
      DBusGProxy *drag_proxy;

      if (!gtk_tree_row_reference_valid (row_reference))
        {
          _row_inserted_cb (model, path, iter, list);
          return;
        }

      drag_proxy = carrick_service_item_get_proxy (item);

      /* If we found the item we're looking for is being dragged,
       * update it. */
      if (!g_strcmp0 (dbus_g_proxy_get_path (proxy),
                      dbus_g_proxy_get_path (drag_proxy)))
        {
          carrick_list_add_pending_service_update (list,
                                                   CARRICK_SERVICE_ITEM (item));
          return;
        }
    }

  /* We don't have a widget for this item, so create it! */
  _row_inserted_cb (model, path, iter, list);
}

static gboolean
_create_service_item (GtkTreeModel *model,
                      GtkTreePath  *path,
                      GtkTreeIter  *iter,
                      gpointer      user_data)
{
  _row_inserted_cb (model, path, iter, user_data);

  return FALSE;
}

static void
list_update_property (const gchar *property,
                      GValue      *value,
                      gpointer     user_data)
{
  CarrickList        *list = user_data;
  CarrickListPrivate *priv = list->priv;

  if (g_str_equal (property, "OfflineMode"))
    {
      priv->offline_mode = g_value_get_boolean (value);
    }
  else if (g_str_equal (property, "AvailableTechnologies"))
    {
      gchar **tech = g_value_get_boxed (value);
      gint    i;

      priv->have_wifi = FALSE;
      priv->have_ethernet = FALSE;
      priv->have_threeg = FALSE;
      priv->have_wimax = FALSE;
      priv->have_bluetooth = FALSE;

      for (i = 0; i < g_strv_length (tech); i++)
        {
          if (g_str_equal ("wifi", *(tech + i)))
            priv->have_wifi = TRUE;
          else if (g_str_equal ("wimax", *(tech + i)))
            priv->have_wimax = TRUE;
          else if (g_str_equal ("bluetooth", *(tech + i)))
            priv->have_bluetooth = TRUE;
          else if (g_str_equal ("cellular", *(tech + i)))
            priv->have_threeg = TRUE;
          else if (g_str_equal ("ethernet", *(tech + i)))
            priv->have_ethernet = TRUE;
        }
    }
  else if (g_str_equal (property, "EnabledTechnologies"))
    {
      gchar **tech = g_value_get_boxed (value);
      gint    i;

      priv->wifi_enabled = FALSE;
      priv->ethernet_enabled = FALSE;
      priv->threeg_enabled = FALSE;
      priv->wimax_enabled = FALSE;
      priv->bluetooth_enabled = FALSE;

      for (i = 0; i < g_strv_length (tech); i++)
        {
          if (g_str_equal ("wifi", *(tech + i)))
            priv->wifi_enabled = TRUE;
          else if (g_str_equal ("wimax", *(tech + i)))
            priv->wimax_enabled = TRUE;
          else if (g_str_equal ("bluetooth", *(tech + i)))
            priv->bluetooth_enabled = TRUE;
          else if (g_str_equal ("cellular", *(tech + i)))
            priv->threeg_enabled = TRUE;
          else if (g_str_equal ("ethernet", *(tech + i)))
            priv->ethernet_enabled = TRUE;
        }
    }
}

static void
_mngr_property_changed_cb (DBusGProxy  *manager,
                           const gchar *property,
                           GValue      *value,
                           CarrickList *list)
{
  list_update_property (property, value, list);

  if (gtk_widget_get_visible (list->priv->fallback))
      carrick_list_update_fallback (list);
}

static void
_mngr_get_properties_cb (DBusGProxy     *manager,
                         GHashTable     *properties,
                         GError         *error,
                         gpointer        user_data)
{
  CarrickList *list = user_data;
  CarrickListPrivate *priv = list->priv;

  if (error)
    {
      g_debug ("Error when ending GetProperties call: %s",
               error->message);
      g_error_free (error);

      priv->have_daemon = FALSE;
    }
  else
    {
      priv->have_daemon = TRUE;
      g_hash_table_foreach (properties,
                            (GHFunc) list_update_property,
                            list);
      g_hash_table_unref (properties);
    }

  carrick_list_update_fallback (list);
}

static void
carrick_list_set_model (CarrickList         *list,
                        CarrickNetworkModel *model)
{
  CarrickListPrivate *priv = list->priv;

  if (priv->model)
    {
      g_signal_handlers_disconnect_by_func (priv->model,
                                            _row_inserted_cb,
                                            list);
      g_signal_handlers_disconnect_by_func (priv->model,
                                            _row_deleted_cb,
                                            list);
      g_signal_handlers_disconnect_by_func (priv->model,
                                            _row_changed_cb,
                                            list);
      g_signal_handlers_disconnect_by_func (priv->model,
                                            _rows_reordered_cb,
                                            list);

      g_object_unref (priv->model);
      priv->model = NULL;
    }

  if (model)
    {
      DBusGProxy *manager;

      priv->model = g_object_ref (model);

      /* Keep track of OfflineMode, AvailableTechnologies and
       * EnabledTechnologies using connman.Manager */
      manager = carrick_network_model_get_proxy (priv->model);
      dbus_g_proxy_connect_signal (manager,
                                   "PropertyChanged",
                                   G_CALLBACK (_mngr_property_changed_cb),
                                   list, NULL);
      net_connman_Manager_get_properties_async (manager,
                                                _mngr_get_properties_cb,
                                                list);

      gtk_tree_model_foreach (GTK_TREE_MODEL (model),
                              _create_service_item,
                              NULL);

      /* connect signals for changes in model */
      g_signal_connect (priv->model,
                        "row-inserted",
                        G_CALLBACK (_row_inserted_cb),
                        list);
      g_signal_connect (priv->model,
                        "row-deleted",
                        G_CALLBACK (_row_deleted_cb),
                        list);
      g_signal_connect (priv->model,
                        "row-changed",
                        G_CALLBACK (_row_changed_cb),
                        list);
      g_signal_connect (priv->model,
                        "rows-reordered",
                        G_CALLBACK (_rows_reordered_cb),
                        list);
    }
}

GList*
carrick_list_get_children (CarrickList *list)
{
  CarrickListPrivate *priv = list->priv;

  return gtk_container_get_children (GTK_CONTAINER (priv->box));
}

#define FAST_SCROLL_BUFFER 15
#define SCROLL_BUFFER 60

static gboolean
carrick_list_scroll (CarrickList *list)
{
  CarrickListPrivate *priv = list->priv;
  gdouble             val, page_size;
  gboolean            at_end;

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
  CarrickListPrivate *priv = list->priv;
  int                 list_x, list_y;
  int                 new_speed;
  GtkAllocation       list_alloc;

  gtk_widget_get_allocation (GTK_WIDGET (list), &list_alloc);

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
           list_alloc.height - list_y < FAST_SCROLL_BUFFER)
    {
      new_speed = 12;
    }
  else if (gtk_adjustment_get_value (priv->adjustment) <
           gtk_adjustment_get_upper (priv->adjustment) &&
           list_alloc.height - list_y < SCROLL_BUFFER)
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
                    (40, (GSourceFunc) carrick_list_scroll, list);
        }
    }
}

static void
carrick_list_add (CarrickList *list,
                  GtkTreePath *path)
{
  CarrickListPrivate *priv;
  GtkWidget          *widget;

  g_return_if_fail (CARRICK_IS_LIST (list));

  priv = list->priv;
  widget = carrick_service_item_new (priv->icon_factory,
                                     priv->ofono,
                                     priv->notes,
                                     priv->model,
                                     path);

  gtk_widget_show (widget);


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

  g_signal_connect (widget,
                    "drag-drop",
                    G_CALLBACK (carrick_list_drag_drop),
                    list);

  /* listen to activate so we can deactivate other items in list */
  g_signal_connect (widget,
                    "notify::active",
                    G_CALLBACK (_item_active_changed),
                    list);

  /*listen to type to count cellular services */
  g_signal_connect (widget, "notify::type",
                    G_CALLBACK (_notify_service_item_type_cb), list);
  _notify_service_item_type_cb (widget, NULL, list);

  gtk_box_pack_start (GTK_BOX (priv->box),
                      widget,
                      FALSE,
                      FALSE,
                      2);

  gtk_widget_hide (priv->fallback);
  gtk_widget_show (priv->box);
}


static void
_append_tech_string (GString *technologies, char *tech, gboolean last)
{
  /* Translator note: The disabled technologies available to be turned on is put together at
   * runtime.
   * The conjunction ' or ' will be at the end of a choice of disabled technologies,
   * for example 'You could try enabling WiFi, WiMAX or 3G'.
   * Note that you need to include spaces on both sides of the word/phrase
   * here -- unless you want it joined with previous or next word. */
  gchar              *conjunction = _(" or ");

  /* Translator note: the comma ', ' will be used to join the different
   * disabled technologies as in the example:
   * 'You could try enabling WiFi, WiMAX or 3G'
   * Note that you need to include spaces in the string, unless you
   * want the words to appear right next to the comma. */
  gchar              *comma = _(", ");

  if (technologies->len == 0)
    g_string_append (technologies, tech);
  else if (!last)
    g_string_append_printf (technologies, "%s%s",
                            comma, tech);
  else
    g_string_append_printf (technologies, "%s%s",
                            conjunction, tech);
}

static void
carrick_list_update_fallback (CarrickList *self)
{
  CarrickListPrivate *priv = self->priv;
  gchar              *fallback = NULL;
  GString            *technologies = NULL;
  guint               count = 0;

  /* Translator note: these technology names will be used in forming
   * sentences like : 'You could try enabling WiFi, WiMAX or 3G'
   */
  gchar              *ethernet = _("wired");
  gchar              *wifi = _("WiFi");
  gchar              *wimax = _("WiMAX");
  gchar              *cellular = _("3G");
  gchar              *bluetooth = _("Bluetooth");

  /* Need to add some fall-back content */
  if (!priv->have_daemon)
    {
      /*
       * Translators: this string is displayed if there
       * are no available networks because ConnMan, the
       * connectivity daemon, is not running.
       */
      fallback = g_strdup (_("Sorry, we can't find any networks. "
                             "It appears that the connection manager is not running. "
                             "You could try restarting your computer."));
    }
  else if (priv->offline_mode)
    {
      /*
       * Hint display if we detect that the system is in
       * offline mode and there are no available networks
       */
      fallback = g_strdup (_ ("Sorry, we can't find any networks. "
                              "You could try disabling Offline mode. "
                              ));
    }
  else if ((priv->have_wifi && !priv->wifi_enabled) ||
           (priv->have_ethernet && !priv->ethernet_enabled) ||
           (priv->have_threeg && !priv->threeg_enabled) ||
           (priv->have_wimax && !priv->wimax_enabled) ||
           (priv->have_bluetooth && !priv->bluetooth_enabled))
    {
      guint available, enabled;

      /* How many strings we're joining */
      available = priv->have_wifi + priv->have_ethernet +
                  priv->have_threeg + priv->have_wimax +
                  priv->have_bluetooth;
      enabled = priv->wifi_enabled + priv->ethernet_enabled +
                priv->threeg_enabled + priv->wimax_enabled +
                priv->bluetooth_enabled;
      count = available - enabled;
      technologies = g_string_new (NULL);

      if (priv->have_wifi && !priv->wifi_enabled)
        {
          _append_tech_string (technologies, wifi, count == 1);
          count--;
        }

      if (priv->have_wimax && !priv->wimax_enabled)
        {
          _append_tech_string (technologies, wimax, count == 1);
          count--;
        }

      if (priv->have_threeg && !priv->threeg_enabled)
        {
          _append_tech_string (technologies, cellular, count == 1);
          count--;
        }

      if (priv->have_bluetooth && !priv->bluetooth_enabled)
        {
          _append_tech_string (technologies, bluetooth, count == 1);
          count--;
        }

      if (priv->have_ethernet && !priv->ethernet_enabled)
        {
          _append_tech_string (technologies, ethernet, count == 1);
          count--;
        }

      fallback = g_strdup_printf (_("Sorry, we can't find any networks. "
                                    "You could try enabling %s."),
                                  technologies->str);
      g_string_free (technologies,
                     TRUE);
    }

  if (!fallback)
    {
      /*
       * Generic message to display if all available networking
       * technologies are turned on, but for whatever reason we
       * can not find any networks
       */
      fallback = g_strdup (_ ("Sorry, we can't find any networks"));
    }

  gtk_label_set_text (GTK_LABEL (priv->fallback), fallback);
  g_free (fallback);
}

static GObject *
carrick_list_constructor (GType                  gtype,
                          guint                  n_properties,
                          GObjectConstructParam *properties)
{
  GObject            *obj;
  GObjectClass       *parent_class;
  CarrickListPrivate *priv;
  GtkWidget          *viewport, *box;

  parent_class = G_OBJECT_CLASS (carrick_list_parent_class);
  obj = parent_class->constructor (gtype, n_properties, properties);

  priv = LIST_PRIVATE (obj);

  priv->drag_window = NULL;

  priv->adjustment = gtk_scrolled_window_get_vadjustment
                (GTK_SCROLLED_WINDOW (obj));
  g_signal_connect (priv->adjustment, "value-changed",
                    G_CALLBACK (_adjustment_value_changed_cb), obj);
  viewport = gtk_viewport_new (gtk_scrolled_window_get_hadjustment
                                           (GTK_SCROLLED_WINDOW (obj)),
                               gtk_scrolled_window_get_vadjustment
                                           (GTK_SCROLLED_WINDOW (obj)));
  gtk_viewport_set_shadow_type (GTK_VIEWPORT (viewport),
                                GTK_SHADOW_NONE);
  gtk_widget_show (viewport);
  gtk_container_add (GTK_CONTAINER (obj), viewport);


  box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_widget_show (box);
  gtk_container_add (GTK_CONTAINER (viewport),
                     box);

  gtk_label_set_line_wrap (GTK_LABEL (priv->fallback),
                           TRUE);
  gtk_widget_set_size_request (priv->fallback,
                               550,
                               -1);
  gtk_widget_show (priv->fallback);
  gtk_misc_set_padding (GTK_MISC (priv->fallback), 0, 12);
  gtk_box_pack_start (GTK_BOX (box), priv->fallback,
                      FALSE, FALSE, 2);

  priv->modem_box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_box_pack_start (GTK_BOX (box), priv->modem_box,
                      FALSE, FALSE, 2);
  gtk_widget_set_vexpand (priv->modem_box, FALSE);
  gtk_widget_set_no_show_all (priv->modem_box, TRUE);

  priv->box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_box_pack_start (GTK_BOX (box), priv->box,
                      FALSE, FALSE, 0);

  carrick_list_create_modem_service (CARRICK_LIST (obj));

  return obj;
}

static void
carrick_list_class_init (CarrickListClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GParamSpec   *pspec;

  g_type_class_add_private (klass, sizeof (CarrickListPrivate));

  object_class->constructor = carrick_list_constructor;
  object_class->get_property = carrick_list_get_property;
  object_class->set_property = carrick_list_set_property;
  object_class->dispose = carrick_list_dispose;
  object_class->finalize = carrick_list_finalize;

  pspec = g_param_spec_object ("icon-factory",
                               "Icon factory",
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

  pspec = g_param_spec_object ("model",
                               "Network model",
                               "CarrickNetworkModel which the list represents",
                               CARRICK_TYPE_NETWORK_MODEL,
                               G_PARAM_READWRITE | G_PARAM_CONSTRUCT);
  g_object_class_install_property (object_class,
                                   PROP_MODEL,
                                   pspec);
}

static void
_ofono_notify_n_connection_managers_cb (CarrickOfonoAgent *ofono,
                                        GParamSpec *arg1,
                                        CarrickList *self)
{
  g_object_get (ofono, "n-connection-managers", &self->priv->ofono_connmans, NULL);
  carrick_list_update_modem_service (self);
}

static void
_ofono_notify_required_pins_cb (CarrickOfonoAgent *ofono,
                                GParamSpec *arg1,
                                CarrickList *self)
{
  GHashTable *required_pins;

  g_object_get (ofono, "required-pins", &required_pins, NULL);

  self->priv->pin_requests = g_hash_table_size (required_pins);
  carrick_list_update_modem_service (self);
}

static void
carrick_list_init (CarrickList *self)
{
  CarrickListPrivate *priv;

  priv = self->priv = LIST_PRIVATE (self);

  priv->fallback = gtk_label_new ("");
  priv->have_daemon = FALSE;
  priv->cell_services = g_hash_table_new (g_direct_hash, g_direct_equal);
  priv->ofono = carrick_ofono_agent_new ();
  g_signal_connect (priv->ofono, "notify::n-connection-managers",
                    G_CALLBACK (_ofono_notify_n_connection_managers_cb), self);
  _ofono_notify_n_connection_managers_cb (priv->ofono, NULL, self);
  g_signal_connect (priv->ofono, "notify::required-pins",
                    G_CALLBACK (_ofono_notify_required_pins_cb), self);
  _ofono_notify_required_pins_cb (priv->ofono, NULL, self);

  priv->updates_pending = g_hash_table_new (g_direct_hash, g_direct_equal);

  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (self),
                                  GTK_POLICY_NEVER,
                                  GTK_POLICY_AUTOMATIC);
}

GtkWidget*
carrick_list_new (CarrickIconFactory         *icon_factory,
                  CarrickNotificationManager *notifications,
                  CarrickNetworkModel        *model)
{
  return g_object_new (CARRICK_TYPE_LIST,
                       "icon-factory", icon_factory,
                       "notification-manager", notifications,
                       "model", model,
                       NULL);
}
