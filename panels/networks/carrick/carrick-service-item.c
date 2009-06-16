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
#include <nbtk/nbtk-gtk.h>
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

typedef enum
{
  UNKNOWN,
  IDLE,
  FAIL,
  CONFIGURE,
  READY
} ServiceItemState;

struct _CarrickServiceItemPrivate
{
  CmService          *service;
  GtkWidget          *icon;
  GtkWidget          *name_label;
  GtkWidget          *security_label;
  GtkWidget          *connect_button;
  GtkWidget          *delete_button;
  GtkWidget          *expando;
  ServiceItemState    state;
  CarrickIconFactory *icon_factory;
  gboolean            failed;
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
  while (widget)
  {
    if (GTK_IS_PLUG (widget))
      return widget;
    widget = gtk_widget_get_parent (widget);
  }
  return NULL;
}

void
_set_state (CmService          *service,
            CarrickServiceItem *item)
{
  CarrickServiceItemPrivate *priv = SERVICE_ITEM_PRIVATE (item);
  gchar *label = NULL;
  gchar *button = NULL;
  GdkPixbuf *pixbuf = NULL;
  gchar *name = NULL;
  gchar *security = NULL;
  gchar *security_label = NULL;

  name = g_strdup (cm_service_get_name (service));
  security = g_strdup (cm_service_get_security (service));

  if (g_strcmp0 ("ethernet", name) == 0)
  {
    g_free (name);
    name = g_strdup (_("Wired"));
  }

  if (priv->state == READY)
  {
    gtk_widget_set_sensitive (GTK_WIDGET (priv->connect_button),
                              TRUE);
    gtk_widget_set_no_show_all (GTK_WIDGET (priv->delete_button),
                                FALSE);
    gtk_widget_show (GTK_WIDGET (priv->delete_button));
    gtk_widget_set_sensitive (GTK_WIDGET (priv->delete_button),
                              TRUE);
    button = g_strdup (_("Disconnect"));
    label = g_strdup_printf ("%s - %s",
                             name,
                             _("Connected"));
    priv->failed = FALSE;
  }
  else if (priv->state == CONFIGURE)
  {
    gtk_widget_set_sensitive (GTK_WIDGET (priv->connect_button),
                              FALSE);
    button = g_strdup_printf (_("Connecting"));
    label = g_strdup_printf ("%s - %s",
                             name,
                             _("Configuring"));
  }
  else if (priv->state == IDLE)
  {
    gtk_widget_set_sensitive (GTK_WIDGET (priv->connect_button),
                              TRUE);
    gtk_widget_set_no_show_all (GTK_WIDGET (priv->delete_button),
                                TRUE);
    gtk_widget_hide (GTK_WIDGET (priv->delete_button));
    gtk_widget_set_sensitive (GTK_WIDGET (priv->delete_button),
                              FALSE);
    button = g_strdup (_("Connect"));
    label = g_strdup (name);
  }
  else if (priv->state == FAIL)
  {
    gtk_widget_set_sensitive (GTK_WIDGET (priv->connect_button),
                              TRUE);
    gtk_widget_set_no_show_all (GTK_WIDGET (priv->delete_button),
                                TRUE);
    gtk_widget_hide (GTK_WIDGET (priv->delete_button));
    gtk_widget_set_sensitive (GTK_WIDGET (priv->delete_button),
                              FALSE);
    button = g_strdup (_("Connect"));
    label = g_strdup_printf ("%s - %s",
                             name,
                             _("Connection failed"));
    priv->failed = TRUE;
  }

  if (security && security[0] != '\0' && g_strcmp0 ("none", security) != 0)
  {
      if (g_strcmp0 ("rsn", security) == 0)
      {
	  g_free (security);
	  security = g_strdup ("WPA2");
      }
      else
      {
	  gint i;

	  for (i = 0; security[i] != '\0'; i++)
          {
	      security[i] = g_ascii_toupper (security[i]);
          }
      }
      security_label = g_strdup_printf (_("%s encrypted"),
                                        security);
      gtk_label_set_text (GTK_LABEL (priv->security_label),
                          security_label);
    }

  gtk_label_set_text (GTK_LABEL (priv->name_label),
                      label);
  gtk_button_set_label (GTK_BUTTON (priv->connect_button),
                        button);

  pixbuf = carrick_icon_factory_get_pixbuf_for_service (priv->icon_factory,
                                                        service);
  gtk_image_set_from_pixbuf (GTK_IMAGE (priv->icon),
                             pixbuf);

  g_free (name);
  g_free (security);
  g_free (security_label);
  g_free (label);
  g_free (button);
}

void
_show_pass_toggled_cb (GtkToggleButton *button,
                       gpointer         user_data)
{
  GtkEntry *entry = GTK_ENTRY (user_data);
  gboolean vis = gtk_toggle_button_get_active (button);
  gtk_entry_set_visibility (entry,
                            vis);
}

gchar *
_request_passphrase (CarrickServiceItem *item)
{
  GtkWidget *dialog;
  GtkWidget *label;
  GtkWidget *title;
  GtkWidget *icon;
  GtkWidget *entry;
  GtkWidget *table;
  GtkWidget *checkbox;
  GtkWidget *tmp;
  gchar *passphrase;

  dialog = gtk_dialog_new_with_buttons (_("Passphrase required"),
                                        NULL,
                                        GTK_DIALOG_MODAL |
                                        GTK_DIALOG_DESTROY_WITH_PARENT,
                                        GTK_STOCK_CANCEL,
                                        GTK_RESPONSE_REJECT,
                                        GTK_STOCK_CONNECT,
                                        GTK_RESPONSE_ACCEPT,
                                        NULL);

  gtk_dialog_set_has_separator (GTK_DIALOG (dialog),
                                FALSE);
  gtk_dialog_set_default_response (GTK_DIALOG (dialog),
                                   GTK_RESPONSE_ACCEPT);
  gtk_container_set_border_width (GTK_CONTAINER (dialog), 5);
  gtk_window_set_icon_name (GTK_WINDOW (dialog),
                            GTK_STOCK_NETWORK);
  gtk_box_set_spacing (GTK_BOX (GTK_DIALOG (dialog)->vbox),
                       6);

  table = gtk_table_new (3, 3, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table),
                              6);
  gtk_table_set_row_spacings (GTK_TABLE (table),
                              6);
  title = gtk_label_new ("");
  gtk_label_set_markup (GTK_LABEL (title),
                        _("<b><big>Network Requires a Password</big></b>"));
  gtk_misc_set_alignment (GTK_MISC (title),
                          0.00,
                          0.50);
  gtk_table_attach (GTK_TABLE (table),
                    title,
                    1, 2,
                    0, 1,
                    GTK_EXPAND | GTK_FILL,
                    GTK_EXPAND | GTK_FILL,
                    0, 0);
  icon = gtk_image_new_from_icon_name (GTK_STOCK_NETWORK,
                                       GTK_ICON_SIZE_DIALOG);
  gtk_table_attach (GTK_TABLE (table),
                    icon,
                    0, 1,
                    0, 1,
                    GTK_FILL,
                    GTK_EXPAND | GTK_FILL,
                    0, 0);
  label = gtk_label_new (_("Please enter the password for this network:"));
  gtk_table_attach (GTK_TABLE (table),
                    label,
                    1, 2,
                    1, 2,
                    GTK_EXPAND | GTK_FILL,
                    GTK_EXPAND | GTK_FILL,
                    0, 0);
  gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
  entry = gtk_entry_new ();
  gtk_entry_set_visibility (GTK_ENTRY (entry),
                            FALSE);
  gtk_table_attach (GTK_TABLE (table),
                    entry,
                    1, 2,
                    2, 3,
                    GTK_EXPAND | GTK_FILL,
                    GTK_EXPAND | GTK_FILL,
                    0, 0);
  checkbox = gtk_check_button_new_with_label (_("Show password"));
  g_signal_connect (checkbox,
                    "toggled",
                    G_CALLBACK (_show_pass_toggled_cb),
                    (gpointer) entry);
  gtk_table_attach (GTK_TABLE (table),
                    checkbox,
                    1, 2,
                    3, 4,
                    GTK_FILL,
                    GTK_FILL,
                    0, 0);

  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox),
                      table,
                      FALSE,
                      FALSE,
                      6);

  gtk_widget_show_all (dialog);
  tmp = service_item_find_plug (GTK_WIDGET (item));
  if (tmp)
  {
    gtk_widget_hide (tmp);
  }

  if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT)
  {
    passphrase = g_strdup (gtk_entry_get_text (GTK_ENTRY (entry)));
  }
  else
  {
    passphrase = g_strdup ("");
  }
  gtk_widget_destroy (dialog);
  return passphrase;
}

void
_delete_button_cb (GtkButton *delete_button,
                   gpointer   user_data)
{
  GtkWidget *dialog;
  GtkWidget *label;
  gchar *label_text = NULL;
  const gchar *ssid = NULL;
  const gchar *type = NULL;
  CmService *service = CM_SERVICE (user_data);

  ssid = cm_service_get_name (service);
  type = cm_service_get_type (service);

  dialog = gtk_dialog_new_with_buttons (_("Really remove?"),
                                        NULL,
                                        GTK_DIALOG_MODAL |
                                        GTK_DIALOG_DESTROY_WITH_PARENT,
                                        GTK_STOCK_CANCEL,
                                        GTK_RESPONSE_REJECT,
                                        GTK_STOCK_OK,
                                        GTK_RESPONSE_ACCEPT,
                                        NULL);

  gtk_dialog_set_has_separator (GTK_DIALOG (dialog),
                                FALSE);
  gtk_dialog_set_default_response (GTK_DIALOG (dialog),
                                   GTK_RESPONSE_ACCEPT);
  gtk_window_set_icon_name (GTK_WINDOW(dialog),
                            GTK_STOCK_DELETE);

  label_text = g_strdup_printf (_("Do you want to remove the %s %s network? "
                                  "This\nwill forget the password and you will"
                                  " no longer be\nautomatically connected to "
                                  "%s."),
                                ssid,
                                type,
                                ssid);
  label = gtk_label_new (label_text);

  gtk_box_set_spacing (GTK_BOX (GTK_DIALOG (dialog)->vbox),
                       12);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox),
                      label,
                      TRUE,
                      TRUE,
                      6);
  gtk_widget_show_all (dialog);

  if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT)
    cm_service_remove (service);
}

void
_connect_button_cb (GtkButton          *connect_button,
                    CarrickServiceItem *item)
{
  CarrickServiceItemPrivate *priv = SERVICE_ITEM_PRIVATE (item);
  gchar *passphrase = NULL;

  if (priv->state == READY)
  {
    cm_service_disconnect (priv->service);
  }
  else
  {
    const gchar *security = g_strdup (cm_service_get_security (priv->service));

    if (security && g_strcmp0 ("none", security) != 0)
    {
      if (priv->failed || !cm_service_get_passphrase (priv->service))
      {
        passphrase = _request_passphrase (item);
        if (passphrase && passphrase[0] != '\0')
        {
          cm_service_set_passphrase (priv->service,
                                     passphrase);
          cm_service_connect (CM_SERVICE (priv->service));
          g_free (passphrase);
        }
        else
        {
          /* passphrase empty, return */
          g_free (passphrase);
        }
      }
      else
      {
        /* We have the passphrase already, just connect */
        cm_service_connect (priv->service);
      }
    }
    else
    {
      /* No security, just connect */
      cm_service_connect (priv->service);
    }
  }
  /* Make sure widget is displaying correct state */
  _set_state (priv->service,
              item);
}

ServiceItemState
_get_service_state (CmService *service)
{
  const gchar *state = NULL;

  state = cm_service_get_state (service);
  if (g_strcmp0 (state, "idle") == 0)
  {
    return IDLE;
  }
  else if (g_strcmp0 (state, "failure") == 0)
  {
    return FAIL;
  }
  else if (g_strcmp0 (state, "association") == 0
           || g_strcmp0 (state, "configuration") == 0)
  {
    return CONFIGURE;
  }
  else if (g_strcmp0 (state, "ready") == 0)
  {
    return READY;
  }

  return UNKNOWN;
}

void
carrick_service_item_set_service (CarrickServiceItem *service_item,
                                  CmService          *service)
{
  CarrickServiceItemPrivate *priv = SERVICE_ITEM_PRIVATE (service_item);

  if (priv->service)
  {
    g_object_unref (priv->service);
    priv->service = NULL;
  }

  if (service)
  {
    priv->service = g_object_ref (service);

    priv->state = _get_service_state (service);

    _set_state (service,
                service_item);

    g_signal_connect (priv->connect_button,
                      "clicked",
                      G_CALLBACK (_connect_button_cb),
                      service_item);

    g_signal_connect (priv->delete_button,
                      "clicked",
                      G_CALLBACK (_delete_button_cb),
                      service);
  }
}

CmService *
carrick_service_item_get_service (CarrickServiceItem *item)
{
  CarrickServiceItemPrivate *priv = SERVICE_ITEM_PRIVATE (item);
  return priv->service;
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
  CarrickServiceItemPrivate *priv = SERVICE_ITEM_PRIVATE (object);

  if (priv->service)
  {
    g_object_unref (priv->service);
    priv->service = NULL;
  }

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
  GtkWidget *box, *hbox, *vbox;
  GtkWidget *image;

  priv->service = NULL;
  priv->failed = FALSE;

  box = gtk_hbox_new (FALSE,
                      6);
  priv->expando = nbtk_gtk_expander_new ();
  gtk_container_add (GTK_CONTAINER (self),
                     priv->expando);
  nbtk_gtk_expander_set_label_widget (NBTK_GTK_EXPANDER (priv->expando),
                                      box);
  nbtk_gtk_expander_set_has_indicator (NBTK_GTK_EXPANDER (priv->expando),
                                       FALSE);

  priv->icon = gtk_image_new ();
  gtk_box_pack_start (GTK_BOX (box),
                      priv->icon,
                      FALSE,
                      FALSE,
                      6);

  vbox = gtk_vbox_new (FALSE,
                       6);
  gtk_box_pack_start (GTK_BOX (box),
                      vbox,
                      TRUE,
                      TRUE,
                      6);

  hbox = gtk_hbox_new (FALSE,
                       6);
  gtk_box_pack_start (GTK_BOX (vbox),
                      hbox,
                      TRUE,
                      TRUE,
                      6);

  priv->name_label = gtk_label_new ("");
  gtk_misc_set_alignment (GTK_MISC (priv->name_label),
                          0.05, 0.5);
  gtk_box_pack_start (GTK_BOX (hbox),
                      priv->name_label,
                      TRUE,
                      TRUE,
                      6);

  image = gtk_image_new_from_stock ("gtk-delete",
                                    GTK_ICON_SIZE_MENU);
  priv->delete_button = gtk_button_new ();
  gtk_button_set_image (GTK_BUTTON (priv->delete_button),
                        image);
  gtk_box_pack_start (GTK_BOX (hbox),
                      priv->delete_button,
                      FALSE,
                      FALSE,
                      6);

  hbox = gtk_hbox_new (FALSE,
                       6);
  gtk_box_pack_start (GTK_BOX (vbox),
                      hbox,
                      TRUE,
                      TRUE,
                      6);

  priv->security_label = gtk_label_new ("");
  gtk_box_pack_start (GTK_BOX (hbox),
                      priv->security_label,
                      FALSE,
                      FALSE,
                      6);

  priv->connect_button = gtk_button_new ();
  gtk_box_pack_start (GTK_BOX (hbox),
                      priv->connect_button,
                      FALSE,
                      FALSE,
                      6);

  /*priv->default_button = gtk_button_new ();
  gtk_button_set_label (GTK_BUTTON (priv->default_button),
                        _("Make default connection"));
  gtk_widget_set_sensitive (GTK_WIDGET (priv->default_button),
                            FALSE);
  gtk_box_pack_start (GTK_BOX (hbox),
                      priv->default_button,
                      FALSE,
                      FALSE,
                      6);*/

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
