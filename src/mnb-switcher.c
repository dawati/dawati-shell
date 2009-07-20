/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */

/*
 * Moblin Netbook
 * Copyright © 2009, Intel Corporation.
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
f */

#include "mnb-switcher.h"
#include "moblin-netbook.h"
#include "mnb-toolbar.h"

#include <string.h>
#include <display.h>
#include <clutter/clutter.h>
#include <clutter/x11/clutter-x11.h>
#include <nbtk/nbtk.h>
#include <keybindings.h>

#include <glib/gi18n.h>

#define HOVER_TIMEOUT  800

static ClutterActor *table_find_child (ClutterContainer *, gint, gint);
static gint          tablist_sort_func (gconstpointer a, gconstpointer b);
static gint mnb_switcher_get_active_workspace (MnbSwitcher *switcher);
static NbtkTable *mnb_switcher_append_workspace (MnbSwitcher *switcher);

struct _MnbSwitcherPrivate {
  MutterPlugin *plugin;
  NbtkWidget   *table;
  NbtkWidget   *new_workspace;
  NbtkWidget   *new_label;
  NbtkTooltip  *active_tooltip;
  GList        *last_workspaces;
  GList        *global_tab_list;

  ClutterActor *last_focused;
  MutterWindow *selected;
  GList        *tab_list;

  guint         show_completed_id;
  guint         hide_panel_cb_id;

  gboolean      dnd_in_progress : 1;
  gboolean      constructing    : 1;
  gboolean      in_alt_grab     : 1;

  gint          active_ws;
};

/*
 * MnbSwitcherApp
 *
 * A NbtkBin subclass represening a single thumb in the switcher.
 *
 * FIXME -- MnbSwitcherApp and MnbSwitcherZone should be split out into their
 *          own source files (they were meant to be really trivial classes, but
 *          with the new d&d it's got really out of hand).
 */
#define MNB_TYPE_SWITCHER_APP                 (mnb_switcher_app_get_type ())
#define MNB_SWITCHER_APP(obj)                 (G_TYPE_CHECK_INSTANCE_CAST ((obj), MNB_TYPE_SWITCHER_APP, MnbSwitcherApp))
#define MNB_IS_SWITCHER_APP(obj)              (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MNB_TYPE_SWITCHER_APP))
#define MNB_SWITCHER_APP_CLASS(klass)         (G_TYPE_CHECK_CLASS_CAST ((klass), MNB_TYPE_SWITCHER_APP, MnbSwitcherAppClass))
#define MNB_IS_SWITCHER_APP_CLASS(klass)      (G_TYPE_CHECK_CLASS_TYPE ((klass), MNB_TYPE_SWITCHER_APP))
#define MNB_SWITCHER_APP_GET_CLASS(obj)       (G_TYPE_INSTANCE_GET_CLASS ((obj), MNB_TYPE_SWITCHER_APP, MnbSwitcherAppClass))

typedef struct _MnbSwitcherApp               MnbSwitcherApp;
typedef struct _MnbSwitcherAppPrivate        MnbSwitcherAppPrivate;
typedef struct _MnbSwitcherAppClass          MnbSwitcherAppClass;

#define MNB_TYPE_SWITCHER_ZONE                 (mnb_switcher_zone_get_type ())
#define MNB_SWITCHER_ZONE(obj)                 (G_TYPE_CHECK_INSTANCE_CAST ((obj), MNB_TYPE_SWITCHER_ZONE, MnbSwitcherZone))
#define MNB_IS_SWITCHER_ZONE(obj)              (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MNB_TYPE_SWITCHER_ZONE))
#define MNB_SWITCHER_ZONE_CLASS(klass)         (G_TYPE_CHECK_CLASS_CAST ((klass), MNB_TYPE_SWITCHER_ZONE, MnbSwitcherZoneClass))
#define MNB_IS_SWITCHER_ZONE_CLASS(klass)      (G_TYPE_CHECK_CLASS_TYPE ((klass), MNB_TYPE_SWITCHER_ZONE))
#define MNB_SWITCHER_ZONE_GET_CLASS(obj)       (G_TYPE_INSTANCE_GET_CLASS ((obj), MNB_TYPE_SWITCHER_ZONE, MnbSwitcherZoneClass))

typedef struct _MnbSwitcherZone               MnbSwitcherZone;
typedef struct _MnbSwitcherZonePrivate        MnbSwitcherZonePrivate;
typedef struct _MnbSwitcherZoneClass          MnbSwitcherZoneClass;

typedef enum
  {
    MNB_SWITCHER_ZONE_NORMAL,
    MNB_SWITCHER_ZONE_ACTIVE,
    MNB_SWITCHER_ZONE_HOVER,
  }
  MnbSwitcherZoneState;

GType mnb_switcher_zone_get_type (void);
static void mnb_switcher_zone_set_state (MnbSwitcherZone *zone,
                                         MnbSwitcherZoneState state);

struct _MnbSwitcherApp
{
  /*< private >*/
  NbtkBin parent_instance;

  MnbSwitcherAppPrivate *priv;
};

struct _MnbSwitcherAppClass
{
  /*< private >*/
  NbtkBinClass parent_class;
};

struct _MnbSwitcherAppPrivate
{
  MnbSwitcher  *switcher;
  MutterWindow *mw;
  guint         hover_timeout_id;
  ClutterActor *tooltip;
  guint         focus_id;
  guint         raised_id;

  /* Draggable properties */
  guint               threshold;
  NbtkDragAxis        axis;
  NbtkDragContainment containment;
  ClutterActorBox     area;

  gboolean            enabled               : 1;
  gboolean            ignore_button_release : 1; /* Set to ingore release
                                                  * immediately following drop
                                                  */

  gint                orig_col;
  gint                orig_row;
  ClutterActor       *orig_parent;
  ClutterActor       *clone;
};

enum
{
  APP_PROP_0 = 0,

  APP_PROP_DRAG_THRESHOLD,
  APP_PROP_AXIS,
  APP_PROP_CONTAINMENT_TYPE,
  APP_PROP_CONTAINMENT_AREA,
  APP_PROP_ENABLED
};

GType mnb_switcher_app_get_type (void);

static void mnb_switcher_enable_new_workspace (MnbSwitcher *, MnbSwitcherZone*);
static void mnb_switcher_disable_new_workspace (MnbSwitcher*,MnbSwitcherZone *);

static void nbtk_draggable_iface_init (NbtkDraggableIface *iface);

G_DEFINE_TYPE_WITH_CODE (MnbSwitcherApp,
                         mnb_switcher_app,
                         NBTK_TYPE_BIN,
                         G_IMPLEMENT_INTERFACE (NBTK_TYPE_DRAGGABLE,
                                                nbtk_draggable_iface_init));

#define MNB_SWITCHER_APP_GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MNB_TYPE_SWITCHER_APP,\
                                MnbSwitcherAppPrivate))

static void
mnb_switcher_app_dispose (GObject *object)
{
  MnbSwitcherAppPrivate *priv = MNB_SWITCHER_APP (object)->priv;
  MetaWindow            *meta_win;

  meta_win = mutter_window_get_meta_window (priv->mw);

  if (priv->hover_timeout_id)
    {
      g_source_remove (priv->hover_timeout_id);
      priv->hover_timeout_id = 0;
    }

  if (priv->focus_id)
    {
      g_signal_handler_disconnect (meta_win, priv->focus_id);
      priv->focus_id = 0;
    }

  if (priv->raised_id)
    {
      g_signal_handler_disconnect (meta_win, priv->raised_id);
      priv->raised_id = 0;
    }

  if (priv->tooltip)
    {
      clutter_container_remove_actor (CLUTTER_CONTAINER (clutter_actor_get_parent (priv->tooltip)),
                                      priv->tooltip);
    }

  G_OBJECT_CLASS (mnb_switcher_app_parent_class)->dispose (object);
}

static void
mnb_switcher_app_disable_draggable (gpointer clone, gpointer data)
{
  nbtk_draggable_disable (NBTK_DRAGGABLE (clone));
}

static void
mnb_switcher_app_allocate (ClutterActor          *actor,
                           const ClutterActorBox *box,
                           ClutterAllocationFlags flags)
{
  MnbSwitcherAppPrivate *priv = MNB_SWITCHER_APP (actor)->priv;
  ClutterGeometry area;
  gfloat x, y, w, h;

  CLUTTER_ACTOR_CLASS (mnb_switcher_app_parent_class)->allocate (actor, box, flags);

  /* set the tooltip area */
  clutter_actor_get_transformed_position (actor, &x, &y);
  clutter_actor_get_size (actor, &w, &h);

  area.x = x;
  area.y = y;
  area.width = w;
  area.height = h;
  nbtk_tooltip_set_tip_area ((NbtkTooltip*) priv->tooltip, &area);
}

static void
mnb_switcher_app_drag_begin (NbtkDraggable       *draggable,
                             gfloat               event_x,
                             gfloat               event_y,
                             gint                 event_button,
                             ClutterModifierType  modifiers)
{
  MnbSwitcherAppPrivate *app_priv = MNB_SWITCHER_APP (draggable)->priv;
  ClutterActor          *self     = CLUTTER_ACTOR (draggable);
  MnbSwitcher           *switcher = app_priv->switcher;
  MnbSwitcherPrivate    *priv     = switcher->priv;
  ClutterActor          *stage    = clutter_stage_get_default ();
  ClutterActor          *parent;
  ClutterActor          *clone;
  gfloat                 x, y, width, height;
  gint                   col, row;

  priv->dnd_in_progress = TRUE;

  if (app_priv->hover_timeout_id)
    {
      g_source_remove (app_priv->hover_timeout_id);
      app_priv->hover_timeout_id = 0;
    }

  if (CLUTTER_ACTOR_IS_MAPPED (app_priv->tooltip))
    {
      nbtk_tooltip_hide (NBTK_TOOLTIP (app_priv->tooltip));

      if (priv->active_tooltip == (NbtkTooltip*)app_priv->tooltip)
        priv->active_tooltip = NULL;
    }

  /*
   * Store the original parent and row/column, so we can put the actor
   * back if the d&d is cancelled.
   */
  parent = clutter_actor_get_parent (self);

  clutter_container_child_get (CLUTTER_CONTAINER (parent), self,
                               "col", &col,
                               "row", &row, NULL);

  app_priv->orig_parent = parent;
  app_priv->orig_col = col;
  app_priv->orig_row = row;

  mnb_switcher_enable_new_workspace (switcher, MNB_SWITCHER_ZONE (parent));

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
   * Create a clone of ourself and put it in our present place in the Zone.
   */
  clone = clutter_clone_new (self);
  app_priv->clone = clone;
  clutter_actor_set_opacity (clone, 0x7f);
  clutter_actor_set_size (clone, width, height);
  nbtk_table_add_actor (NBTK_TABLE (parent), clone, row, col);
  clutter_container_child_set (CLUTTER_CONTAINER (parent), clone,
                               "y-fill", FALSE,
                               "x-fill", FALSE,  NULL);

  /*
   * Release self from the Zone by reparenting to stage, so we can move about
   */
  clutter_actor_reparent (self, stage);
  clutter_actor_set_position (self, x, y);
  clutter_actor_set_size (self, width, height);
}

static void
mnb_switcher_app_drag_motion (NbtkDraggable *draggable,
                              gfloat         delta_x,
                              gfloat         delta_y)
{
  clutter_actor_move_by (CLUTTER_ACTOR (draggable), delta_x, delta_y);
}

static void
mnb_switcher_app_drag_end (NbtkDraggable *draggable,
                           gfloat         event_x,
                           gfloat         event_y)
{
  MnbSwitcherAppPrivate *app_priv = MNB_SWITCHER_APP (draggable)->priv;
  ClutterActor          *self     = CLUTTER_ACTOR (draggable);
  MnbSwitcher           *switcher = app_priv->switcher;
  MnbSwitcherPrivate    *priv     = switcher->priv;
  ClutterActor          *parent;
  ClutterActor          *clone;

  if (!priv->dnd_in_progress)
    {
      /*
       * The drag-end signal is emited on button release even when there was no
       * drag !
       */
      return;
    }

  mnb_switcher_disable_new_workspace (switcher,
                                  MNB_SWITCHER_ZONE (app_priv->orig_parent));

  clone = app_priv->clone;
  app_priv->clone = NULL;

  priv->dnd_in_progress = FALSE;

  /*
   * See if there was a drop (if not, we are still parented to stage)
   */
  parent = clutter_actor_get_parent (self);

  if (parent == clutter_stage_get_default ())
    {
      ClutterContainer *orig_parent = CLUTTER_CONTAINER (app_priv->orig_parent);
      gint active_ws;
      gint col = app_priv->orig_col;

      /*
       * This is the case where the drop was cancelled; put ourselves back
       * where we were.
       */
      g_object_ref (self);

      clutter_container_remove_actor (CLUTTER_CONTAINER (parent), self);
      clutter_actor_set_size (self, -1.0, -1.0);
      nbtk_table_add_actor (NBTK_TABLE (orig_parent), self,
                            app_priv->orig_row, app_priv->orig_col);

      clutter_container_child_set (orig_parent, self,
                                   "y-fill", FALSE,
                                   "x-fill", FALSE,  NULL);

      active_ws = mnb_switcher_get_active_workspace (app_priv->switcher);

      clutter_container_child_get (CLUTTER_CONTAINER (priv->table),
                                   CLUTTER_ACTOR (orig_parent),
                                   "col", &col, NULL);

      mnb_switcher_zone_set_state (MNB_SWITCHER_ZONE (orig_parent),
                                   active_ws == col ?
                                   MNB_SWITCHER_ZONE_ACTIVE :
                                   MNB_SWITCHER_ZONE_NORMAL);

      g_object_unref (self);

    }

  /*
   * Now get rid of the clone that we put in our place
   */
  parent = clutter_actor_get_parent (clone);
  clutter_container_remove_actor (CLUTTER_CONTAINER (parent), clone);
}

static void
nbtk_draggable_iface_init (NbtkDraggableIface *iface)
{
  iface->drag_begin  = mnb_switcher_app_drag_begin;
  iface->drag_motion = mnb_switcher_app_drag_motion;
  iface->drag_end    = mnb_switcher_app_drag_end;
}

static void
mnb_switcher_app_set_property (GObject      *gobject,
                               guint         prop_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
  MnbSwitcherAppPrivate *priv = MNB_SWITCHER_APP (gobject)->priv;

  switch (prop_id)
    {
    case APP_PROP_DRAG_THRESHOLD:
      priv->threshold = g_value_get_uint (value);
      break;

    case APP_PROP_AXIS:
      priv->axis = g_value_get_enum (value);
      break;

    case APP_PROP_CONTAINMENT_TYPE:
      priv->containment = g_value_get_enum (value);
      break;

    case APP_PROP_CONTAINMENT_AREA:
      {
        ClutterActorBox *box = g_value_get_boxed (value);

        if (box)
          priv->area = *box;
        else
          memset (&priv->area, 0, sizeof (ClutterActorBox));
      }
      break;

    case APP_PROP_ENABLED:
      priv->enabled = g_value_get_boolean (value);
      if (priv->enabled)
        nbtk_draggable_enable (NBTK_DRAGGABLE (gobject));
      else
        nbtk_draggable_disable (NBTK_DRAGGABLE (gobject));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
      break;
    }
}

static void
mnb_switcher_app_get_property (GObject    *gobject,
                               guint       prop_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
  MnbSwitcherAppPrivate *priv = MNB_SWITCHER_APP (gobject)->priv;

  switch (prop_id)
    {
    case APP_PROP_DRAG_THRESHOLD:
      g_value_set_uint (value, priv->threshold);
      break;

    case APP_PROP_AXIS:
      g_value_set_enum (value, priv->axis);
      break;

    case APP_PROP_CONTAINMENT_TYPE:
      g_value_set_enum (value, priv->containment);
      break;

    case APP_PROP_CONTAINMENT_AREA:
      g_value_set_boxed (value, &priv->area);
      break;

    case APP_PROP_ENABLED:
      g_value_set_boolean (value, priv->enabled);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
      break;
    }
}

/*
 * Closures for ClutterActor::button-press/release-event
 *
 * Even when the the press-motion-release sequence is interpreted as a drag,
 * we still get the press and release events because of the way the
 * NbtkDraggable implementation is done. So, we have a custom closures that
 * prevent the events from propagating.
 */
static gboolean
mnb_switcher_app_button_press_event (ClutterActor         *actor,
                                     ClutterButtonEvent   *event)
{
  /*
   * We never ever want press events propagated; the way d&d works, they are
   * pretty meaningless, and any custom handling needs to be done on the
   * button releases anyway.
   */
  return TRUE;
}

static gboolean
mnb_switcher_app_button_release_event (ClutterActor         *actor,
                                       ClutterButtonEvent   *event)
{
  MnbSwitcherAppPrivate *priv   = MNB_SWITCHER_APP (actor)->priv;
  gboolean               retval = priv->ignore_button_release;

  /*
   * If the ingore_button_release flag is set, we stop it from propagating
   * to parents.
   */
  priv->ignore_button_release = FALSE;

  return retval;
}

static void
mnb_switcher_app_class_init (MnbSwitcherAppClass *klass)
{
  GObjectClass      *object_class = G_OBJECT_CLASS (klass);
  ClutterActorClass *actor_class = CLUTTER_ACTOR_CLASS (klass);

  object_class->dispose             = mnb_switcher_app_dispose;
  object_class->get_property        = mnb_switcher_app_get_property;
  object_class->set_property        = mnb_switcher_app_set_property;

  actor_class->allocate             = mnb_switcher_app_allocate;
  actor_class->button_release_event = mnb_switcher_app_button_release_event;
  actor_class->button_press_event   = mnb_switcher_app_button_press_event;

  g_type_class_add_private (klass, sizeof (MnbSwitcherAppPrivate));

  g_object_class_override_property (object_class,
                                    APP_PROP_DRAG_THRESHOLD,
                                    "drag-threshold");
  g_object_class_override_property (object_class,
                                    APP_PROP_AXIS,
                                    "axis");
  g_object_class_override_property (object_class,
                                    APP_PROP_CONTAINMENT_TYPE,
                                    "containment-type");
  g_object_class_override_property (object_class,
                                    APP_PROP_CONTAINMENT_AREA,
                                    "containment-area");
  g_object_class_override_property (object_class,
                                    APP_PROP_ENABLED,
                                    "enabled");
}

static void
mnb_switcher_app_init (MnbSwitcherApp *self)
{
  MnbSwitcherAppPrivate *priv;

  priv = self->priv = MNB_SWITCHER_APP_GET_PRIVATE (self);

  priv->threshold = 5;
  priv->axis = 0;
  priv->containment = NBTK_DISABLE_CONTAINMENT;
  priv->enabled = TRUE;
}

struct _MnbSwitcherZone
{
  /*< private >*/
  NbtkTable parent_instance;

  MnbSwitcherZonePrivate *priv;
};

struct _MnbSwitcherZoneClass
{
  /*< private >*/
  NbtkTableClass parent_class;
};

struct _MnbSwitcherZonePrivate
{
  MnbSwitcher *switcher;

  gboolean     enabled  : 1;
  gboolean     new_zone : 1;
};

enum
{
  ZONE_PROP_0 = 0,
  ZONE_PROP_ENABLED
};

static void nbtk_droppable_iface_init (NbtkDroppableIface *iface);

G_DEFINE_TYPE_WITH_CODE (MnbSwitcherZone, mnb_switcher_zone, NBTK_TYPE_TABLE,
                         G_IMPLEMENT_INTERFACE (NBTK_TYPE_DROPPABLE,
                                                nbtk_droppable_iface_init));

#define MNB_SWITCHER_ZONE_GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MNB_TYPE_SWITCHER_ZONE,\
                                MnbSwitcherZonePrivate))

static void
mnb_switcher_zone_dispose (GObject *object)
{
  G_OBJECT_CLASS (mnb_switcher_zone_parent_class)->dispose (object);
}

static void
mnb_switcher_zone_get_preferred_width (ClutterActor *actor,
                                       gfloat for_height,
                                       gfloat *min_width,
                                       gfloat *pref_width)
{
  /* This is is a nasty hack to ignore the size of the children.
   * It allows the parent to allocate each of the zones equally, because they
   * all report the same width */

  if (min_width)
    *min_width = 0;

  if (pref_width)
    *pref_width = 0;
}

static void
mnb_switcher_zone_set_state (MnbSwitcherZone      *zone,
                             MnbSwitcherZoneState  state)
{
  ClutterActor *self = CLUTTER_ACTOR (zone);
  ClutterActor *parent;
  ClutterActor *label;
  gint          col;
  const gchar  *zone_name;
  const gchar  *label_name;
  const gchar  *text_name;

  switch (state)
    {
    default:
    case MNB_SWITCHER_ZONE_NORMAL:
      zone_name = "";
      label_name = "";
      text_name = "";
      break;
    case MNB_SWITCHER_ZONE_ACTIVE:
      zone_name = "switcher-workspace-active";
      label_name = "workspace-title-active";
      text_name = "workspace-title-label-active";
      break;
    case MNB_SWITCHER_ZONE_HOVER:
      zone_name = "switcher-workspace-new-over";
      label_name = "workspace-title-new-over";
      text_name = "workspace-title-label-over";
      break;
    }

  parent = clutter_actor_get_parent (self);

  clutter_container_child_get (CLUTTER_CONTAINER (parent), self,
                               "col", &col, NULL);

  label = table_find_child (CLUTTER_CONTAINER (parent), 0, col);

  clutter_actor_set_name (self, zone_name);

  if (label)
    {
      ClutterActor *child = nbtk_bin_get_child (NBTK_BIN (label));

      clutter_actor_set_name (label, label_name);

      if (child)
        clutter_actor_set_name (child, text_name);
    }
}

static void
mnb_switcher_zone_over_in (NbtkDroppable *droppable,
                           NbtkDraggable *draggable)
{
  MnbSwitcherZone *zone  = MNB_SWITCHER_ZONE (droppable);

  if (!zone->priv->enabled)
    return;

  mnb_switcher_zone_set_state (zone, MNB_SWITCHER_ZONE_HOVER);
}

static void
mnb_switcher_zone_over_out (NbtkDroppable *droppable,
                            NbtkDraggable *draggable)
{
  MnbSwitcherZonePrivate *priv = MNB_SWITCHER_ZONE (droppable)->priv;
  ClutterActor           *self = CLUTTER_ACTOR (droppable);
  ClutterActor           *parent;
  gint                    col;
  gint                    active_ws;

  if (!priv->enabled)
    return;

  parent = clutter_actor_get_parent (self);

  clutter_container_child_get (CLUTTER_CONTAINER (parent),
                               self, "col", &col, NULL);

  active_ws = mnb_switcher_get_active_workspace (priv->switcher);

  mnb_switcher_zone_set_state (MNB_SWITCHER_ZONE (droppable),
                               active_ws == col ?
                               MNB_SWITCHER_ZONE_ACTIVE :
                               MNB_SWITCHER_ZONE_NORMAL);
}

static void
mnb_switcher_zone_drop (NbtkDroppable       *droppable,
                        NbtkDraggable       *draggable,
                        gfloat               event_x,
                        gfloat               event_y,
                        gint                 button,
                        ClutterModifierType  modifiers)
{
  MnbSwitcher           *switcher =
    MNB_SWITCHER_ZONE (droppable)->priv->switcher;
  MnbSwitcherPrivate    *priv = switcher->priv;
  MnbSwitcherAppPrivate *app_priv = MNB_SWITCHER_APP (draggable)->priv;
  ClutterChildMeta      *meta;
  ClutterActor          *zone_parent;
  ClutterActor          *app_parent;
  ClutterActor          *zone_actor = CLUTTER_ACTOR (droppable);
  ClutterActor          *app_actor = CLUTTER_ACTOR (draggable);
  MnbSwitcherZone       *zone = MNB_SWITCHER_ZONE (droppable);
  MetaWindow            *meta_win;
  gint                   col;
  guint32                timestamp;

  if (!(meta_win = mutter_window_get_meta_window (app_priv->mw)))
    {
      g_warning ("No MutterWindow associated with this item.");
      return;
    }

  /*
   * We will get a ButtonPress event just after this, which we must ignore.
   */
  app_priv->ignore_button_release = TRUE;

  /*
   * Check we are not disabled (we should really not be getting drop events on
   * disabled droppables, but we do).
   */
  if (!zone->priv->enabled)
    {
      g_warning ("Bug: received a drop on a disabled droppable -- ignoring");
      return;
    }

  /*
   * Check whether we are not being dropped back on the same zone
   */
  if (app_priv->orig_parent == zone_actor)
    {
      /*
       * Do nothing here; this is equivalent to the d&d being cancelled and is
       * handled in the drag_end () function.
       */
      return;
    }

  zone_parent = clutter_actor_get_parent (zone_actor);
  app_parent = clutter_actor_get_parent (app_actor);

  g_assert (NBTK_IS_TABLE (zone_parent));

  meta = clutter_container_get_child_meta (CLUTTER_CONTAINER (zone_parent),
					   zone_actor);

  g_object_get (meta, "col", &col, NULL);

  if (!zone->priv->new_zone)
    {
      gint          row = nbtk_table_get_row_count (NBTK_TABLE (droppable));
      gint          active_ws;

      g_object_ref (draggable);

      clutter_container_remove_actor (CLUTTER_CONTAINER (app_parent),
                                      app_actor);
      clutter_actor_set_size (app_actor, -1.0, -1.0);
      nbtk_table_add_actor (NBTK_TABLE (droppable), app_actor, row, 0);

      clutter_container_child_set (CLUTTER_CONTAINER (droppable), app_actor,
                                   "y-fill", FALSE, "x-fill", FALSE,  NULL);

      active_ws = mnb_switcher_get_active_workspace (switcher);
      mnb_switcher_zone_set_state (MNB_SWITCHER_ZONE (zone_actor),
                                   active_ws == col ?
                                   MNB_SWITCHER_ZONE_ACTIVE :
                                   MNB_SWITCHER_ZONE_NORMAL);

      g_object_unref (draggable);
    }
  else
    {
      NbtkTable *new_ws;

      new_ws = mnb_switcher_append_workspace (switcher);

      g_object_ref (draggable);

      clutter_container_remove_actor (CLUTTER_CONTAINER (app_parent),
                                      app_actor);
      clutter_actor_set_size (app_actor, -1.0, -1.0);
      nbtk_table_add_actor (new_ws, app_actor, 1, 0);
      clutter_container_child_set (CLUTTER_CONTAINER (new_ws), app_actor,
                                   "y-fill", FALSE, "x-fill", FALSE,  NULL);

      g_object_unref (draggable);
    }

  timestamp = clutter_x11_get_current_event_time ();
  meta_window_change_workspace_by_index (meta_win, col, TRUE, timestamp);

  if (priv->tab_list)
    {
      priv->tab_list = g_list_sort (priv->tab_list, tablist_sort_func);
    }
}

static void
nbtk_droppable_iface_init (NbtkDroppableIface *iface)
{
  iface->over_in  = mnb_switcher_zone_over_in;
  iface->over_out = mnb_switcher_zone_over_out;
  iface->drop     = mnb_switcher_zone_drop;
}

static void
mnb_switcher_zone_set_property (GObject      *gobject,
                                guint         prop_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
  MnbSwitcherZonePrivate *priv = MNB_SWITCHER_ZONE (gobject)->priv;

  switch (prop_id)
    {
    case ZONE_PROP_ENABLED:
      priv->enabled = g_value_get_boolean (value);
      if (priv->enabled)
        nbtk_droppable_enable (NBTK_DROPPABLE (gobject));
      else
        nbtk_droppable_disable (NBTK_DROPPABLE (gobject));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
      break;
    }
}

static void
mnb_switcher_zone_get_property (GObject    *gobject,
                                guint       prop_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
  MnbSwitcherZonePrivate *priv = MNB_SWITCHER_ZONE (gobject)->priv;

  switch (prop_id)
    {
    case ZONE_PROP_ENABLED:
      g_value_set_boolean (value, priv->enabled);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
      break;
    }
}

static void
mnb_switcher_zone_class_init (MnbSwitcherZoneClass *klass)
{
  GObjectClass     *object_class = G_OBJECT_CLASS (klass);
  ClutterActorClass *actor_class = CLUTTER_ACTOR_CLASS (klass);

  object_class->dispose            = mnb_switcher_zone_dispose;
  object_class->get_property       = mnb_switcher_zone_get_property;
  object_class->set_property       = mnb_switcher_zone_set_property;
  actor_class->get_preferred_width = mnb_switcher_zone_get_preferred_width;

  g_type_class_add_private (klass, sizeof (MnbSwitcherZonePrivate));

  g_object_class_override_property (object_class,
                                    ZONE_PROP_ENABLED,
                                    "enabled");
}

static void
mnb_switcher_zone_init (MnbSwitcherZone *self)
{
  self->priv = MNB_SWITCHER_ZONE_GET_PRIVATE (self);
}

G_DEFINE_TYPE (MnbSwitcher, mnb_switcher, MNB_TYPE_DROP_DOWN)

#define MNB_SWITCHER_GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MNB_TYPE_SWITCHER, MnbSwitcherPrivate))

static void mnb_switcher_alt_tab_key_handler (MetaDisplay    *display,
                                              MetaScreen     *screen,
                                              MetaWindow     *window,
                                              XEvent         *event,
                                              MetaKeyBinding *binding,
                                              gpointer        data);

static gint
mnb_switcher_get_active_workspace (MnbSwitcher *switcher)
{
  return switcher->priv->active_ws;
}


struct input_data
{
  gint          index;
  MnbSwitcher  *switcher;
};

/*
 * Calback for clicks on a workspace in the switcher (switches to the
 * appropriate ws).
 */
static gboolean
workspace_input_cb (ClutterActor *clone, ClutterEvent *event, gpointer data)
{
  struct input_data  *input_data = data;
  gint                indx       = input_data->index;
  MnbSwitcher        *switcher   = input_data->switcher;
  MnbSwitcherPrivate *priv       = switcher->priv;
  MutterPlugin       *plugin     = priv->plugin;
  MetaScreen         *screen     = mutter_plugin_get_screen (plugin);
  MetaWorkspace      *workspace;
  guint32             timestamp = clutter_x11_get_current_event_time ();

  if (priv->dnd_in_progress)
    return FALSE;

  workspace = meta_screen_get_workspace_by_index (screen, indx);

  if (!workspace)
    {
      g_warning ("No workspace specified, %s:%d\n", __FILE__, __LINE__);
      return FALSE;
    }

  mnb_drop_down_hide_with_toolbar (MNB_DROP_DOWN (switcher));

  if (priv->in_alt_grab)
    {
      MetaDisplay *display = meta_screen_get_display (screen);

      /*
       * Make sure our stamp is recent enough.
       */
      timestamp = meta_display_get_current_time_roundtrip (display);

      meta_display_end_grab_op (display, timestamp);
      priv->in_alt_grab = FALSE;
    }

  meta_workspace_activate (workspace, timestamp);

  return FALSE;
}

/*
 * Callback for clicks on an individual application in the switcher; activates
 * the application.
 */
static gboolean
workspace_switcher_clone_input_cb (ClutterActor *clone,
                                   ClutterEvent *event,
                                   gpointer      data)
{
  MnbSwitcherAppPrivate      *app_priv = MNB_SWITCHER_APP (clone)->priv;
  MutterWindow               *mw = app_priv->mw;
  MetaWindow                 *window;
  MetaWorkspace              *workspace;
  MetaWorkspace              *active_workspace;
  MetaScreen                 *screen;
  MnbSwitcher                *switcher = app_priv->switcher;
  MnbSwitcherPrivate         *priv = switcher->priv;
  guint32                     timestamp;

  if (priv->dnd_in_progress)
    return FALSE;

  if (app_priv->ignore_button_release)
    return FALSE;

  window           = mutter_window_get_meta_window (mw);
  screen           = meta_window_get_screen (window);
  workspace        = meta_window_get_workspace (window);
  active_workspace = meta_screen_get_active_workspace (screen);
  timestamp        = clutter_x11_get_current_event_time ();

  mnb_drop_down_hide_with_toolbar (MNB_DROP_DOWN (switcher));
  clutter_ungrab_pointer ();

  if (priv->in_alt_grab)
    {
      MetaDisplay *display = meta_screen_get_display (screen);

      /*
       * Make sure our stamp is recent enough.
       */
      timestamp = meta_display_get_current_time_roundtrip (display);

      meta_display_end_grab_op (display, timestamp);
      priv->in_alt_grab = FALSE;
    }

  if (!active_workspace || (active_workspace == workspace))
    {
      meta_window_activate_with_workspace (window, timestamp, workspace);
    }
  else
    {
      meta_workspace_activate_with_focus (workspace, window, timestamp);
    }

  return FALSE;
}

/*
 * Used for sorting the tablist we maintain for kbd navigation to match the
 * contents of the switcher.
 */
static gint
tablist_sort_func (gconstpointer a, gconstpointer b)
{
  ClutterActor          *clone1 = CLUTTER_ACTOR (a);
  ClutterActor          *clone2 = CLUTTER_ACTOR (b);
  ClutterActor          *parent1 = clutter_actor_get_parent (clone1);
  ClutterActor          *parent2 = clutter_actor_get_parent (clone2);
  ClutterActor          *gparent1 = clutter_actor_get_parent (parent1);
  ClutterActor          *gparent2 = clutter_actor_get_parent (parent2);
  gint                   pcol1, pcol2;

  if (parent1 == parent2)
    {
      /*
       * The simple case of both clones on the same workspace.
       */
      gint row1, row2, col1, col2;

      clutter_container_child_get (CLUTTER_CONTAINER (parent1), clone1,
                                   "row", &row1, "col", &col1, NULL);
      clutter_container_child_get (CLUTTER_CONTAINER (parent1), clone2,
                                   "row", &row2, "col", &col2, NULL);

      if (row1 < row2)
        return -1;

      if (row1 > row2)
        return 1;

      if (col1 < col2)
        return -1;

      if (col1 > col2)
        return 1;

      return 0;
    }

  clutter_container_child_get (CLUTTER_CONTAINER (gparent1), parent1,
                               "col", &pcol1, NULL);
  clutter_container_child_get (CLUTTER_CONTAINER (gparent2), parent2,
                               "col", &pcol2, NULL);

  if (pcol1 < pcol2)
    return -1;

  if (pcol1 > pcol2)
    return 1;

  return 0;
}

/*
 * Tooltip showing machinery.
 */
static gboolean
clone_hover_timeout_cb (gpointer data)
{
  MnbSwitcherAppPrivate *app_priv = MNB_SWITCHER_APP (data)->priv;
  MnbSwitcherPrivate    *priv     = app_priv->switcher->priv;

  if (!priv->dnd_in_progress)
    {
      if (priv->active_tooltip)
        nbtk_tooltip_hide (priv->active_tooltip);

      priv->active_tooltip = NBTK_TOOLTIP (app_priv->tooltip);
      nbtk_tooltip_show (priv->active_tooltip);
    }

  app_priv->hover_timeout_id = 0;

  return FALSE;
}

static gboolean
clone_enter_event_cb (ClutterActor *actor,
		      ClutterCrossingEvent *event,
		      gpointer data)
{
  MnbSwitcherAppPrivate *child_priv = MNB_SWITCHER_APP (actor)->priv;
  MnbSwitcherPrivate    *priv       = child_priv->switcher->priv;

  if (!priv->dnd_in_progress)
    child_priv->hover_timeout_id = g_timeout_add (HOVER_TIMEOUT,
						  clone_hover_timeout_cb,
						  actor);

  return FALSE;
}

static gboolean
clone_leave_event_cb (ClutterActor *actor,
		      ClutterCrossingEvent *event,
		      gpointer data)
{
  MnbSwitcherAppPrivate *child_priv = MNB_SWITCHER_APP (actor)->priv;
  MnbSwitcherPrivate    *priv       = child_priv->switcher->priv;

  if (child_priv->hover_timeout_id)
    {
      g_source_remove (child_priv->hover_timeout_id);
      child_priv->hover_timeout_id = 0;
    }

  if (CLUTTER_ACTOR_IS_MAPPED (child_priv->tooltip))
    {
      nbtk_tooltip_hide (NBTK_TOOLTIP (child_priv->tooltip));

      if (priv->active_tooltip == (NbtkTooltip*)child_priv->tooltip)
        priv->active_tooltip = NULL;
    }

  return FALSE;
}

static ClutterActor *
table_find_child (ClutterContainer *table, gint row, gint col)
{
  ClutterActor *child = NULL;
  GList        *l, *kids;

  kids = l = clutter_container_get_children (CLUTTER_CONTAINER (table));

  while (l)
    {
      ClutterActor *a = l->data;
      gint r, c;

      clutter_container_child_get (table, a, "row", &r, "col", &c, NULL);

      if ((r == row) && (c == col))
        {
          child = a;
          break;
        }

      l = l->next;
    }

  g_list_free (kids);

  return child;
}

static NbtkWidget *
make_workspace_content (MnbSwitcher *switcher, gboolean active, gint col)
{
  MnbSwitcherPrivate *priv = switcher->priv;
  NbtkWidget         *table = priv->table;
  NbtkWidget         *new_ws;
  struct input_data  *input_data = g_new (struct input_data, 1);

  input_data = g_new (struct input_data, 1);
  input_data->index = col;
  input_data->switcher = switcher;

  new_ws = g_object_new (MNB_TYPE_SWITCHER_ZONE, NULL);

  MNB_SWITCHER_ZONE (new_ws)->priv->switcher = switcher;

  nbtk_table_set_row_spacing (NBTK_TABLE (new_ws), 6);
  nbtk_table_set_col_spacing (NBTK_TABLE (new_ws), 6);
  clutter_actor_set_reactive (CLUTTER_ACTOR (new_ws), TRUE);

  nbtk_widget_set_style_class_name (new_ws, "switcher-workspace");

  nbtk_table_add_actor (NBTK_TABLE (table), CLUTTER_ACTOR (new_ws), 1, col);
  g_object_set (new_ws, "enabled", TRUE, NULL);

  if (active)
    mnb_switcher_zone_set_state (MNB_SWITCHER_ZONE (new_ws),
                                 MNB_SWITCHER_ZONE_ACTIVE);

  /* switch workspace when the workspace is selected */
  g_signal_connect_data (new_ws, "button-release-event",
                         G_CALLBACK (workspace_input_cb), input_data,
                         (GClosureNotify)g_free, 0);

  return new_ws;
}

static NbtkWidget *
make_workspace_label (MnbSwitcher *switcher, gboolean active, gint col)
{
  MnbSwitcherPrivate *priv = switcher->priv;
  NbtkWidget         *table = priv->table;
  ClutterActor       *ws_label;
  NbtkWidget         *label;
  gchar              *s;
  struct input_data  *input_data = g_new (struct input_data, 1);

  input_data->index = col;
  input_data->switcher = switcher;

  s = g_strdup_printf ("%d", col + 1);

  ws_label = CLUTTER_ACTOR (nbtk_bin_new ());
  label = nbtk_label_new (s);

  nbtk_widget_set_style_class_name (label, "workspace-title-label");

  nbtk_bin_set_child (NBTK_BIN (ws_label), CLUTTER_ACTOR (label));
  nbtk_bin_set_alignment (NBTK_BIN (ws_label),
                          NBTK_ALIGN_CENTER, NBTK_ALIGN_CENTER);

  if (active)
    clutter_actor_set_name (CLUTTER_ACTOR (ws_label), "workspace-title-active");

  nbtk_widget_set_style_class_name (NBTK_WIDGET (ws_label), "workspace-title");

  clutter_actor_set_reactive (CLUTTER_ACTOR (ws_label), TRUE);

  g_signal_connect_data (ws_label, "button-release-event",
                         G_CALLBACK (workspace_input_cb), input_data,
                         (GClosureNotify) g_free, 0);

  nbtk_table_add_actor (NBTK_TABLE (table), ws_label, 0, col);
  clutter_container_child_set (CLUTTER_CONTAINER (table),
                               CLUTTER_ACTOR (ws_label),
                               "y-expand", FALSE,
                               "x-fill", TRUE,
                               NULL);

  return NBTK_WIDGET (ws_label);
}

struct ws_remove_data
{
  MnbSwitcher *switcher;
  gint         col;
  GList       *remove;
};

static void
table_foreach_remove_ws (ClutterActor *child, gpointer data)
{
  struct ws_remove_data *remove_data = data;
  MnbSwitcher           *switcher    = remove_data->switcher;
  NbtkWidget            *table       = switcher->priv->table;
  ClutterChildMeta      *meta;
  gint                   row, col;

  meta = clutter_container_get_child_meta (CLUTTER_CONTAINER (table), child);

  g_assert (meta);
  g_object_get (meta, "row", &row, "col", &col, NULL);

  /*
   * Children below the column we are removing are unaffected.
   */
  if (col < remove_data->col)
    return;

  /*
   * We cannot remove the actors in the foreach function, as that potentially
   * affects a list in which the container holds the data (e.g., NbtkTable).
   * We schedule it for removal, and then remove all once we are finished with
   * the foreach.
   */
  if (col == remove_data->col)
    {
      remove_data->remove = g_list_prepend (remove_data->remove, child);
    }
  else
    {
      /*
       * For some reason changing the colum clears the y-expand property :-(
       * Need to preserve it on the first row.
       */
      if (!row)
        {
          NbtkLabel *label;

          clutter_container_child_set (CLUTTER_CONTAINER (table), child,
                                       "col", col - 1,
                                       "y-expand", FALSE, NULL);

          label = (NbtkLabel*)nbtk_bin_get_child (NBTK_BIN (child));

          if (label && NBTK_IS_LABEL (label))
            {
              gchar *s = g_strdup_printf ("%d", col);

              nbtk_label_set_text (label, s);

              g_free (s);
            }
        }
      else
        clutter_container_child_set (CLUTTER_CONTAINER (table), child,
                                     "col", col - 1, NULL);
    }
}

static void
screen_n_workspaces_notify (MetaScreen *screen,
                            GParamSpec *pspec,
                            gpointer    data)
{
  MnbSwitcher *switcher = MNB_SWITCHER (data);
  gint         n_c_workspaces;
  GList       *c_workspaces;
  GList       *o_workspaces;
  gint         n_o_workspaces;
  gboolean    *map;
  gint         i;
  GList       *k;
  struct ws_remove_data remove_data;

  if (!CLUTTER_ACTOR_IS_MAPPED (CLUTTER_ACTOR (switcher)))
    return;

  n_c_workspaces = meta_screen_get_n_workspaces (screen);
  c_workspaces   = meta_screen_get_workspaces (screen);
  o_workspaces   = switcher->priv->last_workspaces;
  n_o_workspaces = g_list_length (o_workspaces);

  if (n_c_workspaces < 8)
    g_object_set (switcher->priv->new_workspace, "enabled", TRUE, NULL);
  else
    g_object_set (switcher->priv->new_workspace, "enabled", FALSE, NULL);

  if (n_o_workspaces < n_c_workspaces)
    {
      /*
       * The relayout is handled in the dnd_dropped callback.
       */
      g_list_free (switcher->priv->last_workspaces);
      switcher->priv->last_workspaces = g_list_copy (c_workspaces);
      return;
    }

  remove_data.switcher = switcher;
  remove_data.remove = NULL;

  map = g_slice_alloc0 (sizeof (gboolean) * n_o_workspaces);

  k = c_workspaces;

  while (k)
    {
      MetaWorkspace *w = k->data;
      GList         *l = o_workspaces;

      i = 0;

      while (l)
        {
          MetaWorkspace *w2 = l->data;

          if (w == w2)
            {
              map[i] = TRUE;
              break;
            }

          ++i;
          l = l->next;
        }

      k = k->next;
    }

  for (i = 0; i < n_o_workspaces; ++i)
    {
      if (!map[i])
        {
          GList *l;
          ClutterContainer *table = CLUTTER_CONTAINER (switcher->priv->table);

          remove_data.col = i;
          clutter_container_foreach (table,
                                     (ClutterCallback) table_foreach_remove_ws,
                                     &remove_data);

          l = remove_data.remove;
          while (l)
            {
              ClutterActor *child = l->data;

              clutter_container_remove_actor (table, child);

              l = l->next;
            }

          g_list_free (remove_data.remove);
        }
    }

  g_list_free (switcher->priv->last_workspaces);
  switcher->priv->last_workspaces = g_list_copy (c_workspaces);
}

static void
meta_window_focus_cb (MetaWindow *mw, gpointer data)
{
  MnbSwitcherAppPrivate *child_priv = MNB_SWITCHER_APP (data)->priv;
  MnbSwitcher           *switcher = child_priv->switcher;
  MnbSwitcherPrivate    *priv = switcher->priv;

  if (priv->constructing || priv->last_focused == data)
    return;

  if (priv->last_focused)
    clutter_actor_set_name (priv->last_focused, "");

  clutter_actor_set_name (CLUTTER_ACTOR (data), "switcher-application-active");
  priv->last_focused = data;
  priv->selected = child_priv->mw;
}

static void mnb_switcher_clone_weak_notify (gpointer data, GObject *object);

struct origin_data
{
  ClutterActor *clone;
  MutterWindow *mw;
  MnbSwitcher  *switcher;
};

static void
mnb_switcher_origin_weak_notify (gpointer data, GObject *obj)
{
  struct origin_data *origin_data = data;
  ClutterActor       *clone = origin_data->clone;
  MnbSwitcherPrivate *priv  = origin_data->switcher->priv;

  if (priv->tab_list)
    {
      priv->tab_list = g_list_remove (priv->tab_list, clone);
    }

  /*
   * The original MutterWindow destroyed; remove the weak reference the
   * we added to the clone referencing the original window, then
   * destroy the clone.
   */
  g_object_weak_unref (G_OBJECT (clone), mnb_switcher_clone_weak_notify, data);
  clutter_actor_destroy (clone);

  g_free (data);
}

static void
mnb_switcher_clone_weak_notify (gpointer data, GObject *obj)
{
  struct origin_data *origin_data = data;
  GObject            *origin = G_OBJECT (origin_data->mw);

  /*
   * Clone destroyed -- this function gets only called whent the clone
   * is destroyed while the original MutterWindow still exists, so remove
   * the weak reference we added on the origin for sake of the clone.
   */
  if (origin_data->switcher->priv->last_focused == (ClutterActor*)obj)
    origin_data->switcher->priv->last_focused = NULL;

  g_object_weak_unref (origin, mnb_switcher_origin_weak_notify, data);
}

static void
on_show_completed_cb (ClutterActor *self, gpointer data)
{
  MnbSwitcherPrivate    *priv     = MNB_SWITCHER (self)->priv;
  MnbSwitcherAppPrivate *app_priv = MNB_SWITCHER_APP (data)->priv;

  if (priv->in_alt_grab)
    return;

  if (priv->active_tooltip)
    {
      nbtk_tooltip_hide (priv->active_tooltip);
      priv->active_tooltip = NULL;
    }

  if (app_priv->tooltip)
    {
      if (priv->active_tooltip)
        nbtk_tooltip_hide (priv->active_tooltip);

      priv->active_tooltip = NBTK_TOOLTIP (app_priv->tooltip);

      nbtk_tooltip_show (priv->active_tooltip);
    }
}

static void
mnb_switcher_enable_new_workspace (MnbSwitcher *switcher, MnbSwitcherZone *zone)
{
  MnbSwitcherPrivate *priv = switcher->priv;
  gint                ws_count;
  MetaScreen         *screen = mutter_plugin_get_screen (priv->plugin);
  NbtkWidget         *new_ws = priv->new_workspace;
  NbtkWidget         *new_label = priv->new_label;

  ws_count = meta_screen_get_n_workspaces (screen);

  if (ws_count >= 8)
    return;

  /*
   * If the application is the only child in its zone, the new zone remains
   * disabled.
   */
  if (nbtk_table_get_row_count (NBTK_TABLE (zone)) <= 1)
    return;

  g_object_set (new_ws, "enabled", TRUE, NULL);

  clutter_actor_set_name (CLUTTER_ACTOR (new_ws),
                          "switcher-workspace-new-active");
  clutter_actor_set_name (CLUTTER_ACTOR (new_label),
                          "workspace-title-new-active");

  clutter_actor_set_width (CLUTTER_ACTOR (new_label), 44);
  clutter_actor_set_width (CLUTTER_ACTOR (new_ws), 44);
}

static void
mnb_switcher_disable_new_workspace (MnbSwitcher     *switcher,
                                    MnbSwitcherZone *zone)
{
  MnbSwitcherPrivate *priv = switcher->priv;
  NbtkWidget         *new_ws = priv->new_workspace;
  NbtkWidget         *new_label = priv->new_label;

  g_object_set (new_ws, "enabled", FALSE, NULL);
  clutter_actor_set_name (CLUTTER_ACTOR (new_ws), "");
  clutter_actor_set_name (CLUTTER_ACTOR (new_label), "");
  clutter_actor_set_width (CLUTTER_ACTOR (priv->new_label), 22);
  clutter_actor_set_width (CLUTTER_ACTOR (new_ws), 22);
}

static void
mnb_switcher_show (ClutterActor *self)
{
  MnbSwitcherPrivate *priv = MNB_SWITCHER (self)->priv;
  MetaScreen   *screen = mutter_plugin_get_screen (priv->plugin);
  MetaDisplay  *display = meta_screen_get_display (screen);
  gint          ws_count, active_ws;
  gint          i, screen_width, screen_height;
  NbtkWidget   *table;
  GList        *window_list, *l;
  NbtkWidget  **spaces;
  GList        *workspaces = meta_screen_get_workspaces (screen);
  MetaWindow   *current_focus = NULL;
  ClutterActor *current_focus_clone = NULL;
  ClutterActor *top_most_clone = NULL;
  MutterWindow *top_most_mw = NULL;
  gboolean      found_focus = FALSE;
  ClutterActor *toolbar;
  gboolean      switcher_empty = FALSE;

  struct win_location
  {
    gint  row;
    gint  col;
    gint  max_col;
    GSList *wins;
  } *win_locs;

  /*
   * Check the panel is visible, if not get the parent class to take care of
   * it.
   */
  toolbar = clutter_actor_get_parent (self);
  while (toolbar && !MNB_IS_TOOLBAR (toolbar))
    toolbar = clutter_actor_get_parent (toolbar);

  if (!toolbar)
    {
      g_warning ("Cannot show switcher that is not inside the Toolbar.");
      return;
    }

  if (!CLUTTER_ACTOR_IS_MAPPED (toolbar))
    {
      CLUTTER_ACTOR_CLASS (mnb_switcher_parent_class)->show (self);
      return;
    }

  priv->constructing = TRUE;

  current_focus = meta_display_get_focus_window (display);

  mutter_plugin_query_screen_size (priv->plugin, &screen_width, &screen_height);

  priv->last_workspaces = g_list_copy (workspaces);

  if (priv->tab_list)
    {
      g_list_free (priv->tab_list);
      priv->tab_list = NULL;
    }

  /* create the contents */

  table = nbtk_table_new ();
  priv->table = table;
  nbtk_table_set_row_spacing (NBTK_TABLE (table), 4);
  nbtk_table_set_col_spacing (NBTK_TABLE (table), 7);

  clutter_actor_set_name (CLUTTER_ACTOR (table), "switcher-table");

  mnb_drop_down_set_child (MNB_DROP_DOWN (self),
                           CLUTTER_ACTOR (table));

  ws_count = meta_screen_get_n_workspaces (screen);
  priv->active_ws = active_ws = meta_screen_get_active_workspace_index (screen);
  window_list = mutter_plugin_get_windows (priv->plugin);

  /* Handle case where no apps open.. */
  if (ws_count == 1)
    {
      switcher_empty = TRUE;

      for (l = window_list; l; l = g_list_next (l))
        {
          MutterWindow          *mw = l->data;
          MetaWindow            *meta_win = mutter_window_get_meta_window (mw);

          if (mutter_window_get_window_type (mw) == META_COMP_WINDOW_NORMAL)
            {
              switcher_empty = FALSE;
              break;
            }
          else if (mutter_window_get_window_type (mw)
                                         == META_COMP_WINDOW_DIALOG)
            {
              MetaWindow *parent = meta_window_find_root_ancestor (meta_win);

              if (parent == meta_win)
                {
                  switcher_empty = FALSE;
                  break;
                }
            }
        }

      if (switcher_empty)
        {
          NbtkWidget         *table = priv->table;
          ClutterActor       *bin;
          NbtkWidget         *label;

          bin = CLUTTER_ACTOR (nbtk_bin_new ());
          label = nbtk_label_new (_("No Zones yet"));

          nbtk_widget_set_style_class_name (label, "workspace-title-label");

          nbtk_bin_set_child (NBTK_BIN (bin), CLUTTER_ACTOR (label));
          nbtk_bin_set_alignment (NBTK_BIN (bin),
                                  NBTK_ALIGN_CENTER, NBTK_ALIGN_CENTER);

          clutter_actor_set_name (CLUTTER_ACTOR (bin),
                                  "workspace-title-active");

          nbtk_widget_set_style_class_name (NBTK_WIDGET (bin),
                                            "workspace-title");

          nbtk_table_add_actor (NBTK_TABLE (table), bin, 0, 0);
          clutter_container_child_set (CLUTTER_CONTAINER (table),
                                       CLUTTER_ACTOR (bin),
                                       "y-expand", FALSE,
                                       "x-fill", TRUE,
                                       NULL);

          bin = CLUTTER_ACTOR (nbtk_bin_new ());
          label = nbtk_label_new (_("Applications you’re using will show up here. You will be able to switch and organise them to your heart's content."));
          clutter_actor_set_name ((ClutterActor *)label, "workspace-no-wins-label");

          nbtk_bin_set_child (NBTK_BIN (bin), CLUTTER_ACTOR (label));
          nbtk_bin_set_alignment (NBTK_BIN (bin), NBTK_ALIGN_LEFT, NBTK_ALIGN_CENTER);
          clutter_actor_set_name (CLUTTER_ACTOR (bin),
                                  "workspace-no-wins-bin");

          nbtk_table_add_actor (NBTK_TABLE (table), bin, 1, 0);
          clutter_container_child_set (CLUTTER_CONTAINER (table),
                                       CLUTTER_ACTOR (bin),
                                       "y-expand", FALSE,
                                       "x-fill", TRUE,
                                       NULL);

          goto finish_up;
        }
    }

  /* loop through all the workspaces, adding a label for each */
  for (i = 0; i < ws_count; i++)
    {
      gboolean active = FALSE;

      if (i == active_ws)
        active = TRUE;

      make_workspace_label (MNB_SWITCHER (self), active, i);
    }

  /* iterate through the windows, adding them to the correct workspace */

  win_locs    = g_slice_alloc0 (sizeof (struct win_location) * ws_count);
  spaces      = g_slice_alloc0 (sizeof (NbtkWidget*) * ws_count);

  for (l = window_list; l; l = g_list_next (l))
    {
      MutterWindow          *mw = l->data;
      ClutterActor          *texture, *c_tx, *clone;
      gint                   ws_indx;
      MetaCompWindowType     type;
      struct origin_data    *origin_data;
      MetaWindow            *meta_win = mutter_window_get_meta_window (mw);
      gchar                 *title;
      MnbSwitcherAppPrivate *app_priv;

      ws_indx = mutter_window_get_workspace (mw);
      type = mutter_window_get_window_type (mw);
      /*
       * Only show regular windows that are not sticky (getting stacking order
       * right for sticky windows would be really hard, and since they appear
       * on each workspace, they do not help in identifying which workspace
       * it is).
       *
       * We show dialogs transient to root, as these can be top level
       * application windows.
       */
      if (ws_indx < 0                             ||
          mutter_window_is_override_redirect (mw) ||
          (type != META_COMP_WINDOW_NORMAL  &&
           type != META_COMP_WINDOW_DIALOG))
        {
          continue;
        }

      if (type ==  META_COMP_WINDOW_DIALOG)
        {
          MetaWindow *parent = meta_window_find_root_ancestor (meta_win);

          if (parent != meta_win)
            continue;
        }

      /* create the table for this workspace if we don't already have one */
      if (!spaces[ws_indx])
        {
          gboolean active = FALSE;

          if (ws_indx == active_ws)
            active = TRUE;

          spaces[ws_indx] =
            make_workspace_content (MNB_SWITCHER (self), active, ws_indx);
        }

      texture = mutter_window_get_texture (mw);
      g_object_set (texture, "keep-aspect-ratio", TRUE, NULL);

      c_tx    = clutter_clone_new (texture);
      clone   = g_object_new (MNB_TYPE_SWITCHER_APP, NULL);
      nbtk_widget_set_style_class_name (NBTK_WIDGET (clone),
                                        "switcher-application");

      clutter_container_add_actor (CLUTTER_CONTAINER (clone), c_tx);
      clutter_actor_show (clone);
      clutter_actor_set_reactive (clone, TRUE);

      /*
       * If the window has focus, apply the active style.
       */
      if (!found_focus &&
          (current_focus == meta_win ||
           (current_focus &&
            meta_window_is_ancestor_of_transient (meta_win, current_focus))))
        {
          found_focus = TRUE;

          clutter_actor_set_name (clone, "switcher-application-active");

          priv->last_focused = clone;
          priv->selected = mw;

          current_focus_clone = clone;
        }

      /*
       * Find the topmost window on the current workspace. We will used this
       * in case no window currently has focus.
       */
      if (active_ws == ws_indx)
        {
          top_most_clone = clone;
          top_most_mw = mw;
        }

      origin_data = g_new0 (struct origin_data, 1);
      origin_data->clone = clone;
      origin_data->mw = mw;
      origin_data->switcher = MNB_SWITCHER (self);
      priv->tab_list = g_list_prepend (priv->tab_list, clone);

      g_object_weak_ref (G_OBJECT (mw),
                         mnb_switcher_origin_weak_notify, origin_data);
      g_object_weak_ref (G_OBJECT (clone),
                         mnb_switcher_clone_weak_notify, origin_data);

      g_object_get (meta_win, "title", &title, NULL);

      app_priv = MNB_SWITCHER_APP (clone)->priv;
      app_priv->switcher = MNB_SWITCHER (self);
      app_priv->mw       = mw;
      app_priv->tooltip  = g_object_new (NBTK_TYPE_TOOLTIP,
                                         "label", title,
                                         NULL);
      clutter_actor_set_parent (app_priv->tooltip, clone);
      g_free (title);

      g_signal_connect (clone, "enter-event",
                        G_CALLBACK (clone_enter_event_cb), NULL);
      g_signal_connect (clone, "leave-event",
                        G_CALLBACK (clone_leave_event_cb), NULL);

      app_priv->focus_id =
        g_signal_connect (meta_win, "focus",
                          G_CALLBACK (meta_window_focus_cb), clone);
      app_priv->raised_id =
        g_signal_connect (meta_win, "raised",
                          G_CALLBACK (meta_window_focus_cb), clone);

      /*
       * FIXME -- this depends on the styling, should not be hardcoded.
       */
      win_locs[ws_indx].wins = g_slist_append (win_locs[ws_indx].wins, clone);

      nbtk_table_add_actor (NBTK_TABLE (spaces[ws_indx]), clone,
                            win_locs[ws_indx].row, win_locs[ws_indx].col);

      g_object_set (clone, "enabled", TRUE, NULL);

      win_locs[ws_indx].row++;
      clutter_container_child_set (CLUTTER_CONTAINER (spaces[ws_indx]), clone,
                                   "y-fill", FALSE, "x-fill", FALSE, NULL);
      g_signal_connect (clone, "button-release-event",
                        G_CALLBACK (workspace_switcher_clone_input_cb),
			NULL);
    }

  /*
   * If no window is currenlty focused, then try to focus the topmost window.
   */
  if (!current_focus && top_most_clone)
    {
      MetaWindow    *meta_win = mutter_window_get_meta_window (top_most_mw);
      MetaWorkspace *workspace;
      guint32        timestamp;

      clutter_actor_set_name (top_most_clone, "switcher-application-active");

      priv->last_focused = top_most_clone;
      priv->selected = top_most_mw;

      timestamp = clutter_x11_get_current_event_time ();
      workspace = meta_window_get_workspace (meta_win);

      current_focus_clone = top_most_clone;

      meta_window_activate_with_workspace (meta_win, timestamp, workspace);
    }

  /* add an "empty" message for empty workspaces */
  for (i = 0; i < ws_count; i++)
    {
      if (!spaces[i])
        {
          NbtkWidget        *label;
          MnbSwitcherZone   *zone = g_object_new (MNB_TYPE_SWITCHER_ZONE, NULL);
          struct input_data *input_data = g_new (struct input_data, 1);
          MnbSwitcher       *switcher = MNB_SWITCHER (self);

          input_data = g_new (struct input_data, 1);
          input_data->index = i;
          input_data->switcher = switcher;

          zone->priv->switcher = switcher;

          clutter_actor_set_reactive (CLUTTER_ACTOR (zone), TRUE);

          nbtk_widget_set_style_class_name (NBTK_WIDGET (zone),
                                            "switcher-workspace");
          if (i == active_ws)
            clutter_actor_set_name (CLUTTER_ACTOR (zone),
                                    "switcher-workspace-active");

          label = nbtk_label_new (_("No applications on this zone"));
          nbtk_table_add_actor (NBTK_TABLE (zone), CLUTTER_ACTOR (label), 0, 0);
          nbtk_table_add_actor (NBTK_TABLE (table), CLUTTER_ACTOR (zone),
                                1, i);

          /* switch workspace when the workspace is selected */
          g_signal_connect_data (zone, "button-release-event",
                                 G_CALLBACK (workspace_input_cb), input_data,
                                 (GClosureNotify)g_free, 0);
        }
    }

  /*
   * Now create the new workspace column.
   */
  {
    NbtkWidget *new_ws = g_object_new (MNB_TYPE_SWITCHER_ZONE, NULL);
    NbtkWidget *label;
    MnbSwitcherZone *zone = MNB_SWITCHER_ZONE (new_ws);

    zone->priv->switcher = MNB_SWITCHER (self);
    zone->priv->new_zone = TRUE;

    label = NBTK_WIDGET (nbtk_bin_new ());
    clutter_actor_set_width (CLUTTER_ACTOR (label), 22);
    nbtk_table_add_actor (NBTK_TABLE (table), CLUTTER_ACTOR (label),
                          0, ws_count);
    nbtk_widget_set_style_class_name (label, "workspace-title-new");
    clutter_container_child_set (CLUTTER_CONTAINER (table),
                                 CLUTTER_ACTOR (label),
                                 "y-expand", FALSE,
                                 "x-expand", FALSE, NULL);

    nbtk_table_set_row_spacing (NBTK_TABLE (new_ws), 6);
    nbtk_table_set_col_spacing (NBTK_TABLE (new_ws), 6);
    nbtk_widget_set_style_class_name (NBTK_WIDGET (new_ws),
                                      "switcher-workspace-new");

    clutter_actor_set_reactive (CLUTTER_ACTOR (new_ws), TRUE);

    priv->new_workspace = new_ws;
    priv->new_label = label;

    clutter_actor_set_width (CLUTTER_ACTOR (new_ws), 22);
    nbtk_table_add_actor (NBTK_TABLE (table), CLUTTER_ACTOR (new_ws),
                          1, ws_count);

    /*
     * Disable the new droppable; it will get enabled if appropriate when
     * the d&d begins.
     */
    g_object_set (new_ws, "enabled", FALSE, NULL);

    clutter_container_child_set (CLUTTER_CONTAINER (table),
                                 CLUTTER_ACTOR (new_ws),
                                 "y-expand", FALSE,
                                 "x-expand", FALSE, NULL);
  }

  for (i = 0; i < ws_count; ++i)
    {
      GSList *l = win_locs[i].wins;
      g_slist_free (l);
    }

  g_slice_free1 (sizeof (NbtkWidget*) * ws_count, spaces);
  g_slice_free1 (sizeof (struct win_location) * ws_count, win_locs);

  priv->tab_list = g_list_sort (priv->tab_list, tablist_sort_func);

 finish_up:

  if (!switcher_empty)
    clutter_actor_set_height (CLUTTER_ACTOR (table),
                              screen_height - TOOLBAR_HEIGHT * 1.5);

  priv->constructing = FALSE;

  /*
   * We connect to the show-completed signal, and if there is something focused
   * in the switcher (should be most of the time), we try to pop up the
   * tooltip from the callback.
   */
  if (priv->show_completed_id)
    {
      g_signal_handler_disconnect (self, priv->show_completed_id);
    }

  if (current_focus_clone)
    priv->show_completed_id =
      g_signal_connect (self, "show-completed",
                        G_CALLBACK (on_show_completed_cb),
                        current_focus_clone);

  CLUTTER_ACTOR_CLASS (mnb_switcher_parent_class)->show (self);
}

static NbtkTable *
mnb_switcher_append_workspace (MnbSwitcher *switcher)
{
  MnbSwitcherPrivate *priv = switcher->priv;
  NbtkWidget         *table = priv->table;
  NbtkWidget         *last_ws = priv->new_workspace;
  NbtkWidget         *last_label = priv->new_label;
  NbtkTable          *new_ws;
  gint                col;
  ClutterChildMeta   *meta;

  meta = clutter_container_get_child_meta (CLUTTER_CONTAINER (table),
                                           CLUTTER_ACTOR (last_ws));

  g_object_get (meta, "col", &col, NULL);

  clutter_container_child_set (CLUTTER_CONTAINER (table),
                               CLUTTER_ACTOR (last_ws),
                               "col", col + 1, NULL);

  clutter_container_child_set (CLUTTER_CONTAINER (table),
                               CLUTTER_ACTOR (last_label),
                               "col", col + 1,
                               "y-expand", FALSE, NULL);

  /*
   * Insert new workspace label and content pane where the new workspace
   * area was.
   */
  make_workspace_label   (switcher, FALSE, col);
  new_ws = NBTK_TABLE (make_workspace_content (switcher, FALSE, col));

  return new_ws;
}

static void
mnb_switcher_hide (ClutterActor *self)
{
  MnbSwitcherPrivate *priv = MNB_SWITCHER (self)->priv;

  if (priv->show_completed_id)
    {
      g_signal_handler_disconnect (self, priv->show_completed_id);
      priv->show_completed_id = 0;
    }

  if (priv->in_alt_grab)
    {
      MutterPlugin *plugin  = priv->plugin;
      MetaScreen   *screen  = mutter_plugin_get_screen (plugin);
      MetaDisplay  *display = meta_screen_get_display (screen);
      guint32       timestamp;
      ClutterActor *toolbar;

      /*
       * Make sure our stamp is recent enough.
       */
      timestamp = meta_display_get_current_time_roundtrip (display);

      meta_display_end_grab_op (display, timestamp);
      priv->in_alt_grab = FALSE;

      /*
       * Clear the dont_autohide flag we previously set on the toolbar.
       */
      toolbar = clutter_actor_get_parent (self);
      while (toolbar && !MNB_IS_TOOLBAR (toolbar))
        toolbar = clutter_actor_get_parent (toolbar);

      if (toolbar)
        mnb_toolbar_set_dont_autohide (MNB_TOOLBAR (toolbar), FALSE);
    }

  CLUTTER_ACTOR_CLASS (mnb_switcher_parent_class)->hide (self);
}

static void
mnb_switcher_finalize (GObject *object)
{
  MnbSwitcherPrivate *priv = MNB_SWITCHER (object)->priv;

  g_list_free (priv->last_workspaces);
  g_list_free (priv->global_tab_list);

  G_OBJECT_CLASS (mnb_switcher_parent_class)->finalize (object);
}

static void
mnb_switcher_kbd_grab_notify_cb (MetaScreen  *screen,
                                 GParamSpec  *pspec,
                                 MnbSwitcher *switcher)
{
  MnbSwitcherPrivate *priv = switcher->priv;
  gboolean            grabbed;

  if (!priv->in_alt_grab)
    return;

  g_object_get (screen, "keyboard-grabbed", &grabbed, NULL);

  /*
   * If the property has changed to FALSE, i.e., Mutter just called
   * XUngrabKeyboard(), reset the flag
   */
  if (!grabbed )
    {
      priv->in_alt_grab = FALSE;
      clutter_actor_hide (CLUTTER_ACTOR (switcher));
    }
}

static void
mnb_switcher_constructed (GObject *self)
{
  MnbSwitcher *switcher  = MNB_SWITCHER (self);
  MutterPlugin *plugin;

  g_object_get (self, "mutter-plugin", &plugin, NULL);

  switcher->priv->plugin = plugin;

  g_signal_connect (mutter_plugin_get_screen (plugin), "notify::n-workspaces",
                    G_CALLBACK (screen_n_workspaces_notify), self);

  g_signal_connect (mutter_plugin_get_screen (plugin),
                    "notify::keyboard-grabbed",
                    G_CALLBACK (mnb_switcher_kbd_grab_notify_cb),
                    self);
}

static void
mnb_switcher_class_init (MnbSwitcherClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  ClutterActorClass *actor_class = CLUTTER_ACTOR_CLASS (klass);

  object_class->finalize    = mnb_switcher_finalize;
  object_class->constructed = mnb_switcher_constructed;

  actor_class->show = mnb_switcher_show;
  actor_class->hide = mnb_switcher_hide;

  g_type_class_add_private (klass, sizeof (MnbSwitcherPrivate));
}

static void
on_switcher_hide_completed_cb (ClutterActor *self, gpointer data)
{
  MnbSwitcherPrivate         *priv;
  MoblinNetbookPluginPrivate *ppriv;
  ClutterActor               *toolbar;

  g_return_if_fail (MNB_IS_SWITCHER (self));

  priv  = MNB_SWITCHER (self)->priv;
  ppriv = MOBLIN_NETBOOK_PLUGIN (priv->plugin)->priv;

  if (priv->tab_list)
    {
      g_list_foreach (priv->tab_list, (GFunc) mnb_switcher_app_disable_draggable, NULL);
      g_list_free (priv->tab_list);
      priv->tab_list = NULL;
    }

  priv->table = NULL;
  priv->last_focused = NULL;
  priv->selected = NULL;
  priv->active_tooltip = NULL;
  priv->new_workspace = NULL;
  priv->new_label = NULL;

  mnb_drop_down_set_child (MNB_DROP_DOWN (self), NULL);

  /*
   * Fix for bug 1690.
   *
   * The Switcher is 'special'; in order for the thumbs to look right (namely
   * the active thumb have the current decorations), the Switcher relinguishes
   * focus to the active application. The problem with this is that if the
   * user then goes on to open another Panel without closing the Toolbar in
   * between, the focus is lost. So, when we hide the switcher, we get focus
   * back to the UI if the panel is visible.
   *
   * NB: We need to rethink this for the multiproc stuff.
   */
  toolbar = clutter_actor_get_parent (self);
  while (toolbar && !MNB_IS_TOOLBAR (toolbar))
    toolbar = clutter_actor_get_parent (toolbar);

  if (toolbar && CLUTTER_ACTOR_IS_MAPPED (toolbar))
    {
      moblin_netbook_focus_stage (priv->plugin, CurrentTime);
    }
}

/*
 * Metacity key handler for default Metacity bindings we want disabled.
 *
 * (This is necessary for keybidings that are related to the Alt+Tab shortcut.
 * In metacity these all use the src/ui/tabpopup.c object, which we have
 * disabled, so we need to take over all of those.)
 */
static void
mnb_switcher_nop_key_handler (MetaDisplay    *display,
                              MetaScreen     *screen,
                              MetaWindow     *window,
                              XEvent         *event,
                              MetaKeyBinding *binding,
                              gpointer        data)
{
}

static void
mnb_switcher_setup_metacity_keybindings (MnbSwitcher *switcher)
{
  meta_keybindings_set_custom_handler ("switch_windows",
                                       mnb_switcher_alt_tab_key_handler,
                                       switcher, NULL);
  meta_keybindings_set_custom_handler ("switch_windows_backward",
                                       mnb_switcher_alt_tab_key_handler,
                                       switcher, NULL);
  meta_keybindings_set_custom_handler ("switch_panels",
                                       mnb_switcher_alt_tab_key_handler,
                                       switcher, NULL);
  meta_keybindings_set_custom_handler ("switch_panels_backward",
                                       mnb_switcher_alt_tab_key_handler,
                                       switcher, NULL);
  meta_keybindings_set_custom_handler ("cycle_group",
                                       mnb_switcher_alt_tab_key_handler,
                                       switcher, NULL);
  meta_keybindings_set_custom_handler ("cycle_group_backward",
                                       mnb_switcher_alt_tab_key_handler,
                                       switcher, NULL);
  meta_keybindings_set_custom_handler ("cycle_windows",
                                       mnb_switcher_alt_tab_key_handler,
                                       switcher, NULL);
  meta_keybindings_set_custom_handler ("cycle_windows_backward",
                                       mnb_switcher_alt_tab_key_handler,
                                       switcher, NULL);
  meta_keybindings_set_custom_handler ("cycle_panels",
                                       mnb_switcher_alt_tab_key_handler,
                                       switcher, NULL);
  meta_keybindings_set_custom_handler ("cycle_panels_backward",
                                       mnb_switcher_alt_tab_key_handler,
                                       switcher, NULL);

  /*
   * Install NOP handler for shortcuts that are related to Alt+Tab.
   */
  meta_keybindings_set_custom_handler ("switch_group",
                                       mnb_switcher_nop_key_handler,
                                       switcher, NULL);
  meta_keybindings_set_custom_handler ("switch_group_backward",
                                       mnb_switcher_nop_key_handler,
                                       switcher, NULL);
  meta_keybindings_set_custom_handler ("switch_group_backward",
                                       mnb_switcher_nop_key_handler,
                                       switcher, NULL);

  /*
   * Disable various shortcuts that are not compatible with moblin UI paradigm
   * -- strictly speaking not switcher related, but for now here.
   */
  meta_keybindings_set_custom_handler ("activate_window_menu",
                                       mnb_switcher_nop_key_handler,
                                       switcher, NULL);
  meta_keybindings_set_custom_handler ("show_desktop",
                                       mnb_switcher_nop_key_handler,
                                       switcher, NULL);
  meta_keybindings_set_custom_handler ("toggle_maximized",
                                       mnb_switcher_nop_key_handler,
                                       switcher, NULL);
  meta_keybindings_set_custom_handler ("maximize",
                                       mnb_switcher_nop_key_handler,
                                       switcher, NULL);
  meta_keybindings_set_custom_handler ("maximize_vertically",
                                       mnb_switcher_nop_key_handler,
                                       switcher, NULL);
  meta_keybindings_set_custom_handler ("maximize_horizontally",
                                       mnb_switcher_nop_key_handler,
                                       switcher, NULL);
  meta_keybindings_set_custom_handler ("unmaximize",
                                       mnb_switcher_nop_key_handler,
                                       switcher, NULL);
  meta_keybindings_set_custom_handler ("minimize",
                                       mnb_switcher_nop_key_handler,
                                       switcher, NULL);
  meta_keybindings_set_custom_handler ("toggle_shadow",
                                       mnb_switcher_nop_key_handler,
                                       switcher, NULL);
}

static void
mnb_switcher_init (MnbSwitcher *self)
{
  self->priv = MNB_SWITCHER_GET_PRIVATE (self);

  g_signal_connect (self, "hide-completed",
                    G_CALLBACK (on_switcher_hide_completed_cb), NULL);

  mnb_switcher_setup_metacity_keybindings (self);
}

NbtkWidget*
mnb_switcher_new (MutterPlugin *plugin)
{
  MnbSwitcher *switcher;

  g_return_val_if_fail (MUTTER_PLUGIN (plugin), NULL);

  /*
   * TODO the plugin should be construction time property.
   */
  switcher = g_object_new (MNB_TYPE_SWITCHER,
                           "mutter-plugin", plugin,
                           NULL);

  return NBTK_WIDGET (switcher);
}


static void
select_inner_foreach_cb (ClutterActor *child, gpointer data)
{
  MnbSwitcherAppPrivate *app_priv;
  MetaWindow            *meta_win = data;
  MetaWindow            *my_win;
  MnbSwitcherPrivate    *priv;

  /*
   * Skip anything that is not a MnbSwitcherApp
   */
  if (!MNB_IS_SWITCHER_APP (child))
    return;

  app_priv = MNB_SWITCHER_APP (child)->priv;
  priv     = app_priv->switcher->priv;

  my_win = mutter_window_get_meta_window (app_priv->mw);

  if (meta_win == my_win)
    {
      clutter_actor_set_name (child, "switcher-application-active");

      app_priv->switcher->priv->selected = app_priv->mw;

      if (app_priv->tooltip)
        {
          if (priv->active_tooltip)
            nbtk_tooltip_hide (priv->active_tooltip);

          priv->active_tooltip = NBTK_TOOLTIP (app_priv->tooltip);
        }
    }
  else
    {
      clutter_actor_set_name (child, "");

      if (app_priv->tooltip)
        {
          if (priv->active_tooltip == (NbtkTooltip*)app_priv->tooltip)
            priv->active_tooltip = NULL;

          nbtk_tooltip_hide (NBTK_TOOLTIP (app_priv->tooltip));
        }
    }
}

static void
select_outer_foreach_cb (ClutterActor *child, gpointer data)
{
  gint          row;
  ClutterActor *parent;
  MetaWindow   *meta_win = data;

  parent = clutter_actor_get_parent (child);

  clutter_container_child_get (CLUTTER_CONTAINER (parent), child,
                               "row", &row, NULL);

  /* Skip the header row */
  if (!row)
    return;

  if (!NBTK_IS_TABLE (child))
    return;

  clutter_container_foreach (CLUTTER_CONTAINER (child),
                             select_inner_foreach_cb,
                             meta_win);
}

/*
 * Selects a thumb in the switcher that matches given MetaWindow.
 */
void
mnb_switcher_select_window (MnbSwitcher *switcher, MetaWindow *meta_win)
{
  MnbSwitcherPrivate *priv = switcher->priv;

  if (!priv->table)
    return;

  clutter_container_foreach (CLUTTER_CONTAINER (priv->table),
                             select_outer_foreach_cb, meta_win);

  if (priv->active_tooltip)
    {
      nbtk_tooltip_show (priv->active_tooltip);
    }
}

/*
 * Activates the window currently selected in the switcher.
 */
void
mnb_switcher_activate_selection (MnbSwitcher *switcher, gboolean close,
                                 guint timestamp)
{
  MnbSwitcherPrivate *priv = switcher->priv;

  MetaWindow                 *window;
  MetaWorkspace              *workspace;
  MetaWorkspace              *active_workspace;
  MetaScreen                 *screen;
  MutterPlugin               *plugin;

  if (close)
    mnb_drop_down_hide_with_toolbar (MNB_DROP_DOWN (switcher));

  if (!priv->selected)
    return;

  plugin           = switcher->priv->plugin;
  window           = mutter_window_get_meta_window (priv->selected);
  screen           = meta_window_get_screen (window);
  workspace        = meta_window_get_workspace (window);
  active_workspace = meta_screen_get_active_workspace (screen);

  if (!active_workspace || (active_workspace == workspace))
    {
      meta_window_activate_with_workspace (window, timestamp, workspace);
    }
  else
    {
      meta_workspace_activate_with_focus (workspace, window, timestamp);
    }
}

MetaWindow *
mnb_switcher_get_selection (MnbSwitcher *switcher)
{
  MnbSwitcherPrivate *priv = switcher->priv;

  if (!priv->selected)
    return NULL;

  return mutter_window_get_meta_window (priv->selected);
}

static gint
tablist_find_func (gconstpointer a, gconstpointer b)
{
  ClutterActor          *clone    = CLUTTER_ACTOR (a);
  MetaWindow            *meta_win = META_WINDOW (b);
  MetaWindow            *my_win;
  MnbSwitcherAppPrivate *app_priv = MNB_SWITCHER_APP (clone)->priv;

  my_win = mutter_window_get_meta_window (app_priv->mw);

  if (my_win == meta_win)
    return 0;

  return 1;
}

/*
 * Return the next window that Alt+Tab should advance to.
 *
 * The current parameter indicates where we should start from; if NULL, start
 * from the beginning of our Alt+Tab cycle list.
 */
MetaWindow *
mnb_switcher_get_next_window (MnbSwitcher *switcher,
                              MetaWindow  *current,
                              gboolean     backward)
{
  MnbSwitcherPrivate    *priv = switcher->priv;
  GList                 *l;
  ClutterActor          *next = NULL;
  MnbSwitcherAppPrivate *next_priv;

  if (!current)
    {
      if (!priv->selected)
        return NULL;

      current = mutter_window_get_meta_window (priv->selected);
    }

  if (!priv->tab_list)
    {
      g_warning ("No tablist in existence!\n");

      return NULL;
    }

  l = g_list_find_custom (priv->tab_list, current, tablist_find_func);

  if (!backward)
    {
      if (!l || !l->next)
        next = priv->tab_list->data;
      else
        next = l->next->data;
    }
  else
    {
      if (!l || !l->prev)
        next = g_list_last (priv->tab_list)->data;
      else
        next = l->prev->data;
    }

  next_priv = MNB_SWITCHER_APP (next)->priv;

  return mutter_window_get_meta_window (next_priv->mw);
}

/*
 * Alt_tab machinery.
 *
 * Based on do_choose_window() in metacity keybinding.c
 *
 * This is the machinery we need for handling Alt+Tab
 *
 * To start with, Metacity has a passive grab on this key, so we hook up a
 * custom handler to its keybingins.
 *
 * Once we get the initial Alt+Tab, we establish a global key grab (we use
 * metacity API for this, as we need the regular bindings to be correctly
 * restored when we are finished). When the alt key is released, we
 * release the grab.
 */
static gboolean
alt_still_down (MetaDisplay *display, MetaScreen *screen, Window xwin,
                guint entire_binding_mask)
{
  gint        x, y, root_x, root_y, i;
  Window      root, child;
  guint       mask, primary_modifier = 0;
  Display    *xdpy = meta_display_get_xdisplay (display);
  guint       masks[] = { Mod5Mask, Mod4Mask, Mod3Mask,
                          Mod2Mask, Mod1Mask, ControlMask,
                          ShiftMask, LockMask };

  i = 0;
  while (i < (int) G_N_ELEMENTS (masks))
    {
      if (entire_binding_mask & masks[i])
        {
          primary_modifier = masks[i];
          break;
        }

      ++i;
    }

  XQueryPointer (xdpy,
                 xwin, /* some random window */
                 &root, &child,
                 &root_x, &root_y,
                 &x, &y,
                 &mask);

  if ((mask & primary_modifier) == 0)
    return FALSE;
  else
    return TRUE;
}

/*
 * Helper function for metacity_alt_tab_key_handler().
 *
 * The advance parameter indicates whether if the grab succeeds the switcher
 * selection should be advanced.
 */
static void
try_alt_tab_grab (MnbSwitcher  *switcher,
                  gulong        mask,
                  guint         timestamp,
                  gboolean      backward,
                  gboolean      advance)
{
  MnbSwitcherPrivate *priv     = switcher->priv;
  MutterPlugin       *plugin   = priv->plugin;
  MetaScreen         *screen   = mutter_plugin_get_screen (plugin);
  MetaDisplay        *display  = meta_screen_get_display (screen);
  MetaWindow         *next     = NULL;
  MetaWindow         *current  = NULL;

  current = meta_display_get_tab_current (display,
                                          META_TAB_LIST_NORMAL,
                                          screen,
                                          NULL);

  next = mnb_switcher_get_next_window (switcher, NULL, backward);

  /*
   * If we cannot get a window from the switcher (i.e., the switcher is not
   * visible), get the next suitable window from the global tab list.
   */
  if (!next && priv->global_tab_list)
    {
      GList *l;
      MetaWindow *focused = NULL;

      l = priv->global_tab_list;

      while (l)
        {
          MetaWindow *mw = l->data;
          gboolean    sticky;

          sticky = meta_window_is_on_all_workspaces (mw);

          if (sticky)
            {
              /*
               * The loop runs forward when looking for the focused window and
               * when looking for the next window in forward direction; when
               * looking for the next window in backward direction, runs, ehm,
               * backward.
               */
              if (focused && backward)
                l = l->prev;
              else
                l = l->next;
              continue;
            }

          if (!focused)
            {
              if (meta_window_has_focus (mw))
                focused = mw;
            }
          else
            {
              next = mw;
              break;
            }

          /*
           * The loop runs forward when looking for the focused window and
           * when looking for the next window in forward direction; when
           * looking for the next window in backward direction, runs, ehm,
           * backward.
           */
          if (focused && backward)
            l = l->prev;
          else
            l = l->next;
        }

      /*
       * If all fails, fall back at the start/end of the list.
       */
      if (!next && priv->global_tab_list)
        {
          if (backward)
            next = META_WINDOW (priv->global_tab_list->data);
          else
            next = META_WINDOW (g_list_last (priv->global_tab_list)->data);
        }
    }

  /*
   * If we still do not have the next window, or the one we got so far matches
   * the current window, we fall back onto metacity's focus list and try to
   * switch to that.
   */
  if (current && (!next || (advance  && (next == current))))
    {
      MetaWorkspace *ws = meta_window_get_workspace (current);

      next = meta_display_get_tab_next (display,
                                        META_TAB_LIST_NORMAL,
                                        screen,
                                        ws,
                                        current,
                                        backward);
    }

  if (!next || (advance && (next == current)))
    return;


  /*
   * For some reaon, XGrabKeyboard() does not like real timestamps, or
   * we are getting rubish out of clutter ... using CurrentTime here makes it
   * work.
   */
  if (meta_display_begin_grab_op (display,
                                  screen,
                                  NULL,
                                  META_GRAB_OP_KEYBOARD_TABBING_NORMAL,
                                  FALSE,
                                  FALSE,
                                  0,
                                  mask,
                                  timestamp,
                                  0, 0))
    {
      ClutterActor               *stage = mutter_get_stage_for_screen (screen);
      Window                      xwin;

      xwin = clutter_x11_get_stage_window (CLUTTER_STAGE (stage));

      priv->in_alt_grab = TRUE;

      if (!alt_still_down (display, screen, xwin, mask))
        {
          MetaWorkspace *workspace;
          MetaWorkspace *active_workspace;

          meta_display_end_grab_op (display, timestamp);
          priv->in_alt_grab = FALSE;

          workspace        = meta_window_get_workspace (next);
          active_workspace = meta_screen_get_active_workspace (screen);

          mnb_drop_down_hide_with_toolbar (MNB_DROP_DOWN (switcher));

          if (!active_workspace || (active_workspace == workspace))
            {
              meta_window_activate_with_workspace (next,
                                                   timestamp,
                                                   workspace);
            }
          else
            {
              meta_workspace_activate_with_focus (workspace,
                                                  next,
                                                  timestamp);
            }
        }
      else
        {
          if (advance)
            mnb_switcher_select_window (switcher, next);
          else if (current)
            mnb_switcher_select_window (switcher, current);
        }
    }
}

static void
handle_alt_tab (MnbSwitcher    *switcher,
                MetaDisplay    *display,
                MetaScreen     *screen,
                MetaWindow     *event_window,
                XEvent         *event,
                MetaKeyBinding *binding)
{
  MnbSwitcherPrivate  *priv = switcher->priv;
  gboolean             backward = FALSE;
  MetaWindow          *next;
  MetaWorkspace       *workspace;
  guint32              timestamp;

  if (event->type != KeyPress)
    return;

  timestamp = meta_display_get_current_time_roundtrip (display);

#if 0
  printf ("got key event (%d) for keycode %d on window 0x%x, sub 0x%x, state %d\n",
          event->type,
          event->xkey.keycode,
          (guint) event->xkey.window,
          (guint) event->xkey.subwindow,
          event->xkey.state);
#endif

  /* reverse direction if shift is down */
  if (event->xkey.state & ShiftMask)
    backward = !backward;

  workspace = meta_screen_get_active_workspace (screen);

  if (priv->in_alt_grab)
    {
      MetaWindow *selected = mnb_switcher_get_selection (switcher);

      next = mnb_switcher_get_next_window (switcher, selected, backward);

      if (!next)
        {
          g_warning ("No idea what the next selected window should be.\n");
          return;
        }

      mnb_switcher_select_window (MNB_SWITCHER (switcher), next);
      return;
    }

  try_alt_tab_grab (switcher, binding->mask, timestamp, backward, TRUE);
}

struct alt_tab_show_complete_data
{
  MnbSwitcher    *switcher;
  MetaDisplay    *display;
  MetaScreen     *screen;
  MetaWindow     *window;
  MetaKeyBinding *binding;
  XEvent          xevent;
};

static void
alt_tab_switcher_show_completed_cb (ClutterActor *switcher, gpointer data)
{
  struct alt_tab_show_complete_data *alt_data = data;

  handle_alt_tab (MNB_SWITCHER (switcher), alt_data->display, alt_data->screen,
                  alt_data->window, &alt_data->xevent, alt_data->binding);

  /*
   * This is a one-off, disconnect ourselves.
   */
  g_signal_handlers_disconnect_by_func (switcher,
                                        alt_tab_switcher_show_completed_cb,
                                        data);

  g_free (data);
}

static gboolean
alt_tab_timeout_cb (gpointer data)
{
  struct alt_tab_show_complete_data *alt_data = data;
  ClutterActor                      *stage;
  Window                             xwin;

  stage = mutter_get_stage_for_screen (alt_data->screen);
  xwin  = clutter_x11_get_stage_window (CLUTTER_STAGE (stage));

  /*
   * Check wether the Alt key is still down; if so, show the Switcher, and
   * wait for the show-completed signal to process the Alt+Tab.
   */
  if (alt_still_down (alt_data->display, alt_data->screen, xwin, Mod1Mask))
    {
      ClutterActor *toolbar;

      toolbar = clutter_actor_get_parent (CLUTTER_ACTOR (alt_data->switcher));
      while (toolbar && !MNB_IS_TOOLBAR (toolbar))
        toolbar = clutter_actor_get_parent (toolbar);

      g_signal_connect (alt_data->switcher, "show-completed",
                        G_CALLBACK (alt_tab_switcher_show_completed_cb),
                        alt_data);

      /*
       * Set the dont_autohide flag -- autohiding needs to be disabled whenever
       * the toolbar and panels are getting manipulated via keyboar; we clear
       * this flag in our hide cb.
       */
      if (toolbar)
        {
          mnb_toolbar_set_dont_autohide (MNB_TOOLBAR (toolbar), TRUE);
          mnb_toolbar_activate_panel (MNB_TOOLBAR (toolbar), "zones");
        }
    }
  else
    {
      gboolean backward  = FALSE;

      if (alt_data->xevent.xkey.state & ShiftMask)
        backward = !backward;

      try_alt_tab_grab (alt_data->switcher,
                        alt_data->binding->mask,
                        alt_data->xevent.xkey.time, backward, TRUE);
      g_free (data);
    }

  /* One off */
  return FALSE;
}

/*
 * The handler for Alt+Tab that we register with metacity.
 */
static void
mnb_switcher_alt_tab_key_handler (MetaDisplay    *display,
                                  MetaScreen     *screen,
                                  MetaWindow     *window,
                                  XEvent         *event,
                                  MetaKeyBinding *binding,
                                  gpointer        data)
{
  MnbSwitcher *switcher = MNB_SWITCHER (data);

  if (!CLUTTER_ACTOR_IS_MAPPED (switcher))
    {
      struct alt_tab_show_complete_data *alt_data;

      /*
       * If the switcher is not visible we want to show it; this is, however,
       * complicated by several factors:
       *
       *  a) If the panel is visible, we have to show the panel first. In this
       *     case, the Panel slides out, when the effect finishes, the Switcher
       *     slides from underneath -- clutter_actor_show() is only called on
       *     the switcher when the Panel effect completes, and as the contents
       *     of the Switcher are being built in the _show() virtual, we do not
       *     have those until the effects are all over. We need the switcher
       *     contents initialized before we can start the actual processing of
       *     the Alt+Tab key, so we need to wait for the "show-completed" signal
       *
       *  b) If the user just hits and immediately releases Alt+Tab, we need to
       *     avoid initiating the effects alltogether, otherwise we just get
       *     bit of a flicker as the Switcher starts coming out and immediately
       *     disappears.
       *
       *  So, instead of showing the switcher, we install a timeout to introduce
       *  a short delay, so we can test whether the Alt key is still down. We
       *  then handle the actual show from the timeout.
       */
      alt_data = g_new0 (struct alt_tab_show_complete_data, 1);
      alt_data->display  = display;
      alt_data->screen   = screen;
      alt_data->binding  = binding;
      alt_data->switcher = switcher;

      memcpy (&alt_data->xevent, event, sizeof (XEvent));

      g_timeout_add (100, alt_tab_timeout_cb, alt_data);
      return;
    }

  handle_alt_tab (switcher, display, screen, window, event, binding);
}

/*
 * The following two functions are used to maintain the global tab list.
 * When a window is focused, we move it to the top of the list, when it
 * is destroyed, we remove it.
 *
 * NB: the global tab list is a fallback when we cannot use the Switcher list
 * (e.g., for fast Alt+Tab switching without the Switcher.
 */
void
mnb_switcher_meta_window_focus_cb (MetaWindow *mw, gpointer data)
{
  MnbSwitcherPrivate *priv = MNB_SWITCHER (data)->priv;

  /*
   * Push to the top of the global tablist.
   */
  priv->global_tab_list = g_list_remove (priv->global_tab_list, mw);
  priv->global_tab_list = g_list_prepend (priv->global_tab_list, mw);
}

void
mnb_switcher_meta_window_weak_ref_cb (gpointer data, GObject *mw)
{
  MnbSwitcherPrivate *priv = MNB_SWITCHER (data)->priv;

  priv->global_tab_list = g_list_remove (priv->global_tab_list, mw);
}

/*
 * Handles X button and pointer events during Alt_tab grab.
 *
 * Returns true if handled.
 */
gboolean
mnb_switcher_handle_xevent (MnbSwitcher *switcher, XEvent *xev)
{
  MnbSwitcherPrivate *priv   = switcher->priv;
  MutterPlugin       *plugin = priv->plugin;

  if (!priv->in_alt_grab)
    return FALSE;

  if (xev->type == KeyRelease)
    {
      if (XKeycodeToKeysym (xev->xkey.display, xev->xkey.keycode, 0)==XK_Alt_L)
        {
          MetaScreen   *screen  = mutter_plugin_get_screen (plugin);
          MetaDisplay  *display = meta_screen_get_display (screen);
          Time          timestamp = xev->xkey.time;

          meta_display_end_grab_op (display, timestamp);
          priv->in_alt_grab = FALSE;

          mnb_switcher_activate_selection (switcher, TRUE, timestamp);
        }

      /*
       * Block further processing.
       */
      return TRUE;
    }
  else
    {
      /*
       * Block processing of all keyboard and pointer events.
       */
      if (xev->type == KeyPress      ||
          xev->type == ButtonPress   ||
          xev->type == ButtonRelease ||
          xev->type == MotionNotify)
        return TRUE;
    }

  return FALSE;
}
