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
#include <glib/gi18n-lib.h>
#include <gtk/gtk.h>
#include <clutter-gtk/clutter-gtk.h>
#include "mtp-cc-panel.h"
#include "mtp-bin.h"

struct _MtpCcPanelPrivate
{
  GtkWidget    *frame;
  ClutterActor *bin;
};

#define GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), MTP_TYPE_CC_PANEL, MtpCcPanelPrivate))

G_DEFINE_DYNAMIC_TYPE (MtpCcPanel, mtp_cc_panel, CC_TYPE_PANEL);

static void
mtp_cc_panel_active_changed (CcPanel  *base_panel,
                              gboolean is_active)
{
  MtpCcPanel *panel = MTP_CC_PANEL (base_panel);
  static gboolean populated = FALSE;

  if (is_active && !populated)
    {
      mtp_bin_load_contents ((MtpBin *)panel->priv->bin);
      populated = TRUE;
    }
}

static void
embed_size_allocate_cb (GtkWidget     *embed,
                        GtkAllocation *allocation,
                        ClutterActor  *box)
{
  clutter_actor_set_size (box, allocation->width, allocation->height);
}

static void
mtp_cc_panel_make_contents (MtpCcPanel *panel)
{
  MtpCcPanelPrivate *priv = panel->priv;
  GError            *error = NULL;
  ClutterActor      *bin;
  ClutterActor      *stage;
  GtkWidget         *embed;

  mx_style_load_from_file (mx_style_get_default (),
                           THEMEDIR "/meego-toolbar-properties.css",
                           &error);

  if (error)
    {
      g_warning ("Could not load stylesheet" THEMEDIR
                 "/meego-toolbar-properties.css: %s", error->message);
      g_clear_error (&error);
    }

  priv->bin = bin = mtp_bin_new ();
  clutter_actor_set_name (bin, "frameless");

  embed = gtk_clutter_embed_new ();

  stage = gtk_clutter_embed_get_stage (GTK_CLUTTER_EMBED (embed));

  g_signal_connect (embed, "size-allocate",
                    G_CALLBACK (embed_size_allocate_cb),
                    bin);

  clutter_container_add_actor (CLUTTER_CONTAINER (stage), bin);

  priv->frame = embed;
}

static void
mtp_cc_panel_init (MtpCcPanel *self)
{
  self->priv = GET_PRIVATE (self);

  mtp_cc_panel_make_contents (self);

  gtk_widget_show (self->priv->frame);
  gtk_container_add (GTK_CONTAINER (self), self->priv->frame);
}

static GObject *
mtp_cc_panel_constructor (GType                  type,
                          guint                  n_construct_properties,
                          GObjectConstructParam *construct_properties)
{
  MtpCcPanel *panel;

  panel = MTP_CC_PANEL (G_OBJECT_CLASS (mtp_cc_panel_parent_class)->constructor
                          (type, n_construct_properties, construct_properties));

  g_object_set (panel,
                "display-name", _("Toolbar"),
                "id", "meego-toolbar-properties.desktop",
                NULL);

  return G_OBJECT (panel);
}

static void
mtp_cc_panel_class_init (MtpCcPanelClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  CcPanelClass *panel_class = CC_PANEL_CLASS (klass);

  object_class->constructor = mtp_cc_panel_constructor;

  panel_class->active_changed = mtp_cc_panel_active_changed;

  g_type_class_add_private (klass, sizeof (MtpCcPanelPrivate));
}

static void
mtp_cc_panel_class_finalize (MtpCcPanelClass *klass)
{
}

void
mtp_cc_panel_register (GIOModule *module)
{
  gtk_clutter_init (NULL, NULL);

  mtp_cc_panel_register_type (G_TYPE_MODULE (module));
  g_io_extension_point_implement (CC_PANEL_EXTENSION_POINT_NAME,
                                  MTP_TYPE_CC_PANEL,
                                  "desktopsettings",
                                  10);
}
