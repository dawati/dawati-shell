
/*
 * Copyright (c) 2010 Intel Corp.
 *
 * Author: Robert Staudinger <robert.staudinger@intel.com>
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

#include <glib/gi18n.h>
#include "mpd-computer-pane.h"

G_DEFINE_TYPE (MpdComputerPane, mpd_computer_pane, MX_TYPE_BOX_LAYOUT)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MPD_TYPE_COMPUTER_PANE, MpdComputerPanePrivate))

typedef struct
{
  int dummy;
} MpdComputerPanePrivate;

static void
_settings_clicked_cb (MxButton        *button,
                      MpdComputerPane *self)
{
  GError *error = NULL;

  g_spawn_command_line_async ("gnome-control-center", &error);
  if (error)
  {
    g_warning ("%s : %s", G_STRLOC, error->message);
    g_clear_error (&error);
  }

  // TODO emit "hide"
}

static void
_dispose (GObject *object)
{
  G_OBJECT_CLASS (mpd_computer_pane_parent_class)->dispose (object);
}

static void
mpd_computer_pane_class_init (MpdComputerPaneClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MpdComputerPanePrivate));

  object_class->dispose = _dispose;
}

static void
mpd_computer_pane_init (MpdComputerPane *self)
{
  ClutterActor *label;
  ClutterActor *button;

  mx_box_layout_set_vertical (MX_BOX_LAYOUT (self), TRUE);

  label = mx_label_new (_("Your Computer"));
  clutter_container_add_actor (CLUTTER_CONTAINER (self), label);

  button = mx_button_new_with_label (_("All settings"));
  g_signal_connect (button, "clicked",
                    G_CALLBACK (_settings_clicked_cb), self);
  clutter_container_add_actor (CLUTTER_CONTAINER (self), button);
}

MpdComputerPane *
mpd_computer_pane_new (void)
{
  return g_object_new (MPD_TYPE_COMPUTER_PANE, NULL);
}


