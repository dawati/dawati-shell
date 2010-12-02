/*
 * Copyright (C) 2010 Intel Corporation.
 *
 * Author: Rob Bradford <rob@linux.intel.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU Lesser General Public License,
 * version 2.1, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
 * more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <config.h>
#include <glib/gi18n-lib.h>
#include "myzone-cc-panel.h"
#include <gtk/gtk.h>
#include <mx-gtk/mx-gtk.h>
#include <gconf/gconf-client.h>
#include "gconf-bridge.h"
#include <gio/gdesktopappinfo.h>

#define MYZONE_KEY_DIR "/desktop/meego/myzone"
#define BG_KEY_BG_FILENAME MYZONE_KEY_DIR "/background_filename"
#define MYZONE_RATIO MYZONE_KEY_DIR "/ratio"
#define MEEGO_MYZONE_SHOW_CALENDAR MYZONE_KEY_DIR "/show_calendar"
#define MEEGO_MYZONE_SHOW_EMAIL MYZONE_KEY_DIR "/show_email"
#define UI_FILE UI_DIR "/capplet.ui"

G_DEFINE_DYNAMIC_TYPE (MyzoneCcPanel, myzone_cc_panel, CC_TYPE_PANEL)

static GConfBridge *bridge;
static GtkBuilder *builder;

static gchar *
ratio_slider_format_value_cb (GtkScale *scale,
                              double    value,
                              gpointer  userdata)
{
  if (value == 0.0)
    return g_strdup (_("Show as much web service content as possible"));
  else if (value > 0.0 && value < 0.50)
    return g_strdup (_("Show mostly web service content"));
  else if (value == 0.50)
    return g_strdup (_("Show half and half"));
  else if (value > 0.50 && value < 1.0)
    return g_strdup (_("Show mostly recent files"));
  else if (value == 1.0)
    return g_strdup (_("Show as many recent files as possible"));
  else
    return g_strdup ("");
}

static void
clear_recent_files_button_clicked_cb (GtkButton *button,
                                      gpointer   userdata)
{
  GtkRecentManager *manager;
  GtkWidget *dialog;
  GtkWindow *window;
  gint res;

  window = (GtkWindow *)gtk_widget_get_toplevel (button);
  if (!GTK_WIDGET_TOPLEVEL (window))
    window = NULL;

  dialog = gtk_message_dialog_new (window,
                                   GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                   GTK_MESSAGE_OTHER,
                                   GTK_BUTTONS_YES_NO,
                                   _("Do you want to clear your recent documents? "
                                     "This will remove the local content from your myzone"));
  res = gtk_dialog_run (dialog);

  if (res == GTK_RESPONSE_YES)
  {
    manager = gtk_recent_manager_get_default ();
    gtk_recent_manager_purge_items (manager, NULL);
  }

  gtk_widget_destroy (dialog);
}

static void
change_web_accounts_button_clicked_cb (GtkButton *button,
                                       gpointer   userdata)
{
  GDesktopAppInfo *app_info;
  gchar *args[3] = { NULL, };
  GError *error = NULL;

  app_info = g_desktop_app_info_new ("gnome-control-center.desktop");
  args[0] = g_app_info_get_commandline (G_APP_INFO (app_info));
  args[1] = "bisho.desktop";
  args[2] = NULL;

  if (!g_spawn_async (NULL,
                      args,
                      NULL,
                      G_SPAWN_SEARCH_PATH,
                      NULL,
                      NULL,
                      NULL,
                      &error))
  {
    g_warning (G_STRLOC ": Error starting control center for bisho: %s",
               error->message);
    g_clear_error (&error);
  }

  g_object_unref (app_info);
}

static void
background_chooser_button_file_set_cb (GtkFileChooserButton *button,
                                       gpointer              userdata)
{
  gchar *filename;

  filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (button));

  if (filename)
  {
    gconf_client_set_string (gconf_bridge_get_client (bridge),
                             BG_KEY_BG_FILENAME,
                             filename,
                             NULL);
  } else {
    gconf_client_unset (gconf_bridge_get_client (bridge),
                        BG_KEY_BG_FILENAME,
                        NULL);
  }

  g_free (filename);
}

static void
reset_background_button_clicked_cb (GtkButton *button,
                                    gpointer   userdata)
{
  GtkWidget *chooser_button;

  chooser_button = (GtkWidget *)gtk_builder_get_object (builder,
                                                        "background_chooser_button");
  gtk_file_chooser_set_filename (GTK_FILE_CHOOSER (chooser_button),
                                 NULL);
  gconf_client_unset (gconf_bridge_get_client (bridge),
                      BG_KEY_BG_FILENAME,
                      NULL);
}

static GObject *
myzone_cc_panel_constructor (GType                 type,
                            guint                  n_construct_properties,
                            GObjectConstructParam *construct_properties)
{
  MyzoneCcPanel *panel;

  panel = MYZONE_CC_PANEL (G_OBJECT_CLASS (myzone_cc_panel_parent_class)->constructor
                           (type, n_construct_properties, construct_properties));

  g_object_set (panel,
                "display-name", _("Myzone"),
                "id", "myzone.desktop",
                NULL);

  return G_OBJECT (panel);
}


static void
myzone_cc_panel_class_init (MyzoneCcPanelClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  /* CcPanelClass *panel_class = CC_PANEL_CLASS (klass); */

  object_class->constructor = myzone_cc_panel_constructor;
}

static void
myzone_cc_panel_class_finalize (MyzoneCcPanelClass *klass)
{
}

static void
update_preview_cb (GtkFileChooser *file_chooser,
                   gpointer        data)
{
  GtkWidget *preview;
  char *filename;
  GdkPixbuf *pixbuf = NULL;
  gboolean have_preview = TRUE;

  preview = GTK_WIDGET (data);
  filename = gtk_file_chooser_get_preview_filename (file_chooser);

  if (filename)
    pixbuf = gdk_pixbuf_new_from_file_at_size (filename, 128, 128, NULL);

  if (!pixbuf)
    have_preview = FALSE;

  g_free (filename);
  gtk_image_set_from_pixbuf (GTK_IMAGE (preview), pixbuf);

  if (pixbuf)
    g_object_unref (pixbuf);

  gtk_file_chooser_set_preview_widget_active (file_chooser, have_preview);
}

static void
myzone_cc_panel_init (MyzoneCcPanel *self)
{
  GtkWidget *main_vbox;
  GConfClient *client;

  bridge = gconf_bridge_get ();
  client = gconf_bridge_get_client (bridge);

  bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");

  builder = gtk_builder_new ();
  gtk_builder_set_translation_domain (builder, GETTEXT_PACKAGE);
  gtk_builder_add_from_file (builder,
                             UI_FILE,
                             NULL);

  main_vbox = (GtkWidget *)gtk_builder_get_object (builder, "main_vbox");

  gconf_client_add_dir (client,
                        MYZONE_KEY_DIR,
                        GCONF_CLIENT_PRELOAD_ONELEVEL,
                        NULL);

  {
    GtkAdjustment *adj;
    adj = gtk_range_get_adjustment ((GtkRange *)gtk_builder_get_object (builder,
                                                                        "ratio_slider"));
    g_signal_connect (gtk_builder_get_object (builder, "ratio_slider"),
                      "format-value",
                      (GCallback)ratio_slider_format_value_cb,
                      NULL);
    gconf_bridge_bind_property_full (bridge,
                                     MYZONE_RATIO,
                                     (GObject *)adj,
                                     "value",
                                     FALSE);
  }

  {
    GtkWidget *toggle;

    toggle = mx_gtk_light_switch_new ();
    gtk_box_pack_start (GTK_BOX (gtk_builder_get_object (builder,
                                                         "calendar_button_hbox")),
                        toggle,
                        FALSE,
                        FALSE,
                        2);
    gconf_bridge_bind_property_full (bridge,
                                     MEEGO_MYZONE_SHOW_CALENDAR,
                                     (GObject *)toggle,
                                     "active",
                                     FALSE);
  }

  {
    GtkWidget *toggle;

    toggle = mx_gtk_light_switch_new ();
    gtk_box_pack_start (GTK_BOX (gtk_builder_get_object (builder,
                                                         "email_button_hbox")),
                        toggle,
                        FALSE,
                        FALSE,
                        2);
    gconf_bridge_bind_property_full (bridge,
                                     MEEGO_MYZONE_SHOW_EMAIL,
                                     (GObject *)toggle,
                                     "active",
                                     FALSE);
  }

  {
    GtkWidget *button;

    button = (GtkWidget *)gtk_builder_get_object (builder,
                                                  "clear_recent_files_button");

    g_signal_connect (button,
                      "clicked",
                      (GCallback)clear_recent_files_button_clicked_cb,
                      NULL);
  }

  {
    GtkWidget *button;

    button = (GtkWidget *)gtk_builder_get_object (builder,
                                                  "change_web_accounts_button");

    g_signal_connect (button,
                      "clicked",
                      (GCallback)change_web_accounts_button_clicked_cb,
                      NULL);
  }

  {
    GtkWidget *button;

    button = (GtkWidget *)gtk_builder_get_object (builder,
                                                  "reset_background_button");

    g_signal_connect (button,
                      "clicked",
                      (GCallback)reset_background_button_clicked_cb,
                      NULL);
  }

  {
    GtkWidget *preview;
    GtkWidget *button;
    const gchar *pictures_dir;
    gchar *current_filename;

    button = (GtkWidget *)gtk_builder_get_object (builder, "background_chooser_button");
    preview = gtk_image_new ();
    gtk_file_chooser_set_preview_widget (GTK_FILE_CHOOSER (button), preview);

    pictures_dir = g_get_user_special_dir (G_USER_DIRECTORY_PICTURES);

    gtk_file_chooser_add_shortcut_folder (GTK_FILE_CHOOSER (button),
                                          pictures_dir,
                                          NULL);
    current_filename = gconf_client_get_string (gconf_bridge_get_client (bridge),
                                                BG_KEY_BG_FILENAME,
                                                NULL);
    if (!g_str_has_prefix (current_filename, "/usr"))
    {
      gtk_file_chooser_set_filename (GTK_FILE_CHOOSER (button), current_filename);
    } else {
      gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (button),
                                           pictures_dir);
    }

    g_free (current_filename);

    g_signal_connect (button,
                      "file-set",
                      (GCallback)background_chooser_button_file_set_cb,
                      NULL);

    g_signal_connect (button,
                      "update-preview",
                      G_CALLBACK (update_preview_cb),
                      preview);
  }


  gtk_widget_show (main_vbox);
  gtk_container_add (GTK_CONTAINER (self), main_vbox);
}

void
g_io_module_load (GIOModule *module)
{
  bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");

  myzone_cc_panel_register_type (G_TYPE_MODULE (module));
  g_io_extension_point_implement (CC_PANEL_EXTENSION_POINT_NAME,
                                  MYZONE_TYPE_CC_PANEL,
                                  "myzone",
                                  10);
}

void
g_io_module_unload (GIOModule *module)
{
}
