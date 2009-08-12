/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */

/*
 * Moblin Netbook
 * Copyright © 2009, Intel Corporation.
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

#include <glib/gi18n.h>
#include <clutter/x11/clutter-x11.h>
#include <display.h>

#include "mnb-switcher-zone-apps.h"
#include "mnb-switcher-app.h"

static void mnb_switcher_zone_apps_check_if_empty (MnbSwitcherZoneApps *zone);

struct _MnbSwitcherZoneAppsPrivate
{
  ClutterActor *empty_label;

  gboolean      enabled  : 1;
  gboolean      disposed : 1;
};

enum
{
  ZONE_PROP_0 = 0,

  /* d&d */
  ZONE_PROP_ENABLED
};

static void nbtk_droppable_iface_init (NbtkDroppableIface *iface);

G_DEFINE_TYPE_WITH_CODE (MnbSwitcherZoneApps, mnb_switcher_zone_apps,
                         MNB_TYPE_SWITCHER_ZONE,
                         G_IMPLEMENT_INTERFACE (NBTK_TYPE_DROPPABLE,
                                                nbtk_droppable_iface_init));

#define MNB_SWITCHER_ZONE_APPS_GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MNB_TYPE_SWITCHER_ZONE_APPS,\
                                MnbSwitcherZoneAppsPrivate))

static void
mnb_switcher_zone_apps_dispose (GObject *object)
{
  MnbSwitcherZoneAppsPrivate *priv = MNB_SWITCHER_ZONE_APPS (object)->priv;

  if (priv->disposed)
    return;

  priv->disposed = TRUE;

  if (!clutter_actor_get_parent (priv->empty_label))
    clutter_actor_destroy (priv->empty_label);

  priv->empty_label = NULL;

#if 1
  {
    MnbSwitcherZone  *zone = MNB_SWITCHER_ZONE (object);
    ClutterContainer *table;
    GList            *l, *o;
    /*
     * Workaround for http://bugzilla.moblin.org/show_bug.cgi?id=5078
     *
     * We have to disable dnd on the draggables, otherwise, we will leak them.
     *
     * NB: this fixes the leak, but generates warning, as Nbtk assumes the
     * draggable is still on stage and tries to disconnect its stage callback
     * function from the stage this actor is on.
     */
    table = CLUTTER_CONTAINER (mnb_switcher_zone_get_content_area (zone));
    o = l = clutter_container_get_children (table);

    for (; l; l = l->next)
      {
        NbtkDraggable *d = l->data;

        if (NBTK_IS_DRAGGABLE (d))
          nbtk_draggable_disable (d);
      }

    g_list_free (o);
  }
#endif

  G_OBJECT_CLASS (mnb_switcher_zone_apps_parent_class)->dispose (object);
}

static void
mnb_switcher_zone_apps_get_preferred_width (ClutterActor *actor,
                                            gfloat        for_height,
                                            gfloat       *min_width,
                                            gfloat       *pref_width)
{
  /*
   * This is is a nasty hack to ignore the size of the children.
   * It allows the parent to allocate each of the zones equally, because they
   * all report the same width
   */

  if (min_width)
    *min_width = 0;

  if (pref_width)
    *pref_width = 0;
}

/*
 * D&D machinery
 */
static void
mnb_switcher_zone_apps_over_in (NbtkDroppable *droppable,
                                NbtkDraggable *draggable)
{
  MnbSwitcherZone            *zone = MNB_SWITCHER_ZONE (droppable);
  MnbSwitcherZoneAppsPrivate *priv = MNB_SWITCHER_ZONE_APPS (droppable)->priv;

  if (!priv->enabled)
    return;

  mnb_switcher_zone_set_state (zone, MNB_SWITCHER_ZONE_HOVER);
}

static void
mnb_switcher_zone_apps_over_out (NbtkDroppable *droppable,
                                 NbtkDraggable *draggable)
{
  MnbSwitcherZone            *zone = MNB_SWITCHER_ZONE (droppable);
  MnbSwitcherZoneAppsPrivate *priv = MNB_SWITCHER_ZONE_APPS (droppable)->priv;

  if (!priv->enabled)
    return;

  mnb_switcher_zone_reset_state (zone);
}

/*
 * D&D drop handler
 */
static void
mnb_switcher_zone_apps_drop (NbtkDroppable       *droppable,
                             NbtkDraggable       *draggable,
                             gfloat               event_x,
                             gfloat               event_y,
                             gint                 button,
                             ClutterModifierType  modifiers)
{
  MnbSwitcherZone            *zone = MNB_SWITCHER_ZONE (droppable);
  MnbSwitcherZoneAppsPrivate *priv = MNB_SWITCHER_ZONE_APPS (droppable)->priv;
  MnbSwitcherApp             *app  = MNB_SWITCHER_APP (draggable);
  ClutterActor               *zone_actor = CLUTTER_ACTOR (droppable);
  ClutterActor               *app_actor = CLUTTER_ACTOR (draggable);
  ClutterActor               *parent;
  MutterWindow               *mcw;
  MetaWindow                 *meta_win;
  guint32                     timestamp;
  gint                        row;
  NbtkTable                  *content_area;

  mcw      = mnb_switcher_app_get_window (app);
  meta_win = mutter_window_get_meta_window (mcw);

  if (!meta_win)
    {
      g_warning ("No MutterWindow associated with this item.");
      return;
    }

  /*
   * Check we are not disabled (we should really not be getting drop events on
   * disabled droppables, but we do).
   */
  if (!priv->enabled)
    {
      g_warning ("Bug: received a drop on a disabled droppable -- ignoring");
      return;
    }

  /*
   * Check whether we are not being dropped back on the same zone
   */
  if (mnb_switcher_app_get_pre_drag_parent (app) == zone_actor)
    {
      /*
       * Do nothing here; this is equivalent to the d&d being cancelled and is
       * handled in the drag_end () function.
       */
      return;
    }

  content_area = mnb_switcher_zone_get_content_area (zone);
  row          = nbtk_table_get_row_count (content_area);
  parent       = clutter_actor_get_parent (app_actor);

  /*
   * Move the actor to this zone; must reset the fixed size, so that the actor
   * is allocated correct size for this zone.
   */
  g_object_ref (draggable);
  clutter_container_remove_actor (CLUTTER_CONTAINER (parent), app_actor);
  clutter_actor_set_size (app_actor, -1.0, -1.0);
  nbtk_table_add_actor (content_area, app_actor, row, 0);

  clutter_container_child_set (CLUTTER_CONTAINER (content_area), app_actor,
                               "y-fill", FALSE, "x-fill", FALSE,  NULL);

  if (mnb_switcher_item_is_active (MNB_SWITCHER_ITEM (app)))
    mnb_switcher_zone_set_active (zone, TRUE);
  else
    mnb_switcher_zone_reset_state (zone);

  g_object_unref (draggable);

  timestamp = clutter_x11_get_current_event_time ();
  meta_window_change_workspace_by_index (meta_win,
                                         mnb_switcher_zone_get_index (zone),
                                         TRUE, timestamp);
}

static void
nbtk_droppable_iface_init (NbtkDroppableIface *iface)
{
  iface->over_in  = mnb_switcher_zone_apps_over_in;
  iface->over_out = mnb_switcher_zone_apps_over_out;
  iface->drop     = mnb_switcher_zone_apps_drop;
}

static void
mnb_switcher_zone_apps_set_property (GObject      *gobject,
                                     guint         prop_id,
                                     const GValue *value,
                                     GParamSpec   *pspec)
{
  MnbSwitcherZoneAppsPrivate *priv = MNB_SWITCHER_ZONE_APPS (gobject)->priv;

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
mnb_switcher_zone_apps_get_property (GObject    *gobject,
                                     guint       prop_id,
                                     GValue     *value,
                                     GParamSpec *pspec)
{
  MnbSwitcherZoneAppsPrivate *priv = MNB_SWITCHER_ZONE_APPS (gobject)->priv;

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

/*
 * Appends item representing window mcw to the zone; if ignore_focus is TRUE
 * no examination of focus status is done; if we applied the focused state to
 * the window returns TRUE, FALSE otherwise (the ignore_focus and return value
 * allow us to avoid unnecessary traversal of the window descendants necessary
 * to determine whether a window should have focus because one of its transients
 * is the the current focus window.
 */
static gboolean
mnb_switcher_zone_apps_append_window (MnbSwitcherZoneApps *zone,
                                      MutterWindow        *mcw,
                                      gboolean             ignore_focus)
{
  NbtkTable      *table;
  gint            row;
  MnbSwitcherApp *app;
  MnbSwitcher    *switcher;
  MetaWindow     *w = mutter_window_get_meta_window (mcw);
  gboolean        retval = FALSE;

  table = mnb_switcher_zone_get_content_area ((MnbSwitcherZone*)zone);
  switcher = mnb_switcher_zone_get_switcher ((MnbSwitcherZone*)zone);
  app = mnb_switcher_app_new (switcher, mcw);

  if (!ignore_focus)
    { MetaDisplay *d = meta_window_get_display (w);
      MetaWindow  *f = meta_display_get_focus_window (d);

      if (meta_window_has_focus (w) ||
          (f && meta_window_is_ancestor_of_transient (w, f)))
        {
          retval = TRUE;
          mnb_switcher_item_set_active (MNB_SWITCHER_ITEM (app), TRUE);
        }
    }

  row = nbtk_table_get_row_count (table);

  nbtk_table_add_actor (table, CLUTTER_ACTOR (app), row, 0);

  /*
   * Enable d&d
   */
  g_object_set (app, "enabled", TRUE, NULL);

  clutter_container_child_set (CLUTTER_CONTAINER (table), CLUTTER_ACTOR (app),
                               "y-fill", FALSE, "x-fill", FALSE, NULL);

  return retval;
}

/*
 * Can't fold this into the constructor, because the d&d on the items can
 * only be enabled after the actor was added to stage (i.e., not before the
 * whole construction is done).
 */
void
mnb_switcher_zone_apps_populate (MnbSwitcherZoneApps *zone)
{
  MutterPlugin  *plugin;
  MetaScreen    *screen;
  MetaWorkspace *ws;
  gint           index;
  GList         *l, *o;
  gboolean       ignore_focus;

  index = mnb_switcher_zone_get_index (MNB_SWITCHER_ZONE (zone));

  g_assert (index >= 0);

  plugin = moblin_netbook_get_plugin_singleton ();
  screen = mutter_plugin_get_screen (plugin);

  ws = meta_screen_get_workspace_by_index (screen, index);

  if (!ws)
    {
      g_warning (G_STRLOC " no workspace at index %d", index);
      return;
    }

  /*
   * If this is not an active zone, then we do not care about focus.
   */
  ignore_focus = !mnb_switcher_zone_is_active (MNB_SWITCHER_ZONE (zone));

  for (o = l = meta_workspace_list_windows (ws); l; l = l->next)
    {
      MetaWindow   *w = l->data;
      MutterWindow *m = (MutterWindow*)meta_window_get_compositor_private (w);
      MetaCompWindowType type;

      type = mutter_window_get_window_type (m);

      if (meta_window_is_on_all_workspaces (w)   ||
          mutter_window_is_override_redirect (m) || /* Unecessary */
          (type != META_COMP_WINDOW_NORMAL  &&
           type != META_COMP_WINDOW_DIALOG))
        {
          continue;
        }

      /*
       * Only intersted in top-level dialogs, i.e., those not transient
       * to anything or to root.
       */
      if (type ==  META_COMP_WINDOW_DIALOG)
        {
          MetaWindow *p = meta_window_find_root_ancestor (w);

          if (p != w)
            continue;
        }

      ignore_focus |= mnb_switcher_zone_apps_append_window (zone, m,
                                                            ignore_focus);
    }

  g_list_free (o);

  mnb_switcher_zone_apps_check_if_empty (zone);

  /*
   * Enable d&d
   */
  g_object_set (zone, "enabled", TRUE, NULL);
}

static void
mnb_switcher_zone_apps_constructed (GObject *self)
{
  MnbSwitcherZoneApps        *zone = MNB_SWITCHER_ZONE_APPS (self);
  MnbSwitcherZoneAppsPrivate *priv = zone->priv;
  NbtkTable                  *table;

  if (G_OBJECT_CLASS (mnb_switcher_zone_apps_parent_class)->constructed)
    G_OBJECT_CLASS (mnb_switcher_zone_apps_parent_class)->constructed (self);

  table = mnb_switcher_zone_get_content_area ((MnbSwitcherZone*)zone);

  nbtk_table_set_row_spacing (NBTK_TABLE (table), 6);
  nbtk_table_set_col_spacing (NBTK_TABLE (table), 6);
  clutter_actor_set_reactive (CLUTTER_ACTOR (self), TRUE);

  priv->empty_label =
    (ClutterActor*)nbtk_label_new (_("No applications on this zone"));
  g_object_ref_sink (priv->empty_label);
}

/*
 * Implementation of MnbSwitcherItem::next_item vfunction
 */
static MnbSwitcherItem *
mnb_switcher_zone_apps_next_item  (MnbSwitcherZone *zone,
                                   MnbSwitcherItem *current)
{
  GList            *l, *o;
  gpointer          next = NULL;
  ClutterContainer *table;

  if (current && !MNB_IS_SWITCHER_APP (current))
    return NULL;

  table = CLUTTER_CONTAINER (mnb_switcher_zone_get_content_area (zone));

  o = l = clutter_container_get_children (table);

  if (!current)
    {
      if (MNB_IS_SWITCHER_APP (l->data))
        next = l->data;
    }
  else
    {
      while (l)
        {
          if (l->data == current)
            {
              GList *n = l->next;

              while (n && !MNB_IS_SWITCHER_APP (n->data))
                n = n->next;

              if (n)
                next = n->data;

              break;
            }

          l = l->next;
        }
    }

  g_list_free (o);

  return next;
}

/*
 * Implementation of MnbSwitcherItem::prev_item vfunction
 */
static MnbSwitcherItem *
mnb_switcher_zone_apps_prev_item  (MnbSwitcherZone *zone,
                                   MnbSwitcherItem *current)
{
  GList            *l, *o;
  gpointer          prev = NULL;
  ClutterContainer *table;

  if (current && !MNB_IS_SWITCHER_APP (current))
    return NULL;

  table = CLUTTER_CONTAINER (mnb_switcher_zone_get_content_area (zone));

  o = l = clutter_container_get_children (table);
  if (!current)
    {
      l = g_list_last (o);

      if (MNB_IS_SWITCHER_APP (l->data))
        prev = l->data;
    }
  else
    {
      while (l)
        {
          if (l->data == current)
            {
              GList *p = l->prev;

              while (p && !MNB_IS_SWITCHER_APP (p->data))
                p = p->prev;

              if (p)
                prev = p->data;

              break;
            }

          l = l->next;
        }
    }

  g_list_free (o);

  return prev;
}

/*
 * Implementation of MnbSwitcherItem::unselect_all vfunction
 */
static void
mnb_switcher_zone_apps_unselect_all (MnbSwitcherZone *zone)
{
  GList *l, *o;

  o = l = clutter_container_get_children (CLUTTER_CONTAINER (zone));

  while (l)
    {
      if (MNB_IS_SWITCHER_APP (l->data))
        {
          MnbSwitcherApp *app = l->data;
          mnb_switcher_item_set_active (MNB_SWITCHER_ITEM (app), FALSE);
        }

      l = l->next;
    }

  g_list_free (o);
}

/*
 * Implementation of MnbSwitcherItem::select_item vfunction
 */
static gboolean
mnb_switcher_zone_apps_select_item (MnbSwitcherZone *zone,
                                    MnbSwitcherItem *item)
{
  GList            *l, *o;
  gboolean          found = FALSE;
  ClutterContainer *table;

  if (!MNB_IS_SWITCHER_APP (item))
    return FALSE;

  table = CLUTTER_CONTAINER (mnb_switcher_zone_get_content_area (zone));

  o = l = clutter_container_get_children (CLUTTER_CONTAINER (table));
  while (l)
    {
      if (l->data == item)
        {
          mnb_switcher_item_set_active (MNB_SWITCHER_ITEM (item), TRUE);
          found = TRUE;
        }
      else if (MNB_IS_SWITCHER_APP (l->data))
        {
          MnbSwitcherApp *app = l->data;
          mnb_switcher_item_set_active (MNB_SWITCHER_ITEM (app), FALSE);
        }

      l = l->next;
    }

  g_list_free (o);

  return found;
}

static void
mnb_switcher_zone_apps_class_init (MnbSwitcherZoneAppsClass *klass)
{
  GObjectClass         *object_class = G_OBJECT_CLASS (klass);
  ClutterActorClass    *actor_class  = CLUTTER_ACTOR_CLASS (klass);
  MnbSwitcherZoneClass *zone_class   = MNB_SWITCHER_ZONE_CLASS (klass);

  object_class->dispose            = mnb_switcher_zone_apps_dispose;
  object_class->constructed        = mnb_switcher_zone_apps_constructed;
  object_class->get_property       = mnb_switcher_zone_apps_get_property;
  object_class->set_property       = mnb_switcher_zone_apps_set_property;

  actor_class->get_preferred_width = mnb_switcher_zone_apps_get_preferred_width;

  zone_class->next_item            = mnb_switcher_zone_apps_next_item;
  zone_class->prev_item            = mnb_switcher_zone_apps_prev_item;
  zone_class->select_item          = mnb_switcher_zone_apps_select_item;
  zone_class->unselect_all         = mnb_switcher_zone_apps_unselect_all;

  g_type_class_add_private (klass, sizeof (MnbSwitcherZoneAppsPrivate));

  g_object_class_override_property (object_class,
                                    ZONE_PROP_ENABLED,
                                    "enabled");
}

static void
mnb_switcher_zone_apps_init (MnbSwitcherZoneApps *self)
{
  self->priv = MNB_SWITCHER_ZONE_APPS_GET_PRIVATE (self);
}

MnbSwitcherZoneApps *
mnb_switcher_zone_apps_new (MnbSwitcher *switcher, gboolean active, gint index)
{
  return g_object_new (MNB_TYPE_SWITCHER_ZONE_APPS,
                       "switcher", switcher,
                       "active",   active,
                       "index",    index,
                       NULL);
}

/*
 * Checks if the zone is empty, and if so, appends appropriate notice.
 */
static void
mnb_switcher_zone_apps_check_if_empty (MnbSwitcherZoneApps *zone)
{
  MnbSwitcherZoneAppsPrivate *priv = zone->priv;
  GList *l, *o;
  gboolean empty = TRUE;
  NbtkTable *table;

  table = mnb_switcher_zone_get_content_area ((MnbSwitcherZone*)zone);

  o = l = clutter_container_get_children (CLUTTER_CONTAINER (table));

  while (l)
    {
      if (MNB_IS_SWITCHER_APP (l->data))
        {
          empty = FALSE;
          break;
        }

      l = l->next;
    }

  if (empty)
    {
      mnb_switcher_zone_set_has_items ((MnbSwitcherZone*)zone, FALSE);
      if (!clutter_actor_get_parent (priv->empty_label))
        {
          NbtkTable *table;

          table = mnb_switcher_zone_get_content_area ((MnbSwitcherZone*)zone);
          nbtk_table_add_actor (table, priv->empty_label, 0, 0);
        }
    }
  else
    {
      ClutterActor *parent = clutter_actor_get_parent (priv->empty_label);

      mnb_switcher_zone_set_has_items ((MnbSwitcherZone*)zone, TRUE);
      if (parent)
        {
          clutter_container_remove_actor (CLUTTER_CONTAINER (parent),
                                          priv->empty_label);
        }
    }
}

