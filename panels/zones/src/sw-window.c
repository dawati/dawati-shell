/*
 * Copyright © 2009, 2010, Intel Corporation.
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
 *
 * Authors: Thomas Wood <thomas.wood@intel.com>
 */

#include "sw-window.h"
#include "sw-zone.h"

#include <clutter/clutter-keysyms.h>

static void mx_draggable_iface_init (MxDraggableIface *iface);
static void mx_focusable_iface_init (MxFocusableIface *iface);

G_DEFINE_TYPE_WITH_CODE (SwWindow, sw_window, MX_TYPE_WIDGET,
                         G_IMPLEMENT_INTERFACE (MX_TYPE_DRAGGABLE, mx_draggable_iface_init)
                         G_IMPLEMENT_INTERFACE (MX_TYPE_FOCUSABLE, mx_focusable_iface_init))

#define WINDOW_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), SW_TYPE_WINDOW, SwWindowPrivate))

#define INFO_BOX_HEIGHT 32
#define SPACING 14
#define LABEL_SPACING 8

enum
{
  CLICKED,
  WORKSPACE_CHANGED,
  CLOSE,

  LAST_SIGNAL
};

static guint window_signals[LAST_SIGNAL] = { 0, };

enum
{
  PROP_ENABLED = 1,
  PROP_AXIS,
  PROP_DRAG_ACTOR,
  PROP_DRAG_THRESHOLD
};

struct _SwWindowPrivate
{
  gpointer dummy;
  gboolean is_enabled : 1;
  gboolean focused;

  ClutterActorBox area;

  ClutterActor *clone;

  ClutterActor *text;
  ClutterActor *texture;
  ClutterActor *background;
  ClutterActor *close_button;
  ClutterActor *icon;

  gulong xid;

  gint workspace;
};

/* focusable implementation */

static MxFocusable *
sw_window_accept_focus (MxFocusable *focusable,
                        MxFocusHint  hint)
{
  clutter_actor_grab_key_focus (CLUTTER_ACTOR (focusable));

  mx_stylable_set_style_pseudo_class(MX_STYLABLE (focusable), "hover");

  return focusable;
}

static MxFocusable *
sw_window_move_focus (MxFocusable *focusable,
                      MxFocusDirection direction,
                      MxFocusable *from)
{
  if (from == focusable)
    mx_stylable_set_style_pseudo_class (MX_STYLABLE (focusable), "");

  return NULL;
}



static void
mx_focusable_iface_init (MxFocusableIface *iface)
{
  iface->accept_focus = sw_window_accept_focus;
  iface->move_focus = sw_window_move_focus;
}


/* draggable implementation */
static void
sw_window_drag_begin (MxDraggable         *draggable,
                      gfloat               event_x,
                      gfloat               event_y,
                      gint                 event_button,
                      ClutterModifierType  modifiers)
{
  SwWindowPrivate *priv = SW_WINDOW (draggable)->priv;
  gfloat orig_x, orig_y, orig_height, orig_width;
  ClutterActor *stage;
  ClutterActor *actor;

  g_object_ref (draggable);

  actor = CLUTTER_ACTOR (draggable);

  stage = clutter_actor_get_stage (actor);

  if (priv->clone)
    clutter_actor_destroy (priv->clone);
  priv->clone = clutter_clone_new (actor);

  clutter_actor_get_transformed_position (actor, &orig_x, &orig_y);
  clutter_actor_get_size (actor, &orig_width, &orig_height);

  clutter_container_add_actor (CLUTTER_CONTAINER (stage), priv->clone);
  clutter_actor_set_position (priv->clone, orig_x, orig_y);
  clutter_actor_set_size (priv->clone, orig_width, orig_height);

  clutter_actor_set_opacity (actor, 0x55);

  g_object_unref (draggable);
}

static void
sw_window_drag_motion (MxDraggable *draggable,
                       gfloat       delta_x,
                       gfloat       delta_y)
{
  SwWindowPrivate *priv = SW_WINDOW (draggable)->priv;
  gfloat orig_x, orig_y;

  clutter_actor_get_transformed_position (CLUTTER_ACTOR (draggable),
                                          &orig_x, &orig_y);

  clutter_actor_set_position (CLUTTER_ACTOR (priv->clone), orig_x + delta_x,
                              orig_y + delta_y);

}

static void
sw_window_clone_animation_completed (ClutterAnimation *animation,
                                     SwWindowPrivate  *priv)
{
  clutter_actor_destroy (priv->clone);
  priv->clone = NULL;
}

static void
sw_window_drag_end (MxDraggable *draggable,
                    gfloat       event_x,
                    gfloat       event_y)
{
  gfloat x, y, width, height;
  SwZone *zone;
  gint num;

  SwWindowPrivate *priv = SW_WINDOW (draggable)->priv;

  clutter_actor_get_size (CLUTTER_ACTOR (draggable), &width, &height);
  clutter_actor_get_transformed_position (CLUTTER_ACTOR (draggable),
                                          &x, &y);
  clutter_actor_animate (CLUTTER_ACTOR (priv->clone), CLUTTER_LINEAR, 200,
                         "x", x, "y", y, "width", width, "height", height,
                         "opacity", 0x00,
                         "signal-after::completed",
                         sw_window_clone_animation_completed,
                         priv,
                         NULL);

  clutter_actor_set_opacity (CLUTTER_ACTOR (draggable), 0xff);


  zone = (SwZone*) clutter_actor_get_parent (CLUTTER_ACTOR (draggable));

  /* the parent may be NULL if the zone was destroyed */
  if (!zone)
    return;

  num = sw_zone_get_number (zone);

  sw_window_workspace_changed (SW_WINDOW (draggable), num);
}

static void
mx_draggable_iface_init (MxDraggableIface *iface)
{
  iface->drag_begin = sw_window_drag_begin;
  iface->drag_motion = sw_window_drag_motion;
  iface->drag_end = sw_window_drag_end;
}


static void
sw_window_get_property (GObject    *object,
                        guint       property_id,
                        GValue     *value,
                        GParamSpec *pspec)
{
  SwWindowPrivate *priv = SW_WINDOW (object)->priv;

  switch (property_id)
    {
  case PROP_ENABLED:
    g_value_set_boolean (value, priv->is_enabled);
    break;
  case PROP_AXIS:
    g_value_set_enum (value, 0);
    break;
  case PROP_DRAG_THRESHOLD:
    g_value_set_uint (value, 8);
    break;
  case PROP_DRAG_ACTOR:
    g_value_set_object (value, priv->clone);
    break;
  default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
sw_window_set_property (GObject      *object,
                        guint         property_id,
                        const GValue *value,
                        GParamSpec   *pspec)
{
  SwWindowPrivate *priv = SW_WINDOW (object)->priv;

  switch (property_id)
    {
  case PROP_ENABLED:
    priv->is_enabled = g_value_get_boolean (value);
    break;
  case PROP_AXIS:
  case PROP_DRAG_ACTOR:
  case PROP_DRAG_THRESHOLD:
    break;

  default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
sw_window_dispose (GObject *object)
{
  SwWindowPrivate *priv = SW_WINDOW (object)->priv;

  if (priv->text)
    {
      clutter_actor_destroy (priv->text);
      priv->text = NULL;
    }

  if (priv->close_button)
    {
      clutter_actor_destroy (priv->close_button);
      priv->close_button = NULL;
    }

  if (priv->icon)
    {
      clutter_actor_destroy (priv->icon);
      priv->icon = NULL;
    }

  if (priv->texture)
    {
      clutter_actor_destroy (priv->texture);
      priv->texture = NULL;
    }

  if (priv->background)
    {
      clutter_actor_destroy (priv->background);
      priv->background = NULL;
    }

  if (priv->clone)
    {
      clutter_actor_destroy (priv->clone);
      priv->clone = NULL;
    }

  G_OBJECT_CLASS (sw_window_parent_class)->dispose (object);
}

static void
sw_window_finalize (GObject *object)
{
  G_OBJECT_CLASS (sw_window_parent_class)->finalize (object);
}


static gboolean
sw_window_button_release_event (ClutterActor       *actor,
                                ClutterButtonEvent *event)
{
  if (event->button == 1)
    {
      g_signal_emit (actor, window_signals[CLICKED], 0);

      return TRUE;
    }

  return FALSE;
}

static gboolean
sw_window_key_release_event (ClutterActor *actor,
                             ClutterKeyEvent *event)
{
  if (event->keyval == CLUTTER_Return)
    {
      g_signal_emit (actor, window_signals[CLICKED], 0);

      return TRUE;
    }

  return FALSE;
}

static void
sw_window_map (ClutterActor *actor)
{
  SwWindowPrivate *priv = SW_WINDOW (actor)->priv;

  CLUTTER_ACTOR_CLASS (sw_window_parent_class)->map (actor);

  mx_draggable_enable (MX_DRAGGABLE (actor));

  clutter_actor_map (priv->text);
  clutter_actor_map (priv->close_button);

  if (priv->icon)
    clutter_actor_map (priv->icon);

  if (priv->texture)
    clutter_actor_map (priv->texture);

  if (priv->background)
    clutter_actor_map (priv->background);
}

static void
sw_window_unmap (ClutterActor *actor)
{
  SwWindowPrivate *priv = SW_WINDOW (actor)->priv;

  clutter_actor_unmap (priv->text);

  if (priv->texture)
    clutter_actor_unmap (priv->texture);

  if (priv->background)
    clutter_actor_unmap (priv->background);

  if (priv->icon)
    clutter_actor_unmap (priv->icon);

  CLUTTER_ACTOR_CLASS (sw_window_parent_class)->unmap (actor);
}

static void
sw_window_paint (ClutterActor *actor)
{
  SwWindowPrivate *priv = SW_WINDOW (actor)->priv;

  CLUTTER_ACTOR_CLASS (sw_window_parent_class)->paint (actor);

  if (priv->background)
    clutter_actor_paint (priv->background);

  if (priv->texture)
    clutter_actor_paint (priv->texture);

  clutter_actor_paint (priv->text);

  if (priv->icon)
    clutter_actor_paint (priv->icon);

  if (g_strcmp0 (mx_stylable_get_style_class (MX_STYLABLE (actor)),
                 "placeholder"))
    clutter_actor_paint (priv->close_button);
}

static void
sw_window_pick (ClutterActor       *actor,
                const ClutterColor *color)
{
  SwWindowPrivate *priv = SW_WINDOW (actor)->priv;

  CLUTTER_ACTOR_CLASS (sw_window_parent_class)->pick (actor, color);

  clutter_actor_paint (priv->close_button);
}

static void
sw_window_get_preferred_width (ClutterActor *actor,
                               gfloat        for_height,
                               gfloat       *min_width,
                               gfloat       *pref_width)
{
  if (min_width)
    *min_width = 240;

  if (pref_width)
    *pref_width = 240;
}

static void
sw_window_get_preferred_height (ClutterActor *actor,
                                gfloat        for_width,
                                gfloat       *min_height,
                                gfloat       *pref_height)
{
  if (min_height)
    *min_height = 240;

  if (pref_height)
    *pref_height = 240;
}

static void
sw_window_allocate (ClutterActor           *actor,
                    const ClutterActorBox  *box,
                    ClutterAllocationFlags  flags)
{
  SwWindowPrivate *priv = SW_WINDOW (actor)->priv;
  ClutterActorBox childbox, avail_box, infobox;
  gfloat label_height, icon_size, button_w, button_h;

  CLUTTER_ACTOR_CLASS (sw_window_parent_class)->allocate (actor, box, flags);

  mx_widget_get_available_area (MX_WIDGET (actor), box, &avail_box);

  infobox.x1 = avail_box.x1;
  infobox.y1 = avail_box.y2 - INFO_BOX_HEIGHT;
  infobox.x2 = avail_box.x2;
  infobox.y2 = avail_box.y2;

  /* icon */
  if (priv->icon)
    {
      icon_size = 32;

      childbox = infobox;
      mx_allocate_align_fill (priv->icon, &childbox, MX_ALIGN_START,
                              MX_ALIGN_MIDDLE, FALSE, FALSE);

      clutter_actor_allocate (priv->icon, &childbox, flags);
    }
  else
    icon_size = 0;

  /* close button */

  clutter_actor_get_preferred_size (priv->close_button, NULL, NULL, &button_w,
                                    &button_h);

  childbox = infobox;
  mx_allocate_align_fill (priv->close_button, &childbox, MX_ALIGN_END,
                          MX_ALIGN_MIDDLE, FALSE, FALSE);

  clutter_actor_allocate (priv->close_button, &childbox, flags);

  /* label */
  clutter_actor_get_preferred_height (priv->text, -1, NULL, &label_height);

  childbox.x1 = (int) (avail_box.x1 + icon_size + LABEL_SPACING);
  childbox.y1 = (int) (avail_box.y2 - (INFO_BOX_HEIGHT / 2)
                       - (label_height / 2));
  childbox.x2 = (int) MAX (childbox.x1 + 1, (avail_box.x2 - button_w - LABEL_SPACING));
  childbox.y2 = (int) (childbox.y1 + label_height);

  clutter_actor_allocate (priv->text, &childbox, flags);

  /* thumbnail */
  if (priv->texture)
    {
      mx_widget_get_available_area (MX_WIDGET (actor), box, &childbox);

      /* add a small border */
      childbox.x1 += 2;
      childbox.y1 += 2;
      childbox.x2 -= 2;
      childbox.y2 -= 2;

      childbox.y2 = MAX (childbox.y1, childbox.y2 - INFO_BOX_HEIGHT - SPACING);
      mx_allocate_align_fill (priv->texture, &childbox, MX_ALIGN_MIDDLE,
                              MX_ALIGN_MIDDLE, FALSE, FALSE);
      clutter_actor_allocate (priv->texture, &childbox, flags);
    }

  /* background */
  if (priv->background)
    {
      mx_widget_get_available_area (MX_WIDGET (actor), box, &childbox);
      childbox.y2 = MAX (childbox.y1, childbox.y2 - INFO_BOX_HEIGHT - SPACING);
      clutter_actor_allocate (priv->background, &childbox, flags);
    }
}

static void
sw_window_class_init (SwWindowClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  ClutterActorClass *actor_class = CLUTTER_ACTOR_CLASS (klass);

  g_type_class_add_private (klass, sizeof (SwWindowPrivate));

  object_class->get_property = sw_window_get_property;
  object_class->set_property = sw_window_set_property;
  object_class->dispose = sw_window_dispose;
  object_class->finalize = sw_window_finalize;

  actor_class->button_release_event = sw_window_button_release_event;
  actor_class->key_release_event = sw_window_key_release_event;
  actor_class->map = sw_window_map;
  actor_class->unmap = sw_window_unmap;
  actor_class->paint = sw_window_paint;
  actor_class->pick = sw_window_pick;
  actor_class->allocate = sw_window_allocate;
  actor_class->get_preferred_width = sw_window_get_preferred_width;
  actor_class->get_preferred_height = sw_window_get_preferred_height;


  g_object_class_override_property (object_class, PROP_ENABLED, "drag-enabled");
  g_object_class_override_property (object_class, PROP_AXIS, "axis");
  g_object_class_override_property (object_class, PROP_DRAG_ACTOR,
                                    "drag-actor");
  g_object_class_override_property (object_class, PROP_DRAG_THRESHOLD,
                                    "drag-threshold");

  window_signals[CLICKED] = g_signal_new ("clicked",
                                          G_TYPE_FROM_CLASS (klass),
                                          G_SIGNAL_RUN_LAST,
                                          0, NULL, NULL,
                                          g_cclosure_marshal_VOID__VOID,
                                          G_TYPE_NONE, 0);


  window_signals[WORKSPACE_CHANGED] = g_signal_new ("workspace-changed",
                                                    G_TYPE_FROM_CLASS (klass),
                                                    G_SIGNAL_RUN_LAST,
                                                    0, NULL, NULL,
                                                    g_cclosure_marshal_VOID__INT,
                                                    G_TYPE_NONE,
                                                    1, G_TYPE_INT);

  window_signals[CLOSE] = g_signal_new ("close",
                                        G_TYPE_FROM_CLASS (klass),
                                        G_SIGNAL_RUN_LAST,
                                        0, NULL, NULL,
                                        g_cclosure_marshal_VOID__VOID,
                                        G_TYPE_NONE, 0);

}

static void
sw_window_close_clicked_cb (MxButton *button,
                            SwWindow *window)
{
  g_signal_emit (window, window_signals[CLOSE], 0);
}

static void
sw_window_init (SwWindow *self)
{
  self->priv = WINDOW_PRIVATE (self);
  clutter_actor_set_reactive (CLUTTER_ACTOR (self), TRUE);

  /* label */
  self->priv->text = clutter_text_new ();
  clutter_actor_set_parent (self->priv->text, CLUTTER_ACTOR (self));
  clutter_text_set_ellipsize (CLUTTER_TEXT (self->priv->text),
                              PANGO_ELLIPSIZE_END);

  /* close button */
  self->priv->close_button = mx_button_new ();
  clutter_actor_set_parent (self->priv->close_button, CLUTTER_ACTOR (self));
  g_signal_connect (self->priv->close_button, "clicked",
                    G_CALLBACK (sw_window_close_clicked_cb), self);

}

ClutterActor *
sw_window_new (void)
{
  return g_object_new (SW_TYPE_WINDOW, NULL);
}

void
sw_window_set_xid (SwWindow *window,
                   gulong    xid)
{
  SwWindowPrivate *priv = SW_WINDOW (window)->priv;

  priv->xid = xid;
}

gulong
sw_window_get_xid (SwWindow *window)
{
  return window->priv->xid;
}

void
sw_window_set_thumbnail (SwWindow     *window,
                         ClutterActor *thumbnail)
{
  SwWindowPrivate *priv = SW_WINDOW (window)->priv;

  if (priv->texture)
    clutter_actor_destroy (priv->texture);

  priv->texture = thumbnail;
  clutter_actor_set_parent (priv->texture, CLUTTER_ACTOR (window));
}

void
sw_window_set_background (SwWindow     *window,
                          ClutterActor *background)
{
  SwWindowPrivate *priv = SW_WINDOW (window)->priv;

  if (priv->background)
    clutter_actor_destroy (priv->background);

  priv->background = background;
  clutter_actor_set_parent (priv->background, CLUTTER_ACTOR (window));
}

void
sw_window_set_icon (SwWindow *window,
                    ClutterTexture *icon)
{
  SwWindowPrivate *priv = SW_WINDOW (window)->priv;

  if (priv->icon)
    clutter_actor_destroy (priv->icon);

  priv->icon = CLUTTER_ACTOR (icon);
  clutter_actor_set_parent (priv->icon, CLUTTER_ACTOR (window));
}

void
sw_window_set_focused (SwWindow *window,
                       gboolean focused)
{
  window->priv->focused = focused;

  if (focused)
    clutter_actor_set_name (CLUTTER_ACTOR (window), "focused-window");
  else
    clutter_actor_set_name (CLUTTER_ACTOR (window), "");

}

void
sw_window_set_title (SwWindow    *window,
                     const gchar *title)
{
  clutter_text_set_text (CLUTTER_TEXT (window->priv->text), title);
}

void
sw_window_workspace_changed (SwWindow *window,
                             gint      new_workspace)
{
  if (new_workspace != window->priv->workspace)
    {
      window->priv->workspace = new_workspace;

      g_signal_emit (window, window_signals[WORKSPACE_CHANGED], 0,
                     new_workspace);
    }
}
