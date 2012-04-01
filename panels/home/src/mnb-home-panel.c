/*
 * Copyright (c) 2012 Intel Corp.
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

#include "mnb-home-grid.h"
#include "mnb-home-panel.h"
#include "mnb-home-widget.h"
#include "utils.h"

#define WIDTH 3
#define HEIGHT 2

G_DEFINE_TYPE (MnbHomePanel, mnb_home_panel, MX_TYPE_BOX_LAYOUT);

enum /* properties */
{
  PROP_0,
  PROP_PANEL_CLIENT,
  PROP_LAST
};

struct _MnbHomePanelPrivate
{
  MplPanelClient *panel_client;
  ClutterActor   *background;
  ClutterActor   *grid;
};

static void
toggle_edit_mode (MxButton *button, MnbHomePanel *self)
{
  MnbHomePanelPrivate *priv = self->priv;

  mnb_home_grid_set_edit_mode (MNB_HOME_GRID (priv->grid),
                               mx_button_get_toggled (button));
}

/* Object implementation */

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
mnb_home_panel_paint (ClutterActor *self)
{
  MnbHomePanelPrivate *priv = MNB_HOME_PANEL (self)->priv;

  clutter_actor_paint (priv->background);

  CLUTTER_ACTOR_CLASS (mnb_home_panel_parent_class)->paint (self);
}

static void
mnb_home_panel_map (ClutterActor *self)
{
  MnbHomePanelPrivate *priv = MNB_HOME_PANEL (self)->priv;

  CLUTTER_ACTOR_CLASS (mnb_home_panel_parent_class)->map (self);

  clutter_actor_map (priv->background);
}

static void
mnb_home_panel_unmap (ClutterActor *self)
{
  MnbHomePanelPrivate *priv = MNB_HOME_PANEL (self)->priv;

  CLUTTER_ACTOR_CLASS (mnb_home_panel_parent_class)->unmap (self);

  clutter_actor_unmap (priv->background);
}

static void
mnb_home_panel_allocate (ClutterActor *self,
    const ClutterActorBox *box,
    ClutterAllocationFlags flags)
{
  MnbHomePanelPrivate *priv = MNB_HOME_PANEL (self)->priv;
  ClutterActorBox child_box;

  child_box.x1 = 0;
  child_box.y1 = 0;
  child_box.x2 = box->x2 - box->x1;
  child_box.y2 = box->y2 - box->y1;

  clutter_actor_allocate (priv->background, &child_box, flags);

  CLUTTER_ACTOR_CLASS (mnb_home_panel_parent_class)->allocate (self,
      box, flags);
}

static void
mnb_home_panel_class_init (MnbHomePanelClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  ClutterActorClass *actor_class = CLUTTER_ACTOR_CLASS (klass);

  gobject_class->get_property = mnb_home_panel_get_property;
  gobject_class->set_property = mnb_home_panel_set_property;
  gobject_class->dispose = mnb_home_panel_dispose;

  actor_class->paint = mnb_home_panel_paint;
  actor_class->allocate = mnb_home_panel_allocate;
  actor_class->map = mnb_home_panel_map;
  actor_class->unmap = mnb_home_panel_unmap;

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
  MnbHomePanelPrivate *priv;
  ClutterActor *edit, *item;
  ClutterColor *color;

  self->priv = priv = G_TYPE_INSTANCE_GET_PRIVATE (self, MNB_TYPE_HOME_PANEL,
                                                   MnbHomePanelPrivate);

  mx_box_layout_set_orientation (MX_BOX_LAYOUT (self), MX_ORIENTATION_VERTICAL);

  /* background */
  /* FIXME: make this awesomer */
  self->priv->background = clutter_texture_new_from_file (
      "/usr/share/backgrounds/gnome/Aqua.jpg",
      NULL);
  clutter_actor_set_parent (self->priv->background, CLUTTER_ACTOR (self));

  /* Grid */
  priv->grid = mnb_home_grid_new ();
  mnb_home_grid_set_grid_size (MNB_HOME_GRID (priv->grid), 14, 7); /* TODO: auto! */
  mx_box_layout_insert_actor_with_properties (MX_BOX_LAYOUT (self),
                                              priv->grid,
                                              0,
                                              "expand", TRUE,
                                              "x-fill", TRUE,
                                              "y-fill", TRUE,
                                              NULL);

  /* edit-mode */
  edit = mx_button_new_with_label (_("Edit"));
  mx_button_set_is_toggle (MX_BUTTON (edit), TRUE);
  mx_box_layout_insert_actor_with_properties (MX_BOX_LAYOUT (self),
                                              edit,
                                              1,
                                              "expand", FALSE,
                                              "x-fill", FALSE,
                                              "y-fill", FALSE,
                                              NULL);

  g_signal_connect (edit, "clicked",
                    G_CALLBACK (toggle_edit_mode),
                    self);

  /* TODO: Demo, to remove when we have actual tiles */
  color = clutter_color_new (0x0, 0, 0, 0xff);

  /**/
  clutter_color_from_string (color, "red");
  item = clutter_rectangle_new_with_color (color);
  clutter_actor_set_size (item, 64, 64);
  mnb_home_grid_insert_actor (MNB_HOME_GRID (priv->grid), item, 0, 0);

  /**/
  clutter_color_from_string (color, "green");
  item = clutter_rectangle_new_with_color (color);
  clutter_actor_set_size (item, 64, 64);
  mnb_home_grid_insert_actor (MNB_HOME_GRID (priv->grid), item, 3, 0);

  /**/
  clutter_color_from_string (color, "black");
  item = clutter_rectangle_new_with_color (color);
  clutter_actor_set_size (item, 50, 50);
  mnb_home_grid_insert_actor (MNB_HOME_GRID (priv->grid), item, 3, 3);

  /**/
  clutter_color_from_string (color, "grey");
  item = clutter_rectangle_new_with_color (color);
  clutter_actor_set_size (item, 138, 138);
  mnb_home_grid_insert_actor (MNB_HOME_GRID (priv->grid), item, 5, 3);

  clutter_color_free (color);

  clutter_actor_show_all (CLUTTER_ACTOR (self));
}

ClutterActor *
mnb_home_panel_new (MplPanelClient *client)
{
  return g_object_new (MNB_TYPE_HOME_PANEL,
                       "panel-client", client,
                       NULL);
}
