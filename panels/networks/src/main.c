#include <gtk/gtk.h>
#include <glib.h>
#include <glib/gi18n.h>
#include <dbus/dbus-glib.h>

#include "mux/mux-switch-box.h"
#include "mux/mux-cell-renderer-text.h"

#include "connman-common/connman-client.h"

#include "moblin-netbook-system-tray.h"

#define PKG_ICON_DIR PKG_DATA_DIR "/" "icons"

typedef enum {
        CARRICK_ICON_NO_NETWORK,
        CARRICK_ICON_NO_NETWORK_ACTIVE,
        CARRICK_ICON_WIRED_NETWORK,
        CARRICK_ICON_WIRED_NETWORK_ACTIVE,
        CARRICK_ICON_WIRELESS_NETWORK_1,
        CARRICK_ICON_WIRELESS_NETWORK_1_ACTIVE,
        CARRICK_ICON_WIRELESS_NETWORK_2,
        CARRICK_ICON_WIRELESS_NETWORK_2_ACTIVE,
        CARRICK_ICON_WIRELESS_NETWORK_3,
        CARRICK_ICON_WIRELESS_NETWORK_3_ACTIVE
} CarrickIconState;

static const gchar *icon_names[] = {
        PKG_ICON_DIR "/" "carrick-no-network-normal.png",
        PKG_ICON_DIR "/" "carrick-no-network-active.png",
        PKG_ICON_DIR "/" "carrick-wired-normal.png",
        PKG_ICON_DIR "/" "carrick-wired-active.png",
        PKG_ICON_DIR "/" "carrick-wireless-1-normal.png",
        PKG_ICON_DIR "/" "carrick-wireless-1-active.png",
        PKG_ICON_DIR "/" "carrick-wireless-2-normal.png",
        PKG_ICON_DIR "/" "carrick-wireless-2-active.png",
        PKG_ICON_DIR "/" "carrick-wireless-3-normal.png",
        PKG_ICON_DIR "/" "carrick-wireless-3-active.png"
};

static gboolean standalone = FALSE;
static ConnmanClient *client;
static GtkStatusIcon *icon;
static CarrickIconState state;

static gboolean
window_delete_event_cb (GtkWidget *window,
                        GdkEvent  *event,
                        gpointer   data)
{
        gtk_main_quit ();

        return FALSE;
}

static void
connman_client_cb (const char *status,
                   void       *user_data)
{
        GtkTreeModel *model;
        GtkTreeIter iter;
        gboolean cont;
        guint type;
        guint strength;
        gboolean enabled;

        g_debug ("Connman is %s", status);

        if (g_str_equal (status, "unavailable") == TRUE) {
                state = CARRICK_ICON_NO_NETWORK;
        } else if (g_str_equal (status, "offline") == TRUE) {
                state = CARRICK_ICON_NO_NETWORK;
        } else if (g_str_equal (status, "connecting") == TRUE) {
                state = CARRICK_ICON_NO_NETWORK;
        } else if (g_str_equal (status, "connected") == TRUE ||
                   g_str_equal (status, "online") == TRUE)
        {
                model = connman_client_get_model (client);

                cont = gtk_tree_model_get_iter_first (model, &iter);

                while (cont) {
                        gtk_tree_model_get (model,
                                            &iter,
                                            CONNMAN_COLUMN_TYPE, &type,
                                            CONNMAN_COLUMN_ENABLED, &enabled,
                                            CONNMAN_COLUMN_STRENGTH, &strength,
                                            -1);

                        if (enabled) {
                                cont = FALSE;
                        } else {
                                cont = gtk_tree_model_iter_next (model, &iter);
                        }
                }

                switch (type) {
                case CONNMAN_TYPE_ETHERNET:
                        state = CARRICK_ICON_WIRED_NETWORK;
                        break;
                case CONNMAN_TYPE_WIFI:
                case CONNMAN_TYPE_WIMAX: /* FIXME: */
                case CONNMAN_TYPE_BLUETOOTH: /* FIXME: */
                        if (strength > 66) {
                                state = CARRICK_ICON_WIRELESS_NETWORK_3;
                        } else if (strength > 33) {
                                state = CARRICK_ICON_WIRELESS_NETWORK_2;
                        } else {
                                state = CARRICK_ICON_WIRELESS_NETWORK_1;
                        }
                        break;
                default:
                        break;
                }
        }
        if (icon) {
                gtk_status_icon_set_from_file (icon,
                                               icon_names[state]);
        } else {
                icon = gtk_status_icon_new_from_file (icon_names[state]);
        }
}

static gboolean
offline_mode_toggled_cb (GtkWidget *offline_switch,
                         gboolean   new_state,
                         gpointer   user_data)
{
        g_debug ("Offline mode toggled");
        connman_client_set_offline_mode (client, new_state);

        /* FIXME: Make connection list insensitive */

        return TRUE;
}

static void
secret_check_toggled_cb (GtkToggleButton *toggle,
                         gpointer user_data)
{
        GtkEntry *entry = GTK_ENTRY (user_data);
        gboolean vis = gtk_toggle_button_get_active (toggle);
        gtk_entry_set_visibility (entry, vis);
}

static gboolean
new_conn_cb (GtkButton *button,
             gpointer   user_data)
{
        GtkWidget *dialog;
        GtkWidget *desc;
        GtkWidget *ssid_entry, *ssid_label;
        GtkWidget *security_combo, *security_label;
        GtkWidget *secret_entry, *secret_label;
        GtkWidget *secret_check;
        GtkWidget *table;
        GtkWidget *hbox;
        const gchar *network, *secret;
        gchar *security;
        GtkWidget *image;

        if (!standalone) {
                gtk_widget_hide (GTK_WIDGET (user_data));
        }

        dialog = gtk_dialog_new_with_buttons (_("New connection settings"),
                                              GTK_WINDOW (user_data),
                                              GTK_DIALOG_MODAL |
                                              GTK_DIALOG_DESTROY_WITH_PARENT,
                                              GTK_STOCK_OK,
                                              GTK_RESPONSE_ACCEPT,
                                              GTK_STOCK_CANCEL,
                                              GTK_RESPONSE_REJECT,
                                              NULL);

        if (icon) {
                gtk_window_set_icon (GTK_WINDOW (dialog), icon);
        } else {
                gtk_window_set_icon_name (GTK_WINDOW (dialog),
                                          GTK_STOCK_DIALOG_AUTHENTICATION);
        }

        gtk_box_set_spacing (GTK_BOX (GTK_DIALOG (dialog)->vbox),
                             6);

        table = gtk_table_new (5, 2, FALSE);
        gtk_table_set_row_spacings (GTK_TABLE (table), 6);
        gtk_table_set_col_spacings (GTK_TABLE (table), 6);
        gtk_container_set_border_width (GTK_CONTAINER (table), 6);
        gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox),
                           table);
        hbox = gtk_hbox_new (FALSE, 2);
        image = gtk_image_new_from_icon_name (GTK_STOCK_DIALOG_AUTHENTICATION,
                                              GTK_ICON_SIZE_DIALOG);
        gtk_box_pack_start_defaults (GTK_BOX (hbox),
                                     image);
        desc = gtk_label_new (_("Enter the name of the network you want\nto "
                                "connect to and, where necessary, the\n"
                                "password and security type."));
        gtk_box_pack_start_defaults (GTK_BOX (hbox),
                                     desc);

        gtk_table_attach_defaults (GTK_TABLE (table),
                                   hbox,
                                   0, 2,
                                   0, 1);

        ssid_label = gtk_label_new (_("Network name"));
        gtk_misc_set_alignment (GTK_MISC (ssid_label),
                                0.0, 0.5);
        gtk_table_attach_defaults (GTK_TABLE (table),
                                   ssid_label,
                                   0, 1,
                                   1, 2);
        ssid_entry = gtk_entry_new ();
        gtk_table_attach_defaults (GTK_TABLE (table),
                                   ssid_entry,
                                   1, 2,
                                   1, 2);

        security_label = gtk_label_new (_("Network security"));
        gtk_misc_set_alignment (GTK_MISC (security_label),
                                0.0, 0.5);
        gtk_table_attach_defaults (GTK_TABLE (table),
                                   security_label,
                                   0, 1,
                                   2, 3);

        security_combo = gtk_combo_box_new_text ();
        gtk_combo_box_append_text (GTK_COMBO_BOX (security_combo),
                                   "None");
        gtk_combo_box_append_text (GTK_COMBO_BOX (security_combo),
                                   "WEP");
        gtk_combo_box_append_text (GTK_COMBO_BOX (security_combo),
                                   "WPA");
        gtk_combo_box_append_text (GTK_COMBO_BOX (security_combo),
                                   "WPA2");
	gtk_combo_box_set_active (GTK_COMBO_BOX (security_combo),
				  0);
        gtk_table_attach_defaults (GTK_TABLE (table),
                                   security_combo,
                                   1, 2,
                                   2, 3);

        secret_label = gtk_label_new (_("Password"));
        gtk_misc_set_alignment (GTK_MISC (secret_label),
                                0.0, 0.5);
        gtk_table_attach_defaults (GTK_TABLE (table),
                                   secret_label,
                                   0, 1,
                                   3, 4);
        secret_entry = gtk_entry_new ();
        gtk_entry_set_visibility (GTK_ENTRY (secret_entry), FALSE);
        gtk_table_attach_defaults (GTK_TABLE (table),
                                   secret_entry,
                                   1, 2,
                                   3, 4);

        secret_check = gtk_check_button_new_with_label (_("Show passphrase"));
        g_signal_connect (secret_check,
                          "toggled",
                          G_CALLBACK (secret_check_toggled_cb),
                          secret_entry);
        gtk_table_attach_defaults (GTK_TABLE (table),
                                   secret_check,
                                   0, 2,
                                   4, 5);

        gtk_widget_show_all (dialog);

        if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT) {
                network = gtk_entry_get_text (GTK_ENTRY (ssid_entry));
                security = gtk_combo_box_get_active_text (GTK_COMBO_BOX
                                                          (security_combo));
                secret = gtk_entry_get_text (GTK_ENTRY (secret_entry));

                if (network == NULL)
                        return FALSE;

                connman_client_join_network (client,
                                             network,
                                             secret,
                                             security);
        }
        gtk_widget_destroy (dialog);

        return TRUE;
}

static void
show_passphrase_dialog (gchar *path,
                        GdkWindow *parent)
{
        GtkWidget *dialog;
        GtkWidget *table;
        GtkWidget *icon;
        GtkWidget *label;
        GtkWidget *entry;
        GtkWidget *button;

        if (!standalone) {
                gdk_window_hide (parent);
        }

        dialog = gtk_dialog_new_with_buttons (_("Enter passphrase"),
                                              NULL,
                                              GTK_DIALOG_MODAL |
                                              GTK_DIALOG_DESTROY_WITH_PARENT,
                                              GTK_STOCK_OK,
                                              GTK_RESPONSE_ACCEPT,
                                              GTK_STOCK_CANCEL,
                                              GTK_RESPONSE_REJECT,
                                              NULL);

        table = gtk_table_new (4, 2, FALSE);
	gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox),
			   table);
        gtk_window_set_icon_name (GTK_WINDOW (dialog),
                                  GTK_STOCK_DIALOG_AUTHENTICATION);
        icon = gtk_image_new_from_icon_name (GTK_STOCK_DIALOG_AUTHENTICATION,
                                             GTK_ICON_SIZE_DIALOG);
        gtk_table_attach_defaults (GTK_TABLE (table),
                                   icon,
                                   0, 1,
                                   0, 1);

        label = gtk_label_new (_("Network requires a passphrase"));
        gtk_misc_set_alignment (GTK_MISC (label),
                                0.0, 0.5);
        gtk_table_attach_defaults (GTK_TABLE (table),
                                   label,
                                   0, 1,
                                   1, 2);

        entry = gtk_entry_new ();
        gtk_table_attach_defaults (GTK_TABLE (table),
                                   entry,
                                   1, 2,
                                   1, 2);

        button = gtk_check_button_new_with_label (_("Remember network"));
        gtk_table_attach_defaults (GTK_TABLE (table),
                                   button,
                                   0, 1,
                                   2, 3);

        gtk_widget_show_all (dialog);

        if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT) {
                const gchar *passphrase;
                gboolean remember;

                passphrase = gtk_entry_get_text (GTK_ENTRY (entry));
                remember = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON
                                                         (button));

                connman_client_set_passphrase (client, path, passphrase);
                connman_client_set_remember (client, path, remember);
                connman_client_connect (client, path);
        }

        gtk_widget_destroy (dialog);
}

static void
status_clicked_cb (GtkCellRenderer *renderer,
                   gchar *path,
                   gpointer user_data)
{
        GtkTreeSelection *selection;
        GtkTreeModel *model;
        GtkTreeIter iter;
        DBusGProxy *proxy;
        gchar *proxy_path;
        gboolean enabled;
        guint security;
        gchar *passphrase;
        GdkWindow *window;
        GtkTreeView *view;

        view = GTK_TREE_VIEW (user_data);
        selection = gtk_tree_view_get_selection (view);
        gtk_tree_selection_get_selected (selection,
                                         &model,
                                         &iter);
        gtk_tree_model_get (model, &iter,
                            CONNMAN_COLUMN_PROXY, &proxy,
                            CONNMAN_COLUMN_ENABLED, &enabled,
                            -1);

        proxy_path = g_strdup (dbus_g_proxy_get_path (proxy));

        g_debug ("Cell clicked: %s", proxy_path);

        if (enabled) {
                connman_client_disconnect (client, proxy_path);
        } else {
                security = connman_client_get_security (client,
                                                        proxy_path);
                switch (security) {
                case CONNMAN_SECURITY_UNKNOWN:
                case CONNMAN_SECURITY_NONE:
                        connman_client_connect (client,
                                                proxy_path);
                        break;
                default:
                        passphrase = connman_client_get_passphrase (client,
                                                                    proxy_path);
                        if (passphrase) {
                                g_free (passphrase);
                                connman_client_connect (client,
                                                        proxy_path);
                        } else {
                                window = gtk_widget_get_parent_window
                                        (GTK_WIDGET (view));
                                show_passphrase_dialog (proxy_path,
                                                        window);
                        }
                }
        }
}

static void
_icon_to_pixbuf (GtkTreeViewColumn *column,
                 GtkCellRenderer   *cell,
                 GtkTreeModel      *model,
                 GtkTreeIter       *iter,
                 gpointer           data)
{
	gchar *name;

	gtk_tree_model_get (model, iter,
                            CONNMAN_COLUMN_ICON, &name,
                            -1);

	g_object_set (cell, "icon-name", name, NULL);

	g_free (name);
}

static void
_name_to_text (GtkTreeViewColumn *column,
               GtkCellRenderer   *cell,
               GtkTreeModel      *model,
               GtkTreeIter       *iter,
               gpointer           data)
{
        gchar *name;
	gchar *type_name = NULL, *status = NULL;
	guint type;
	gboolean enabled, in_range;
	gchar *markup = NULL;

	gtk_tree_model_get (model, iter,
                            CONNMAN_COLUMN_NAME, &name,
                            CONNMAN_COLUMN_ENABLED, &enabled,
                            CONNMAN_COLUMN_TYPE, &type,
                            CONNMAN_COLUMN_INRANGE, &in_range,
                            -1);

	switch (type) {
	case CONNMAN_TYPE_ETHERNET:
		type_name = g_strdup_printf (_("Wired"));
		break;
	case CONNMAN_TYPE_WIFI:
		type_name  = g_strdup_printf (_("Wifi"));
		break;
	case CONNMAN_TYPE_BLUETOOTH:
		type_name = g_strdup_printf (_("Bluetooth"));
		break;
	case CONNMAN_TYPE_WIMAX:
		type_name = g_strdup_printf (_("Wimax"));
		break;
        case CONNMAN_TYPE_UNKNOWN:
	default:
		type_name = g_strdup_printf ("%s", "");
		break;
	}

	if (enabled && in_range) {
		status = g_strdup_printf ("- Connected");
	} else {
		status = g_strdup_printf ("%s", "");
	}

	if (name) {
		markup = g_strdup_printf ("%s %s",
                                          name,
                                          status);
	}

	g_object_set (cell, "markup", markup, NULL);
	g_free (markup);
	g_free (name);
	g_free (type_name);
}

static void
_status_to_text (GtkTreeViewColumn *column,
                 GtkCellRenderer   *cell,
                 GtkTreeModel      *model,
                 GtkTreeIter       *iter,
                 gpointer           data)
{
	gboolean enabled, inrange;
	gchar *markup = NULL;
        DBusGProxy *proxy;
        GtkWidget *view;

        view = gtk_tree_view_column_get_tree_view (column);

	gtk_tree_model_get (model, iter,
                            CONNMAN_COLUMN_PROXY, &proxy,
                            CONNMAN_COLUMN_ENABLED, &enabled,
                            CONNMAN_COLUMN_INRANGE, &inrange,
                            -1);

        /* TODO: Verify logic here */
	if (enabled && inrange) {
		markup = g_strdup_printf (_("Disconnect"));
	} else if (inrange) {
		markup = g_strdup_printf (_("Connect"));
	} else {
		markup = g_strdup_printf ("%s", "");
	}

	g_object_set (cell, "markup", markup, NULL);

	g_free (markup);
}

static void
_security_to_text (GtkTreeViewColumn *column,
                   GtkCellRenderer   *cell,
                   GtkTreeModel      *model,
                   GtkTreeIter       *iter,
                   gpointer           data)
{
	guint security;
	gchar *secret;
	gchar *markup = NULL;

	gtk_tree_model_get (model, iter,
                            CONNMAN_COLUMN_SECURITY, &security,
                            CONNMAN_COLUMN_PASSPHRASE, &secret,
                            -1);

	if (secret) {
		switch (security) {
		case CONNMAN_SECURITY_WEP:
			markup = g_strdup_printf (_("WEP encrypted (%s)"),
                                                  secret);
			break;
		case CONNMAN_SECURITY_WPA:
		case CONNMAN_SECURITY_WPA2:
			markup = g_strdup_printf (_("WPA encrypted (%s)"),
                                                  secret);
			break;
		default:
			markup = g_strdup_printf ("%s", "");
			break;
		}
	}

        g_object_set (cell, "markup", markup, NULL);

	g_free (markup);
	g_free (secret);
}

int
main (int argc,
      char **argv)
{
        GtkWidget *window;
	GtkWidget *view;
	GtkWidget *new_conn_button;
        GtkWidget *offline_mode_switch;
        GtkWidget *offline_mode_label;
        GtkWidget *table;
	GtkWidget *tree;
        GtkWidget *vbox;
	GtkTreeModel *model;
	GtkTreeSelection *selection;
	GdkScreen *screen;
	gint screen_width, screen_height;
        GtkCellRenderer *cell;

        GError *error = NULL;
        GOptionContext *ctx;
        const GOptionEntry entries[] = {
                { "standalone", 's', 0, G_OPTION_ARG_NONE, &standalone,
                  _("Operate in standalone mode"), NULL },
                { NULL }
        };

        gtk_init (&argc, &argv);

        client = connman_client_new ();
        connman_client_set_callback (client,
                                     connman_client_cb,
                                     NULL);

	screen = gdk_screen_get_default ();
	screen_width = gdk_screen_get_width (screen);
	screen_height = gdk_screen_get_height (screen);

        g_set_application_name (_("Moblin connection manager"));
        ctx = g_option_context_new (_("Moblin connection manager options"));
        g_option_context_add_main_entries (ctx, entries, NULL);
        g_option_context_set_translation_domain (ctx, NULL);
        g_option_context_add_group (ctx, gtk_get_option_group (TRUE));
        g_option_context_set_ignore_unknown_options (ctx, TRUE);

        g_option_context_parse (ctx, &argc, &argv, &error);
        if (error != NULL) {
                g_printerr ("Error parsing command line options: %s",
                            error->message);
                return 1;
        }
        g_option_context_free (ctx);

        if (standalone) {
                window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
                gtk_window_set_title (GTK_WINDOW (window),
                                      _("Moblin connection manager"));
                g_signal_connect (window,
                                  "delete-event",
                                  G_CALLBACK (window_delete_event_cb),
                                  NULL);
                gtk_window_set_icon_name (GTK_WINDOW (window),
                                          GTK_STOCK_NETWORK);
        } else {
                window = gtk_plug_new (0);
	}
        gtk_container_set_border_width (GTK_CONTAINER (window), 6);

        table = gtk_table_new (4, 5, FALSE);

	tree = gtk_tree_view_new ();
	gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (tree), FALSE);

	gtk_tree_view_insert_column_with_data_func (GTK_TREE_VIEW (tree),
                                                    -1, "Icon",
                                                    gtk_cell_renderer_pixbuf_new (),
                                                    _icon_to_pixbuf,
                                                    NULL, NULL);
	gtk_tree_view_insert_column_with_data_func (GTK_TREE_VIEW (tree),
                                                    -1, "Name",
                                                    gtk_cell_renderer_text_new (),
                                                    _name_to_text,
                                                    NULL, NULL);
        cell = mux_cell_renderer_text_new ();
        g_signal_connect (cell,
                          "activated",
                          G_CALLBACK (status_clicked_cb),
                          tree);
	gtk_tree_view_insert_column_with_data_func (GTK_TREE_VIEW (tree),
                                                    -1, "Status",
                                                    cell,
                                                    _status_to_text,
                                                    NULL, NULL);
	gtk_tree_view_insert_column_with_data_func (GTK_TREE_VIEW (tree),
                                                    -1, "Security",
                                                    gtk_cell_renderer_text_new (),
                                                    _security_to_text,
                                                    NULL, NULL);

	model = connman_client_get_model (client);
	gtk_tree_view_set_model (GTK_TREE_VIEW (tree), model);
	g_object_unref (model);

        gtk_tree_view_expand_all (GTK_TREE_VIEW (tree));

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (tree));
	gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE);

        view = gtk_scrolled_window_new (NULL, NULL);
        gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (view),
                                        GTK_POLICY_NEVER,
                                        GTK_POLICY_AUTOMATIC);
	gtk_container_add (GTK_CONTAINER (view), tree);
        gtk_table_attach (GTK_TABLE (table),
                          view,
                          0, 3,
                          0, 3,
                          GTK_EXPAND | GTK_FILL,
                          GTK_EXPAND | GTK_FILL,
                          0, 0);

        vbox = gtk_vbox_new (FALSE, 6);

        offline_mode_switch = mux_switch_box_new (_("Flight mode"));
        g_signal_connect (offline_mode_switch,
                          "switch-toggled",
                          G_CALLBACK (offline_mode_toggled_cb),
                          NULL);
        gtk_box_pack_start (GTK_BOX (vbox),
                            offline_mode_switch,
                            FALSE,
                            FALSE,
                            2);

        offline_mode_label = gtk_label_new (
                _("This will disable all wireless\nconnections"));
        gtk_box_pack_start (GTK_BOX (vbox),
                            offline_mode_label,
                            FALSE,
                            FALSE,
                            2);

        gtk_table_attach (GTK_TABLE (table),
                          vbox,
                          3, 4,
                          0, 2,
                          GTK_SHRINK,
                          GTK_SHRINK,
                          0, 0);

        new_conn_button = gtk_button_new_with_label (_("Add a new connection"));
        g_signal_connect (new_conn_button,
                          "clicked",
                          G_CALLBACK (new_conn_cb),
                          window);
        gtk_table_attach (GTK_TABLE (table),
                          new_conn_button,
                          0, 1,
                          4, 5,
                          GTK_SHRINK,
                          GTK_SHRINK,
                          0, 0);

        gtk_table_set_col_spacings (GTK_TABLE (table), 6);
        gtk_table_set_row_spacings (GTK_TABLE (table), 6);
        gtk_container_add (GTK_CONTAINER (window), table);

	if (!standalone) {
                icon = gtk_status_icon_new_from_file (icon_names[0]);
                mnbk_system_tray_init (icon,
                                       GTK_PLUG (window),
                                       "wifi");
                gtk_widget_set_size_request (window,
                                             screen_width - 10,
                                             -1);
	}

        gtk_widget_show_all (window);

        gtk_main ();

        g_object_unref (client);

        return 0;
}
