/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */

/*
 * Copyright (c) 2010 Intel Corp.
 *
 * Author: Tomas Frydrych <tf@linux.intel.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 */

/*
 * Toolbar applet for gnome-control-center
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>

#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <clutter-gtk/clutter-gtk.h>

#include "mtp-bin.h"

static void
embed_size_allocate_cb (GtkWidget     *embed,
                        GtkAllocation *allocation,
                        ClutterActor  *box)
{
  clutter_actor_set_size (box, allocation->width, allocation->height);
}

static gboolean
window_delete_event_cb (GtkWidget *widget, GdkEvent *event, gpointer data)
{
  gtk_main_quit ();

  return FALSE;
}

int
main (int argc, char **argv)
{
  GtkWidget      *window, *embed;
  ClutterActor   *stage;
  ClutterActor   *bin;
  ClutterColor    clr = {0xff, 0xff, 0xff, 0xff};
  gint            socket_id = 0;
  GError         *error = NULL;
  GOptionContext *context;
  GOptionEntry    cap_options[] =
    {
      /*
       * We support --socket / -s option to allow embedding in the new control
       * center.
       */
      {"socket", 's', G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_INT, &socket_id,
       N_("Socket for embedding"), N_("socket") },
      {NULL}
    };

#ifdef ENABLE_NLS
  bindtextdomain (GETTEXT_PACKAGE, PLUGIN_LOCALEDIR);
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
  textdomain (GETTEXT_PACKAGE);
#endif

  context = g_option_context_new (_("- Moblin Toolbar Preferences"));

  g_option_context_add_main_entries (context, cap_options, GETTEXT_PACKAGE);
  g_option_context_set_translation_domain (context, GETTEXT_PACKAGE);
  g_option_context_add_group (context, gtk_get_option_group (TRUE));

  if (!g_option_context_parse (context, &argc, &argv, &error))
    {
      g_printerr ("%s\n", error->message);
      exit (1);
    }

  gtk_clutter_init (&argc, &argv);

  mx_style_load_from_file (mx_style_get_default (),
                           THEMEDIR "/moblin-toolbar-properties.css",
                           &error);

  if (error)
    {
      g_warning ("Could not load stylesheet" THEMEDIR
                 "/moblin-toolbar-properties.css: %s", error->message);
      g_clear_error (&error);
    }

  if (socket_id)
    {
      window = gtk_plug_new ((Window) socket_id);
    }
  else
    {
      window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
      gtk_window_set_default_size (GTK_WINDOW (window), 1000, 500);
    }

  embed = gtk_clutter_embed_new ();
  gtk_container_add (GTK_CONTAINER (window), embed);

  stage = gtk_clutter_embed_get_stage (GTK_CLUTTER_EMBED (embed));
  clutter_stage_set_color (CLUTTER_STAGE (stage), &clr);

  bin = mtp_bin_new ();
  clutter_container_add_actor (CLUTTER_CONTAINER (stage), bin);

  mtp_bin_load_contents ((MtpBin *)bin);

  g_signal_connect (embed, "size-allocate",
                    G_CALLBACK (embed_size_allocate_cb),
                    bin);

  gtk_window_set_default_icon_name ("moblin-toolbar");
  gtk_window_set_icon_name (GTK_WINDOW (window), "moblin-toolbar");

  gtk_widget_show_all (window);

  g_signal_connect (window, "delete-event",
                    G_CALLBACK (window_delete_event_cb), NULL);

  gtk_main ();

  return 0;
}
