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

static void
stage_size_allocation_changed_cb (ClutterActor           *stage,
                                  ClutterActorBox        *allocation,
                                  ClutterAllocationFlags  flags,
                                  ClutterActor           *box)
{
  gfloat width  = allocation->x2 - allocation->x1;
  gfloat height = allocation->y2 - allocation->y1;

  clutter_actor_set_size (box, width, height);
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

  if (socket_id)
    gtk_clutter_init (&argc, &argv);
  else
    clutter_init (&argc, &argv);

  mx_style_load_from_file (mx_style_get_default (),
                           THEMEDIR "/moblin-toolbar-properties.css",
                           &error);

  if (error)
    {
      g_warning ("Could not load stylesheet" THEMEDIR
                 "/moblin-toolbar-properties.css: %s", error->message);
      g_clear_error (&error);
    }

  bin = mtp_bin_new ();

  if (socket_id)
    {
      GtkWidget *window, *embed;

      window = gtk_plug_new ((Window) socket_id);
      embed = gtk_clutter_embed_new ();
      gtk_container_add (GTK_CONTAINER (window), embed);

      stage = gtk_clutter_embed_get_stage (GTK_CLUTTER_EMBED (embed));

      g_signal_connect (embed, "size-allocate",
                        G_CALLBACK (embed_size_allocate_cb),
                        bin);

      gtk_window_set_default_icon_name ("moblin-toolbar");
      gtk_window_set_icon_name (GTK_WINDOW (window), "moblin-toolbar");

      gtk_widget_show_all (window);

      g_signal_connect (window, "delete-event",
                        G_CALLBACK (window_delete_event_cb), NULL);
    }
  else
    {
      stage = clutter_stage_get_default ();

      clutter_actor_set_size (stage, 1024.0, 400.0);

      g_signal_connect (stage, "allocation-changed",
                        G_CALLBACK (stage_size_allocation_changed_cb),
                        bin);
    }

  clutter_stage_set_color (CLUTTER_STAGE (stage), &clr);
  clutter_container_add_actor (CLUTTER_CONTAINER (stage), bin);

  mtp_bin_load_contents ((MtpBin *)bin);

  if (socket_id)
    gtk_main ();
  else
    {
      clutter_actor_show (stage);
      clutter_main ();
    }

  return 0;
}
