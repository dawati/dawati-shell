/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */

/* mtp-toolbar-button.c */
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

#include "mtp-toolbar.h"
#include "mtp-toolbar-button.h"
#include "mtp-space.h"

#define TOOLBAR_X_PADDING 4.0

#define CLOCK_WIDTH 164

#define BUTTON_WIDTH 66
#define BUTTON_SPACING 10

#define TRAY_WIDTH 170
#define TRAY_PADDING   3
#define TRAY_BUTTON_WIDTH 44

static void mx_droppable_iface_init (MxDroppableIface *iface);

G_DEFINE_TYPE_WITH_CODE (MtpToolbar, mtp_toolbar, MX_TYPE_WIDGET,
                         G_IMPLEMENT_INTERFACE (MX_TYPE_DROPPABLE,
                                                mx_droppable_iface_init));

#define MTP_TOOLBAR_GET_PRIVATE(obj)    \
        (G_TYPE_INSTANCE_GET_PRIVATE ((obj), MTP_TYPE_TOOLBAR, MtpToolbarPrivate))

enum
{
  ZONE_PROP_0 = 0,
  ZONE_PROP_FREE_SPACE,

  /* d&d */
  ZONE_PROP_ENABLED
};

struct _MtpToolbarPrivate
{
  ClutterActor *applet_area;
  ClutterActor *panel_area;
  ClutterActor *clock_area;

  gboolean free_space : 1;
  gboolean enabled    : 1;
  gboolean modified   : 1;
  gboolean disposed   : 1;
};

static void
mtp_toolbar_dispose (GObject *object)
{
  MtpToolbarPrivate *priv = MTP_TOOLBAR (object)->priv;

  if (priv->disposed)
    return;

  priv->disposed = TRUE;

  clutter_actor_destroy (priv->panel_area);
  priv->panel_area = NULL;

  clutter_actor_destroy (priv->applet_area);
  priv->applet_area = NULL;

  clutter_actor_destroy (priv->clock_area);
  priv->clock_area = NULL;

  G_OBJECT_CLASS (mtp_toolbar_parent_class)->dispose (object);
}

static void
mtp_toolbar_map (ClutterActor *actor)
{
  MtpToolbarPrivate *priv = MTP_TOOLBAR (actor)->priv;

  CLUTTER_ACTOR_CLASS (mtp_toolbar_parent_class)->map (actor);

  clutter_actor_map (priv->panel_area);
  clutter_actor_map (priv->clock_area);
  clutter_actor_map (priv->applet_area);
}

static void
mtp_toolbar_unmap (ClutterActor *actor)
{
  MtpToolbarPrivate *priv = MTP_TOOLBAR (actor)->priv;

  clutter_actor_unmap (priv->panel_area);
  clutter_actor_unmap (priv->clock_area);
  clutter_actor_unmap (priv->applet_area);

  CLUTTER_ACTOR_CLASS (mtp_toolbar_parent_class)->unmap (actor);
}

static void
mtp_toolbar_allocate (ClutterActor          *actor,
                      const ClutterActorBox *box,
                      ClutterAllocationFlags flags)
{
  MtpToolbar        *toolbar = MTP_TOOLBAR (actor);
  MtpToolbarPrivate *priv = toolbar->priv;
  MxPadding          padding;
  ClutterActorBox    childbox;
  gfloat             clock_x;
  gfloat             tray_x;

  CLUTTER_ACTOR_CLASS (
             mtp_toolbar_parent_class)->allocate (actor, box, flags);

  mx_widget_get_padding (MX_WIDGET (actor), &padding);

  clock_x = box->x2 - box->x1 - 334.0;
  tray_x  = clock_x + 164.0 + 2.0;

  childbox.x1 = padding.left + 4.0;
  childbox.y1 = padding.top;
  childbox.x2 = clock_x - 2.0;
  childbox.y2 = box->y2 - box->y1 - padding.top - padding.bottom;

  mx_allocate_align_fill (priv->panel_area, &childbox,
                          MX_ALIGN_START, MX_ALIGN_MIDDLE,
                          FALSE, FALSE);
  clutter_actor_allocate (priv->panel_area, &childbox, flags);

  childbox.x1 = clock_x;
  childbox.y1 = padding.top;
  childbox.x2 = tray_x - 2.0;
  childbox.y2 = box->y2 - box->y1 - padding.top - padding.bottom;
  mx_allocate_align_fill (priv->clock_area, &childbox,
                          MX_ALIGN_START, MX_ALIGN_MIDDLE,
                          FALSE, FALSE);
  clutter_actor_allocate (priv->clock_area, &childbox, flags);

  childbox.x1 = tray_x;
  childbox.y1 = padding.top;
  childbox.x2 = box->x2 - box->x1;
  childbox.y2 = box->y2 - box->y1 - padding.top - padding.bottom;
  mx_allocate_align_fill (priv->applet_area, &childbox,
                          MX_ALIGN_START, MX_ALIGN_MIDDLE,
                          FALSE, FALSE);
  clutter_actor_allocate (priv->applet_area, &childbox, flags);

  mtp_toolbar_fill_space (toolbar);
}

static void
mtp_toolbar_constructed (GObject *self)
{
  MtpToolbarPrivate *priv = MTP_TOOLBAR (self)->priv;
  ClutterActor      *actor = CLUTTER_ACTOR (self);

  priv->panel_area  = mx_box_layout_new ();
  clutter_actor_set_name (priv->panel_area, "panel-area");
  mx_box_layout_set_spacing (MX_BOX_LAYOUT (priv->panel_area), 2);
  mx_box_layout_set_enable_animations (MX_BOX_LAYOUT (priv->panel_area), TRUE);
  clutter_actor_set_parent (priv->panel_area, actor);

  priv->clock_area  = mx_box_layout_new ();
  clutter_actor_set_name (priv->clock_area, "clock-area");
  mx_box_layout_set_spacing (MX_BOX_LAYOUT (priv->clock_area), 2);
  mx_box_layout_set_enable_animations (MX_BOX_LAYOUT (priv->clock_area), TRUE);
  clutter_actor_set_parent (priv->clock_area, actor);

  priv->applet_area = mx_box_layout_new ();
  clutter_actor_set_name (priv->applet_area, "applet-area");
  mx_box_layout_set_spacing (MX_BOX_LAYOUT (priv->applet_area), 2);
  /* mx_box_layout_set_pack_start (MX_BOX_LAYOUT (priv->applet_area), FALSE); */
  mx_box_layout_set_enable_animations (MX_BOX_LAYOUT (priv->applet_area), TRUE);
  clutter_actor_set_parent (priv->applet_area, actor);
}

static void
mtp_toolbar_set_property (GObject      *gobject,
                          guint         prop_id,
                          const GValue *value,
                          GParamSpec   *pspec)
{
  MtpToolbarPrivate *priv = MTP_TOOLBAR (gobject)->priv;

  switch (prop_id)
    {
    case ZONE_PROP_ENABLED:
      priv->enabled = g_value_get_boolean (value);
      if (priv->enabled)
        mx_droppable_enable (MX_DROPPABLE (gobject));
      else
        mx_droppable_disable (MX_DROPPABLE (gobject));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
      break;
    }
}

static void
mtp_toolbar_get_property (GObject    *gobject,
                          guint       prop_id,
                          GValue     *value,
                          GParamSpec *pspec)
{
  MtpToolbarPrivate *priv = MTP_TOOLBAR (gobject)->priv;

  switch (prop_id)
    {
    case ZONE_PROP_ENABLED:
      g_value_set_boolean (value, priv->enabled);
      break;
    case ZONE_PROP_FREE_SPACE:
      g_value_set_boolean (value, priv->free_space);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
      break;
    }
}

static void
mtp_toolbar_over_in (MxDroppable *droppable, MxDraggable *draggable)
{
  MtpToolbarPrivate *priv = MTP_TOOLBAR (droppable)->priv;

  if (!priv->enabled)
    return;
}

static void
mtp_toolbar_over_out (MxDroppable *droppable, MxDraggable *draggable)
{
  MtpToolbarPrivate *priv = MTP_TOOLBAR (droppable)->priv;

  if (!priv->enabled)
    return;
}

/*
 * returns FALSE if no space could be removed.
 */
static gboolean
mtp_toolbar_remove_space (MtpToolbar *toolbar, gboolean applet)
{
  MtpToolbarPrivate *priv = MTP_TOOLBAR (toolbar)->priv;
  GList             *children;
  GList             *last;
  gboolean           retval = FALSE;
  ClutterContainer  *container;

  container = applet ? CLUTTER_CONTAINER (priv->applet_area) :
    CLUTTER_CONTAINER (priv->panel_area);

  children = clutter_container_get_children (container);

  last = applet ? g_list_first (children) : g_list_last (children);

  while (last && !MTP_IS_SPACE (last->data))
    {
      last = applet ? last->next : last->prev;
    }

  if (last && !applet)
    {
      GList    *penult = last->prev;
      gboolean  not_free_space = !priv->free_space;

      while (penult && !MTP_IS_SPACE (penult->data))
        {
          penult = penult->prev;
        }

      if (penult)
        priv->free_space = TRUE;
      else
        priv->free_space = FALSE;

      if ((!priv->free_space) != not_free_space)
        g_object_notify (G_OBJECT (toolbar), "free-space");
    }

  if (last)
    {
      ClutterActor *actor = last->data;

      retval = TRUE;

      clutter_container_remove_actor (container, actor);
    }
  else
    g_warning (G_STRLOC ":%s: no space left on toolbar !!!", __FUNCTION__);

  g_list_free (children);

  return retval;
}

/*
 * D&D drop handler
 */
static void
mtp_toolbar_drop (MxDroppable         *droppable,
                  MxDraggable         *draggable,
                  gfloat               event_x,
                  gfloat               event_y,
                  gint                 button,
                  ClutterModifierType  modifiers)
{
  MtpToolbar        *toolbar = MTP_TOOLBAR (droppable);
  MtpToolbarPrivate *priv = toolbar->priv;
  MtpToolbarButton  *tbutton = MTP_TOOLBAR_BUTTON (draggable);
  ClutterActor      *actor = CLUTTER_ACTOR (draggable);
  ClutterActor      *stage = clutter_actor_get_stage (actor);
  MtpToolbarButton  *before = NULL;

  /*
   * Check we are not disabled (we should really not be getting drop events on
   * disabled droppables, but we do).
   */
  if (!priv->enabled)
    {
      g_warning ("Bug: received a drop on a disabled droppable -- ignoring");
      return;
    }

  if (mtp_toolbar_button_is_clock (tbutton))
    {
      mtp_toolbar_add_button (toolbar, actor);
      return;
    }

  /*
   * We need to work out where on the Toolbar the drop happened; specifically,
   * we are interested which current button is the new button being inserted
   * before. Using the pick for this is simple.
   */
  {
    ClutterActor *target;

    /*
     * Disable pick on the current draggable, since that is the top-most actor
     * at the position we care about.
     */
    mtp_toolbar_button_set_dont_pick (tbutton, TRUE);

    target = clutter_stage_get_actor_at_pos (CLUTTER_STAGE (stage),
                                             CLUTTER_PICK_REACTIVE,
                                             event_x, event_y);

    while (target && !(MTP_IS_TOOLBAR_BUTTON (target) ||
                       MTP_IS_TOOLBAR (target)))
      target = clutter_actor_get_parent (target);

    if (target)
      {
        if (!(MTP_IS_TOOLBAR_BUTTON (target)))
          {
            /*
             * We either hit the empty part of the toolbar, or a gap between
             * buttons. See what's 20px to the right.
             */
            target = clutter_stage_get_actor_at_pos (CLUTTER_STAGE (stage),
                                                     CLUTTER_PICK_REACTIVE,
                                                     event_x + 20, event_y);

            if (!MTP_IS_TOOLBAR_BUTTON (target))
              target = NULL;
          }

        before = (MtpToolbarButton*)target;
      }

    mtp_toolbar_button_set_dont_pick (tbutton, FALSE);
  }

  /*
   * If we do not have a 'before' button, find the first space.
   */
  if (!before)
    {
      GList *children, *l;

      if (mtp_toolbar_button_is_applet (tbutton))
        children = clutter_container_get_children (CLUTTER_CONTAINER (
                                                          priv->applet_area));
      else
        children = clutter_container_get_children (CLUTTER_CONTAINER (
                                                          priv->panel_area));

      for (l = children; l; l = l->next)
        {
          /*
           * The first space will be removed, so the space we want to insert
           * before is the one after it.
           */
          if (MTP_IS_SPACE (l->data) && l->next && MTP_IS_SPACE (l->next->data))
            {
              before = l->data;
              break;
            }
        }

      g_list_free (children);
    }

  if (mtp_toolbar_remove_space (toolbar,
                                mtp_toolbar_button_is_applet (tbutton)))
    {
      clutter_actor_set_size ((ClutterActor*)tbutton, -1.0, -1.0);
      mtp_toolbar_insert_button (toolbar,
                                 (ClutterActor*)tbutton, (ClutterActor*)before);
    }
}

static void
mx_droppable_iface_init (MxDroppableIface *iface)
{
  iface->over_in  = mtp_toolbar_over_in;
  iface->over_out = mtp_toolbar_over_out;
  iface->drop     = mtp_toolbar_drop;
}

static void
mtp_toolbar_pick (ClutterActor *self, const ClutterColor *color)
{
  MtpToolbarPrivate *priv = MTP_TOOLBAR (self)->priv;

  CLUTTER_ACTOR_CLASS (mtp_toolbar_parent_class)->pick (self, color);

  clutter_actor_paint (priv->panel_area);
  clutter_actor_paint (priv->clock_area);
  clutter_actor_paint (priv->applet_area);
}

static void
mtp_toolbar_paint (ClutterActor *self)
{
  MtpToolbarPrivate *priv = MTP_TOOLBAR (self)->priv;

  CLUTTER_ACTOR_CLASS (mtp_toolbar_parent_class)->paint (self);

  clutter_actor_paint (priv->panel_area);
  clutter_actor_paint (priv->clock_area);
  clutter_actor_paint (priv->applet_area);
}

static void
mtp_toolbar_paint_border_image (MxTextureFrame *frame)
{
  CoglHandle cogl_texture = COGL_INVALID_HANDLE;
  CoglHandle cogl_material = COGL_INVALID_HANDLE;
  ClutterActorBox box = { 0, };
  gfloat width, height;
  gfloat tex_width, tex_height;
  gfloat ex, ey;
  gfloat tx1, ty1, tx2, ty2;
  gfloat left, right, top, bottom;
  guint8 opacity;
  ClutterTexture *parent_texture;
  gfloat margin;

  parent_texture = mx_texture_frame_get_parent_texture (frame);

  mx_texture_frame_get_frame(frame, &top, &right, &bottom, &left);

  /* no need to paint stuff if we don't have a texture */
  if (G_UNLIKELY (parent_texture == NULL))
    return;

  /* parent texture may have been hidden, so need to make sure it gets
   * realized
   */
  if (!CLUTTER_ACTOR_IS_REALIZED (parent_texture))
    clutter_actor_realize (CLUTTER_ACTOR (parent_texture));

  cogl_texture = clutter_texture_get_cogl_texture (parent_texture);

  if (cogl_texture == COGL_INVALID_HANDLE)
    return;

  cogl_material = clutter_texture_get_cogl_material (parent_texture);

  if (cogl_material == COGL_INVALID_HANDLE)
    return;

  tex_width  = cogl_texture_get_width (cogl_texture);
  tex_height = cogl_texture_get_height (cogl_texture);

  clutter_actor_get_allocation_box ((ClutterActor*) frame, &box);

  width  = box.x2 - box.x1;
  height = box.y2 - box.y1;

  margin = TOOLBAR_X_PADDING / tex_width;

  tx1 = left / tex_width;
  tx2 = (tex_width - right) / tex_width;
  ty1 = top / tex_height;
  ty2 = (tex_height - bottom) / tex_height;

  ex = width - right;
  if (ex < 0)
    ex = right;           /* FIXME ? */

  ey = height - bottom;
  if (ey < 0)
    ey = bottom;          /* FIXME ? */

  opacity = clutter_actor_get_paint_opacity ((ClutterActor*)frame);

  cogl_material_set_color4ub (cogl_material,
                              opacity, opacity, opacity, opacity);
  cogl_set_source (cogl_material);

  cogl_material_set_layer_filters (cogl_material,
                                   0,
                                   COGL_MATERIAL_FILTER_NEAREST,
                                   COGL_MATERIAL_FILTER_NEAREST);

  {
    GLfloat rectangles[] =
    {
      /* top left corner */
      0.0, 0.0, left, top,
      margin, 0.0,
      tx1, ty1,

      /* top middle */
      left, 0.0, ex, top,
      tx1, 0.0,
      tx2, ty1,

      /* top right */
      ex, 0.0, width, top,
      tx2, 0.0,
      1.0 - margin, ty1,

      /* mid left */
      0.0, top, left, ey,
      margin, ty1,
      tx1, ty2,

      /* center */
      left, top, ex, ey,
      tx1, ty1,
      tx2, ty2,

      /* mid right */
      ex, top, width, ey,
      tx2, ty1,
      1.0 - margin, ty2,

      /* bottom left */
      0.0, ey, left, height,
      margin, ty2,
      tx1, 1.0,

      /* bottom center */
      left, ey, ex, height,
      tx1, ty2,
      tx2, 1.0,

      /* bottom right */
      ex, ey, width, height,
      tx2, ty2,
      1.0 - margin, 1.0
    };

    cogl_rectangles_with_texture_coords (rectangles, 9);
  }
}

static void
mtp_toolbar_paint_background (MxWidget           *self,
                              ClutterActor       *background,
                              const ClutterColor *color)
{
  /* Default implementation just draws the background
   * colour and the image on top
   */
  if (color && color->alpha != 0)
    {
      ClutterActor *actor = CLUTTER_ACTOR (self);
      ClutterActorBox allocation = { 0, };
      ClutterColor bg_color = *color;
      gfloat w, h;

      bg_color.alpha = clutter_actor_get_paint_opacity (actor)
                       * bg_color.alpha
                       / 255;

      clutter_actor_get_allocation_box (actor, &allocation);

      w = allocation.x2 - allocation.x1;
      h = allocation.y2 - allocation.y1;

      cogl_set_source_color4ub (bg_color.red,
                                bg_color.green,
                                bg_color.blue,
                                bg_color.alpha);
      cogl_rectangle (TOOLBAR_X_PADDING, 0, w - TOOLBAR_X_PADDING, h);
    }

  if (background)
    mtp_toolbar_paint_border_image (MX_TEXTURE_FRAME (background));
}

static void
mtp_toolbar_class_init (MtpToolbarClass *klass)
{
  ClutterActorClass *actor_class  = CLUTTER_ACTOR_CLASS (klass);
  GObjectClass      *object_class = G_OBJECT_CLASS (klass);
  MxWidgetClass     *widget_class = MX_WIDGET_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MtpToolbarPrivate));

  actor_class->allocate           = mtp_toolbar_allocate;
  actor_class->map                = mtp_toolbar_map;
  actor_class->unmap              = mtp_toolbar_unmap;
  actor_class->paint              = mtp_toolbar_paint;
  actor_class->pick               = mtp_toolbar_pick;

  object_class->constructed       = mtp_toolbar_constructed;
  object_class->dispose           = mtp_toolbar_dispose;
  object_class->set_property      = mtp_toolbar_set_property;
  object_class->get_property      = mtp_toolbar_get_property;

  widget_class->paint_background  = mtp_toolbar_paint_background;

  g_object_class_override_property (object_class, ZONE_PROP_ENABLED,"enabled");

  g_object_class_install_property (object_class,
                                   ZONE_PROP_FREE_SPACE,
                                   g_param_spec_boolean ("free-space",
                                                         "Free Space",
                                                         "Free Space",
                                                         TRUE,
                                                         G_PARAM_READABLE));
}

static void
mtp_toolbar_init (MtpToolbar *self)
{
  MtpToolbarPrivate *priv;

  priv = self->priv = MTP_TOOLBAR_GET_PRIVATE (self);

  priv->free_space = TRUE;

  clutter_actor_set_reactive ((ClutterActor*)self, TRUE);
}

ClutterActor*
mtp_toolbar_new (void)
{
  return g_object_new (MTP_TYPE_TOOLBAR, NULL);
}

static void
mtp_toolbar_button_parent_set_cb (MtpToolbarButton *button,
                                  ClutterActor     *old_parent,
                                  MtpToolbar       *toolbar)
{
  MtpToolbarPrivate *priv = MTP_TOOLBAR (toolbar)->priv;
  ClutterActor      *parent;

  if (priv->disposed)
    return;

  parent = clutter_actor_get_parent ((ClutterActor*)button);

  if (old_parent == priv->panel_area ||
      old_parent == priv->clock_area ||
      old_parent == priv->applet_area)
    {
      priv->modified = TRUE;

      if (!mtp_toolbar_button_is_clock (button))
        {
          mtp_toolbar_fill_space (toolbar);

          if (!mtp_toolbar_button_is_applet (button))
            {
              gboolean  not_free_space = !priv->free_space;
              GList    *last;

              GList *children = clutter_container_get_children (
                                          CLUTTER_CONTAINER (priv->panel_area));

              last = g_list_last (children);

              if (last && MTP_IS_SPACE (last->data))
                priv->free_space = TRUE;
              else
                priv->free_space = FALSE;

              if ((!priv->free_space) != not_free_space)
                g_object_notify (G_OBJECT (toolbar), "free-space");

              g_list_free (children);
            }
        }
    }

  if (!(parent == priv->panel_area  ||
        parent == priv->applet_area ||
        parent == priv->clock_area))
    g_signal_handlers_disconnect_by_func (button,
                                          mtp_toolbar_button_parent_set_cb,
                                          toolbar);
}

void
mtp_toolbar_add_button (MtpToolbar *toolbar, ClutterActor *button)
{
  MtpToolbarPrivate *priv = MTP_TOOLBAR (toolbar)->priv;

  priv->modified = TRUE;

  if (MTP_IS_SPACE (button))
    {
      clutter_container_add_actor (CLUTTER_CONTAINER (priv->panel_area),
                                   button);
      clutter_container_child_set (CLUTTER_CONTAINER (priv->panel_area),
                                   button,
                                   "expand", FALSE, NULL);
    }
  else if (mtp_toolbar_button_is_clock ((MtpToolbarButton*)button))
    {
      ClutterActor *parent = clutter_actor_get_parent (button);

      if (parent)
        clutter_actor_reparent (button, priv->clock_area);
      else
        clutter_container_add_actor (CLUTTER_CONTAINER (priv->clock_area),
                                     button);

      clutter_container_child_set (CLUTTER_CONTAINER (priv->clock_area),
                                   button,
                                   "expand", FALSE, NULL);
    }
  else if (MTP_IS_TOOLBAR_BUTTON (button))
    {
      if (mtp_toolbar_button_is_applet ((MtpToolbarButton*)button))
        {
          ClutterActor *parent = clutter_actor_get_parent (button);
          gfloat        depth = 0.0;

          GList *children = clutter_container_get_children (
                                       CLUTTER_CONTAINER (priv->applet_area));

          if (children)
            {
              depth = clutter_actor_get_depth (children->data) - 0.05;

              g_list_free (children);
            }

          if (parent)
            clutter_actor_reparent (button, priv->applet_area);
          else
            clutter_container_add_actor (CLUTTER_CONTAINER (priv->applet_area),
                                         button);

          clutter_container_child_set (CLUTTER_CONTAINER (priv->applet_area),
                                       button,
                                       "expand", FALSE, NULL);
          clutter_actor_set_depth (button, depth);
        }
      else
        {
          ClutterActor *parent = clutter_actor_get_parent (button);
          gfloat        depth = 0.0;
          gboolean      not_free_space = !priv->free_space;
          GList        *last;
          GList        *children = clutter_container_get_children (
                                       CLUTTER_CONTAINER (priv->panel_area));

          last = g_list_last (children);

          if (last && MTP_IS_SPACE (last->data))
            priv->free_space = TRUE;
          else
            priv->free_space = FALSE;

          if ((!priv->free_space) != not_free_space)
            g_object_notify (G_OBJECT (toolbar), "free-space");

          if (parent)
            clutter_actor_reparent (button, priv->panel_area);
          else
            clutter_container_add_actor (CLUTTER_CONTAINER (priv->panel_area),
                                         button);

          if (last)
            depth = clutter_actor_get_depth (last->data) + 0.05;

          clutter_container_child_set (CLUTTER_CONTAINER (priv->panel_area),
                                       button,
                                       "expand", FALSE, NULL);
          clutter_actor_set_depth (button, depth);

          g_list_free (children);
        }

      g_signal_connect (button, "parent-set",
                        G_CALLBACK (mtp_toolbar_button_parent_set_cb),
                        toolbar);
    }
  else
    g_warning (G_STRLOC ":%s: unsupported actor type %s",
               __FUNCTION__, G_OBJECT_TYPE_NAME (button));
}

void
mtp_toolbar_readd_button (MtpToolbar *toolbar, ClutterActor *button)
{
  MtpToolbarPrivate *priv = MTP_TOOLBAR (toolbar)->priv;

  if (!MTP_IS_TOOLBAR_BUTTON (button))
    {
      g_warning (G_STRLOC ": only buttons can be readded back");
      return;
    }

  if (mtp_toolbar_button_is_clock ((MtpToolbarButton*)button))
    {
      ClutterActor *parent = clutter_actor_get_parent (button);

      if (parent)
        clutter_actor_reparent (button, priv->clock_area);
      else
        clutter_container_add_actor (CLUTTER_CONTAINER (priv->clock_area),
                                     button);

      clutter_container_child_set (CLUTTER_CONTAINER (priv->clock_area),
                                   button,
                                   "expand", FALSE, NULL);
    }
  else if (MTP_IS_TOOLBAR_BUTTON (button))
    {
      if (mtp_toolbar_button_is_applet ((MtpToolbarButton*)button))
        {
          if (mtp_toolbar_remove_space (toolbar, TRUE))
            {
              ClutterActor *parent = clutter_actor_get_parent (button);

              if (parent)
                clutter_actor_reparent (button, priv->applet_area);
              else
                clutter_container_add_actor (CLUTTER_CONTAINER (
                                                             priv->applet_area),
                                             button);

              clutter_container_child_set (CLUTTER_CONTAINER (
                                                             priv->applet_area),
                                           button,
                                           "expand", FALSE, NULL);

              clutter_container_sort_depth_order (CLUTTER_CONTAINER (
                                                            priv->applet_area));
            }
          else
            {
              g_warning (G_STRLOC ": no space to remove!");
            }
        }
      else
        {
          if (mtp_toolbar_remove_space (toolbar, FALSE))
            {
              ClutterActor *parent = clutter_actor_get_parent (button);
              gboolean      not_free_space = !priv->free_space;
              GList        *last;
              GList        *children = clutter_container_get_children (
                                         CLUTTER_CONTAINER (priv->panel_area));

              last = g_list_last (children);

              if (last && MTP_IS_SPACE (last->data))
                priv->free_space = TRUE;
              else
                priv->free_space = FALSE;

              if ((!priv->free_space) != not_free_space)
                g_object_notify (G_OBJECT (toolbar), "free-space");

              if (parent)
                clutter_actor_reparent (button, priv->panel_area);
              else
                clutter_container_add_actor (CLUTTER_CONTAINER (
                                                             priv->panel_area),
                                             button);


              clutter_container_sort_depth_order (CLUTTER_CONTAINER (
                                                             priv->panel_area));
            }
          else
            {
              g_warning (G_STRLOC ": no space to remove!");
            }
        }

      g_signal_connect (button, "parent-set",
                        G_CALLBACK (mtp_toolbar_button_parent_set_cb),
                        toolbar);
    }
  else
    g_warning (G_STRLOC ":%s: unsupported actor type %s",
               __FUNCTION__, G_OBJECT_TYPE_NAME (button));
}

void
mtp_toolbar_remove_button (MtpToolbar *toolbar, ClutterActor *button)
{
  MtpToolbarPrivate *priv = MTP_TOOLBAR (toolbar)->priv;

  priv->modified = TRUE;

  if (MTP_IS_SPACE (button))
    {
      clutter_container_remove_actor (CLUTTER_CONTAINER (priv->panel_area),
                                      button);
    }
  else if (mtp_toolbar_button_is_clock ((MtpToolbarButton*)button))
    {
      clutter_container_remove_actor (CLUTTER_CONTAINER (priv->clock_area),
                                      button);
    }
  else if (MTP_IS_TOOLBAR_BUTTON (button))
    {
      if (mtp_toolbar_button_is_applet ((MtpToolbarButton*)button))
        clutter_container_remove_actor (CLUTTER_CONTAINER (priv->applet_area),
                                        button);
      else
        clutter_container_remove_actor (CLUTTER_CONTAINER (priv->panel_area),
                                        button);
    }
  else
    g_warning (G_STRLOC ":%s: unsupported actor type %s",
               __FUNCTION__, G_OBJECT_TYPE_NAME (button));
}

/*
 * The MxBoxLayout container does really nice animations for us when we
 * move buttons around, but it does not support inserting an item at a specific
 * position. We work around this taking advantage of the fact that the actors
 * inside this container are always sorted by their depth; we use a very small
 * depth increment, so that even for a large number of buttons the effective
 * depth still remains below 1px.
 */
typedef struct _DepthData
{
  gfloat        depth;
  gfloat        new_depth;
  ClutterActor *before;
  ClutterActor *new;
  gboolean      applets;
} DepthData;

static void
set_depth_for_each (ClutterActor *child, gpointer data)
{
  DepthData *ddata = data;

  if (child == ddata->before)
    ddata->new_depth = ddata->depth;

  if (child != ddata->new)
    {
      if (child == ddata->before)
        ddata->depth += 0.05;

      clutter_actor_set_depth (child, ddata->depth);
    }

  ddata->depth += 0.05;
}

void
mtp_toolbar_insert_button (MtpToolbar   *toolbar,
                           ClutterActor *button,
                           ClutterActor *before)
{
  MtpToolbarPrivate *priv = MTP_TOOLBAR (toolbar)->priv;
  DepthData          ddata;
  GList             *children = NULL;
  ClutterActor      *parent;

  if (!before)
    {
      mtp_toolbar_add_button (toolbar, button);
      return;
    }

  parent = clutter_actor_get_parent (button);

  priv->modified = TRUE;

  ddata.depth = 0.0;
  ddata.before = before;
  ddata.new = button;
  ddata.applets = FALSE;

  if (MTP_IS_SPACE (button))
    {
      children =
        clutter_container_get_children (CLUTTER_CONTAINER (priv->panel_area));

      g_list_foreach (children, (GFunc)set_depth_for_each, &ddata);

      if (parent)
        clutter_actor_reparent (button, priv->panel_area);
      else
        clutter_container_add_actor (CLUTTER_CONTAINER (priv->panel_area),
                                     button);

      clutter_actor_set_depth (ddata.new, ddata.new_depth);
      clutter_container_sort_depth_order (CLUTTER_CONTAINER (priv->panel_area));

      clutter_container_child_set (CLUTTER_CONTAINER (priv->panel_area),
                                   button,
                                   "expand", FALSE, NULL);
    }
  else if (MTP_IS_TOOLBAR_BUTTON (button))
    {
      if (mtp_toolbar_button_is_applet ((MtpToolbarButton*)button))
        {
          ddata.applets = TRUE;

          children =
            clutter_container_get_children (CLUTTER_CONTAINER (
                                                priv->applet_area));

          g_list_foreach (children, (GFunc)set_depth_for_each, &ddata);

          if (parent)
            clutter_actor_reparent (button, priv->applet_area);
          else
            clutter_container_add_actor (CLUTTER_CONTAINER (priv->applet_area),
                                         button);

          clutter_actor_set_depth (ddata.new, ddata.new_depth);
          clutter_container_sort_depth_order (CLUTTER_CONTAINER (priv->applet_area));

          clutter_container_child_set (CLUTTER_CONTAINER (priv->applet_area),
                                       button,
                                       "expand", FALSE, NULL);
        }
      else
        {
          children =
            clutter_container_get_children (CLUTTER_CONTAINER (
                                                        priv->panel_area));

          g_list_foreach (children, (GFunc)set_depth_for_each, &ddata);

          if (parent)
            clutter_actor_reparent (button, priv->panel_area);
          else
            clutter_container_add_actor (CLUTTER_CONTAINER (priv->panel_area),
                                         button);

          clutter_actor_set_depth (ddata.new, ddata.new_depth);
          clutter_container_sort_depth_order (CLUTTER_CONTAINER (priv->panel_area));

          clutter_container_child_set (CLUTTER_CONTAINER (priv->panel_area),
                                       button,
                                       "expand", FALSE, NULL);
        }

      g_signal_connect (button, "parent-set",
                        G_CALLBACK (mtp_toolbar_button_parent_set_cb),
                        toolbar);
    }

  g_list_free (children);
}

gboolean
mtp_toolbar_was_modified (MtpToolbar *toolbar)
{
  MtpToolbarPrivate *priv = MTP_TOOLBAR (toolbar)->priv;

  return priv->modified;
}

GList *
mtp_toolbar_get_panel_buttons (MtpToolbar *toolbar)
{
  MtpToolbarPrivate *priv = MTP_TOOLBAR (toolbar)->priv;
  GList *children,  *ret = NULL, *l;

  children =
    clutter_container_get_children (CLUTTER_CONTAINER (priv->panel_area));

  for (l = children; l; l = l->next)
    if (MTP_IS_TOOLBAR_BUTTON (l->data))
      ret = g_list_append (ret, l->data);

  g_list_free (children);

  children =
    clutter_container_get_children (CLUTTER_CONTAINER (priv->clock_area));

  for (l = children; l; l = l->next)
    if (MTP_IS_TOOLBAR_BUTTON (l->data))
      ret = g_list_append (ret, l->data);

    g_list_free (children);

  return ret;
}

GList *
mtp_toolbar_get_applet_buttons (MtpToolbar *toolbar)
{
  MtpToolbarPrivate *priv = MTP_TOOLBAR (toolbar)->priv;
  GList *children, *ret = NULL, *l;

  children =
    clutter_container_get_children (CLUTTER_CONTAINER (priv->applet_area));

  for (l = children; l; l = l->next)
    if (MTP_IS_TOOLBAR_BUTTON (l->data))
      ret = g_list_append (ret, l->data);

  g_list_free (children);

  return ret;
}

void
mtp_toolbar_clear_modified_state (MtpToolbar *toolbar)
{
  MtpToolbarPrivate *priv = MTP_TOOLBAR (toolbar)->priv;

  priv->modified = FALSE;
}

void
mtp_toolbar_mark_modified (MtpToolbar *toolbar)
{
  MtpToolbarPrivate *priv = MTP_TOOLBAR (toolbar)->priv;

  priv->modified = TRUE;
}

void
mtp_toolbar_fill_space (MtpToolbar *toolbar)
{
  MtpToolbarPrivate *priv = MTP_TOOLBAR (toolbar)->priv;
  GList             *l;
  gint               i, n_panels, n_applets, max_panels = 8;
  gfloat             applet_depth = 0.0;
  gboolean           new_space = FALSE;
  ClutterActor      *stage;

  stage = clutter_actor_get_stage ((ClutterActor*)toolbar);

  if (stage)
    {
      gint screen_width = clutter_actor_get_width (stage);

      max_panels = 1 + /* for the clock */
        (screen_width-CLOCK_WIDTH-TRAY_WIDTH) / (BUTTON_WIDTH+BUTTON_SPACING);

    }

  l = clutter_container_get_children (CLUTTER_CONTAINER (priv->applet_area));
  n_applets = g_list_length (l);

  if (l)
    applet_depth = clutter_actor_get_depth (l->data) - 0.05;

  g_list_free (l);

  l = clutter_container_get_children (CLUTTER_CONTAINER (priv->panel_area));
  n_panels = g_list_length (l);

  for (i = 0; i < max_panels - n_panels; ++i)
    {
      ClutterActor *space = mtp_space_new ();

      clutter_container_add_actor (CLUTTER_CONTAINER (priv->panel_area),
                                   space);
      clutter_container_child_set (CLUTTER_CONTAINER (priv->panel_area),
                                   space,
                                   "expand", FALSE, NULL);

      new_space = TRUE;
    }

  for (i = 0; i < 4 - n_applets; ++i)
    {
      ClutterActor *space = mtp_space_new ();

      mtp_space_set_is_applet ((MtpSpace*)space, TRUE);

      clutter_container_add_actor (CLUTTER_CONTAINER (priv->applet_area),
                                   space);
      clutter_container_child_set (CLUTTER_CONTAINER (priv->applet_area),
                                   space,
                                   "expand", FALSE, NULL);
      clutter_actor_set_depth (space, applet_depth);
      applet_depth -= 0.05;
    }

  if (new_space)
    {
      gboolean not_free_space = !priv->free_space;

      priv->free_space = TRUE;

      if (not_free_space)
        g_object_notify (G_OBJECT (toolbar), "free-space");
    }

  g_list_free (l);
}

gboolean
mtp_toolbar_has_free_space (MtpToolbar *toolbar)
{
  MtpToolbarPrivate *priv = MTP_TOOLBAR (toolbar)->priv;

  return priv->free_space;
}
