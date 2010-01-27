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
#include <gconf/gconf-client.h>
#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <clutter-gtk/clutter-gtk.h>

#include "mtp-jar.h"
#include "mtp-toolbar.h"
#include "mtp-toolbar-button.h"

#define KEY_DIR "/desktop/moblin/toolbar/panels"
#define KEY_ORDER KEY_DIR "/order"

#define TOOLBAR_HEIGHT 64

typedef struct _Panel
{
  gchar    *name;
  gboolean  on_toolbar : 1;
} Panel;

static gint
compare_panel_to_name (gconstpointer p, gconstpointer n)
{
  const Panel *panel = p;
  const gchar *name  = n;

  g_assert (panel && panel->name && n);

  return strcmp (panel->name, name);
}

static GSList *
load_panels (GConfClient *client)
{
  GSList       *order, *l;
  GDir         *panels_dir;
  GError       *error = NULL;
  GSList       *panels = NULL;
  const gchar  *file_name;

  /*
   * First load the panels that should be on the toolbar from gconf prefs.
   */
  order = gconf_client_get_list (client, KEY_ORDER, GCONF_VALUE_STRING, NULL);

  for (l = order; l; l = l->next)
    {
      Panel *panel = g_slice_new0 (Panel);

      /* steal the value here */
      panel->name = l->data;
      panel->on_toolbar = TRUE;

      panels = g_slist_prepend (panels, panel);
    }

  panels = g_slist_reverse (panels);

  g_slist_free (order);

  /*
   * Now load panels that are available but not included in the above
   */
  if (!(panels_dir = g_dir_open (PANELSDIR, 0, &error)))
    {
      if (error)
        {
          g_warning (G_STRLOC ": Failed to open directory '%s': %s",
                     PANELSDIR, error->message);

          g_clear_error (&error);
        }

      return panels;
    }

  while ((file_name = g_dir_read_name (panels_dir)))
    {
      if (g_str_has_suffix (file_name, ".desktop"))
        {
          gchar *name = g_strdup (file_name);

          name[strlen(name) - strlen (".desktop")] = 0;

          /*
           * Check it's not one of those we already got.
           */
          if (!g_slist_find_custom (panels,
                                    name,
                                    compare_panel_to_name))
            {
              Panel *panel = g_slice_new0 (Panel);

              panel->name = name;

              panels = g_slist_append (panels, panel);
            }
        }
    }

  g_dir_close (panels_dir);

  return panels;
}

static void
embed_size_allocate_cb (GtkWidget     *embed,
                        GtkAllocation *allocation,
                        ClutterActor  *box)
{
  clutter_actor_set_size (box, allocation->width, allocation->height);
}

static void
save_toolbar_state (GConfClient *client, MtpToolbar *toolbar)
{
  GError *error = NULL;

  if (mtp_toolbar_was_modified (MTP_TOOLBAR (toolbar)))
    {
      GList  *panels  = mtp_toolbar_get_panel_buttons (MTP_TOOLBAR (toolbar));
      GList  *applets = mtp_toolbar_get_applet_buttons (MTP_TOOLBAR (toolbar));
      GSList *children = NULL;
      GList  *dl;

      for (dl = panels; dl; dl = dl->next)
        {
          MtpToolbarButton *tb   = dl->data;
          const gchar      *name = mtp_toolbar_button_get_name (tb);

          children = g_slist_prepend (children, (gchar*)name);
        }

      for (dl = applets; dl; dl = dl->next)
        {
          MtpToolbarButton *tb   = dl->data;
          const gchar      *name = mtp_toolbar_button_get_name (tb);

          children = g_slist_prepend (children, (gchar*)name);
        }

      children = g_slist_reverse (children);

      if (!gconf_client_set_list (client, KEY_ORDER, GCONF_VALUE_STRING,
                                  children, &error))
        {
          g_warning ("Failed to set key " KEY_ORDER ": %s",
                     error ? error->message : "unknown error");

          g_clear_error (&error);
        }

      g_list_free (panels);
      g_list_free (applets);
      g_slist_free (children);
    }
}

typedef struct _DeleteData
{
  MtpToolbar  *toolbar;
  GConfClient *client;

} DeleteData;

static gboolean
window_delete_event_cb (GtkWidget *widget, GdkEvent *event, gpointer data)
{
  DeleteData *ddata = data;

  save_toolbar_state (ddata->client, ddata->toolbar);

  gtk_main_quit ();

  return FALSE;
}

static void
construct_contents (GtkWidget    *embed,
                    ClutterActor *stage,
                    MtpToolbar   *toolbar,
                    GSList       *panels)
{
  ClutterActor *box = mx_box_layout_new ();
  ClutterActor *jar = mtp_jar_new ();
  GSList       *l;

  clutter_actor_set_name (jar, "jar");
  clutter_actor_set_height ((ClutterActor*)toolbar, TOOLBAR_HEIGHT);
  mx_box_layout_set_vertical (MX_BOX_LAYOUT (box), TRUE);
  clutter_container_add (CLUTTER_CONTAINER (box),
                         (ClutterActor*)toolbar, jar, NULL);

  clutter_container_child_set (CLUTTER_CONTAINER (box), jar,
                               "expand", TRUE, NULL);

  clutter_container_add_actor (CLUTTER_CONTAINER (stage), box);

  g_signal_connect (embed, "size-allocate",
                    G_CALLBACK (embed_size_allocate_cb),
                    box);

  g_object_set (toolbar, "enabled", TRUE, NULL);
  g_object_set (jar, "enabled", TRUE, NULL);

  for (l = panels; l; l = l->next)
    {
      Panel            *panel = l->data;
      MtpToolbarButton *button = (MtpToolbarButton*)mtp_toolbar_button_new ();

      mtp_toolbar_button_set_name (button, panel->name);

      if (!mtp_toolbar_button_is_valid (button))
        {
          clutter_actor_destroy ((ClutterActor*)button);
          continue;
        }

      if (panel->on_toolbar)
        {
          mtp_toolbar_add_button (toolbar, button);

          /*
           * Only enable d&d for buttons that are not marked as required.
           * Make the require buttons semi-translucent to indicate they are
           * disabled.
           *
           * TODO -- we might want to do something fancier here, like handling
           * this in the droppable, so that the user can still drag these
           * on the toolbar, but for now this should do.
           */
          if (!mtp_toolbar_button_is_required (button))
            mx_draggable_enable (MX_DRAGGABLE (button));
          else
            clutter_actor_set_opacity ((ClutterActor*)button, 0x7f);
        }
      else
        {
          mtp_jar_add_button ((MtpJar*)jar, button);
          mx_draggable_enable (MX_DRAGGABLE (button));
        }

    }
}

int
main (int argc, char **argv)
{
  GConfClient    *client;
  GtkWidget      *window, *embed;
  ClutterActor   *stage;
  MtpToolbar     *toolbar;
  ClutterColor    clr = {0xff, 0xff, 0xff, 0xff};
  gint            socket_id = 0;
  GError         *error = NULL;
  GSList         *panels;
  DeleteData      ddata;
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

  client = gconf_client_get_default ();
  gconf_client_add_dir (client, KEY_DIR, GCONF_CLIENT_PRELOAD_RECURSIVE, NULL);

  panels = load_panels (client);

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

  toolbar = (MtpToolbar*) mtp_toolbar_new ();

  construct_contents (embed, stage, toolbar, panels);

  gtk_window_set_default_icon_name ("moblin-toolbar");
  gtk_window_set_icon_name (GTK_WINDOW (window), "moblin-toolbar");

  gtk_widget_show_all (window);

  ddata.client = client;
  ddata.toolbar = toolbar;

  g_signal_connect (window, "delete-event",
                    G_CALLBACK (window_delete_event_cb), &ddata);


  gtk_main ();

  g_object_unref (client);

  return 0;
}
