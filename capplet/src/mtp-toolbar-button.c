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

#include <string.h>

#include "mtp-toolbar-button.h"
#include "mtp-toolbar.h"
#include "mtp-jar.h"
#include "mtp-clock.h"

/*
 * MnbToolbarButton -- a helper widget for MtpToolbarButton
 */
#define MNB_TYPE_TOOLBAR_BUTTON mnb_toolbar_button_get_type()

#define MNB_TOOLBAR_BUTTON(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), MNB_TYPE_TOOLBAR_BUTTON, MnbToolbarButton))

#define MNB_TOOLBAR_BUTTON_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), MNB_TYPE_TOOLBAR_BUTTON, MnbToolbarButtonClass))

#define MNB_IS_TOOLBAR_BUTTON(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MNB_TYPE_TOOLBAR_BUTTON))

#define MNB_IS_TOOLBAR_BUTTON_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), MNB_TYPE_TOOLBAR_BUTTON))

#define MNB_TOOLBAR_BUTTON_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), MNB_TYPE_TOOLBAR_BUTTON, MnbToolbarButtonClass))

typedef struct {
  MxWidget parent;
} MnbToolbarButton;

typedef struct {
  MxWidgetClass parent_class;
} MnbToolbarButtonClass;

GType mnb_toolbar_button_get_type (void);

G_DEFINE_TYPE (MnbToolbarButton, mnb_toolbar_button, MX_TYPE_WIDGET);

static void
mnb_toolbar_button_class_init (MnbToolbarButtonClass *klass)
{
}

static void
mnb_toolbar_button_init (MnbToolbarButton *self)
{
}

/*
 * MtpToolbarButton
 */
static void mx_draggable_iface_init (MxDraggableIface *iface);

G_DEFINE_TYPE_WITH_CODE (MtpToolbarButton, mtp_toolbar_button, MX_TYPE_WIDGET,
                         G_IMPLEMENT_INTERFACE (MX_TYPE_DRAGGABLE,
                                                mx_draggable_iface_init));

#define MTP_TOOLBAR_BUTTON_GET_PRIVATE(obj)    \
        (G_TYPE_INSTANCE_GET_PRIVATE ((obj), MTP_TYPE_TOOLBAR_BUTTON, MtpToolbarButtonPrivate))

enum
{
  PROP_0 = 0,

  /* d&d properties */
  PROP_DRAG_THRESHOLD,
  PROP_AXIS,
  PROP_ENABLED,
  PROP_DRAG_ACTOR
};

enum
{
  REMOVE,

  LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0, };

struct _MtpToolbarButtonPrivate
{
  gchar *name;
  gchar *tooltip;
  gchar *button_style;
  gchar *button_stylesheet;
  gchar *service;

  ClutterActor *real_button;
  ClutterActor *close_button;
  ClutterActor *label;

  gboolean no_pick  : 1;
  gboolean applet   : 1;
  gboolean clock    : 1;
  gboolean builtin  : 1;
  gboolean required : 1;
  gboolean invalid  : 1;
  gboolean disposed : 1;
  gboolean in_jar   : 1;
  gboolean on_stage : 1;

  /* Draggable properties */
  gboolean            enabled               : 1; /* dragging enabled */
  guint               threshold;
  MxDragAxis          axis;

  /* our own draggable stuff */
  ClutterActor *orig_parent;
};

static void
mtp_toolbar_button_dispose (GObject *object)
{
  MtpToolbarButtonPrivate *priv = MTP_TOOLBAR_BUTTON (object)->priv;

  if (priv->disposed)
    return;

  priv->disposed = TRUE;

  if (priv->close_button)
    {
      clutter_actor_destroy (priv->close_button);
      priv->close_button = NULL;
    }

  G_OBJECT_CLASS (mtp_toolbar_button_parent_class)->dispose (object);
}

static void
mtp_toolbar_button_finalize (GObject *object)
{
  MtpToolbarButtonPrivate *priv = MTP_TOOLBAR_BUTTON (object)->priv;

  g_free (priv->name);
  g_free (priv->tooltip);
  g_free (priv->button_style);
  g_free (priv->button_stylesheet);
  g_free (priv->service);

  G_OBJECT_CLASS (mtp_toolbar_button_parent_class)->finalize (object);
}

static void
mtp_toolbar_button_map (ClutterActor *actor)
{
  MtpToolbarButtonPrivate *priv = MTP_TOOLBAR_BUTTON (actor)->priv;

  CLUTTER_ACTOR_CLASS (mtp_toolbar_button_parent_class)->map (actor);

  clutter_actor_map (priv->real_button);

  if (priv->close_button && !priv->in_jar && !priv->on_stage)
    clutter_actor_map (priv->close_button);

  if (priv->label && priv->in_jar)
    clutter_actor_map (priv->label);
}

static void
mtp_toolbar_button_unmap (ClutterActor *actor)
{
  MtpToolbarButtonPrivate *priv = MTP_TOOLBAR_BUTTON (actor)->priv;

  clutter_actor_unmap (priv->real_button);

  if (priv->close_button)
    clutter_actor_unmap (priv->close_button);

  if (priv->label)
    clutter_actor_unmap (priv->label);

  CLUTTER_ACTOR_CLASS (mtp_toolbar_button_parent_class)->unmap (actor);
}

static void
mtp_toolbar_button_allocate (ClutterActor          *actor,
                             const ClutterActorBox *box,
                             ClutterAllocationFlags flags)
{
  MtpToolbarButtonPrivate *priv = MTP_TOOLBAR_BUTTON (actor)->priv;
  ClutterActorBox          childbox;
  MxPadding                padding;
  gfloat                   button_width;

  CLUTTER_ACTOR_CLASS (
             mtp_toolbar_button_parent_class)->allocate (actor,
                                                         box,
                                                         flags);

  mx_widget_get_padding (MX_WIDGET (actor), &padding);

  button_width = priv->in_jar ? (priv->clock ? 120.0 :
                                 (priv->applet ? 40.0 : 60.0)) :
    box->x2 - box->x1 - padding.left - padding.right;

  childbox.x1 = padding.left;
  childbox.y1 = padding.top;
  childbox.x2 = childbox.x1 + button_width;
  childbox.y2 = childbox.y1 +
    (box->y2 - box->y1 - padding.top - padding.bottom);

  clutter_actor_allocate (priv->real_button, &childbox, flags);

  if (priv->close_button)
    {
      childbox.x1 = 0;
      childbox.y1 = 0;
      childbox.x2 = box->x2 - box->x1 - 0.0;
      childbox.y2 = box->y2 - box->y1;

      mx_allocate_align_fill (priv->close_button, &childbox, MX_ALIGN_END,
                              MX_ALIGN_START, FALSE, FALSE);

      clutter_actor_allocate (priv->close_button, &childbox, flags);
    }

  if (priv->label && priv->in_jar)
    {
      childbox.x1 = padding.left + button_width + 10.0;
      childbox.y1 = padding.top;
      childbox.x2 = box->x2 - box->x1 - padding.left - padding.right;
      childbox.y2 = box->y2 - box->y1 - padding.bottom;

      mx_allocate_align_fill (priv->label, &childbox, MX_ALIGN_MIDDLE,
                              MX_ALIGN_MIDDLE, FALSE, FALSE);

      clutter_actor_allocate (priv->label, &childbox, flags);
    }
}

static void
mtp_toolbar_button_drag_begin (MxDraggable         *draggable,
                               gfloat               event_x,
                               gfloat               event_y,
                               gint                 event_button,
                               ClutterModifierType  modifiers)
{
  MtpToolbarButtonPrivate *priv = MTP_TOOLBAR_BUTTON (draggable)->priv;
  ClutterActor            *self  = CLUTTER_ACTOR (draggable);
  ClutterActor            *parent;
  ClutterActor            *stage;
  gfloat                   width, height;

  stage = clutter_actor_get_stage (self);

  /*
   * Hide any tooltip we might be showing.
   */
  mx_widget_hide_tooltip (MX_WIDGET (draggable));

  /*
   * Store the original parent.
   */
  parent = clutter_actor_get_parent (self);

  while (parent && !(MTP_IS_JAR (parent) || MTP_IS_TOOLBAR (parent)))
    parent = clutter_actor_get_parent (parent);

  if (!parent)
    {
      priv->orig_parent = NULL;
      return;
    }

  priv->orig_parent = parent;

  /*
   * Release self from the Zone by reparenting to stage, so we can move about
   */
  clutter_actor_reparent (self, stage);

  clutter_actor_set_opacity (self, 0xbf);

  clutter_actor_set_rotation (self, CLUTTER_Z_AXIS, -10.0, 0.0, 0.0, 0.0);
  clutter_actor_set_rotation (self, CLUTTER_Y_AXIS, -15.0, 0.0, 0.0, 0.0);

  /*
   * Reparent to stage, preserving size and position
   *
   * FIXME -- this does not work, something somewhere seems to be moving the
   * draggable to be centered under the pointer, which we do not want, since
   * our draggable changes size when the drag begins!
   */
  clutter_actor_get_preferred_size (self, NULL, NULL, &width, &height);

  clutter_actor_set_size (self, width, height);
  clutter_actor_set_position (self,
                              event_x - width / 2.0, event_y - height / 2.0);
}

static void
mtp_toolbar_button_drag_motion (MxDraggable *draggable,
                                gfloat       delta_x,
                                gfloat       delta_y)
{
  clutter_actor_move_by (CLUTTER_ACTOR (draggable), delta_x, delta_y);
}

static void
mtp_toolbar_button_drag_end (MxDraggable *draggable,
                             gfloat       event_x,
                             gfloat       event_y)
{
  MtpToolbarButtonPrivate *priv = MTP_TOOLBAR_BUTTON (draggable)->priv;
  ClutterActor            *self = CLUTTER_ACTOR (draggable);
  ClutterActor            *parent;
  ClutterActor            *stage;

  if (!priv->orig_parent)
    return;

  stage = clutter_actor_get_stage (self);

  /*
   * See if there was a drop (if not, we are still parented to stage)
   */
  parent = clutter_actor_get_parent (self);

  if (parent == stage)
    {
      ClutterActor *orig_parent = priv->orig_parent;

      if (MTP_IS_JAR (orig_parent))
        {
          mtp_jar_add_button ((MtpJar*) orig_parent, self);
        }
      else if (MTP_IS_TOOLBAR (orig_parent))
        {
          mtp_toolbar_readd_button ((MtpToolbar*)orig_parent, self);
        }
      else
        g_warning ("Unsupported destination %s",
                   G_OBJECT_TYPE_NAME (orig_parent));
    }
  else
    {
      ClutterActor *parent = priv->orig_parent;

      while (parent && !MTP_IS_TOOLBAR (parent))
        parent = clutter_actor_get_parent (parent);

      if (parent)
        mtp_toolbar_mark_modified ((MtpToolbar*) parent);
    }

  clutter_actor_set_size (self, -1.0, -1.0);
  clutter_actor_set_opacity (self, 0xff);
  clutter_actor_set_rotation (self, CLUTTER_Z_AXIS, 0.0, 0.0, 0.0, 0.0);
  clutter_actor_set_rotation (self, CLUTTER_Y_AXIS, 0.0, 0.0, 0.0, 0.0);
}

static void
mx_draggable_iface_init (MxDraggableIface *iface)
{
  iface->drag_begin  = mtp_toolbar_button_drag_begin;
  iface->drag_motion = mtp_toolbar_button_drag_motion;
  iface->drag_end    = mtp_toolbar_button_drag_end;
}

static void
mtp_toolbar_button_set_property (GObject      *gobject,
                                 guint         prop_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
  MtpToolbarButtonPrivate *priv = MTP_TOOLBAR_BUTTON (gobject)->priv;

  switch (prop_id)
    {
    case PROP_DRAG_THRESHOLD:
      priv->threshold = g_value_get_uint (value);
      break;
    case PROP_AXIS:
      priv->axis = g_value_get_enum (value);
      break;
    case PROP_ENABLED:
      priv->enabled = g_value_get_boolean (value);
      if (priv->enabled)
        mx_draggable_enable (MX_DRAGGABLE (gobject));
      else
        mx_draggable_disable (MX_DRAGGABLE (gobject));
      break;
    case PROP_DRAG_ACTOR:
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
      break;
    }
}

static void
mtp_toolbar_button_get_property (GObject    *gobject,
                                 guint       prop_id,
                                 GValue     *value,
                                 GParamSpec *pspec)
{
  MtpToolbarButtonPrivate *priv = MTP_TOOLBAR_BUTTON (gobject)->priv;

  switch (prop_id)
    {
    case PROP_DRAG_THRESHOLD:
      g_value_set_uint (value, priv->threshold);
      break;
    case PROP_AXIS:
      g_value_set_enum (value, priv->axis);
      break;
    case PROP_ENABLED:
      g_value_set_boolean (value, priv->enabled);
      break;
    case PROP_DRAG_ACTOR:
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
      break;
    }
}

static void
mtp_toolbar_button_get_preferred_width (ClutterActor *self,
                                        gfloat        for_height,
                                        gfloat       *min_width_p,
                                        gfloat       *natural_width_p)
{
  MtpToolbarButtonPrivate *priv = MTP_TOOLBAR_BUTTON (self)->priv;
  gfloat                   width;
  MxPadding                padding;

  mx_widget_get_padding (MX_WIDGET (self), &padding);

  if (priv->in_jar)
    width = 200.0;
  else
    width = priv->clock ? 106.0 : (priv->applet ? 42.0 : 68.0);

  width += (padding.left + padding.right);

  if (min_width_p)
    *min_width_p = width;

  if (natural_width_p)
    *natural_width_p = width;
}

static void
mtp_toolbar_button_get_preferred_height (ClutterActor *self,
                                         gfloat        for_width,
                                         gfloat       *min_height_p,
                                         gfloat       *natural_height_p)
{
  gfloat                   height;
  MxPadding                padding;

  mx_widget_get_padding (MX_WIDGET (self), &padding);

  height = 60.0 + (padding.top + padding.bottom);

  if (min_height_p)
    *min_height_p = height;

  if (natural_height_p)
    *natural_height_p = height;
}

static void
mtp_toolbar_button_pick (ClutterActor *self, const ClutterColor *color)
{
  MtpToolbarButtonPrivate *priv = MTP_TOOLBAR_BUTTON (self)->priv;

  if (priv->no_pick)
    return;

  CLUTTER_ACTOR_CLASS (mtp_toolbar_button_parent_class)->pick (self, color);

  if (priv->close_button)
    clutter_actor_paint (priv->close_button);
}

static void
mtp_toolbar_button_paint (ClutterActor *self)
{
  MtpToolbarButtonPrivate *priv = MTP_TOOLBAR_BUTTON (self)->priv;

  CLUTTER_ACTOR_CLASS (mtp_toolbar_button_parent_class)->paint (self);

  clutter_actor_paint (priv->real_button);

  if (priv->close_button && CLUTTER_ACTOR_IS_MAPPED (priv->close_button))
    clutter_actor_paint (priv->close_button);

  if (priv->label && CLUTTER_ACTOR_IS_MAPPED (priv->label))
    clutter_actor_paint (priv->label);
}

static void
mtp_toolbar_button_parent_set (ClutterActor *button, ClutterActor *old_parent)
{
  MtpToolbarButtonPrivate *priv = MTP_TOOLBAR_BUTTON (button)->priv;
  ClutterActor            *stables;
  ClutterActorClass       *klass;

  klass = CLUTTER_ACTOR_CLASS (mtp_toolbar_button_parent_class);

  if (klass->parent_set)
    klass->parent_set ((ClutterActor*)button, old_parent);

  stables = clutter_actor_get_parent (button);

  if (!stables)
    return;

  while (stables && !(MTP_IS_TOOLBAR (stables) || MTP_IS_JAR (stables)))
    stables = clutter_actor_get_parent (stables);

  if (!stables)
    {
      priv->in_jar   = FALSE;
      priv->on_stage = TRUE;

      clutter_actor_set_name (button, NULL);

      if (priv->tooltip)
        mx_widget_set_tooltip_text (MX_WIDGET (button), NULL);

      if (priv->label && CLUTTER_ACTOR_IS_MAPPED (priv->label))
        clutter_actor_unmap (priv->label);

      if (priv->close_button && CLUTTER_ACTOR_IS_MAPPED (priv->close_button))
        clutter_actor_unmap (priv->close_button);
    }
  else if (MTP_IS_TOOLBAR (stables))
    {
      priv->in_jar   = FALSE;
      priv->on_stage = FALSE;

      clutter_actor_set_name (button, NULL);

      if (priv->tooltip)
        mx_widget_set_tooltip_text (MX_WIDGET (button), priv->tooltip);

      if (priv->label && CLUTTER_ACTOR_IS_MAPPED (priv->label))
        clutter_actor_unmap (priv->label);

      if (priv->close_button &&
          CLUTTER_ACTOR_IS_MAPPED (button) &&
          !CLUTTER_ACTOR_IS_MAPPED (priv->close_button))
        clutter_actor_map (priv->close_button);
    }
  else if (MTP_IS_JAR (stables))
    {
      priv->in_jar   = TRUE;
      priv->on_stage = FALSE;

      clutter_actor_set_name (button, "in-jar");

      if (priv->tooltip)
        mx_widget_set_tooltip_text (MX_WIDGET (button), NULL);

      if (priv->label && CLUTTER_ACTOR_IS_MAPPED (button) &&
          !CLUTTER_ACTOR_IS_MAPPED (priv->label))
        {
          clutter_actor_map (priv->label);
        }

      if (priv->close_button && CLUTTER_ACTOR_IS_MAPPED (priv->close_button))
          clutter_actor_unmap (priv->close_button);
    }
}

static void
mtp_toolbar_button_class_init (MtpToolbarButtonClass *klass)
{
  ClutterActorClass *actor_class  = CLUTTER_ACTOR_CLASS (klass);
  GObjectClass      *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MtpToolbarButtonPrivate));

  actor_class->allocate             = mtp_toolbar_button_allocate;
  actor_class->map                  = mtp_toolbar_button_map;
  actor_class->unmap                = mtp_toolbar_button_unmap;
  actor_class->get_preferred_width  = mtp_toolbar_button_get_preferred_width;
  actor_class->get_preferred_height = mtp_toolbar_button_get_preferred_height;
  actor_class->pick                 = mtp_toolbar_button_pick;
  actor_class->paint                = mtp_toolbar_button_paint;
  actor_class->parent_set           = mtp_toolbar_button_parent_set;

  object_class->dispose             = mtp_toolbar_button_dispose;
  object_class->finalize            = mtp_toolbar_button_finalize;
  object_class->set_property        = mtp_toolbar_button_set_property;
  object_class->get_property        = mtp_toolbar_button_get_property;

  g_object_class_override_property (object_class,
                                    PROP_DRAG_THRESHOLD,
                                    "drag-threshold");
  g_object_class_override_property (object_class,
                                    PROP_AXIS,
                                    "axis");
  g_object_class_override_property (object_class,
                                    PROP_ENABLED,
                                    "drag-enabled");
  g_object_class_override_property (object_class,
                                    PROP_DRAG_ACTOR,
                                    "drag-actor");


  signals[REMOVE] = g_signal_new ("remove",
                                  G_TYPE_FROM_CLASS (klass),
                                  G_SIGNAL_RUN_LAST,
                                  0, NULL, NULL,
                                  g_cclosure_marshal_VOID__VOID,
                                  G_TYPE_NONE, 0);
}

static void
mtp_toolbar_button_init (MtpToolbarButton *self)
{
  MtpToolbarButtonPrivate *priv;

  priv = self->priv = MTP_TOOLBAR_BUTTON_GET_PRIVATE (self);

  priv->threshold = 5;
  priv->axis = 0;

  clutter_actor_set_reactive ((ClutterActor*)self, TRUE);

  priv->real_button = g_object_new (MNB_TYPE_TOOLBAR_BUTTON, NULL);
  clutter_actor_set_parent (priv->real_button, (ClutterActor*)self);
  clutter_actor_set_reactive (priv->real_button, TRUE);
}

static void
mtp_toolbar_button_cbutton_clicked_cb (ClutterActor     *cbutton,
                                       MtpToolbarButton *button)
{
  g_signal_emit (button, signals[REMOVE], 0);
}

static void
mtp_toolbar_button_set_child (MtpToolbarButton *button, ClutterActor *child)
{
  MtpToolbarButtonPrivate *priv = MTP_TOOLBAR_BUTTON (button)->priv;

  if (priv->real_button)
    {
      clutter_actor_destroy (priv->real_button);
      priv->real_button = NULL;
    }

  if (child)
    {
      priv->real_button = child;
      clutter_actor_set_parent (child, (ClutterActor*) button);
    }
}

static void
mtp_toolbar_apply_name (MtpToolbarButton *button, const gchar *name)
{
  MtpToolbarButtonPrivate *priv = button->priv;
  gchar                   *path;
  GKeyFile                *kfile;
  GError                  *error = NULL;

  path = g_strconcat (PANELSDIR, "/", name, ".desktop", NULL);

  kfile = g_key_file_new ();

  if (!g_key_file_load_from_file (kfile, path, G_KEY_FILE_NONE, &error))
    {
      g_warning ("Failed to load %s: %s", path, error->message);
      g_clear_error (&error);
      priv->invalid = TRUE;
    }
  else
    {
      gchar    *s;
      gboolean  b;
      gboolean  builtin = FALSE;
      gchar    *stylesheet = NULL;

      b = g_key_file_get_boolean (kfile,
                                  G_KEY_FILE_DESKTOP_GROUP,
                                  "X-Meego-Panel-Optional",
                                  &error);

      /*
       * If the key does not exist (i.e., there is an error), the panel is
       * not required
       */
      if (!error)
        priv->required = !b;
      else
        g_clear_error (&error);

      if (!priv->required)
        {
          ClutterActor *cbutton;

          g_assert (!priv->close_button);

          cbutton = priv->close_button = mx_button_new ();

          clutter_actor_set_name ((ClutterActor*)cbutton, "close-button");

          clutter_actor_set_parent (cbutton, (ClutterActor*)button);

          g_signal_connect (cbutton, "clicked",
                            G_CALLBACK (mtp_toolbar_button_cbutton_clicked_cb),
                            button);

          clutter_actor_queue_relayout ((ClutterActor*)button);
        }

      s = g_key_file_get_locale_string (kfile,
                                        G_KEY_FILE_DESKTOP_GROUP,
                                        G_KEY_FILE_DESKTOP_KEY_NAME,
                                        NULL,
                                        NULL);

      priv->tooltip = s;

      s = g_key_file_get_string (kfile,
                                 G_KEY_FILE_DESKTOP_GROUP,
                                 "X-Meego-Panel-Button-Style",
                                 NULL);

      if (!s)
        {
          if (!strcmp (name, "meego-panel-myzone"))
            priv->button_style = g_strdup ("myzone-button");
          else
            priv->button_style = g_strdup_printf ("%s-button", name);
        }
      else
        priv->button_style = s;

      s = g_key_file_get_string (kfile,
                                 G_KEY_FILE_DESKTOP_GROUP,
                                 "X-Meego-Panel-Type",
                                 NULL);

      if (s)
        {
          if (!strcmp (s, "builtin"))
            priv->builtin = builtin = TRUE;
          else if (!strcmp (s, "applet"))
            priv->applet = TRUE;
          else if (!strcmp (s, "clock"))
            {
              ClutterActor *clock = mtp_clock_new ();

              priv->clock = TRUE;
              priv->applet = TRUE;
              mtp_toolbar_button_set_child (button, clock);
            }

          g_free (s);
        }

      if (!builtin)
        {
          s = g_key_file_get_string (kfile,
                                     G_KEY_FILE_DESKTOP_GROUP,
                                     "X-Meego-Panel-Stylesheet",
                                     NULL);

          priv->button_stylesheet = stylesheet = s;

          s = g_key_file_get_string (kfile,
                                     G_KEY_FILE_DESKTOP_GROUP,
                                     "X-Meego-Service",
                                     NULL);

          priv->service = s;
        }

      /*
       * Sanity check
       */
      if (!builtin && !priv->service)
        {
          g_warning ("Panel desktop file %s did not contain dbus service",
                     name);

          priv->invalid = TRUE;
        }
      else
        {
          if (stylesheet && *stylesheet)
            {
              GError  *error = NULL;

              if (!mx_style_load_from_file (mx_style_get_default (),
                                            stylesheet, &error))
                {
                  if (error)
                    g_warning ("Unable to load stylesheet %s: %s",
                               stylesheet, error->message);

                  g_clear_error (&error);
                }
            }

          clutter_actor_set_name (priv->real_button, priv->button_style);

          g_debug ("Loaded desktop file for %s: style %s",
                   name, priv->button_style);

          if (priv->tooltip)
            {
              ClutterActor *label = mx_label_new_with_text (priv->tooltip);

              priv->label = label;

              clutter_actor_set_name (label, "button-label");
              clutter_actor_set_parent (label, (ClutterActor*)button);

              mx_widget_set_tooltip_text (MX_WIDGET (button), priv->tooltip);
            }
        }
    }

  g_key_file_free (kfile);
}

void
mtp_toolbar_button_set_name (MtpToolbarButton *button, const gchar *name)
{
  MtpToolbarButtonPrivate *priv = MTP_TOOLBAR_BUTTON (button)->priv;

  g_return_if_fail (!priv->name);

  priv->name = g_strdup (name);

  mtp_toolbar_apply_name (button, name);
}

const gchar *
mtp_toolbar_button_get_name (MtpToolbarButton *button)
{
  MtpToolbarButtonPrivate *priv = MTP_TOOLBAR_BUTTON (button)->priv;

  return priv->name;
}

ClutterActor*
mtp_toolbar_button_new (void)
{
  return g_object_new (MTP_TYPE_TOOLBAR_BUTTON, NULL);
}

gboolean
mtp_toolbar_button_is_applet (MtpToolbarButton *button)
{
  MtpToolbarButtonPrivate *priv = MTP_TOOLBAR_BUTTON (button)->priv;

  return priv->applet;
}

ClutterActor *
mnb_toolbar_button_get_pre_drag_parent (MtpToolbarButton *button)
{
  MtpToolbarButtonPrivate *priv = MTP_TOOLBAR_BUTTON (button)->priv;

  return priv->orig_parent;
}

gboolean
mtp_toolbar_button_is_valid (MtpToolbarButton *button)
{
  MtpToolbarButtonPrivate *priv = MTP_TOOLBAR_BUTTON (button)->priv;

  return !priv->invalid;
}

gboolean
mtp_toolbar_button_is_required (MtpToolbarButton *button)
{
  MtpToolbarButtonPrivate *priv = MTP_TOOLBAR_BUTTON (button)->priv;

  return priv->required;
}

void
mtp_toolbar_button_set_dont_pick (MtpToolbarButton *button, gboolean dont)
{
  MtpToolbarButtonPrivate *priv = MTP_TOOLBAR_BUTTON (button)->priv;

  priv->no_pick = dont;
}

gboolean
mtp_toolbar_button_is_clock (MtpToolbarButton *button)
{
  MtpToolbarButtonPrivate *priv = MTP_TOOLBAR_BUTTON (button)->priv;

  return MTP_IS_CLOCK (priv->real_button);
}

