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

#include "carrick-service-item.h"

#include <config.h>
#include <glib/gi18n.h>
#include <gconnman/gconnman.h>
#include "carrick-icon-factory.h"

G_DEFINE_TYPE (CarrickServiceItem, carrick_service_item, GTK_TYPE_EVENT_BOX)

#define SERVICE_ITEM_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), CARRICK_TYPE_SERVICE_ITEM, CarrickServiceItemPrivate))

typedef struct _CarrickServiceItemPrivate CarrickServiceItemPrivate;

enum
{
  PROP_0,
  PROP_ICON_FACTORY,
  PROP_SERVICE
};

struct _CarrickServiceItemPrivate
{
  CmService          *service;
  GtkWidget          *icon;
  GtkWidget          *name_label;
  GtkWidget          *security_label;
  GtkWidget          *connect_button;
  GtkWidget          *table;
  gboolean            connected;
  CarrickIconFactory *icon_factory;
};

static void
carrick_service_item_get_property (GObject *object, guint property_id,
                                   GValue *value, GParamSpec *pspec)
{
  CarrickServiceItemPrivate *priv;

  g_return_if_fail (object != NULL);
  g_return_if_fail (CARRICK_IS_SERVICE_ITEM (object));

  priv = SERVICE_ITEM_PRIVATE (object);

  switch (property_id)
    {
      case PROP_ICON_FACTORY:
        g_value_set_object (value,
                            priv->icon_factory);
        break;
      case PROP_SERVICE:
        g_value_set_object (value,
                            priv->service);
        break;
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

/*
 * Find the uppermost parent window plug so that
 * we can hide it.
 */
GtkWidget *
service_item_find_plug (GtkWidget *widget)
{
  /* Pippinated */
  GtkWidget *iter;

  for (iter = widget; iter ; iter = gtk_widget_get_parent (iter))
  {
    if (GTK_IS_PLUG (iter))
      break;
  }
  return iter;
}

void
_set_state (CmService          *service,
            CarrickServiceItem *item)
{
  CarrickServiceItemPrivate *priv = SERVICE_ITEM_PRIVATE (item);
  gchar *label = NULL;
  gchar *button = NULL;
  GdkPixbuf *pixbuf = NULL;

  if (priv->connected)
  {
    button = g_strdup (_("Disconnect"));
    label = g_strdup_printf ("%s - %s",
                             cm_service_get_name (service),
                             _("Connected"));
    gtk_widget_set_state (GTK_WIDGET (item),
                          GTK_STATE_SELECTED);
  }
  else
  {
    button = g_strdup (_("Connect"));
    label = g_strdup (cm_service_get_name (service));
    gtk_widget_set_state (GTK_WIDGET (item),
                          GTK_STATE_NORMAL);
  }

  gtk_label_set_text (GTK_LABEL (priv->name_label),
                      label);
  gtk_button_set_label (GTK_BUTTON (priv->connect_button),
                        button);

  pixbuf = carrick_icon_factory_get_pixbuf_for_service (priv->icon_factory,
                                                        service);
  gtk_image_set_from_pixbuf (GTK_IMAGE (priv->icon),
                             pixbuf);

  g_free (label);
  g_free (button);
}

void
_connect_button_cb (GtkButton          *connect_button,
                    CarrickServiceItem *item)
{
  CarrickServiceItemPrivate *priv = SERVICE_ITEM_PRIVATE (item);

  if (priv->connected)
  {
    if (cm_service_disconnect (priv->service))
    {
      priv->connected = FALSE;
    }
  }
  else
  {
    const gchar *security = g_strdup (cm_service_get_security (CM_SERVICE (priv->service)));

    if (g_strcmp0 ("none", security) != 0)
    {
      if (!cm_service_get_passphrase (priv->service))
      {
        GtkWidget *dialog;
        GtkWidget *label;
        GtkWidget *icon;
        GtkWidget *entry;
        GtkWidget *hbox;
        const gchar *passphrase = NULL;

        dialog = gtk_dialog_new_with_buttons (_("Passphrase required"),
                                              NULL,
                                              GTK_DIALOG_MODAL |
                                              GTK_DIALOG_DESTROY_WITH_PARENT,
                                              GTK_STOCK_CANCEL,
                                              GTK_RESPONSE_REJECT,
                                              GTK_STOCK_OK,
                                              GTK_RESPONSE_ACCEPT,
                                              NULL);

        gtk_dialog_set_default_response (GTK_DIALOG (dialog),
                                         GTK_RESPONSE_ACCEPT);
        gtk_window_set_icon_name (GTK_WINDOW (dialog),
                                  GTK_STOCK_DIALOG_AUTHENTICATION);
        gtk_box_set_spacing (GTK_BOX (GTK_DIALOG (dialog)->vbox),
                             6);
        hbox = gtk_hbox_new (FALSE,
                             6);
        label = gtk_label_new (_("Network requires authentication. \n"
                                 "Please enter your passphrase"));
        icon = gtk_image_new_from_icon_name (GTK_STOCK_DIALOG_AUTHENTICATION,
                                             GTK_ICON_SIZE_DIALOG);
        gtk_box_pack_start (GTK_BOX (hbox),
                            icon,
                            FALSE,
                            FALSE,
                            6);
        gtk_box_pack_start (GTK_BOX (hbox),
                            label,
                            FALSE,
                            FALSE,
                            6);
        gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox),
                            hbox,
                            FALSE,
                            FALSE,
                            6);
        entry = gtk_entry_new ();
        gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox),
                            entry,
                            FALSE,
                            FALSE,
                            6);

        gtk_widget_show_all (dialog);
        gtk_widget_hide ( service_item_find_plug (GTK_WIDGET (connect_button)));

        if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT)
        {
          passphrase = gtk_entry_get_text (GTK_ENTRY (entry));
          cm_service_set_passphrase (priv->service,
                                     passphrase);
          if (cm_service_connect (CM_SERVICE (priv->service)))
          {
            priv->connected = TRUE;
            gtk_button_set_label (GTK_BUTTON (priv->connect_button),
                                  _("Disconnect"));
          }
        }
        gtk_widget_destroy (dialog);
      }
      else
      {
        /* We have the passphrase already, just connect */
        if (cm_service_connect (priv->service))
        {
          priv->connected = TRUE;
          gtk_button_set_label (GTK_BUTTON (priv->connect_button),
                                _("Disconnect"));
        }
      }
    }
    else
    {
      /* No security, just connect */
      if (cm_service_connect (priv->service))
      {
        priv->connected = TRUE;
        gtk_button_set_label (GTK_BUTTON (priv->connect_button),
                              _("Disconnect"));
      }
    }
  }
  /* Make sure widget is displaying correct state */
  _set_state (priv->service,
              item);
}

void
_name_changed_cb (CmService          *service,
                  CarrickServiceItem *item)
{
  _set_state (service,
              item);
}

void
_status_changed_cb (CmService          *service,
                    CarrickServiceItem *item)
{
  CarrickServiceItemPrivate *priv = SERVICE_ITEM_PRIVATE (item);
  priv->connected = cm_service_get_connected (service);
  _set_state (service,
              item);
}

static void
carrick_service_item_set_service (CarrickServiceItem *service_item,
                                  CmService          *service)
{
  CarrickServiceItemPrivate *priv = SERVICE_ITEM_PRIVATE (service_item);

  if (priv->service)
  {
    g_signal_handlers_disconnect_by_func (service,
                                         _name_changed_cb,
                                         service_item);
    g_signal_handlers_disconnect_by_func (service,
                                          _status_changed_cb,
                                          service_item);
    g_object_unref (priv->service);
    priv->service = NULL;
  }

  if (service)
  {
    gchar *security = g_strdup (cm_service_get_security (service));
    priv->service = g_object_ref (service);

    if (g_strcmp0 ("none", security) != 0)
    {
      gint i;

      for (i = 0; security[i] != '\0'; i++)
      {
        security[i] = g_ascii_toupper (security[i]);
      }
      gtk_label_set_text (GTK_LABEL (priv->security_label),
                          security);
    }
    priv->connected = cm_service_get_connected (service);

    _set_state (service,
                service_item);

    g_signal_connect (priv->connect_button,
                      "clicked",
                      G_CALLBACK (_connect_button_cb),
                      service_item);
    g_signal_connect (priv->service,
                      "name-changed",
                      G_CALLBACK (_name_changed_cb),
                      service_item);
    g_signal_connect (priv->service,
                      "state-changed",
                      G_CALLBACK (_status_changed_cb),
                      service_item);

    g_free (security);
  }
}

CmService *
carrick_service_item_get_service (CarrickServiceItem *item)
{
  CarrickServiceItemPrivate *priv = SERVICE_ITEM_PRIVATE (item);
  return priv->service;
}

gint
carrick_service_item_get_order (CarrickServiceItem *item)
{
  CarrickServiceItemPrivate *priv = SERVICE_ITEM_PRIVATE (item);
  return (gint)cm_service_get_order (priv->service);
}

static void
carrick_service_item_set_property (GObject *object, guint property_id,
                                   const GValue *value, GParamSpec *pspec)
{
  g_return_if_fail (object != NULL);
  g_return_if_fail (CARRICK_IS_SERVICE_ITEM (object));
  CarrickServiceItemPrivate *priv = SERVICE_ITEM_PRIVATE (object);

  switch (property_id)
    {
      case PROP_ICON_FACTORY:
        priv->icon_factory = CARRICK_ICON_FACTORY (g_value_get_object (value));
        break;
      case PROP_SERVICE:
        carrick_service_item_set_service (CARRICK_SERVICE_ITEM (object),
                                          CM_SERVICE (g_value_get_object (value)));
        break;
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
carrick_service_item_dispose (GObject *object)
{
  G_OBJECT_CLASS (carrick_service_item_parent_class)->dispose (object);
}

static void
carrick_service_item_finalize (GObject *object)
{
  G_OBJECT_CLASS (carrick_service_item_parent_class)->finalize (object);
}

static void
carrick_service_item_class_init (CarrickServiceItemClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GParamSpec *pspec;

  g_type_class_add_private (klass, sizeof (CarrickServiceItemPrivate));

  object_class->get_property = carrick_service_item_get_property;
  object_class->set_property = carrick_service_item_set_property;
  object_class->dispose = carrick_service_item_dispose;
  object_class->finalize = carrick_service_item_finalize;

  pspec = g_param_spec_object ("service",
                               "service",
                               "CmService object",
                               CM_TYPE_SERVICE,
                               G_PARAM_READWRITE | G_PARAM_CONSTRUCT);
  g_object_class_install_property (object_class,
                                   PROP_SERVICE,
                                   pspec);

  pspec = g_param_spec_object ("icon-factory",
                               "icon-factory",
                               "CarrickIconFactory object",
                               CARRICK_TYPE_ICON_FACTORY,
                               G_PARAM_READWRITE | G_PARAM_CONSTRUCT);
  g_object_class_install_property (object_class,
                                   PROP_ICON_FACTORY,
                                   pspec);
}

static void
carrick_service_item_init (CarrickServiceItem *self)
{
  CarrickServiceItemPrivate *priv = SERVICE_ITEM_PRIVATE (self);

  priv->connected = FALSE;

  priv->table = gtk_table_new (2, 3,
                               TRUE);
  gtk_container_add (GTK_CONTAINER (self),
                     priv->table);

  priv->icon = gtk_image_new ();
  gtk_table_attach_defaults (GTK_TABLE (priv->table),
                             priv->icon,
                             0, 1,
                             0, 2);
  priv->name_label = gtk_label_new ("");
  gtk_table_attach_defaults (GTK_TABLE (priv->table),
                             priv->name_label,
                             1, 2,
                             0, 1);

  priv->connect_button = gtk_button_new ();
  gtk_table_attach_defaults (GTK_TABLE (priv->table),
                             priv->connect_button,
                             1, 2,
                             1, 2);

  priv->security_label = gtk_label_new ("");
  gtk_table_attach_defaults (GTK_TABLE (priv->table),
                             priv->security_label,
                             2, 3,
                             0, 1);

  gtk_widget_show_all (GTK_WIDGET (self));
}

GtkWidget*
carrick_service_item_new (CarrickIconFactory *icon_factory,
                          CmService          *service)
{
  return g_object_new (CARRICK_TYPE_SERVICE_ITEM,
                       "icon-factory",
                       icon_factory,
                       "service",
                       service,
                       NULL);
}
