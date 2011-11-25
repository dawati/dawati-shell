/*
 *
 * Copyright (C) 2010 Intel Corp
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 */

#include <config.h>
#include <ctype.h>
#include <stdlib.h>
#include <signal.h>
#include <glib/gi18n-lib.h>
#include <glib/gstdio.h>
#include <glib/gprintf.h>
#include <gtk/gtk.h>
#include <mx-gtk/mx-gtk.h>
#include <libgnome-control-center-extension/mux-label.h>
#include <gconf/gconf-client.h>

#include "app-cc-panel.h"

#define KEY_DIR "/desktop/meego/panel-applications"
#define KEY_SHOW_EXPANDERS KEY_DIR "/show_expanders"

#define APP_CMD_EXPANDERS_ON "gconftool-2 --set "KEY_SHOW_EXPANDERS" --type bool true"
#define APP_CMD_EXPANDERS_OFF "gconftool-2 --set "KEY_SHOW_EXPANDERS" --type bool false"

struct _AppCcPanelPrivate
{
  GtkWidget *frame;
};

#define GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), APP_TYPE_CC_PANEL, AppCcPanelPrivate))

G_DEFINE_DYNAMIC_TYPE (AppCcPanel, app_cc_panel, CC_TYPE_PANEL);

static void
app_cc_panel_switch_flipped_cb (MxGtkLightSwitch *sw,
                                gboolean          on,
                                AppCcPanel      *panel)
{
  g_spawn_command_line_async (
                              (on ? APP_CMD_EXPANDERS_ON : APP_CMD_EXPANDERS_OFF),
                              NULL
  );
}

static void
app_cc_panel_make_contents (AppCcPanel *panel)
{
  AppCcPanelPrivate *priv = panel->priv;
  GtkWidget *vbox, *hbox, *label, *button, *frame;
  gchar *txt;
  GConfClient *gconf_client;

  frame = mx_gtk_frame_new ();
  gtk_widget_show (frame);

  vbox = gtk_vbox_new (FALSE, 15);
  gtk_widget_show (vbox);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 10);
  gtk_container_add (GTK_CONTAINER (frame), vbox);

  hbox = gtk_hbox_new (FALSE, 10);
  gtk_widget_show (hbox);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

  txt = g_strdup_printf ("<span font_desc=\"Liberation Sans Bold\""
                         "foreground=\"#3e3e3e\">%s</span>",
                         _("Use categories"));

  label = gtk_label_new (NULL);
  gtk_label_set_markup (GTK_LABEL (label), txt);

  gtk_widget_show (label);
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

  g_free (txt);

  button = mx_gtk_light_switch_new ();
  gtk_widget_show (button);
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);

  gconf_client = gconf_client_get_default();	
  mx_gtk_light_switch_set_active (MX_GTK_LIGHT_SWITCH (button),
                                  gconf_client_get_bool (gconf_client, KEY_SHOW_EXPANDERS, NULL));
  g_object_unref(gconf_client);

  g_signal_connect (button, "switch-flipped",
                    G_CALLBACK (app_cc_panel_switch_flipped_cb),
                    panel);

  label = mux_label_new ();

  mux_label_set_text (MUX_LABEL (label),
                      _("Enable/disable categories in applications panel."));

  gtk_widget_show (label);
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);

  priv->frame = frame;
}

static void
app_cc_panel_active_changed (CcPanel  *base_panel,
                             gboolean is_active)
{
  static gboolean populated = FALSE;

  if (is_active && !populated)
    {
      populated = TRUE;
    }
}

static void
app_cc_panel_init (AppCcPanel *self)
{
  self->priv = GET_PRIVATE (self);

  app_cc_panel_make_contents (self);

  gtk_widget_show (self->priv->frame);
  gtk_container_add (GTK_CONTAINER (self), self->priv->frame);
}

static GObject *
app_cc_panel_constructor (GType type,
                          guint n_construct_properties,
                          GObjectConstructParam *construct_properties)
{
  AppCcPanel *panel;

  panel = APP_CC_PANEL (G_OBJECT_CLASS (app_cc_panel_parent_class)->constructor
                        (type, n_construct_properties, construct_properties));

  g_object_set (panel,
                "display-name", _("Applications panel"),
                "id", "dawati-applications-panel-properties.desktop",
                NULL);

  return G_OBJECT (panel);
}

static void
app_cc_panel_class_init (AppCcPanelClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  CcPanelClass *panel_class = CC_PANEL_CLASS (klass);

  object_class->constructor = app_cc_panel_constructor;

  panel_class->active_changed = app_cc_panel_active_changed;

  g_type_class_add_private (klass, sizeof (AppCcPanelPrivate));
}

static void
app_cc_panel_class_finalize (AppCcPanelClass *klass)
{
}

void
app_cc_panel_register (GIOModule *module)
{
  app_cc_panel_register_type (G_TYPE_MODULE (module));
  g_io_extension_point_implement (CC_PANEL_EXTENSION_POINT_NAME,
                                  APP_TYPE_CC_PANEL,
                                  "desktopsettings",
                                  10);
}
