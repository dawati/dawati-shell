/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */

/* mpl-panel-windowless.c */
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

#include <gdk/gdkx.h>
#include <math.h>

#include "mpl-panel-windowless.h"

/**
 * SECTION:mpl-panel-windowless
 * @short_description: Class for panels with passive button
 * @title: MplPanelWindowless
 *
 * #MplPanelWindowless is a class for all Panels that use a non-interactive
 * Toolbar button and have no drop-down panel.
 *
 * NB: Since the button is not interactive, windowless panels need to be started
 * through some other mechanism than the user first clicking the associated
 * button (e.g., using the system autostart mechanism).
 *
 * A minimal Panel implementation using #MplPanelWindowless would look as
 * follows:
 *
 * <programlisting>
  #include <meego-panel/mpl-panel-windowless.h>

  int
  main (int argc, char **argv)
  {
    MplPanelClient *client;
    GMainLoop      *loop;

    loop = g_main_loop_new (NULL, FALSE);

    client = mpl_panel_windowless_new ("mypanel",
                                       "this is mypanel",
                                       "somewhere/mypanel-button.css",
                                       "mypanel-button",
                                       FALSE);

    / * do something that justifies having a Toolbar button * /
    setup_my_stuff (client);

    g_main_loop_run (loop);
    g_main_loop_unref (loop);

    return 0;
  }
</programlisting>
 */

G_DEFINE_TYPE (MplPanelWindowless, mpl_panel_windowless, MPL_TYPE_PANEL_CLIENT)

#define MPL_PANEL_WINDOWLESS_GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MPL_TYPE_PANEL_WINDOWLESS, MplPanelWindowlessPrivate))

enum
{
  PROP_0,
};

struct _MplPanelWindowlessPrivate
{
  gboolean disposed : 1;
};

static void
mpl_panel_windowless_dispose (GObject *self)
{
  MplPanelWindowlessPrivate *priv = MPL_PANEL_WINDOWLESS (self)->priv;

  if (priv->disposed)
    return;

  priv->disposed = TRUE;

  G_OBJECT_CLASS (mpl_panel_windowless_parent_class)->dispose (self);
}

static void
mpl_panel_windowless_finalize (GObject *object)
{
  G_OBJECT_CLASS (mpl_panel_windowless_parent_class)->finalize (object);
}

static void
mpl_panel_windowless_set_size (MplPanelClient *self, guint width, guint height)
{
  MplPanelClientClass *p_class;

  p_class = MPL_PANEL_CLIENT_CLASS (mpl_panel_windowless_parent_class);

  if (p_class->set_size)
    p_class->set_size (self, width, height);
}

static void
mpl_panel_windowless_set_position (MplPanelClient *self, gint x, gint y)
{
  MplPanelClientClass *p_class;

  p_class = MPL_PANEL_CLIENT_CLASS (mpl_panel_windowless_parent_class);

  if (p_class->set_position)
    p_class->set_position (self, x, y);
}

static void
mpl_panel_windowless_show (MplPanelClient *self)
{
}

static void
mpl_panel_windowless_hide (MplPanelClient *self)
{
}

static void
mpl_panel_windowless_unload (MplPanelClient *panel)
{
  g_main_loop_quit (NULL);
}

static void
mpl_panel_windowless_class_init (MplPanelWindowlessClass *klass)
{
  GObjectClass        *object_class = G_OBJECT_CLASS (klass);
  MplPanelClientClass *client_class = MPL_PANEL_CLIENT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MplPanelWindowlessPrivate));

  object_class->dispose          = mpl_panel_windowless_dispose;
  object_class->finalize         = mpl_panel_windowless_finalize;

  client_class->set_position     = mpl_panel_windowless_set_position;
  client_class->set_size         = mpl_panel_windowless_set_size;
  client_class->show             = mpl_panel_windowless_show;
  client_class->hide             = mpl_panel_windowless_hide;
  client_class->unload           = mpl_panel_windowless_unload;
}

static void
mpl_panel_windowless_init (MplPanelWindowless *self)
{
  MplPanelWindowlessPrivate *priv;

  priv = self->priv = MPL_PANEL_WINDOWLESS_GET_PRIVATE (self);
}

/**
 * mpl_panel_windowless_new:
 * @name: canonical name of the panel
 * @tooltip: tooltip for the associated Toolbar button
 * @stylesheet: stylesheet for the associated Toolbar button
 * @button_style: css style id for the button style
 * @with_toolbar_service: whether the panel will be using any Toolbar services
 * (e.g., the launching API)
 *
 * Constructs a new #MplPanelWindowless object.
 *
 * Return value: new #MplPanelClient object.
 */
MplPanelClient *
mpl_panel_windowless_new (const gchar *name,
                          const gchar *tooltip,
                          const gchar *stylesheet,
                          const gchar *button_style,
                          gboolean     with_toolbar_service)
{
  MplPanelClient *panel = g_object_new (MPL_TYPE_PANEL_WINDOWLESS,
                                        "name",            name,
                                        "tooltip",         tooltip,
                                        "stylesheet",      stylesheet,
                                        "button-style",    button_style,
                                        "toolbar-service", with_toolbar_service,
                                        "windowless",      TRUE,
                                        NULL);

  return panel;
}

