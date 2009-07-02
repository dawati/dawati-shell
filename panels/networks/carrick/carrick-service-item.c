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
  READY,
  DISCONNECT,
} ServiceItemState;

struct _CarrickServiceItemPrivate
{
  CmService          *service;
  GtkWidget          *icon;
  GtkWidget          *name_label;
  GtkWidget          *connect_box;
  GtkWidget          *security_label;
  GtkWidget          *connect_button;
  GtkWidget          *passphrase_box;
  GtkWidget          *passphrase_entry;
  GtkWidget          *show_password_check;
  GtkWidget          *delete_button;
  GtkWidget          *expando;
  ServiceItemState    state;
  CarrickIconFactory *icon_factory;
  gboolean            failed;
  gboolean            passphrase_hint_visible;
};

enum
{
  SIGNAL_ITEM_ACTIVATE,
  SIGNAL_LAST
};

static gint service_item_signals[SIGNAL_LAST];


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

static ServiceItemState
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
  else if (g_strcmp0 (state, "disconnect") == 0)
  {
    return DISCONNECT;
  }

  return UNKNOWN;
}

static void
_service_item_set_security (CarrickServiceItem *item,
                            gchar *security)
{
  CarrickServiceItemPrivate *priv = SERVICE_ITEM_PRIVATE (item);
  gchar *security_label = NULL;

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
  }
  else
  {
    security_label = g_strdup ("");
  }

  gtk_label_set_text (GTK_LABEL (priv->security_label),
                      security_label);
}

static void
_set_state (CmService          *service,
            CarrickServiceItem *item)
{
  CarrickServiceItemPrivate *priv = SERVICE_ITEM_PRIVATE (item);
  gchar *label = NULL;
  gchar *button = NULL;
  GdkPixbuf *pixbuf = NULL;
  gchar *name = NULL;
  gchar *security = NULL;

  name = g_strdup (cm_service_get_name (service));
  security = g_strdup (cm_service_get_security (service));

  if (security && security[0] != '\0')
    _service_item_set_security (item, security);

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
  else if (priv->state == DISCONNECT)
  {
    gtk_widget_set_sensitive (GTK_WIDGET (priv->connect_button),
                              FALSE);
    button = g_strdup_printf (_("Disconnecting"));
    label = g_strdup_printf ("%s - %s",
                             name,
                             _("Disconnecting"));
  }
  else
  {
    label = g_strdup (name);
  }

  if (label && label[0] != '\0')
  {
    gtk_label_set_text (GTK_LABEL (priv->name_label),
                        label);
    pixbuf = carrick_icon_factory_get_pixbuf_for_service (priv->icon_factory,
                                                          service);
    gtk_image_set_from_pixbuf (GTK_IMAGE (priv->icon),
                               pixbuf);
  }

  if (button && button[0] != '\0')
    gtk_button_set_label (GTK_BUTTON (priv->connect_button),
                          button);

  g_free (name);
  g_free (security);
  g_free (label);
  g_free (button);
}

static void
_show_pass_toggled_cb (GtkToggleButton    *button,
                       CarrickServiceItem *item)
{
  CarrickServiceItemPrivate *priv = SERVICE_ITEM_PRIVATE (item);

  if (!priv->passphrase_hint_visible)
  {
    gboolean vis = gtk_toggle_button_get_active (button);
    gtk_entry_set_visibility (GTK_ENTRY (priv->passphrase_entry),
                              vis);
  }
}

static void
_request_passphrase (CarrickServiceItem *item)
{
  CarrickServiceItemPrivate *priv = SERVICE_ITEM_PRIVATE (item);

  /* TRANSLATORS: text should be 20 characters or less to be entirely
   * visible in the passphrase entry */
  gtk_entry_set_text (GTK_ENTRY (priv->passphrase_entry), 
                      _("Type password here"));
  gtk_entry_set_visibility (GTK_ENTRY (priv->passphrase_entry), TRUE);
  priv->passphrase_hint_visible = TRUE;
  gtk_editable_select_region (GTK_EDITABLE (priv->passphrase_entry),
                              0, -1);
  gtk_widget_grab_focus (priv->passphrase_entry);
  gtk_widget_hide (priv->connect_box);
  gtk_widget_show (priv->passphrase_box);
}

/*static void
_item_service_updated_cb (CmService *service,
                          gpointer user_data)
{
  CarrickServiceItem *item = CARRICK_SERVICE_ITEM (user_data);
  CarrickServiceItemPrivate *priv = SERVICE_ITEM_PRIVATE (item);

  priv->state = _get_service_state (service);
  _set_state (service,
              item);
              }*/

static void
_service_name_changed_cb (CmService *service,
                          gchar     *name,
                          gpointer   user_data)
{
  _set_state (service,
              CARRICK_SERVICE_ITEM (user_data));
}

static void
_service_state_changed_cb (CmService *service,
                           gchar     *state,
                           gpointer   user_data)
{
  CarrickServiceItem *item = CARRICK_SERVICE_ITEM (user_data);
  CarrickServiceItemPrivate *priv = SERVICE_ITEM_PRIVATE (item);

  priv->state = _get_service_state (service);

  _set_state (service,
              item);
}

static void
_service_security_changed_cb (CmService *service,
                              gchar     *security,
                              gpointer   user_data)
{
  CarrickServiceItem *item = CARRICK_SERVICE_ITEM (user_data);
  _service_item_set_security (item, security);
}

static void
_service_strength_changed_cb (CmService *service,
                              guint      strength,
                              gpointer   user_data)
{
  _set_state (service,
              CARRICK_SERVICE_ITEM (user_data));
}

static void
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
  gtk_container_set_border_width (GTK_CONTAINER (dialog), 5);
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

  gtk_widget_destroy (dialog);
}

static void
_connect_button_cb (GtkButton          *connect_button,
                    CarrickServiceItem *item)
{
  CarrickServiceItemPrivate *priv = SERVICE_ITEM_PRIVATE (item);

  g_signal_emit (item, service_item_signals[SIGNAL_ITEM_ACTIVATE], 0);

  if (cm_service_get_connected (priv->service))
  {
    cm_service_disconnect (priv->service);
    _set_state (priv->service, item);
  }
  else
  {
    gchar *security = g_strdup (cm_service_get_security (priv->service));

    if (security && g_strcmp0 ("none", security) != 0)
    {
      if (priv->failed || !cm_service_get_passphrase (priv->service))
      {
        _request_passphrase (item);
      }
      else
      {
        /* We have the passphrase already, just connect */
        cm_service_connect (priv->service);
        _set_state (priv->service, item);
      }
    }
    else
    {
      /* No security, just connect */
      cm_service_connect (priv->service);
      _set_state (priv->service, item);
    }

    g_free (security);
  }
}

static void
_connect_with_password (CarrickServiceItem *item)
{
  CarrickServiceItemPrivate *priv = SERVICE_ITEM_PRIVATE (item);
  const char *passphrase;

  if (priv->passphrase_hint_visible)
  {
    passphrase = "";
  } else {
    passphrase = gtk_entry_get_text (GTK_ENTRY (priv->passphrase_entry));
  }

  cm_service_set_passphrase (priv->service,
                             passphrase);
  cm_service_connect (CM_SERVICE (priv->service));

  _set_state (priv->service, item);

  gtk_widget_hide (priv->passphrase_box);
  gtk_widget_show (priv->connect_box);
}

static void
_connect_with_pw_clicked_cb (GtkButton *btn, CarrickServiceItem *item)
{
  _connect_with_password (item);
}

static void
_passphrase_entry_activated_cb (GtkEntry *entry, CarrickServiceItem *item)
{
  _connect_with_password (item);
}

static void
_entry_changed_cb (GtkEntry *pw_entry, CarrickServiceItem *item)
{
  CarrickServiceItemPrivate *priv = SERVICE_ITEM_PRIVATE (item);

  if (priv->passphrase_hint_visible)
  {
    gboolean visible = gtk_toggle_button_get_active (
      GTK_TOGGLE_BUTTON (priv->show_password_check));

    gtk_entry_set_visibility (pw_entry, visible);
    priv->passphrase_hint_visible = FALSE;
  }
}

static void
_passphrase_entry_clear_released_cb (GtkEntry             *entry,
                                     GtkEntryIconPosition  icon_pos,
                                     GdkEvent             *event,
                                     gpointer              user_data)
{
  gtk_entry_set_text (entry, "");
  gtk_widget_grab_focus (GTK_WIDGET (entry));
}

void
carrick_service_item_set_service (CarrickServiceItem *service_item,
                                  CmService          *service)
{
  CarrickServiceItemPrivate *priv;

  g_return_if_fail (CARRICK_IS_SERVICE_ITEM (service_item));
  g_return_if_fail (service == NULL || CM_IS_SERVICE (service));

  priv = SERVICE_ITEM_PRIVATE (service_item);

  if (priv->service)
  {
    /*g_signal_handlers_disconnect_by_func (priv->service,
                                          _item_service_updated_cb,
                                          service_item);*/
    g_signal_handlers_disconnect_by_func (priv->service,
                                         _service_name_changed_cb,
                                         service_item);
    g_signal_handlers_disconnect_by_func (priv->service,
                                          _service_state_changed_cb,
                                          service_item);
    g_signal_handlers_disconnect_by_func (priv->service,
                                          _service_security_changed_cb,
                                          service_item);
    g_signal_handlers_disconnect_by_func (priv->service,
                                          _service_strength_changed_cb,
                                          service_item);
    g_object_unref (priv->service);
    priv->service = NULL;
  }

  if (service)
  {
    priv->service = g_object_ref (service);

    priv->state = _get_service_state (service);

    g_signal_connect (priv->connect_button,
                      "clicked",
                      G_CALLBACK (_connect_button_cb),
                      service_item);

    g_signal_connect (priv->delete_button,
                      "clicked",
                      G_CALLBACK (_delete_button_cb),
                      service);

    /*g_signal_connect (service,
                      "service-updated",
                      G_CALLBACK (_item_service_updated_cb),
                      service_item);*/

    g_signal_connect (service,
                      "name-changed",
                      G_CALLBACK (_service_name_changed_cb),
                      service_item);

    g_signal_connect (service,
                      "state-changed",
                      G_CALLBACK (_service_state_changed_cb),
                      service_item);

    g_signal_connect (service,
                      "security-changed",
                      G_CALLBACK (_service_security_changed_cb),
                      service_item);

    g_signal_connect (service,
                      "strength-changed",
                      G_CALLBACK (_service_strength_changed_cb),
                      service_item);

    _set_state (service,
                service_item);
  }
}

gint
carrick_service_item_get_order (CarrickServiceItem *item)
{
  CarrickServiceItemPrivate *priv;

  g_return_val_if_fail (CARRICK_IS_SERVICE_ITEM (item), -1);

  priv = SERVICE_ITEM_PRIVATE (item);

  return cm_service_get_order (priv->service);
}

CmService *
carrick_service_item_get_service (CarrickServiceItem *item)
{
  CarrickServiceItemPrivate *priv;

  g_return_val_if_fail (CARRICK_IS_SERVICE_ITEM (item), NULL);

  priv = SERVICE_ITEM_PRIVATE (item);
  return priv->service;
}

void carrick_service_item_set_active (CarrickServiceItem *item,
                                      gboolean active)
{
  g_return_if_fail (CARRICK_IS_SERVICE_ITEM (item));

  if (!active)
  {
    CarrickServiceItemPrivate *priv = SERVICE_ITEM_PRIVATE (item);

    gtk_widget_hide (priv->passphrase_box);
    gtk_widget_show (priv->connect_box);
  }
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

  /* activated == some ui activity in the label part of the item */
  service_item_signals[SIGNAL_ITEM_ACTIVATE] = g_signal_new (
    "activate",
    G_TYPE_FROM_CLASS (object_class),
    G_SIGNAL_RUN_LAST,
    0,
    NULL, NULL,
    g_cclosure_marshal_VOID__VOID,
    G_TYPE_NONE, 0);

}

static void
carrick_service_item_init (CarrickServiceItem *self)
{
  CarrickServiceItemPrivate *priv = SERVICE_ITEM_PRIVATE (self);
  GtkWidget *box, *hbox, *vbox;
  GtkWidget *image;
  GtkWidget *connect_with_pw_button;

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
                          0.0, 0.5);
  gtk_box_pack_start (GTK_BOX (hbox),
                      priv->name_label,
                      TRUE,
                      TRUE,
                      6);

  image = gtk_image_new_from_icon_name ("edit-clear",
                                        GTK_ICON_SIZE_MENU);
  priv->delete_button = gtk_button_new ();
  gtk_button_set_image (GTK_BUTTON (priv->delete_button),
                        image);
  gtk_box_pack_end (GTK_BOX (box),
                      priv->delete_button,
                      FALSE,
                      FALSE,
                      6);

  priv->connect_box = gtk_hbox_new (FALSE,
                       6);
  gtk_box_pack_start (GTK_BOX (vbox),
                      priv->connect_box,
                      TRUE,
                      TRUE,
                      6);

  priv->security_label = gtk_label_new ("");
  gtk_misc_set_alignment (GTK_MISC (priv->security_label),
                          0.0, 0.5);
  gtk_box_pack_start (GTK_BOX (priv->connect_box),
                      priv->security_label,
                      FALSE,
                      FALSE,
                      6);

  priv->connect_button = gtk_button_new ();
  gtk_box_pack_start (GTK_BOX (priv->connect_box),
                      priv->connect_button,
                      FALSE,
                      FALSE,
                      6);

  /*priv->default_button = gtk_button_new ();
  gtk_button_set_label (GTK_BUTTON (priv->default_button),
                        _("Make default connection"));
  gtk_widget_set_sensitive (GTK_WIDGET (priv->default_button),
                            FALSE);
  gtk_box_pack_start (GTK_BOX (priv->connect_box),
                      priv->default_button,
                      FALSE,
                      FALSE,
                      6);*/

  priv->passphrase_box = gtk_hbox_new (FALSE, 6);
  gtk_widget_set_no_show_all (priv->passphrase_box, TRUE);
  gtk_box_pack_start (GTK_BOX (vbox), priv->passphrase_box, 
                      TRUE, TRUE, 6);

  priv->passphrase_entry = gtk_entry_new ();
  gtk_entry_set_width_chars (GTK_ENTRY (priv->passphrase_entry), 20);
  gtk_widget_show (priv->passphrase_entry);
  g_signal_connect (priv->passphrase_entry,
                    "activate",
                    G_CALLBACK (_passphrase_entry_activated_cb),
                    self);
  gtk_entry_set_icon_from_stock (GTK_ENTRY (priv->passphrase_entry),
                                 GTK_ENTRY_ICON_SECONDARY,
                                 GTK_STOCK_CLEAR);
  g_signal_connect (priv->passphrase_entry,
                    "icon-release",
                    G_CALLBACK (_passphrase_entry_clear_released_cb),
                    NULL);
  gtk_box_pack_start (GTK_BOX (priv->passphrase_box),
                      priv->passphrase_entry, 
                      FALSE, FALSE, 6);

  connect_with_pw_button = gtk_button_new_with_label (_("Connect"));
  gtk_widget_show (connect_with_pw_button);
  g_signal_connect (connect_with_pw_button,
                    "clicked",
                    G_CALLBACK (_connect_with_pw_clicked_cb),
                    self);
  gtk_box_pack_start (GTK_BOX (priv->passphrase_box), 
                      connect_with_pw_button,
                      FALSE, FALSE, 6);

  priv->show_password_check = 
    gtk_check_button_new_with_label (_("Show password"));
  gtk_widget_show (priv->show_password_check);
  g_signal_connect (priv->show_password_check, 
                    "toggled",
                    G_CALLBACK (_show_pass_toggled_cb),
                    self);
  gtk_box_pack_start (GTK_BOX (priv->passphrase_box), 
                      priv->show_password_check,
                      FALSE, FALSE, 6);

  g_signal_connect (priv->passphrase_entry, 
                    "changed",
                    G_CALLBACK (_entry_changed_cb), 
                    self);
  

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
