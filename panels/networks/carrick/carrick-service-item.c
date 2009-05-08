/* carrick-service-item.c */

#include "carrick-service-item.h"

#include <config.h>
#include <glib/gi18n.h>
#include <gconnman/gconnman.h>
#include <carrick/carrick-status-icon.h>

G_DEFINE_TYPE (CarrickServiceItem, carrick_service_item, GTK_TYPE_EVENT_BOX)

#define SERVICE_ITEM_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), CARRICK_TYPE_SERVICE_ITEM, CarrickServiceItemPrivate))

typedef struct _CarrickServiceItemPrivate CarrickServiceItemPrivate;

enum
{
  PROP_0,
  PROP_SERVICE
};

struct _CarrickServiceItemPrivate
{
  CmService *service;
  GtkWidget *icon;
  GtkWidget *name_label;
  GtkWidget *security_label;
  GtkWidget *connect_button;
  GtkWidget *table;
  gboolean   connected;
  gchar     *name;
  gchar     *status;
  gchar     *icon_name;
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
      case PROP_SERVICE:
        g_value_set_object (value,
                            priv->service);
        break;
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

void
_connect_button_cb (GtkButton *connect_button,
                    gpointer user_data)
{
  CarrickServiceItemPrivate *priv = SERVICE_ITEM_PRIVATE (user_data);

  if (priv->connected)
  {
    cm_service_disconnect (CM_SERVICE (priv->service));
    gtk_widget_set_state (GTK_WIDGET (user_data),
                          GTK_STATE_NORMAL);
    gtk_widget_set_sensitive (GTK_WIDGET (user_data),
                              FALSE);
    gtk_button_set_label (connect_button,
                          _("Disconnecting"));
  }
  else
  {
    gchar *security = g_strdup (cm_service_get_security (CM_SERVICE (priv->service)));

    gtk_widget_set_state (GTK_WIDGET (user_data),
                          GTK_STATE_SELECTED);
    gtk_widget_set_sensitive (GTK_WIDGET (user_data),
                              FALSE);
    gtk_button_set_label (connect_button,
                          _("Connecting"));

    if (g_strcmp0 ("none", security) != 0)
    {
      if (!cm_service_get_passphrase (priv->service))
      {
        GtkWidget *dialog;
        GtkWidget *label;
        GtkWidget *icon;
        GtkWidget *entry;
        GtkWidget *hbox;
        gchar *passphrase = NULL;

        dialog = gtk_dialog_new_with_buttons (_("Passphrase required"),
                                              NULL,
                                              GTK_DIALOG_MODAL |
                                              GTK_DIALOG_DESTROY_WITH_PARENT,
                                              GTK_STOCK_CANCEL,
                                              GTK_RESPONSE_REJECT,
                                              GTK_STOCK_OK,
                                              GTK_RESPONSE_ACCEPT,
                                              NULL);
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

        if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT)
        {
          passphrase = gtk_entry_get_text (GTK_ENTRY (entry));
          cm_service_set_passphrase (priv->service,
                                     passphrase);
          cm_service_connect (CM_SERVICE (priv->service));
        }
        gtk_widget_destroy (dialog);
      }
    }
    else
    {
        cm_service_connect (CM_SERVICE (priv->service));
    }
  }
}

void
_name_changed_cb (CmService *service,
                  gpointer user_data)
{
  CarrickServiceItemPrivate *priv = SERVICE_ITEM_PRIVATE (user_data);

  gtk_label_set_text (GTK_LABEL (priv->name_label),
                      cm_service_get_name (service));
}

void
_security_changed_cb (CmService *service,
                      gpointer user_data)
{
  CarrickServiceItemPrivate *priv = SERVICE_ITEM_PRIVATE (user_data);
  gchar *security = g_strdup (cm_service_get_security (service));

  if (g_strcmp0 ("none", security) == 0)
  {
    g_free (security);
    security = g_strdup ("");
  }

  gtk_label_set_text (GTK_LABEL (priv->security_label),
                      security);
  g_free (security);
}

void
_status_changed_cb (CmService *service,
                   gpointer user_data)
{
  CarrickServiceItemPrivate *priv = SERVICE_ITEM_PRIVATE (user_data);
  gchar *status = g_strdup (cm_service_get_state (service));

  if (g_strcmp0 ("ready", status) == 0)
  {
    priv->connected = TRUE;
    gtk_button_set_label (GTK_BUTTON (priv->connect_button),
                          "Disconnect");
  }
  else
  {
    priv->connected = FALSE;
    gtk_button_set_label (GTK_BUTTON (priv->connect_button),
                          "Connect");
  }

  gtk_widget_set_sensitive (GTK_WIDGET (priv->connect_button),
                            TRUE);
  gtk_image_set_from_file (GTK_IMAGE (priv->icon),
                           carrick_status_icon_path_for_state (service));
}

static void
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
    const gchar *status = cm_service_get_state (service);
    const gchar *security = cm_service_get_security (service);
    priv->service = g_object_ref (service);
    gtk_image_set_from_file (GTK_IMAGE (priv->icon),
                             carrick_status_icon_path_for_state (service));
    gtk_label_set_text (GTK_LABEL (priv->name_label),
                        cm_service_get_name (service));
    if (g_strcmp0 ("none", security) != 0)
    {
      gtk_label_set_text (GTK_LABEL (priv->security_label),
                          security);
    }
    if (g_strcmp0 ("ready", status) == 0)
    {
      gtk_button_set_label (GTK_BUTTON (priv->connect_button),
                            "Disconnect");
      priv->connected = TRUE;
    }
    else
    {
      gtk_button_set_label (GTK_BUTTON (priv->connect_button),
                            "Connect");
      priv->connected= FALSE;
    }
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
  }
}

static void
carrick_service_item_set_property (GObject *object, guint property_id,
                                   const GValue *value, GParamSpec *pspec)
{
  g_return_if_fail (object != NULL);
  g_return_if_fail (CARRICK_IS_SERVICE_ITEM (object));

  switch (property_id)
    {
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
}

static void
carrick_service_item_init (CarrickServiceItem *self)
{
  CarrickServiceItemPrivate *priv = SERVICE_ITEM_PRIVATE (self);

  priv->table = gtk_table_new (2, 3,
                               TRUE);
  gtk_container_add (GTK_CONTAINER (self),
                     priv->table);

  priv->icon = gtk_image_new_from_file (carrick_status_icon_path_for_state (NULL));
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
carrick_service_item_new (CmService *service)
{
  return g_object_new (CARRICK_TYPE_SERVICE_ITEM,
                       "service",
                       service,
                       NULL);
}
