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
#include "mtp-space.h"

static void mx_draggable_iface_init (MxDraggableIface *iface);

/*
 * This is hack -- we need to reuse the mutter-moblin and panel styling for
 * MnbToolbarButton, which is generally defined using the type name. So, rather
 * that mixing up the formal object namespace in the capplet, we implement the
 * GObject machinery by hand, using the "MnbToolbarButton" as the type name
 * string.
 */
#if 0
G_DEFINE_TYPE_WITH_CODE (MtpToolbarButton, mtp_toolbar_button, MX_TYPE_WIDGET,
                         G_IMPLEMENT_INTERFACE (MX_TYPE_DRAGGABLE,
                                                mx_draggable_iface_init));
#else
static void mtp_toolbar_button_init (MtpToolbarButton *self);
static void mtp_toolbar_button_class_init (MtpToolbarButtonClass *klass);
static gpointer mtp_toolbar_button_parent_class = NULL;
static void mtp_toolbar_button_class_intern_init (gpointer klass)
{
  mtp_toolbar_button_parent_class = g_type_class_peek_parent (klass);
  mtp_toolbar_button_class_init ((MtpToolbarButtonClass*) klass);
}

GType
mtp_toolbar_button_get_type (void)
{
  static volatile gsize g_define_type_id__volatile = 0;
  if (g_once_init_enter (&g_define_type_id__volatile))
    {
      GType g_define_type_id =
        g_type_register_static_simple (MX_TYPE_WIDGET,
                g_intern_static_string ("MnbToolbarButton"),
                sizeof (MtpToolbarButtonClass),
                (GClassInitFunc) mtp_toolbar_button_class_intern_init,
                sizeof (MtpToolbarButton),
                (GInstanceInitFunc) mtp_toolbar_button_init,
                (GTypeFlags) 0);
      {
        const GInterfaceInfo g_implement_interface_info = {
          (GInterfaceInitFunc) mx_draggable_iface_init, NULL, NULL
        };
        g_type_add_interface_static (g_define_type_id, MX_TYPE_DRAGGABLE,
                                     &g_implement_interface_info);
      }

      g_once_init_leave (&g_define_type_id__volatile, g_define_type_id);
    }

  return g_define_type_id__volatile;
}
#endif


#define MTP_TOOLBAR_BUTTON_GET_PRIVATE(obj)    \
        (G_TYPE_INSTANCE_GET_PRIVATE ((obj), MTP_TYPE_TOOLBAR_BUTTON, MtpToolbarButtonPrivate))

enum
{
  PROP_0 = 0,

  /* d&d properties */
  PROP_DRAG_THRESHOLD,
  PROP_AXIS,
  PROP_CONTAINMENT_TYPE,
  PROP_CONTAINMENT_AREA,
  PROP_ENABLED,
  PROP_DRAG_ACTOR
};

struct _MtpToolbarButtonPrivate
{
  gchar *name;
  gchar *tooltip;
  gchar *button_style;
  gchar *button_stylesheet;
  gchar *service;

  gboolean no_pick  : 1;
  gboolean applet   : 1;
  gboolean builtin  : 1;
  gboolean required : 1;
  gboolean invalid  : 1;
  gboolean disposed : 1;

  /* Draggable properties */
  gboolean            enabled               : 1; /* dragging enabled */
  guint               threshold;
  MxDragAxis          axis;
  MxDragContainment   containment;
  ClutterActorBox     area;

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
  /* MtpToolbarButtonPrivate *priv = MTP_TOOLBAR_BUTTON (actor)->priv; */

  CLUTTER_ACTOR_CLASS (mtp_toolbar_button_parent_class)->map (actor);
}

static void
mtp_toolbar_button_unmap (ClutterActor *actor)
{
  /* MtpToolbarButtonPrivate *priv = MTP_TOOLBAR_BUTTON (actor)->priv; */

  CLUTTER_ACTOR_CLASS (mtp_toolbar_button_parent_class)->unmap (actor);
}

static void
mtp_toolbar_button_allocate (ClutterActor          *actor,
                             const ClutterActorBox *box,
                             ClutterAllocationFlags flags)
{
  /* MtpToolbarButtonPrivate *priv = MTP_TOOLBAR_BUTTON (actor)->priv; */

  CLUTTER_ACTOR_CLASS (
             mtp_toolbar_button_parent_class)->allocate (actor,
                                                         box,
                                                         flags);
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
  gfloat                   x, y, width, height;
  ClutterActor            *space;

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
   * Reparent to stage, preserving size and position
   */
  clutter_actor_get_size (self, &width, &height);

  if (!clutter_actor_transform_stage_point (self, event_x, event_y, &x, &y))
    {
      x = event_x;
      y = event_y;
    }
  else
    {
      x = event_x - x;
      y = event_y - y;
    }

  /*
   * Release self from the Zone by reparenting to stage, so we can move about
   */
  clutter_actor_reparent (self, stage);
  clutter_actor_set_position (self, x, y);
  clutter_actor_set_size (self, width, height);

  if (MTP_IS_TOOLBAR (parent))
    {
      space = mtp_space_new ();
      mtp_toolbar_add_button ((MtpToolbar*)parent, space);
    }
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
      ClutterContainer *orig_parent = CLUTTER_CONTAINER (priv->orig_parent);

      /*
       * This is the case where the drop was cancelled; put ourselves back
       * where we were.
       */
      g_object_ref (self);

      clutter_container_remove_actor (CLUTTER_CONTAINER (parent), self);

      if (MTP_IS_JAR (orig_parent))
        {
          mtp_jar_add_button ((MtpJar*) orig_parent, self);
        }
      else if (MTP_IS_TOOLBAR (orig_parent))
        {
          mtp_toolbar_add_button ((MtpToolbar*)orig_parent, self);
        }
      else
        g_warning ("Unsupported destination %s",
                   G_OBJECT_TYPE_NAME (orig_parent));

      g_object_unref (self);
    }
  else
    {
      ClutterActor *parent = priv->orig_parent;

      while (parent && !MTP_IS_TOOLBAR (parent))
        parent = clutter_actor_get_parent (parent);

      if (parent)
        mtp_toolbar_mark_modified ((MtpToolbar*) parent);
    }
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
    case PROP_CONTAINMENT_TYPE:
      priv->containment = g_value_get_enum (value);
      break;
    case PROP_CONTAINMENT_AREA:
      {
        ClutterActorBox *box = g_value_get_boxed (value);

        if (box)
          priv->area = *box;
        else
          memset (&priv->area, 0, sizeof (ClutterActorBox));
      }
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
    case PROP_CONTAINMENT_TYPE:
      g_value_set_enum (value, priv->containment);
      break;
    case PROP_CONTAINMENT_AREA:
      g_value_set_boxed (value, &priv->area);
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
  if (min_width_p)
    *min_width_p = 70.0;

  if (natural_width_p)
    *natural_width_p = 70.0;
}

static void
mtp_toolbar_button_get_preferred_height (ClutterActor *self,
                                         gfloat        for_width,
                                         gfloat       *min_height_p,
                                         gfloat       *natural_height_p)
{
  if (min_height_p)
    *min_height_p = 60.0;

  if (natural_height_p)
    *natural_height_p = 60.0;
}

static void
mtp_toolbar_button_pick (ClutterActor *self, const ClutterColor *color)
{
  MtpToolbarButtonPrivate *priv = MTP_TOOLBAR_BUTTON (self)->priv;

  if (priv->no_pick)
    return;

  CLUTTER_ACTOR_CLASS (mtp_toolbar_button_parent_class)->pick (self, color);
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
                                    PROP_CONTAINMENT_TYPE,
                                    "containment-type");
  g_object_class_override_property (object_class,
                                    PROP_CONTAINMENT_AREA,
                                    "containment-area");
  g_object_class_override_property (object_class,
                                    PROP_ENABLED,
                                    "enabled");
  g_object_class_override_property (object_class,
                                    PROP_DRAG_ACTOR,
                                    "drag-actor");
}

static void
mtp_toolbar_button_init (MtpToolbarButton *self)
{
  self->priv = MTP_TOOLBAR_BUTTON_GET_PRIVATE (self);
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
                                  "X-Moblin-Panel-Optional",
                                  &error);

      /*
       * If the key does not exist (i.e., there is an error), the panel is
       * not required
       */
      if (!error)
        priv->required = !b;
      else
        g_clear_error (&error);

      s = g_key_file_get_locale_string (kfile,
                                        G_KEY_FILE_DESKTOP_GROUP,
                                        G_KEY_FILE_DESKTOP_KEY_NAME,
                                        NULL,
                                        NULL);

      priv->tooltip = s;

      s = g_key_file_get_string (kfile,
                                 G_KEY_FILE_DESKTOP_GROUP,
                                 "X-Moblin-Panel-Button-Style",
                                 NULL);

      if (!s)
        {
          if (!strcmp (name, "moblin-panel-myzone"))
            priv->button_style = g_strdup ("myzone-button");
          else
            priv->button_style = g_strdup_printf ("%s-button", name);
        }
      else
        priv->button_style = s;

      s = g_key_file_get_string (kfile,
                                 G_KEY_FILE_DESKTOP_GROUP,
                                 "X-Moblin-Panel-Type",
                                 NULL);

      if (s)
        {
          if (!strcmp (s, "builtin"))
            priv->builtin = builtin = TRUE;
          else if (!strcmp (s, "applet"))
            priv->applet = TRUE;

          g_free (s);
        }

      if (!builtin)
        {
          s = g_key_file_get_string (kfile,
                                     G_KEY_FILE_DESKTOP_GROUP,
                                     "X-Moblin-Panel-Stylesheet",
                                     NULL);

          priv->button_stylesheet = stylesheet = s;

          s = g_key_file_get_string (kfile,
                                     G_KEY_FILE_DESKTOP_GROUP,
                                     "X-Moblin-Service",
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

          clutter_actor_set_name ((ClutterActor*)button, priv->button_style);

          g_debug ("Loaded desktop file for %s: style %s",
                   name, priv->button_style);

          if (priv->tooltip)
            mx_widget_set_tooltip_text (MX_WIDGET (button), priv->tooltip);
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
