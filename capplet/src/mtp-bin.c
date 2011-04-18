/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */

/* mtp-bin-button.c */
/*
 * Copyright (c) 2009, 2010 Intel Corp.
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

#include <config.h>
#include <glib/gi18n-lib.h>
#include <gconf/gconf-client.h>

#include "mtp-bin.h"
#include "mtp-toolbar.h"
#include "mtp-toolbar-button.h"
#include "mtp-space.h"
#include "mtp-jar.h"

#define KEY_DIR "/desktop/meego/toolbar/panels"
#define KEY_ORDER KEY_DIR "/order"

#define TOOLBAR_HEIGHT 64

G_DEFINE_TYPE (MtpBin, mtp_bin, MX_TYPE_BOX_LAYOUT);

#define MTP_BIN_GET_PRIVATE(obj)    \
        (G_TYPE_INSTANCE_GET_PRIVATE ((obj), MTP_TYPE_BIN, MtpBinPrivate))

struct _MtpBinPrivate
{
  GConfClient  *client;
  ClutterActor *toolbar;
  ClutterActor *jar;
  ClutterActor *message;

  gchar *normal_msg;
  gchar *err_msg;

  gboolean disposed : 1;
};

static void
mtp_bin_dispose (GObject *object)
{
  MtpBinPrivate *priv = MTP_BIN (object)->priv;

  if (priv->disposed)
    return;

  priv->disposed = TRUE;

  priv->toolbar = NULL;
  priv->jar = NULL;

  g_object_unref (priv->client);
  priv->client = NULL;

  G_OBJECT_CLASS (mtp_bin_parent_class)->dispose (object);
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

      for (dl = g_list_last (applets); dl; dl = dl->prev)
        {
          MtpToolbarButton *tb   = dl->data;
          const gchar      *name = mtp_toolbar_button_get_name (tb);

          g_debug ("Adding %s to list", name);
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
  else
    g_debug ("Toolbar not modified");
}

static void
mtp_bin_save_button_clicked_cb (MxButton *button, MtpBin *bin)
{
  MtpBinPrivate *priv = bin->priv;

  save_toolbar_state (priv->client, (MtpToolbar*)priv->toolbar);
}

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
  gint          i;
  const gchar  *file_name;

  /*
   * NB -- This should match the same list in mnb-toolbar.c; the problem here
   *       is that if there is some gconf screwup, we still want these on the
   *       toolbar.
   */
  const gchar  *required[4] = {"meego-panel-myzone",
                               "meego-panel-applications",
                               "meego-panel-zones",
                               "carrick"};

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

      panels = g_slist_append (panels, panel);
    }

  /*
   * Ensure that the required panels are in the list; if not, we insert them
   * at the end.
   */
  for (i = 0; i < G_N_ELEMENTS (required); ++i)
    {
      GSList *l = g_slist_find_custom (panels,
                                       required[i],
                                       (GCompareFunc)compare_panel_to_name);

      if (!l)
        {
          Panel *panel = g_slice_new0 (Panel);

          panel->name = g_strdup (required[i]);
          panel->on_toolbar = TRUE;
          panels = g_slist_append (panels, panel);
        }
    }

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
mtp_toolbar_button_remove_cb (ClutterActor *button, MtpJar *jar)
{
  /*
   * The Jar takes care of the reparent for us.
   */
  mtp_jar_add_button (jar, button);
}

void
mtp_bin_load_contents (MtpBin *bin)
{
  MtpBinPrivate *priv    = MTP_BIN (bin)->priv;
  MtpToolbar    *toolbar = (MtpToolbar*) priv->toolbar;
  MtpJar        *jar     = (MtpJar*)priv->jar;
  GSList        *l, *panels;

  g_object_set (toolbar, "drop-enabled", TRUE, NULL);
  g_object_set (jar, "drop-enabled", TRUE, NULL);

  panels = load_panels (priv->client);

  /* First, remove the spaces */
  while (mtp_toolbar_remove_space (toolbar, FALSE));

  for (l = panels; l; l = l->next)
    {
      Panel        *panel  = l->data;
      ClutterActor *button = mtp_toolbar_button_new ();

      mtp_toolbar_button_set_name ((MtpToolbarButton*)button, panel->name);

      if (!mtp_toolbar_button_is_valid ((MtpToolbarButton*)button))
        {
          clutter_actor_destroy (button);
          continue;
        }

      g_signal_connect (button, "remove",
                        G_CALLBACK (mtp_toolbar_button_remove_cb),
                        jar);

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
          if (!mtp_toolbar_button_is_required ((MtpToolbarButton*)button))
            mx_draggable_enable (MX_DRAGGABLE (button));
          else
            clutter_actor_set_opacity (button, 0x4f);
        }
      else
        {
          mtp_jar_add_button (jar, button);
          mx_draggable_enable (MX_DRAGGABLE (button));
        }
    }

  mtp_toolbar_fill_space (toolbar);

  /*
   * Now clear the modified state on the toolbar (set because we were adding
   * buttons.
   */
  mtp_toolbar_clear_modified_state (toolbar);
}

static void
mtp_bin_toolbar_free_space_cb (MtpToolbar *toolbar,
                               GParamSpec *pspec,
                               MtpBin     *self)
{
  MtpBinPrivate *priv = MTP_BIN (self)->priv;

  if (!mtp_toolbar_has_free_space (toolbar))
    mx_label_set_text (MX_LABEL (priv->message), priv->err_msg);
  else
    mx_label_set_text (MX_LABEL (priv->message), priv->normal_msg);
}

static void
mtp_bin_constructed (GObject *self)
{
  MtpBinPrivate *priv = MTP_BIN (self)->priv;
  MtpToolbar    *toolbar = (MtpToolbar*) mtp_toolbar_new ();
  ClutterActor  *box = (ClutterActor *)self;
  ClutterActor  *jar = mtp_jar_new ();
  GConfClient   *client;

  priv->toolbar = (ClutterActor*)toolbar;
  priv->jar     = jar;

  clutter_actor_set_name (jar, "jar");
  clutter_actor_set_height ((ClutterActor*)toolbar, TOOLBAR_HEIGHT);

  mx_box_layout_set_orientation (MX_BOX_LAYOUT (box), MX_ORIENTATION_VERTICAL);
  mx_box_layout_set_spacing (MX_BOX_LAYOUT (box), 10);

  clutter_container_add (CLUTTER_CONTAINER (box), (ClutterActor*)toolbar, NULL);

  {
    ClutterActor *dummy = mx_label_new ();
    ClutterActor *hbox = mx_box_layout_new ();
    ClutterActor *button = mx_button_new_with_label (_("Save toolbar"));

    clutter_actor_set_name (hbox, "message-box");
    clutter_actor_set_name (button, "save-button");

    priv->err_msg = _("Sorry, you'll have to remove a panel before you can "
                      "add a new one.");
    priv->normal_msg = _("You can add, remove, and reorder many of the panels "
                         "in your toolbar.");

    priv->message = mx_label_new_with_text (priv->normal_msg);

    clutter_actor_set_name (priv->message, "error-message");

    clutter_container_add (CLUTTER_CONTAINER (box), hbox, NULL);
    clutter_container_add (CLUTTER_CONTAINER (hbox), priv->message,
                           dummy, button, NULL);
    clutter_container_child_set (CLUTTER_CONTAINER (hbox), dummy,
                                 "expand", TRUE, NULL);
    clutter_container_child_set (CLUTTER_CONTAINER (hbox), button,
                                 "x-align", MX_ALIGN_END,
                                 "y-align", MX_ALIGN_MIDDLE, NULL);
    clutter_container_child_set (CLUTTER_CONTAINER (hbox), priv->message,
                                 "y-align", MX_ALIGN_MIDDLE, NULL);

    g_signal_connect (button, "clicked",
                      G_CALLBACK (mtp_bin_save_button_clicked_cb),
                      self);

    g_signal_connect (toolbar, "notify::free-space",
                      G_CALLBACK (mtp_bin_toolbar_free_space_cb),
                      self);
  }

  clutter_container_add (CLUTTER_CONTAINER (box), jar, NULL);

  clutter_container_child_set (CLUTTER_CONTAINER (box), jar,
                               "expand", TRUE, NULL);

  client = priv->client = gconf_client_get_default ();
  gconf_client_add_dir (client, KEY_DIR, GCONF_CLIENT_PRELOAD_RECURSIVE, NULL);
}

static void
mtp_bin_class_init (MtpBinClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MtpBinPrivate));

  object_class->constructed       = mtp_bin_constructed;
  object_class->dispose           = mtp_bin_dispose;
}

static void
mtp_bin_init (MtpBin *self)
{
  self->priv = MTP_BIN_GET_PRIVATE (self);
}

ClutterActor*
mtp_bin_new (void)
{
  return g_object_new (MTP_TYPE_BIN, NULL);
}
