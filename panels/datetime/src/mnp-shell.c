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

#include <glib/gi18n.h>


#include <moblin-panel/mpl-panel-clutter.h>
#include <moblin-panel/mpl-panel-common.h>
#include <moblin-panel/mpl-entry.h>

#include "mnp-shell.h"
#include "mnp-datetime.h"


G_DEFINE_TYPE (MnpShell, mnp_shell, MX_TYPE_BOX_LAYOUT)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MNP_TYPE_SHELL, MnpShellPrivate))

struct _MnpShellPrivate {
	MplPanelClient *panel_client;

	ClutterActor *label;
	ClutterActor *datetime;
};

static void
mnp_shell_dispose (GObject *object)
{
  MnpShellPrivate *priv = GET_PRIVATE (object);

  if (priv->panel_client)
  {
    g_object_unref (priv->panel_client);
    priv->panel_client = NULL;
  }


  G_OBJECT_CLASS (mnp_shell_parent_class)->dispose (object);
}

static void
mnp_shell_finalize (GObject *object)
{
  G_OBJECT_CLASS (mnp_shell_parent_class)->finalize (object);
}

static void
mnp_shell_class_init (MnpShellClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MnpShellPrivate));

  object_class->dispose = mnp_shell_dispose;
  object_class->finalize = mnp_shell_finalize;
}


static void
mnp_shell_init (MnpShell *self)
{
  /* MnpShellPrivate *priv = GET_PRIVATE (self); */

}

static void
mnp_shell_construct (MnpShell *shell)
{
  MnpShellPrivate *priv = GET_PRIVATE (shell); 
  	
  mx_box_layout_set_orientation ((MxBoxLayout *)shell, MX_ORIENTATION_VERTICAL);
  clutter_actor_set_name ((ClutterActor *)shell, "datetime-panel");
  mx_box_layout_set_enable_animations ((MxBoxLayout *)shell, TRUE);
  
  priv->datetime = mnp_datetime_new ();

  priv->label = mx_label_new_with_text ("Time and Date");
  clutter_actor_set_name (priv->label, "DateHeading");
  mnp_date_time_set_date_label ((MnpDatetime *)priv->datetime, priv->label);

  mx_box_layout_add_actor ((MxBoxLayout *)shell, priv->label, 1);
  clutter_container_child_set (CLUTTER_CONTAINER (shell),
                               (ClutterActor *)priv->label,
                               "expand", FALSE,
			       "y-fill", FALSE,
			       "x-fill", TRUE,
                               NULL);  
  mx_box_layout_add_actor ((MxBoxLayout *)shell, priv->datetime, 2);
  clutter_container_child_set (CLUTTER_CONTAINER (shell),
                               (ClutterActor *)priv->datetime,
                               "expand", TRUE,
			       "y-fill", TRUE,
			       "x-fill", TRUE,
                               NULL);

}

ClutterActor *
mnp_shell_new (void)
{
  MnpShell *panel = g_object_new (MNP_TYPE_SHELL, NULL);

  mnp_shell_construct (panel);

  return (ClutterActor *)panel;
}

static void
dropdown_show_cb (MplPanelClient *client,
                  gpointer        userdata)
{
 /* MnpShellPrivate *priv = GET_PRIVATE (userdata); */

}


static void
dropdown_hide_cb (MplPanelClient *client,
                  gpointer        userdata)
{
 /* MnpShellPrivate *priv = GET_PRIVATE (userdata); */

}

void
mnp_shell_set_panel_client (MnpShell *shell,
                                   MplPanelClient *panel_client)
{
  MnpShellPrivate *priv = GET_PRIVATE (shell);

  priv->panel_client = g_object_ref (panel_client);

  mnp_datetime_set_panel_client ((MnpDatetime *)priv->datetime, panel_client);

  g_signal_connect (panel_client,
                    "show-end",
                    (GCallback)dropdown_show_cb,
                    shell);

  g_signal_connect (panel_client,
                    "hide-end",
                    (GCallback)dropdown_hide_cb,
                    shell);
}


