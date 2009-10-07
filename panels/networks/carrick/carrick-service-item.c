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
#include <nbtk/nbtk-gtk.h>

#include "connman-service-bindings.h"

#include "carrick-icon-factory.h"
#include "carrick-notification-manager.h"

G_DEFINE_TYPE (CarrickServiceItem, carrick_service_item, GTK_TYPE_EVENT_BOX)

#define SERVICE_ITEM_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), CARRICK_TYPE_SERVICE_ITEM, CarrickServiceItemPrivate))

/* Translators:
   This is the title seen on the top of the connection wizard
   window forconfiguring a 3G  data service.
*/
#define DIALOG_TITLE _("Cellular Data Connection Wizard")

/* Translators:
   In each of the 3g data connection wizard dialog boxes that ask the user
   to select an option, we always provide a button that allows the user
   to manually enter the low level settings for their device.  This is the
   text used in that button.
*/
#define MANUAL_CONFIG _("Manual Config")

/* Translators:
   When configuring a device for 3G roaming, we must ask the user for
   the country that their services is associated with.  This is the
   explaination text at the top of the dialog box for selecting the country.
*/
#define SELECT_COUNTRY_MSG _("Select Country")

/* Translators:
   When configuring a device for 3G roaming, we must ask the user for
   their service provider. This is the explaination text at the that
   dialog box
*/
#define SELECT_PROVIDER_MSG _("Select Provider")

/* Translators:
   When a user first tries to connect to a given provider, they
   must select which plan (i.e. pre-paid, unlimited, etc) to use
   that the associated service provider allows.
   This is the explaination text used at the top of dialog box.
*/
#define SELECT_PROVIDER_PLAN _("Select your 3G Data Plan")

/* Translators:
   This is the text we add to the top of the dialog for allowing the user to
   manually enter configuration settings. This is a single line of nor more
   then 60 chars wide
*/
#define MANUAL_CONFIG_MSG _("Manual Configuration")

/* Translators:
   The following strings are used as labels for each of the
   text entry fields that the user uses to manually enter configuration
   settings.  Each label is a single line of no more then 40 chars:

   "Plan Name":
   This is a uniqe string that the service provider
   uses to identify a give data plan.  This is the only
   setting that is required.

   "Username":
   This is the userame of a username/password combination
   required for some access points.

   "Password":
   Password required by some access points.

   "Gateway":
   Gateway IP Address required by some access points

   "Primary DNS":
   Primary domain name server address required by some access points

   "Secondary DNS":
   Backup domain name server address required by some access points

   "Tertairy DNS":
   Alternate backup domain name server address required by some
   access points
*/
#define PNAME_LABEL _("Plan Name: (required)")
#define UNAME_LABEL _("Username:")
#define PW_LABEL    _("Password:")
#define GW_LABEL    _("Gateway:")
#define DNS1_LABEL  _("Primary DNS:")
#define DNS2_LABEL  _("Secondary DNS:")
#define DNS3_LABEL  _("Tertairy DNS:")

/* Translators:
   If this utility is executed without any arguments specifying which
   service to configure, then a dialog is used to allow the user to pick
   from a list of valid services.  This is the explaination text shown
   at the top of the dialog
*/
#define SELECT_SERVICE_MSG _("Select 3G Service")

/* Translators:
   The following are potential errors that a user might see while
   attempting to configure a 3G data service.
*/
#define INVALID_COUNTRY_CODE _("Critical Error: Invalid country code")
#define MISSING_APN_NAME _("Missing required APN (service plan name)")
#define INVALID_NETWORK_SERVICE _("Internal Error: Invalid network service")
#define INVALID_CONNMAN _("Critical Error: Unable to access Connection Manager")
#define MISSING_3G_HW _("ERROR: No 3G Hardware detected")

enum {
  PROP_0,
  PROP_DRAGGABLE,
  PROP_ICON_FACTORY,
  PROP_NOTIFICATIONS,
  PROP_MODEL,
  PROP_ROW
};

struct _CarrickServiceItemPrivate
{
  GtkWidget *icon;
  GtkWidget *name_label;
  GtkWidget *connect_box;
  GtkWidget *security_label;
  GtkWidget *connect_button;
  GtkWidget *passphrase_box;
  GtkWidget *passphrase_entry;
  GtkWidget *show_password_check;
  GtkWidget *delete_button;
  GtkWidget *expando;
  CarrickIconFactory *icon_factory;
  gboolean failed;
  gboolean passphrase_hint_visible;

  gboolean hover;
  GdkColor prelight_color;
  GdkColor active_color;

  CarrickNotificationManager *note;

  gboolean draggable;
  GdkCursor *hand_cursor;

  CarrickNetworkModel *model;
  GtkTreeRowReference *row;

  DBusGProxy *proxy;
  guint index;
  gchar *name;
  gchar *type;
  gchar *state;
  guint  strength;
  gchar   *security;
  gboolean need_pass;
  gchar *passphrase;
  gboolean setup_required;

  GtkWidget *info_bar;
  GtkWidget *info_label;
};

enum {
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
    case PROP_DRAGGABLE:
      g_value_set_boolean (value, priv->draggable);
      break;

    case PROP_ICON_FACTORY:
      g_value_set_object (value, priv->icon_factory);
      break;

    case PROP_NOTIFICATIONS:
      g_value_set_object (value, priv->note);

    case PROP_MODEL:
      g_value_set_object (value, priv->model);
      break;

    case PROP_ROW:
      g_value_set_boxed (value, priv->row);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
_service_item_set_security (CarrickServiceItem *item)
{
  CarrickServiceItemPrivate *priv = item->priv;
  gchar                     *security = g_strdup (priv->security);
  gchar                     *security_label = NULL;

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
      /* TRANSLATORS: this is a wireless security method, at least WEP,
       *  WPA and WPA2 are possible token values. Example: "WEP encrypted".
       */
      security_label = g_strdup_printf (_ ("%s encrypted"),
                                        security);
    }
  else
    {
      security_label = g_strdup ("");
    }

  /* just in case security_sample has not been translated in _init ... */
  if (strlen (security_label) >
      gtk_label_get_width_chars (GTK_LABEL (priv->security_label)))
    {
      gtk_label_set_width_chars (GTK_LABEL (priv->security_label), -1);
    }

  gtk_label_set_text (GTK_LABEL (priv->security_label),
                      security_label);
  g_free (security_label);
  g_free (security);
}

static void
_populate_variables (CarrickServiceItem *self)
{
  CarrickServiceItemPrivate *priv = self->priv;
  GtkTreeIter                iter;

  g_return_if_fail (priv->model != NULL);
  g_return_if_fail (CARRICK_IS_NETWORK_MODEL (priv->model));

  if (gtk_tree_model_get_iter (GTK_TREE_MODEL (priv->model),
                               &iter,
                               gtk_tree_row_reference_get_path (priv->row)))
    {
      gtk_tree_model_get (GTK_TREE_MODEL (priv->model), &iter,
                          CARRICK_COLUMN_PROXY, &priv->proxy,
                          CARRICK_COLUMN_INDEX, &priv->index,
                          CARRICK_COLUMN_NAME, &priv->name,
                          CARRICK_COLUMN_TYPE, &priv->type,
                          CARRICK_COLUMN_STATE, &priv->state,
                          CARRICK_COLUMN_STRENGTH, &priv->strength,
                          CARRICK_COLUMN_SECURITY, &priv->security,
                          CARRICK_COLUMN_PASSPHRASE_REQUIRED, &priv->need_pass,
                          CARRICK_COLUMN_PASSPHRASE, &priv->passphrase,
                          CARRICK_COLUMN_SETUP_REQUIRED, &priv->setup_required,
                          -1);
    }
}

static void
_set_state (CarrickServiceItem *self)
{
  CarrickServiceItemPrivate *priv = self->priv;
  gchar                     *label = NULL;
  gchar                     *button = NULL;
  GdkPixbuf                 *pixbuf = NULL;
  gchar                     *name = g_strdup (priv->name);

  _service_item_set_security (self);

  if (g_strcmp0 ("ethernet", priv->type) == 0)
    {
      g_free (name);
      name = g_strdup (_ ("Wired"));
    }

  if (priv->hover)
    {
      gtk_widget_modify_bg (priv->expando,
                            GTK_STATE_NORMAL,
                            &priv->prelight_color);
    }
  else if (g_strcmp0 (priv->state, "ready") == 0)
    {
      gtk_widget_modify_bg (priv->expando,
                            GTK_STATE_NORMAL,
                            &priv->active_color);
    }
  else
    {
      gtk_widget_modify_bg (priv->expando,
                            GTK_STATE_NORMAL,
                            NULL);
    }

  if (g_strcmp0 (priv->state, "ready") == 0)
    {
      if (g_strcmp0 (priv->type, "ethernet") != 0)
        {
          /* Only expose delete button for non-ethernet devices */
          gtk_widget_show (GTK_WIDGET (priv->delete_button));
          gtk_widget_set_sensitive (GTK_WIDGET (priv->delete_button),
                                    TRUE);
        }
      button = g_strdup (_ ("Disconnect"));
      label = g_strdup_printf ("%s - %s",
                               name,
                               _ ("Connected"));
      priv->failed = FALSE;
      gtk_label_set_text (GTK_LABEL (priv->info_label),
                          "");
    }
  else if (g_strcmp0 (priv->state, "configuration") == 0)
    {
      button = g_strdup_printf (_ ("Cancel"));
      label = g_strdup_printf ("%s - %s",
                               name,
                               _ ("Configuring"));
      gtk_label_set_text (GTK_LABEL (priv->info_label),
                          "");
    }
  else if (g_strcmp0 (priv->state, "association") == 0)
    {
      button = g_strdup_printf (_("Cancel"));
      label = g_strdup_printf ("%s - %s",
                               name,
                               _("Associating"));
      gtk_label_set_text (GTK_LABEL (priv->info_label),
                          "");
    }
  else if (g_strcmp0 (priv->state, "idle") == 0)
    {
      gtk_widget_hide (GTK_WIDGET (priv->delete_button));
      gtk_widget_set_sensitive (GTK_WIDGET (priv->delete_button),
                                FALSE);
      button = g_strdup (_ ("Connect"));
      label = g_strdup (name);
      gtk_label_set_text (GTK_LABEL (priv->info_label),
                          "");
    }
  else if (g_strcmp0 (priv->state, "failure") == 0)
    {
      /*
       * If the connection failed we should allow the user
       * to remove the connection (and forget the password.
       * Except, of course, when the connection is Ethernet ...
       */
      if (g_strcmp0 (priv->type, "ethernet") == 0)
        {
          gtk_widget_hide (GTK_WIDGET (priv->delete_button));
          gtk_widget_set_sensitive (GTK_WIDGET (priv->delete_button),
                                    FALSE);
        }
      else
        {
          gtk_widget_show (GTK_WIDGET (priv->delete_button));
          gtk_widget_set_sensitive (GTK_WIDGET (priv->delete_button),
                                    TRUE);
        }

      button = g_strdup (_ ("Connect"));
      label = g_strdup_printf ("%s - %s",
                               name,
                               _ ("Connection failed"));
      priv->failed = TRUE;
      gtk_label_set_text (GTK_LABEL (priv->info_label),
                          _("Sorry, the connection failed. You could try again."));
      gtk_info_bar_set_message_type (GTK_INFO_BAR (priv->info_bar),
                                     GTK_MESSAGE_ERROR);
    }
  else if (g_strcmp0 (priv->state, "disconnect") == 0)
    {
      button = g_strdup_printf (_ ("Disconnecting"));
      label = g_strdup_printf ("%s - %s",
                               name,
                               _ ("Disconnecting"));
      gtk_label_set_text (GTK_LABEL (priv->info_label),
                          "");
    }
  else
    {
      label = g_strdup (name);
    }

  if (label && label[0] != '\0')
    {
      gtk_label_set_text (GTK_LABEL (priv->name_label),
                          label);
      pixbuf = carrick_icon_factory_get_pixbuf_for_state (
                  priv->icon_factory,
                  carrick_icon_factory_get_state (self->priv->type,
                                                  self->priv->strength));

      gtk_image_set_from_pixbuf (GTK_IMAGE (priv->icon),
                                 pixbuf);
    }

  if (button && button[0] != '\0')
    gtk_button_set_label (GTK_BUTTON (priv->connect_button),
                          button);

  if (g_str_equal (gtk_label_get_text (GTK_LABEL (priv->info_label)), ""))
    gtk_widget_hide (priv->info_bar);
  else
    gtk_widget_show (priv->info_bar);

  g_free (name);
  g_free (label);
  g_free (button);
}

static void
_show_pass_toggled_cb (GtkToggleButton    *button,
                       CarrickServiceItem *item)
{
  CarrickServiceItemPrivate *priv = item->priv;

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
  CarrickServiceItemPrivate *priv = item->priv;

  /* TRANSLATORS: text should be 20 characters or less to be entirely
   * visible in the passphrase entry */
  gtk_entry_set_text (GTK_ENTRY (priv->passphrase_entry),
                      _ ("Type password here"));
  gtk_entry_set_visibility (GTK_ENTRY (priv->passphrase_entry), TRUE);
  priv->passphrase_hint_visible = TRUE;
  gtk_editable_select_region (GTK_EDITABLE (priv->passphrase_entry),
                              0, -1);
  gtk_widget_grab_focus (priv->passphrase_entry);
  gtk_widget_hide (priv->connect_box);
  gtk_widget_show (priv->passphrase_box);
}

/*
 * Generic call_notify function for async d-bus calls
 */
static void
dbus_proxy_notify_cb (DBusGProxy *proxy,
                      GError     *error,
                      gpointer    user_data)
{
  if (error)
    {
      g_debug ("Error when ending call: %s",
               error->message);
      g_error_free (error);
    }
}

/*
 * After the connect method calls return re-set the default timeout on the
 * proxy to something saner.
 */
static void
connect_notify_cb (DBusGProxy *proxy,
                   GError     *error,
                   gpointer    user_data)
{
  if (error)
    {
      g_debug ("Error when connecting: %s",
               error->message);
      g_error_free (error);
    }

  dbus_g_proxy_set_default_timeout (proxy,
                                    25000);
}

static void
set_passphrase_notify_cb (DBusGProxy *proxy,
                          GError     *error,
                          gpointer    user_data)
{
  CarrickServiceItem *item = CARRICK_SERVICE_ITEM (user_data);

  if (error)
    {
      g_debug ("Error when ending call: %s",
               error->message);
      g_error_free (error);
    }
  else
    {
      /* Set an unusually long timeout because the Connect
       * method doesn't return until there has been either
       * success or an error.
       */
      dbus_g_proxy_set_default_timeout (proxy,
                                        120000);
      org_moblin_connman_Service_connect_async (proxy,
                                                connect_notify_cb,
                                                item);
    }
}

static void
_delete_button_cb (GtkButton *delete_button,
                   gpointer   user_data)
{
  CarrickServiceItem        *item = CARRICK_SERVICE_ITEM (user_data);
  CarrickServiceItemPrivate *priv = item->priv;
  GtkWidget                 *dialog;
  GtkWidget                 *label;
  gchar                     *label_text = NULL;
  GValue                    *value;

  dialog = gtk_dialog_new_with_buttons (_ ("Really remove?"),
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
  gtk_window_set_icon_name (GTK_WINDOW (dialog),
                            GTK_STOCK_DELETE);

  label_text = g_strdup_printf (_ ("Do you want to remove the %s %s network? "
                                   "This\nwill forget the password and you will"
                                   " no longer be\nautomatically connected to "
                                   "%s."),
                                priv->name,
                                priv->type,
                                priv->name);
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
    {
      if (priv->failed)
        {
          /* Clear the passphrase */
          value = g_slice_new0 (GValue);
          g_value_init (value, G_TYPE_STRING);
          g_value_set_string (value, "");

          org_moblin_connman_Service_set_property_async (priv->proxy,
                                                         "Passphrase",
                                                         value,
                                                         dbus_proxy_notify_cb,
                                                         item);

          g_value_unset (value);
          g_slice_free (GValue, value);
        }
      else
        {
          carrick_notification_manager_queue_event (priv->note,
                                                    priv->type,
                                                    "idle",
                                                    priv->name);
          org_moblin_connman_Service_remove_async (priv->proxy,
                                                   dbus_proxy_notify_cb,
                                                   item);
        }
    }

  gtk_widget_destroy (dialog);
}

static void
_connect_button_cb (GtkButton          *connect_button,
                    CarrickServiceItem *item)
{
  CarrickServiceItemPrivate *priv = item->priv;

  g_signal_emit (item, service_item_signals[SIGNAL_ITEM_ACTIVATE], 0);

  if (g_str_equal (priv->state, "ready") ||
      g_str_equal (priv->state, "configuration") ||
      g_str_equal (priv->state, "association"))
    {
      carrick_notification_manager_queue_event (priv->note,
                                                priv->type,
                                                "idle",
                                                priv->name);
      org_moblin_connman_Service_disconnect_async (priv->proxy,
                                                   dbus_proxy_notify_cb,
                                                   item);
    }
  else
    {
      if (priv->security && g_str_equal (priv->security, "none") == FALSE)
        {
          if (priv->failed ||
              (priv->need_pass && priv->passphrase == NULL))
            {
              _request_passphrase (item);
            }
          else
            {
              carrick_notification_manager_queue_event (priv->note,
                                                        priv->type,
                                                        "ready",
                                                        priv->name);
              /* Set an unusually long timeout because the Connect
               * method doesn't return until there has been either
               * success or an error.
               */
              dbus_g_proxy_set_default_timeout (priv->proxy,
                                                120000);
              org_moblin_connman_Service_connect_async (priv->proxy,
                                                        connect_notify_cb,
                                                        item);
            }
        }
      else
        {
	  if (priv->setup_required)
	  {
	    GPid pid;
	    gulong flags = G_SPAWN_SEARCH_PATH | \
	      G_SPAWN_STDOUT_TO_DEV_NULL | \
	      G_SPAWN_STDERR_TO_DEV_NULL;
	    GError *error = NULL;
	    char *argv[] = {
              LIBEXECDIR "/carrick-3g-wizard",
	      NULL
	    };

	    if (!g_spawn_async(NULL, argv, NULL, flags, NULL,
			       NULL, &pid, &error))
	    {
	      g_debug("Unable to spawn 3g connection wizard");
	      g_error_free(error);
	    }
	  }
	  else
	  {
	    carrick_notification_manager_queue_event (priv->note,
						      priv->type,
						      "ready",
						      priv->name);
	    /* Set an unusually long timeout because the Connect
	     * method doesn't return until there has been either
	     * success or an error.
	     */
	    dbus_g_proxy_set_default_timeout (priv->proxy,
					      120000);
	    org_moblin_connman_Service_connect_async (priv->proxy,
						      connect_notify_cb,
						      item);
	  }
        }
    }
}

static void
_connect_with_password (CarrickServiceItem *item)
{
  CarrickServiceItemPrivate *priv = item->priv;
  const gchar               *passphrase;
  GValue                    *value;
  guint                      len;
  gchar                     *label = NULL;

  carrick_notification_manager_queue_event (priv->note,
                                            priv->type,
                                            "ready",
                                            priv->name);

  if (priv->passphrase_hint_visible)
    {
      passphrase = "";
    }
  else
    {
      /* Basic validation of the passphrase */
      len = gtk_entry_get_text_length (GTK_ENTRY (priv->passphrase_entry));
      if (g_str_equal (priv->security, "wep"))
        {
          /* WEP passphrase must be 10 chars or 28 */
          if (len != 10 || len != 28)
            {
              label = g_strdup_printf (_("Your password isn't the right length."
                                         " For a WEP connection it needs to be"
                                         " either 10 or 28 characters, you"
                                         " have %i."), len);
            }
        }
      else if (g_str_equal (priv->security, "wpa"))
        {
          /* WPA passphrase must be more than 9 chars, less than 62 */
          if (len < 8)
            label = g_strdup_printf (_("Your password is too short. For a WPA "
                                       " connection it needs to be at least"
                                       " 8 characters long, you have %i"), len);
          else if (len > 62)
            label = g_strdup_printf (_("Your password is too long. For a WPA "
                                       " connection it needs to have fewer than"
                                       " 62 characters, you have %i"), len);
        }
      else if (g_str_equal (priv->security, "rsn"))
        {
          /* WPA2 passphrase must be more than 9 chars, less than 62 */
          if (len < 8)
            label = g_strdup_printf (_("Your password is too short. For a WPA2 "
                                       " connection it needs to be at least"
                                       " 8 characters long, you have %i"), len);
          else if (len > 62)
            label = g_strdup_printf (_("Your password is too long. For a WPA2 "
                                       " connection it needs to have fewer than"
                                       " 62 characters, you have %i"), len);
        }

      if (!label)
        {
          passphrase = gtk_entry_get_text (GTK_ENTRY (priv->passphrase_entry));
          value = g_slice_new0 (GValue);
          g_value_init (value, G_TYPE_STRING);
          g_value_set_string (value, passphrase);

          org_moblin_connman_Service_set_property_async (priv->proxy,
                                                         "Passphrase",
                                                         value,
                                                         set_passphrase_notify_cb,
                                                         item);

          g_value_unset (value);
          g_slice_free (GValue, value);

          gtk_widget_hide (priv->passphrase_box);
          gtk_widget_show (priv->connect_box);
        }
      else
        {
          gtk_label_set_text (GTK_LABEL (priv->info_label),
                              label);
          gtk_info_bar_set_message_type (GTK_INFO_BAR (priv->info_bar),
                                         GTK_MESSAGE_WARNING);
          gtk_widget_show (priv->info_bar);
          gtk_widget_grab_focus (priv->passphrase_entry);
        }
    }
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
  CarrickServiceItemPrivate *priv = item->priv;

  if (priv->passphrase_hint_visible)
    {
      gboolean visible = gtk_toggle_button_get_active (
        GTK_TOGGLE_BUTTON (priv->show_password_check));

      gtk_entry_set_visibility (pw_entry, visible);
      priv->passphrase_hint_visible = FALSE;
    }
}

static void
_passphrase_entry_clear_released_cb (GtkEntry            *entry,
                                     GtkEntryIconPosition icon_pos,
                                     GdkEvent            *event,
                                     gpointer             user_data)
{
  CarrickServiceItem *self = CARRICK_SERVICE_ITEM (user_data);

  if (gtk_entry_get_text_length (entry) > 0)
    {
      gtk_entry_set_text (entry, "");
      gtk_widget_grab_focus (GTK_WIDGET (entry));
    }
  /* On the second click of the clear button hide the passphrase widget */
  else
    {
      carrick_service_item_set_active (self,
                                       FALSE);
    }
}

gboolean
carrick_service_item_get_draggable (CarrickServiceItem *item)
{
  CarrickServiceItemPrivate *priv;

  g_return_val_if_fail (CARRICK_IS_SERVICE_ITEM (item), FALSE);

  priv = item->priv;
  return priv->draggable;
}

void
carrick_service_item_set_draggable (CarrickServiceItem *item,
                                    gboolean            draggable)
{
  CarrickServiceItemPrivate *priv;

  g_return_if_fail (CARRICK_IS_SERVICE_ITEM (item));

  priv = item->priv;
  priv->draggable = draggable;
}

void
carrick_service_item_set_active (CarrickServiceItem *item,
                                 gboolean            active)
{
  g_return_if_fail (CARRICK_IS_SERVICE_ITEM (item));

  if (!active)
    {
      CarrickServiceItemPrivate *priv = item->priv;

      gtk_widget_hide (priv->passphrase_box);
      gtk_widget_show (priv->connect_box);
      gtk_widget_hide (priv->info_bar);
    }
}

DBusGProxy*
carrick_service_item_get_proxy (CarrickServiceItem *item)
{
  CarrickServiceItemPrivate *priv = item->priv;

  return priv->proxy;
}

GtkTreeRowReference*
carrick_service_item_get_row_reference (CarrickServiceItem *item)
{
  CarrickServiceItemPrivate *priv = item->priv;

  return priv->row;
}


gint
carrick_service_item_get_order (CarrickServiceItem *item)
{
  CarrickServiceItemPrivate *priv = item->priv;

  return priv->index;
}

static void
_set_model (CarrickServiceItem  *self,
            CarrickNetworkModel *model)
{
  g_return_if_fail (model != NULL);
  CarrickServiceItemPrivate *priv = self->priv;

  priv->model = model;


  if (priv->row)
    {
      _populate_variables (self);
      _set_state (self);
    }
}

static void
_set_row (CarrickServiceItem *self,
          GtkTreePath        *path)
{
  g_return_if_fail (path != NULL);
  CarrickServiceItemPrivate *priv = self->priv;

  if (priv->row)
    {
      gtk_tree_row_reference_free (priv->row);
      priv->row = NULL;
    }

  if (path)
    priv->row = gtk_tree_row_reference_new (GTK_TREE_MODEL (priv->model),
                                            path);

  if (priv->model)
    {
      _populate_variables (self);
      _set_state (self);
    }
}

void
carrick_service_item_update (CarrickServiceItem *self)
{
  _populate_variables (self);
  _set_state (self);
}

static void
carrick_service_item_set_property (GObject *object, guint property_id,
                                   const GValue *value, GParamSpec *pspec)
{
  g_return_if_fail (object != NULL);
  g_return_if_fail (CARRICK_IS_SERVICE_ITEM (object));
  CarrickServiceItem        *self = CARRICK_SERVICE_ITEM (object);
  CarrickServiceItemPrivate *priv = self->priv;

  switch (property_id)
    {
    case PROP_DRAGGABLE:
      priv->draggable = g_value_get_boolean (value);
      break;

    case PROP_ICON_FACTORY:
      priv->icon_factory = CARRICK_ICON_FACTORY (g_value_get_object (value));
      break;

    case PROP_NOTIFICATIONS:
      priv->note = CARRICK_NOTIFICATION_MANAGER (g_value_get_object (value));
      break;

    case PROP_MODEL:
      _set_model (self,
                  CARRICK_NETWORK_MODEL (g_value_get_object (value)));
      break;

    case PROP_ROW:
      _set_row (self,
                (GtkTreePath *) g_value_get_boxed (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
carrick_service_item_dispose (GObject *object)
{
  CarrickServiceItem        *item = CARRICK_SERVICE_ITEM (object);
  CarrickServiceItemPrivate *priv = item->priv;

  if (priv->row)
    {
      gtk_tree_row_reference_free (priv->row);
      priv->row = NULL;
    }

  if (priv->hand_cursor)
    {
      gdk_cursor_unref (priv->hand_cursor);
      priv->hand_cursor = NULL;
    }

  if (priv->proxy)
    {
      g_object_unref (priv->proxy);
      priv->proxy = NULL;
    }

  G_OBJECT_CLASS (carrick_service_item_parent_class)->dispose (object);
}

static void
carrick_service_item_finalize (GObject *object)
{
  CarrickServiceItem        *item = CARRICK_SERVICE_ITEM (object);
  CarrickServiceItemPrivate *priv = item->priv;

  g_free (priv->name);
  g_free (priv->type);
  g_free (priv->state);
  g_free (priv->security);
  g_free (priv->passphrase);

  G_OBJECT_CLASS (carrick_service_item_parent_class)->finalize (object);
}

static gboolean
carrick_service_item_enter_notify_event (GtkWidget        *widget,
                                         GdkEventCrossing *event)
{
  CarrickServiceItem        *item = CARRICK_SERVICE_ITEM (widget);
  CarrickServiceItemPrivate *priv = item->priv;

  priv->hover = TRUE;

  gtk_widget_modify_bg (priv->expando,
                        GTK_STATE_NORMAL,
                        &priv->prelight_color);
  if (priv->draggable)
    gdk_window_set_cursor (widget->window, priv->hand_cursor);

  return TRUE;
}

static gboolean
carrick_service_item_leave_notify_event (GtkWidget        *widget,
                                         GdkEventCrossing *event)
{
  /* if pointer moves to a child widget, we want to keep the
   * ServiceItem prelighted, but show the normal cursor */
  if (event->detail == GDK_NOTIFY_INFERIOR)
    {
      gdk_window_set_cursor (widget->window, NULL);
    }
  else
    {
      CarrickServiceItem        *item = CARRICK_SERVICE_ITEM (widget);
      CarrickServiceItemPrivate *priv = item->priv;

      priv->hover = FALSE;

      if (g_str_equal (priv->state, "ready"))
        {
          gtk_widget_modify_bg (priv->expando,
                                GTK_STATE_NORMAL,
                                &priv->active_color);
        }
      else
        {
          gtk_widget_modify_bg (priv->expando,
                                GTK_STATE_NORMAL,
                                NULL);
        }
    }

  return TRUE;
}

static void
carrick_service_item_class_init (CarrickServiceItemClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  GParamSpec *pspec;

  g_type_class_add_private (klass, sizeof (CarrickServiceItemPrivate));

  object_class->get_property = carrick_service_item_get_property;
  object_class->set_property = carrick_service_item_set_property;
  object_class->dispose = carrick_service_item_dispose;
  object_class->finalize = carrick_service_item_finalize;

  widget_class->enter_notify_event = carrick_service_item_enter_notify_event;
  widget_class->leave_notify_event = carrick_service_item_leave_notify_event;

  pspec = g_param_spec_boolean ("draggable",
                                "draggable",
                                "Should the service item show a draggable cursor on hover",
                                FALSE,
                                G_PARAM_READWRITE | G_PARAM_CONSTRUCT);
  g_object_class_install_property (object_class,
                                   PROP_DRAGGABLE,
                                   pspec);

  pspec = g_param_spec_object ("notification-manager",
                               "CarrickNotificationManager",
                               "Notification manager to use",
                               CARRICK_TYPE_NOTIFICATION_MANAGER,
                               G_PARAM_READWRITE | G_PARAM_CONSTRUCT);
  g_object_class_install_property (object_class,
                                   PROP_NOTIFICATIONS,
                                   pspec);

  pspec = g_param_spec_object ("icon-factory",
                               "icon-factory",
                               "CarrickIconFactory object",
                               CARRICK_TYPE_ICON_FACTORY,
                               G_PARAM_READWRITE | G_PARAM_CONSTRUCT);
  g_object_class_install_property (object_class,
                                   PROP_ICON_FACTORY,
                                   pspec);

  pspec = g_param_spec_object ("model",
                               "model",
                               "CarrickNetworkModel object",
                               CARRICK_TYPE_NETWORK_MODEL,
                               G_PARAM_READWRITE | G_PARAM_CONSTRUCT);
  g_object_class_install_property (object_class,
                                   PROP_MODEL,
                                   pspec);

  pspec = g_param_spec_boxed ("tree-path",
                              "tree-path",
                              "GtkTreePath of the row to represent",
                              GTK_TYPE_TREE_PATH,
                              G_PARAM_READWRITE | G_PARAM_CONSTRUCT);
  g_object_class_install_property (object_class,
                                   PROP_ROW,
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
  CarrickServiceItemPrivate *priv;
  GtkWidget                 *box, *hbox, *vbox;
  GtkWidget                 *image;
  GtkWidget                 *connect_with_pw_button;
  GtkWidget                 *content_area;
  char                      *security_sample;

  priv = self->priv = SERVICE_ITEM_PRIVATE (self);

  priv->model = NULL;
  priv->row = NULL;
  priv->failed = FALSE;

  priv->proxy = NULL;
  priv->index = 0;
  priv->name = NULL;
  priv->type = NULL;
  priv->state = NULL;
  priv->strength = 0;
  priv->security = NULL;
  priv->need_pass = FALSE;
  priv->passphrase = NULL;
  priv->icon_factory = NULL;
  priv->note = NULL;
  priv->setup_required = FALSE;

  priv->hand_cursor = gdk_cursor_new (GDK_HAND1);

  box = gtk_hbox_new (FALSE,
                      6);
  gtk_widget_show (box);
  priv->expando = nbtk_gtk_expander_new ();
  gtk_container_add (GTK_CONTAINER (self),
                     priv->expando);
  nbtk_gtk_expander_set_label_widget (NBTK_GTK_EXPANDER (priv->expando),
                                      box);
  nbtk_gtk_expander_set_has_indicator (NBTK_GTK_EXPANDER (priv->expando),
                                       FALSE);
  gtk_widget_show (priv->expando);

  gdk_color_parse ("#e8e8e8", &priv->active_color);
  gdk_color_parse ("#cbcbcb", &priv->prelight_color);

  priv->icon = gtk_image_new ();
  gtk_widget_show (priv->icon);
  gtk_box_pack_start (GTK_BOX (box),
                      priv->icon,
                      FALSE,
                      FALSE,
                      6);

  vbox = gtk_vbox_new (FALSE,
                       6);
  gtk_widget_show (vbox);
  gtk_box_pack_start (GTK_BOX (box),
                      vbox,
                      TRUE,
                      TRUE,
                      6);

  hbox = gtk_hbox_new (FALSE,
                       6);
  gtk_widget_show (hbox);
  gtk_box_pack_start (GTK_BOX (vbox),
                      hbox,
                      TRUE,
                      TRUE,
                      6);

  priv->name_label = gtk_label_new ("");
  gtk_misc_set_alignment (GTK_MISC (priv->name_label),
                          0.0, 0.5);
  gtk_widget_show (priv->name_label);
  gtk_box_pack_start (GTK_BOX (hbox),
                      priv->name_label,
                      TRUE,
                      TRUE,
                      6);

  image = gtk_image_new_from_icon_name ("edit-clear",
                                        GTK_ICON_SIZE_MENU);
  gtk_widget_show (image);
  priv->delete_button = gtk_button_new ();
  g_signal_connect (priv->delete_button,
                    "clicked",
                    G_CALLBACK (_delete_button_cb),
                    self);
  gtk_button_set_relief (GTK_BUTTON (priv->delete_button),
                         GTK_RELIEF_NONE);
  gtk_button_set_image (GTK_BUTTON (priv->delete_button),
                        image);
  gtk_box_pack_end (GTK_BOX (box),
                    priv->delete_button,
                    FALSE,
                    FALSE,
                    6);
  gtk_widget_set_sensitive (GTK_WIDGET (priv->delete_button),
                            FALSE);

  priv->connect_box = gtk_hbox_new (FALSE,
                                    6);
  gtk_widget_show (priv->connect_box);
  gtk_box_pack_start (GTK_BOX (vbox),
                      priv->connect_box,
                      TRUE,
                      TRUE,
                      6);

  priv->connect_button = gtk_button_new_with_label (_("Scanning"));
  gtk_widget_show (priv->connect_button);
  g_signal_connect (priv->connect_button,
                    "clicked",
                    G_CALLBACK (_connect_button_cb),
                    self);
  gtk_box_pack_start (GTK_BOX (priv->connect_box),
                      priv->connect_button,
                      FALSE,
                      FALSE,
                      6);

  priv->security_label = gtk_label_new ("");
  gtk_misc_set_alignment (GTK_MISC (priv->security_label),
                          0.0, 0.5);
  /* Note: security_sample should contain the longest possible security method */
  /* TRANSLATORS: This is an example of a wireless security method
     (see another translator comment), used to estimate the string length.
     It should look like the ones below network name in the UI */
  security_sample = _ ("WPA2 encrypted");
  gtk_label_set_width_chars (GTK_LABEL (priv->security_label),
                             strlen (security_sample));
  gtk_widget_show (priv->security_label);
  gtk_box_pack_start (GTK_BOX (priv->connect_box),
                      priv->security_label,
                      FALSE,
                      FALSE,
                      6);

  /*priv->default_button = gtk_button_new ();
     gtk_widget_show (priv->default_button);
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
  gtk_box_pack_start (GTK_BOX (vbox), priv->passphrase_box,
                      TRUE, TRUE, 6);

  priv->passphrase_entry = gtk_entry_new ();
  gtk_entry_set_width_chars (GTK_ENTRY (priv->passphrase_entry), 20);
  gtk_entry_set_visibility (GTK_ENTRY (priv->passphrase_entry), TRUE);
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
                    self);
  gtk_box_pack_start (GTK_BOX (priv->passphrase_box),
                      priv->passphrase_entry,
                      FALSE, FALSE, 6);

  connect_with_pw_button = gtk_button_new_with_label (_ ("Connect"));
  gtk_widget_show (connect_with_pw_button);
  g_signal_connect (connect_with_pw_button,
                    "clicked",
                    G_CALLBACK (_connect_with_pw_clicked_cb),
                    self);
  gtk_box_pack_start (GTK_BOX (priv->passphrase_box),
                      connect_with_pw_button,
                      FALSE, FALSE, 6);

  priv->show_password_check =
    gtk_check_button_new_with_label (_ ("Show password"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (priv->show_password_check),
                                                   TRUE);
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

  priv->info_bar = gtk_info_bar_new ();
  gtk_widget_set_no_show_all (priv->info_bar, TRUE);
  priv->info_label = gtk_label_new ("");
  gtk_widget_show (priv->info_label);
  content_area = gtk_info_bar_get_content_area (GTK_INFO_BAR (priv->info_bar));
  gtk_container_add (GTK_CONTAINER (content_area), priv->info_label);
  gtk_box_pack_start (GTK_BOX (vbox),
                      priv->info_bar,
                      FALSE, FALSE, 6);
}

GtkWidget*
carrick_service_item_new (CarrickIconFactory         *icon_factory,
                          CarrickNotificationManager *notifications,
                          CarrickNetworkModel        *model,
                          GtkTreePath                *path)
{
  return g_object_new (CARRICK_TYPE_SERVICE_ITEM,
                       "icon-factory", icon_factory,
                       "notification-manager", notifications,
                       "model", model,
                       "tree-path", path,
                       NULL);
}
