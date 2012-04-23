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

G_DEFINE_TYPE (MnbHomePanel, mnb_home_panel, MX_TYPE_WIDGET);

#define BUTTON_HOVER_TIMEOUT (750)
#define TAB_SWITCHING_TIMEOUT (400)
#define NB_TABS (5)

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
  ClutterActor   *vbox;

  ClutterActor   *scrollview; /* Scroll view for the grid */
  ClutterActor   *grid;
  MxButtonGroup  *tabs_group; /* Group of button to naviguate the grid */

  guint           timeout_switch_tab;
  gint            workspace_dest;
  MxButton       *workspace_button;
};

static void
toggle_edit_mode (MxButton *button, MnbHomePanel *self)
{
  MnbHomePanelPrivate *priv = self->priv;

  mnb_home_grid_set_edit_mode (MNB_HOME_GRID (priv->grid),
                               mx_button_get_toggled (button));
}

/**/

static void
button_clicked (ClutterActor *button, MnbHomePanel *self)
{
  MnbHomePanelPrivate *priv = self->priv;
  ClutterActorIter iter;
  ClutterActor *child;
  MxAdjustment *adj;
  gint i;

  i = 0;
  clutter_actor_iter_init (&iter, clutter_actor_get_parent (button));
  while (clutter_actor_iter_next (&iter, &child))
    {
      if (child == button)
        {
          priv->workspace_button = MX_BUTTON (button);
          priv->workspace_dest = i;
          break;
        }
      i++;
    }

  mx_scrollable_get_adjustments (MX_SCROLLABLE (priv->grid), &adj, NULL);
  mx_adjustment_interpolate (adj,
                             (gdouble) priv->workspace_dest / NB_TABS * (mx_adjustment_get_upper (adj) - mx_adjustment_get_page_size (adj)),
                             TAB_SWITCHING_TIMEOUT,
                             CLUTTER_EASE_OUT_BACK);
}

static gboolean
button_hover_timeout (MnbHomePanel *self)
{
  MnbHomePanelPrivate *priv = self->priv;
  MxAdjustment *adj;

  priv->timeout_switch_tab = 0;
  mx_button_set_toggled (priv->workspace_button, TRUE);

  mx_scrollable_get_adjustments (MX_SCROLLABLE (priv->grid), &adj, NULL);
  mx_adjustment_interpolate (adj,
                             (gdouble) priv->workspace_dest / NB_TABS * (mx_adjustment_get_upper (adj) - mx_adjustment_get_page_size (adj)),
                             TAB_SWITCHING_TIMEOUT,
                             CLUTTER_EASE_OUT_BACK);

  return FALSE;
}

static gboolean
button_leave_event (ClutterActor         *button,
                    ClutterCrossingEvent *event,
                    MnbHomePanel         *self)
{
  MnbHomePanelPrivate *priv = self->priv;

  if (priv->timeout_switch_tab)
    {
      g_source_remove (priv->timeout_switch_tab);
      priv->timeout_switch_tab = 0;
    }

  return FALSE;
}

static gboolean
button_enter_event (ClutterActor         *button,
                    ClutterCrossingEvent *event,
                    MnbHomePanel         *self)
{
  MnbHomePanelPrivate *priv = self->priv;
  ClutterActorIter iter;
  ClutterActor *child;
  gint i;

  i = 0;
  clutter_actor_iter_init (&iter, clutter_actor_get_parent (button));
  while (clutter_actor_iter_next (&iter, &child))
    {
      if (child == button)
        {
          priv->workspace_button = MX_BUTTON (button);
          priv->workspace_dest = i;
          break;
        }
      i++;
    }

  if (priv->timeout_switch_tab)
    g_source_remove (priv->timeout_switch_tab);
  priv->timeout_switch_tab = g_timeout_add (BUTTON_HOVER_TIMEOUT,
                                            (GSourceFunc) button_hover_timeout,
                                            self);

  return FALSE;
}

static void
grid_drag_begin (MnbHomeGrid *grid, ClutterActor *actor, MnbHomePanel *self)
{
  MnbHomePanelPrivate *priv = self->priv;
  const GSList *buttons = mx_button_group_get_buttons (priv->tabs_group);

  while (buttons)
    {
      ClutterActor *button = buttons->data;

      g_signal_connect (button, "enter-event",
                        G_CALLBACK (button_enter_event), self);
      g_signal_connect (button, "leave-event",
                        G_CALLBACK (button_leave_event), self);

      buttons = buttons->next;
    }
}

static void
grid_drag_end (MnbHomeGrid *grid, ClutterActor *actor, MnbHomePanel *self)
{
  MnbHomePanelPrivate *priv = self->priv;
  const GSList *buttons = mx_button_group_get_buttons (priv->tabs_group);

  while (buttons)
    {
      ClutterActor *button = buttons->data;

      g_signal_handlers_disconnect_by_func (button,
                                            G_CALLBACK (button_enter_event),
                                            self);
      g_signal_handlers_disconnect_by_func (button,
                                            G_CALLBACK (button_leave_event),
                                            self);

      buttons = buttons->next;
    }
}

/* Object implementation */

static void
mnb_home_panel_get_property (GObject    *self,
                             guint       prop_id,
                             GValue     *value,
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
mnb_home_panel_set_property (GObject      *self,
                             guint         prop_id,
                             const GValue *value,
                             GParamSpec   *pspec)
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

  g_clear_object (&priv->panel_client);
  g_clear_object (&priv->background);
  g_clear_object (&priv->grid);

  G_OBJECT_CLASS (mnb_home_panel_parent_class)->dispose (self);
}

static void
mnb_home_panel_allocate (ClutterActor           *self,
                         const ClutterActorBox  *box,
                         ClutterAllocationFlags  flags)
{
  MnbHomePanelPrivate *priv = MNB_HOME_PANEL (self)->priv;
  ClutterActorBox child_box;
  MxPadding padding;

  mx_widget_get_padding (MX_WIDGET (self), &padding);

  CLUTTER_ACTOR_CLASS (mnb_home_panel_parent_class)->allocate (self,
                                                               box, flags);

  child_box.x1 = padding.left;
  child_box.y1 = padding.top;
  child_box.x2 = box->x2 - box->x1 - (padding.left + padding.right);
  child_box.y2 = box->y2 - box->y1 - (padding.top + padding.bottom);

  clutter_actor_allocate (priv->background, &child_box, flags);
  clutter_actor_allocate (priv->vbox, &child_box, flags);
}

static void
mnb_home_panel_paint (ClutterActor *self)
{
  MnbHomePanelPrivate *priv = MNB_HOME_PANEL (self)->priv;

  clutter_actor_paint (priv->background);
  clutter_actor_paint (priv->vbox);
}

static void
mnb_home_panel_pick (ClutterActor       *self,
                     const ClutterColor *color)
{
  MnbHomePanelPrivate *priv = MNB_HOME_PANEL (self)->priv;

  CLUTTER_ACTOR_CLASS (mnb_home_panel_parent_class)->pick (self, color);

  clutter_actor_paint (priv->background);
  clutter_actor_paint (priv->vbox);
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
  actor_class->pick = mnb_home_panel_pick;
  actor_class->allocate = mnb_home_panel_allocate;

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
  ClutterActor *edit, *item, *box;
  ClutterColor *color;
  gint i;

  self->priv = priv = G_TYPE_INSTANCE_GET_PRIVATE (self, MNB_TYPE_HOME_PANEL,
                                                   MnbHomePanelPrivate);

  /* background */
  /* FIXME: make this awesomer */
  priv->background = mx_image_new ();
  mx_image_set_from_file (MX_IMAGE (priv->background),
                          "/usr/share/backgrounds/gnome/Aqua.jpg", NULL);
  clutter_actor_add_child (CLUTTER_ACTOR (self), priv->background);

  priv->vbox = mx_box_layout_new_with_orientation (MX_ORIENTATION_VERTICAL);
  clutter_actor_add_child (CLUTTER_ACTOR (self), priv->vbox);
  clutter_actor_raise_top (priv->vbox);

  /* Grid */
  priv->scrollview = mx_scroll_view_new ();
  mx_scroll_view_set_scroll_visibility (MX_SCROLL_VIEW (priv->scrollview),
                                        MX_SCROLL_POLICY_NONE);
  mx_box_layout_insert_actor_with_properties (MX_BOX_LAYOUT (priv->vbox),
                                              priv->scrollview,
                                              0,
                                              "expand", TRUE,
                                              "x-fill", TRUE,
                                              "y-fill", TRUE,
                                              NULL);

  priv->grid = mnb_home_grid_new ();
  mnb_home_grid_set_grid_size (MNB_HOME_GRID (priv->grid), 40, 6); /* TODO: auto! */
  mx_bin_set_child (MX_BIN (priv->scrollview), priv->grid);

  g_signal_connect (priv->grid, "drag-begin",
                    G_CALLBACK (grid_drag_begin), self);
  g_signal_connect (priv->grid, "drag-end",
                    G_CALLBACK (grid_drag_end), self);


  /* Tabs */
  box = mx_box_layout_new_with_orientation (MX_ORIENTATION_HORIZONTAL);
  mx_box_layout_insert_actor_with_properties (MX_BOX_LAYOUT (priv->vbox),
                                              box,
                                              1,
                                              "expand", FALSE,
                                              "x-fill", FALSE,
                                              "y-fill", FALSE,
                                              "x-align", MX_ALIGN_MIDDLE,
                                              NULL);

  priv->tabs_group = mx_button_group_new ();

  for (i = 0; i < NB_TABS; i++)
    {
      MxButton *button = MX_BUTTON (mx_button_new ());
      mx_button_set_is_toggle (button, TRUE);
      mx_button_group_add (priv->tabs_group, button);
      mx_box_layout_insert_actor (MX_BOX_LAYOUT (box),
                                  CLUTTER_ACTOR (button),
                                  -1);

      g_signal_connect (button, "clicked",
                        G_CALLBACK (button_clicked),
                        self);
    }
  mx_button_group_set_active_button (priv->tabs_group,
                                     mx_button_group_get_active_button (priv->tabs_group));

  /* edit-mode */
  edit = mx_button_new_with_label (_("Edit"));
  mx_button_set_is_toggle (MX_BUTTON (edit), TRUE);
  mx_box_layout_insert_actor_with_properties (MX_BOX_LAYOUT (priv->vbox),
                                              edit,
                                              2,
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
