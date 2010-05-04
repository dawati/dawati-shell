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

#include "syst-cc-panel.h"
#define SYST_UID  {0x6aa9c364, 0x7620a0c3, 0x666f2075, 0x35392720, 0}
#define SYST_CMD "matchbox-panel --geometry=200x36+0-0 --reserve-extra-height=4 --fullscreen --start-applets=systray"

struct _SystCcPanelPrivate
{
  GtkWidget    *frame;
};

#define GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), SYST_TYPE_CC_PANEL, SystCcPanelPrivate))

G_DEFINE_DYNAMIC_TYPE (SystCcPanel, syst_cc_panel, CC_TYPE_PANEL);

static pid_t
is_panel_running (void)
{
  GError      *error = NULL;
  GDir        *proc;
  pid_t        pid = -1;
  const gchar *entry;

  if (!(proc = g_dir_open ("/proc", 0, &error)))
    {
      g_warning ("Unable to open /proc for reading: %s",
                 error ? error->message : "unknown error");
      return pid;
    }

  while ((entry = g_dir_read_name (proc)))
    {
      const char *p = entry;
      gboolean    pid_entry = TRUE;

      while (*p)
        {
          if (!isdigit (*p))
            {
              pid_entry = FALSE;
              break;
            }

          p++;
        }

      if (pid_entry)
        {
          gchar *cmdline = g_strdup_printf ("/proc/%s/cmdline", entry);
          FILE  *f;

          if ((f = g_fopen (cmdline, "r")))
            {
              char cmd[PATH_MAX];

              if (fgets (&cmd[0], sizeof (cmd), f))
                {
                  if (strstr (cmd, "matchbox-panel"))
                    pid = atoi (entry);
                }

              fclose (f);
            }

          g_free (cmdline);
        }

      if (pid > -1)
        break;
    }

  g_dir_close (proc);

  return pid;
}

static void
syst_cc_panel_switch_flipped_cb (MxGtkLightSwitch *sw,
                                 gboolean          on,
                                 SystCcPanel      *panel)
{
  static guint32  uid[] = SYST_UID;
  /* $HOME/.config/legacy-systray */
  gchar *path = g_build_filename (g_get_user_config_dir (),
                                  "legacy-systray",
                                  NULL);

  if (on)
    {
      pid_t pid;

      if (g_mkdir_with_parents (g_get_user_config_dir (), 751) < 0)
        {
          g_warning ("Could not create directory %s", g_get_user_config_dir ());
        }
      else
        {
          FILE *f = g_fopen (path, "w");

          g_fprintf (f, "%s", (char*) &uid[0]);

          fclose (f);
        }

      if ((pid = is_panel_running ()) < 0)
        {
          g_spawn_command_line_async (SYST_CMD, NULL);
        }
    }
  else
    {
      pid_t pid;

      g_remove (path);

      if ((pid = is_panel_running ()) > -1)
        {
          kill (pid, SIGKILL);
        }
    }

  g_free (path);
}

static void
syst_cc_panel_make_contents (SystCcPanel *panel)
{
  SystCcPanelPrivate *priv = panel->priv;
  GtkWidget          *vbox, *hbox, *label, *button, *frame;
  gchar              *txt;

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
                           _ ("Show system tray"));

  label = gtk_label_new (NULL);
  gtk_label_set_markup (GTK_LABEL (label), txt);

  gtk_widget_show (label);
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

  g_free (txt);

  button = mx_gtk_light_switch_new ();
  gtk_widget_show (button);
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);

  if (is_panel_running () > -1)
    mx_gtk_light_switch_set_active (MX_GTK_LIGHT_SWITCH (button), TRUE);

  g_signal_connect (button, "switch-flipped",
                    G_CALLBACK (syst_cc_panel_switch_flipped_cb),
                    panel);

  label = gtk_label_new (_("The system tray is a concept used by some older "
                           "applications to let you know when something "
                           "happens or to stay running when you close them."));
  gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_widget_show (label);
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);

  priv->frame = frame;
}

static void
syst_cc_panel_active_changed (CcPanel  *base_panel,
                              gboolean is_active)
{
  /* SystCcPanel *panel = SYST_CC_PANEL (base_panel); */
  static gboolean populated = FALSE;

  if (is_active && !populated)
    {
      populated = TRUE;
    }
}

static void
syst_cc_panel_init (SystCcPanel *self)
{
  self->priv = GET_PRIVATE (self);

  syst_cc_panel_make_contents (self);

  gtk_widget_show (self->priv->frame);
  gtk_container_add (GTK_CONTAINER (self), self->priv->frame);
}

static GObject *
syst_cc_panel_constructor (GType                  type,
                           guint                  n_construct_properties,
                           GObjectConstructParam *construct_properties)
{
  SystCcPanel *panel;

  panel = SYST_CC_PANEL (G_OBJECT_CLASS (syst_cc_panel_parent_class)->constructor
                          (type, n_construct_properties, construct_properties));

  g_object_set (panel,
                "display-name", _("System Tray"),
                "id", "system-tray-properties.desktop",
                NULL);

  return G_OBJECT (panel);
}

static void
syst_cc_panel_class_init (SystCcPanelClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  CcPanelClass *panel_class = CC_PANEL_CLASS (klass);

  object_class->constructor = syst_cc_panel_constructor;

  panel_class->active_changed = syst_cc_panel_active_changed;

  g_type_class_add_private (klass, sizeof (SystCcPanelPrivate));
}

static void
syst_cc_panel_class_finalize (SystCcPanelClass *klass)
{
}

void
syst_cc_panel_register (GIOModule *module)
{
  syst_cc_panel_register_type (G_TYPE_MODULE (module));
  g_io_extension_point_implement (CC_PANEL_EXTENSION_POINT_NAME,
                                  SYST_TYPE_CC_PANEL,
                                  "desktopsettings",
                                  10);
}
