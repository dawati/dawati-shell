/*
 * Carrick - a connection panel for the MeeGo Netbook
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

#include <config.h>

#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <dbus/dbus-glib.h>

#include "carrick/carrick-applet.h"
#include "carrick/carrick-pane.h"

/*
 * The GtkWindow in standalone mode, the GtkPlug in embedded mode, the
 * undecorated popup window in non-MeeGo mode, or NULL in MeeGo panel mode.
 */
static GtkWidget *window = NULL;

static gchar *
get_tip_and_icon_state (const gchar      *connection_type,
                        const gchar      *connection_name,
                        const gchar      *state,
                        guint            strength,
                        CarrickIconState *icon_state)
{
  gchar *tip;

  if (g_str_equal (state, "idle"))
    {
      /* TRANSLATORS: Tooltips for the toolbar icon
       * The possible placeholder is service name (such as 
       * a wireless network name) */
      tip = g_strdup (_("networks - not connected"));
      *icon_state = ICON_OFFLINE;
    }
  else if (g_str_equal (state, "association") ||
           g_str_equal (state, "configuration"))
    {
      tip = g_strdup (_("networks - connecting"));
      *icon_state = ICON_CONNECTING;
    }
  else if (g_str_equal (state, "ready") || g_str_equal (state, "online"))
    {
      if (g_str_equal (connection_type, "ethernet"))
        tip = g_strdup (_("networks - wired"));
      else if (g_str_equal (connection_type, "wifi"))
        tip = g_strdup_printf (_("networks - %s - wifi"),
                               connection_name);
      else if (g_str_equal (connection_type, "wimax"))
        tip = g_strdup_printf (_("networks - %s - wimax"),
                               connection_name);
      else if (g_str_equal (connection_type, "cellular"))
        tip = g_strdup_printf (_("networks - %s - 3G"),
                               connection_name);
      else if (g_str_equal (connection_type, "bluetooth"))
        tip = g_strdup_printf (_("networks - %s - bluetooth"),
                               connection_name);
      else if (g_str_equal (connection_type, "vpn"))
        tip = g_strdup_printf (_("networks - %s - VPN"),
                               connection_name);
      else
        tip = g_strdup (_("networks"));

      *icon_state = carrick_icon_factory_get_state (connection_type,
                                                   strength);
    }
  else if (g_str_equal (state, "error"))
    {
      tip = g_strdup (_("networks - error"));
      *icon_state = ICON_ERROR;
    }
  else
    {
      tip = g_strdup (_("networks"));
      *icon_state = ICON_OFFLINE;
    }

  return tip;
}


#if WITH_MEEGO

#include <meego-panel/mpl-panel-common.h>
#include <meego-panel/mpl-panel-gtk.h>

static MplPanelClient *panel_client = NULL;

static void
_client_show_cb (MplPanelClient *client,
                 gpointer        user_data)
{
  CarrickApplet *applet = CARRICK_APPLET (user_data);
  CarrickPane   *pane = CARRICK_PANE (carrick_applet_get_pane (applet));

  carrick_pane_update (pane);
}

static void
_connection_changed_cb (CarrickPane     *pane,
                        const gchar     *connection_type,
                        const gchar     *connection_name,
                        const gchar     *state,
                        guint            strength,
                        MplPanelClient  *panel_client)
{
  CarrickIconState icon_state;
  const gchar      *icon_id;
  gchar            *tip;

  tip = get_tip_and_icon_state (connection_type,
                                connection_name,
                                state,
                                strength,
                                &icon_state);

  icon_id = carrick_icon_factory_get_name_for_state (icon_state);

  mpl_panel_client_request_button_style (panel_client,
                                         icon_id);

  mpl_panel_client_request_tooltip (panel_client,
                                    tip);

  g_free (tip);
}

void
carrick_shell_request_focus (void)
{
  if (panel_client)
    mpl_panel_client_request_focus (panel_client);
}

void
carrick_shell_hide (void)
{
  if (panel_client)
    mpl_panel_client_hide (panel_client);
}

void
carrick_shell_show (void)
{
  if (panel_client)
    mpl_panel_client_show (panel_client);
}

static void
panel_hide_cb (MplPanelClient *panel_client, gpointer user_data)
{
  GtkDialog *dialog = GTK_DIALOG (user_data);

  gtk_dialog_response (dialog, GTK_RESPONSE_DELETE_EVENT);
}

static void
dialog_destroy_cb (gpointer data, GObject *object)
{
  g_signal_handler_disconnect (panel_client, GPOINTER_TO_INT (data));
}

void
carrick_shell_close_dialog_on_hide (GtkDialog *dialog)
{
  guint32 signal_id;

  if (!panel_client)
    return;

  signal_id = g_signal_connect (panel_client, "hide", G_CALLBACK (panel_hide_cb), dialog);
  g_object_weak_ref (G_OBJECT (dialog), dialog_destroy_cb, GINT_TO_POINTER (signal_id));
}

#else

static GtkStatusIcon *status_icon = NULL;
static gboolean dialog_visible = FALSE;

static gboolean
popup_window (GtkWindow *window,
              guint     time)
{
  GdkRectangle   area;
  GtkOrientation orientation;
  GdkScreen     *screen;
  gboolean       res;
  int            x;
  int            y;

  screen = gtk_status_icon_get_screen (GTK_STATUS_ICON (status_icon));
  res = gtk_status_icon_get_geometry (GTK_STATUS_ICON (status_icon),
                                      &screen,
                                      &area,
                                      &orientation);
  if (! res)
    {
      g_warning ("Unable to determine geometry of status icon");
      return FALSE;
    }

  /* position roughly */
  gtk_window_set_screen (window, screen);

  x = area.x;
  y = area.y + area.height;

  gtk_window_move (window, x, y);

  gtk_window_present_with_time (window, time);

  return TRUE;
}

/* TODO: Stubs for now */
void carrick_shell_request_focus (void) {}
void carrick_shell_hide (void) {}
void carrick_shell_show (void) {}
void carrick_shell_close_dialog_on_hide (GtkDialog *dialog) {}

static void
_connection_changed_cb (CarrickPane *pane,
                        const gchar *connection_type,
                        const gchar *connection_name,
                        const gchar *state,
                        guint       strength,
                        gpointer    *data)
{
  CarrickIconState icon_state;
  const gchar      *icon_name;
  gchar            *tip;

  tip = get_tip_and_icon_state (connection_type,
                                connection_name,
                                state,
                                strength,
                                &icon_state);

  icon_name = carrick_icon_factory_get_path_for_state (icon_state);

  gtk_status_icon_set_from_file (status_icon, icon_name);

  gtk_status_icon_set_tooltip_text (status_icon, tip);

  g_free (tip);
}

static
void _activate_cb (GObject *object, gpointer user_data)
{
  if (dialog_visible)
    {
      gtk_widget_hide (GTK_WIDGET (window));
      dialog_visible = FALSE;
    }
  else
    {
      popup_window (window, GDK_CURRENT_TIME);
      dialog_visible = TRUE;
    }
}

#endif

gboolean
carrick_shell_is_visible (void)
{
#if WITH_MEEGO
  if (panel_client)
    return gtk_widget_get_visible (mpl_panel_gtk_get_window (MPL_PANEL_GTK (panel_client)));
  else
#endif
    if (window)
      return gtk_widget_get_visible (window);
    else
      return TRUE;
}

int
main (int    argc,
      char **argv)
{
  CarrickApplet *applet;
  GtkWidget     *pane;
  gboolean       standalone = FALSE;
  GdkNativeWindow windowid = 0;
  GError        *error = NULL;
  GOptionEntry   entries[] = {
    { "standalone", 's', 0, G_OPTION_ARG_NONE, &standalone,
      _("Run in standalone mode"), NULL },
    { "embed", 'w', 0, G_OPTION_ARG_INT, &windowid,
      _("Embed in another window (overrides --standalone)"), NULL },
    { NULL }
  };

  if (!g_thread_supported ())
    g_thread_init (NULL);

  bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
  textdomain (GETTEXT_PACKAGE);

  g_set_application_name (_ ("Carrick connectivity applet"));
  gtk_init_with_args (&argc, &argv, _ ("- MeeGo connectivity applet"),
                      entries, GETTEXT_PACKAGE, &error);
  dbus_g_thread_init ();

  if (error)
    {
      g_printerr ("%s\n", error->message);
      g_error_free (error);
      return 1;
    }

  applet = carrick_applet_new ();
  pane = carrick_applet_get_pane (applet);

  if (windowid) {
    window = gtk_plug_new (windowid);
    g_signal_connect (window,
                      "delete-event",
                      (GCallback) gtk_main_quit,
                                            NULL);
    gtk_container_add (GTK_CONTAINER (window),
                       pane);

    gtk_widget_show (window);
  } else if (standalone) {
    window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    g_signal_connect (window,
                      "delete-event",
                      (GCallback) gtk_main_quit,
                      NULL);
    gtk_container_add (GTK_CONTAINER (window),
                       pane);

    gtk_widget_show (window);
  } else {
#if WITH_MEEGO
    GdkScreen *screen;
    GtkWidget *box, *label;
    char *s;

    panel_client = mpl_panel_gtk_new (MPL_PANEL_NETWORK,
                                      _("network"),
                                      THEME_DIR "/network-applet.css",
                                      "offline",
                                      TRUE);
    g_signal_connect (panel_client,
                      "show",
                      (GCallback) _client_show_cb,
                      applet);
    window = mpl_panel_gtk_get_window (MPL_PANEL_GTK (panel_client));

    box = gtk_vbox_new (FALSE, 0);
    gtk_container_add (GTK_CONTAINER (window), box);

    label = gtk_label_new (NULL);
    screen = gdk_screen_get_default ();
    s = g_strdup_printf ("<span foreground=\"#31c2ee\" weight=\"bold\" size=\"%d\">%s</span>",
                         (int)(PANGO_SCALE * (22 * 72 / gdk_screen_get_resolution (screen))),
                         _("Networks"));
    g_object_set (label,
                  "label", s,
                  "use-markup", TRUE,
                  "xalign", 0.0f,
                  "xpad", 16,
                  "ypad", 8,
                  NULL);
    g_free (s);
    gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);

    gtk_box_pack_start (GTK_BOX (box), pane, TRUE, TRUE, 0);
    g_signal_connect (pane,
                      "connection-changed",
                      (GCallback) _connection_changed_cb,
                      panel_client);

    gtk_widget_show_all (box);
#else
    window = GTK_WINDOW (gtk_window_new (GTK_WINDOW_TOPLEVEL));
    gtk_window_set_decorated (GTK_WINDOW (window), FALSE);
    gtk_window_set_deletable (GTK_WINDOW (window), FALSE);
    gtk_window_set_keep_above (GTK_WINDOW (window), TRUE);
    gtk_window_set_skip_pager_hint (GTK_WINDOW (window), TRUE);
    gtk_window_set_skip_taskbar_hint (GTK_WINDOW (window), TRUE);
    gtk_container_add (GTK_CONTAINER (window), pane);

    status_icon = gtk_status_icon_new ();
    gtk_status_icon_set_visible (status_icon, TRUE);

    g_signal_connect (status_icon,
                      "activate",
                      (GCallback) _activate_cb,
                      NULL);

    g_signal_connect (pane,
                      "connection-changed",
                      (GCallback) _connection_changed_cb,
                      NULL);
#endif
  }

  gtk_main ();
}
