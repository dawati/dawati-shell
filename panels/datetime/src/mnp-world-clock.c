/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */

/*
 * Copyright (C) 2010 Intel Corporation.
 *
 * Author: Srinivasa Ragavan <srini@linux.intel.com>
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
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <gtk/gtk.h>
#include <gio/gdesktopappinfo.h>
#include <glib/gi18n.h>


#include <moblin-panel/mpl-panel-clutter.h>
#include <moblin-panel/mpl-panel-common.h>
#include <moblin-panel/mpl-entry.h>

#include "mnp-world-clock.h"

G_DEFINE_TYPE (MnpWorldClock, mnp_world_clock, MX_TYPE_TABLE)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MNB_TYPE_WORLD_CLOCK, MnpWorldClockPrivate))

#define TIMEOUT 250


typedef struct _MnpWorldClockPrivate MnpWorldClockPrivate;

struct _MnpWorldClockPrivate {

};

static void
mnp_world_clock_dispose (GObject *object)
{
  MnpWorldClockPrivate *priv = GET_PRIVATE (object);

  if (priv->panel_client)
  {
    g_object_unref (priv->panel_client);
    priv->panel_client = NULL;
  }

  G_OBJECT_CLASS (mnp_world_clock_parent_class)->dispose (object);
}

static void
mnp_world_clock_finalize (GObject *object)
{
  G_OBJECT_CLASS (mnp_world_clock_parent_class)->finalize (object);
}

static void
mnp_world_clock_class_init (MnpWorldClockClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MnpWorldClockPrivate));

  object_class->dispose = mnp_world_clock_dispose;
  object_class->finalize = mnp_world_clock_finalize;
}


static void
mnp_world_clock_init (MnpWorldClock *self)
{
  MnpWorldClockPrivate *priv = GET_PRIVATE (self);

}

static void
mnp_world_clock_construct (void)
{

}

ClutterActor *
mnp_world_clock_new (void)
{
  MnpWorldClock *panel = g_object_new (MNP_TYPE_WORLD_CLOCK, NULL);

  mnp_world_clock_construct ();

  return (ClutterActor *)panel;
}

static void
dropdown_show_cb (MplPanelClient *client,
                  gpointer        userdata)
{
  MnpWorldClockPrivate *priv = GET_PRIVATE (userdata);

  /* give focus to the actor */
  /*clutter_actor_grab_key_focus (priv->entry);*/
}


static void
dropdown_hide_cb (MplPanelClient *client,
                  gpointer        userdata)
{
  MnpWorldClockPrivate *priv = GET_PRIVATE (userdata);

  /* Reset search. */
  /* mpl_entry_set_text (MPL_ENTRY (priv->entry), ""); */
}

void
mnp_world_clock_set_panel_client (MnpWorldClock *world_clock,
                                   MplPanelClient *panel_client)
{
  MnpWorldClockPrivate *priv = GET_PRIVATE (world_clock);

  priv->panel_client = g_object_ref (panel_client);

  g_signal_connect (panel_client,
                    "show-end",
                    (GCallback)dropdown_show_cb,
                    world_clock);

  g_signal_connect (panel_client,
                    "hide-end",
                    (GCallback)dropdown_hide_cb,
                    world_clock);
}


