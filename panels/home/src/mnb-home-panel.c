/*
 * Copyright (c) 2011 Intel Corp.
 *
 * Author: Danielle Madeley <danielle.madeley@collabora.co.uk>
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

#include "mnb-home-panel.h"
#include "mnb-home-widget.h"

#define WIDTH 3
#define HEIGHT 2

#define DEBUG g_message

G_DEFINE_TYPE (MnbHomePanel, mnb_home_panel, MX_TYPE_TABLE);

enum /* properties */
{
  PROP_0,
  PROP_PANEL_CLIENT,
  PROP_LAST
};

struct _MnbHomePanelPrivate
{
  MplPanelClient *panel_client;
};

static void
mnb_home_panel_get_property (GObject *self,
    guint prop_id,
    GValue *value,
    GParamSpec *pspec)
{
  MnbHomePanelPrivate *priv = MNB_HOME_PANEL (self)->priv;

  switch (prop_id)
    {
      case PROP_PANEL_CLIENT:
        g_value_set_object (value, priv->panel_client);
        break;

      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (self, prop_id, pspec);
        break;
    }
}

static void
mnb_home_panel_set_property (GObject *self,
    guint prop_id,
    const GValue *value,
    GParamSpec *pspec)
{
  MnbHomePanelPrivate *priv = MNB_HOME_PANEL (self)->priv;

  switch (prop_id)
    {
      case PROP_PANEL_CLIENT:
        g_assert (priv->panel_client == NULL); /* construct-only */
        priv->panel_client = g_value_dup_object (value);
        break;

      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (self, prop_id, pspec);
        break;
    }
}

static void
mnb_home_panel_dispose (GObject *self)
{
  MnbHomePanelPrivate *priv = MNB_HOME_PANEL (self)->priv;

  g_object_unref (priv->panel_client);

  G_OBJECT_CLASS (mnb_home_panel_parent_class)->dispose (self);
}

static void
mnb_home_panel_class_init (MnbHomePanelClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->get_property = mnb_home_panel_get_property;
  gobject_class->set_property = mnb_home_panel_set_property;
  gobject_class->dispose = mnb_home_panel_dispose;

  g_type_class_add_private (gobject_class, sizeof (MnbHomePanelPrivate));

  g_object_class_install_property (gobject_class, PROP_PANEL_CLIENT,
      g_param_spec_object ("panel-client",
        "Panel Client",
        "",
        MPL_TYPE_PANEL_CLIENT,
        G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS));
}

static void
mnb_home_panel_init (MnbHomePanel *self)
{
  ClutterActor *edit;
  guint r, c;

  self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, MNB_TYPE_HOME_PANEL,
      MnbHomePanelPrivate);

  /* edit-mode */
  edit = mx_button_new_with_label (_("Edit"));
  mx_button_set_is_toggle (MX_BUTTON (edit), TRUE);
  mx_table_add_actor_with_properties (MX_TABLE (self), edit,
      HEIGHT + 1, WIDTH / 2,
      "x-expand", FALSE,
      "y-expand", FALSE,
      "x-fill", FALSE,
      "y-fill", FALSE,
      NULL);

  for (c = 0; c < WIDTH; c++)
    {
      for (r = 0; r < HEIGHT; r++)
        {
          ClutterActor *widget;

          widget = mnb_home_widget_new (r, c);
          g_object_bind_property (edit, "toggled", widget, "edit-mode", 0);

          mx_table_add_actor_with_properties (MX_TABLE (self), widget, r, c,
              "x-expand", TRUE,
              "y-expand", TRUE,
              "x-fill", TRUE,
              "y-fill", TRUE,
              NULL);
        }
    }

  clutter_actor_show_all (CLUTTER_ACTOR (self));
}

ClutterActor *
mnb_home_panel_new (MplPanelClient *client)
{
  return g_object_new (MNB_TYPE_HOME_PANEL,
      "panel-client", client,
      NULL);
}
