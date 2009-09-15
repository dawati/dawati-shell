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
 */

#include "mnb-switcher.h"
#include "mnb-switcher-keys.h"
#include "mnb-switcher-app.h"
#include "mnb-switcher-zone-apps.h"
#include "mnb-switcher-zone-new.h"
#include "../moblin-netbook.h"
#include "../mnb-toolbar.h"

#include <string.h>
#include <display.h>
#include <clutter/clutter.h>
#include <clutter/x11/clutter-x11.h>
#include <nbtk/nbtk.h>
#include <keybindings.h>

#include <glib/gi18n.h>

#include "mnb-switcher-private.h"

/*
 * MnbSwitcher is an MnbDropDown subclass implementing the Zones panel of the
 * shell.
 */
G_DEFINE_TYPE (MnbSwitcher, mnb_switcher, MNB_TYPE_DROP_DOWN)

#define MNB_SWITCHER_GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MNB_TYPE_SWITCHER, MnbSwitcherPrivate))

/*
 * When we show ourselves, we want to pop up a tooltip.
 */
static void
on_show_completed_cb (ClutterActor *self, gpointer data)
{
  MnbSwitcherPrivate *priv = MNB_SWITCHER (self)->priv;

  /*
   * We probably have no selected item, just a selected zone (this is because
   * of how the focus stashing works; we unstashed focus, but it will not take
   * place until the main loop had a chance to spin and process the X events).
   *
   * So, if there is a focused window, activate the item that corresponds to
   * it.
   */

  if (!priv->selected_item && priv->selected_zone &&
      MNB_IS_SWITCHER_ZONE_APPS (priv->selected_zone))
    {
      MetaScreen  *s = mutter_plugin_get_screen (priv->plugin);
      MetaDisplay *d = meta_screen_get_display (s);
      MetaWindow  *w = meta_display_get_focus_window (d);

      if (w)
        {
          MetaWindow          *t;
          MetaWindow          *f = w;
          MnbSwitcherZoneApps *apps;
          MutterWindow        *m;

          apps = MNB_SWITCHER_ZONE_APPS (priv->selected_zone);

          /*
           * Deal with transiency, in case this window is a dialog or something
           */
          while ((t = meta_window_get_transient_for (f)) && t != f)
            f = t;

          m = (MutterWindow*) meta_window_get_compositor_private (f);

          priv->selected_item =
            mnb_switcher_zone_apps_activate_window (apps, m);
        }
    }

  /*
   * Not if we are showing ourselves because of Alt+Tab press (we have a
   * separate show callback for that).
   */
  if (priv->in_alt_grab)
    return;

  if (priv->selected_item)
    mnb_switcher_item_show_tooltip (priv->selected_item);
}

/*
 * Implements the logic for conditionally enabling the new Zone during d&d
 */
static void
mnb_switcher_enable_new_workspace (MnbSwitcher *switcher, MnbSwitcherZone *zone)
{
  MnbSwitcherPrivate *priv = switcher->priv;
  gint                ws_count;
  MetaScreen         *screen = mutter_plugin_get_screen (priv->plugin);
  MnbSwitcherZone    *new_ws = priv->new_zone;

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

  mnb_switcher_zone_set_state (new_ws, MNB_SWITCHER_ZONE_ACTIVE);

  clutter_actor_set_width (CLUTTER_ACTOR (new_ws), 44);
}

/*
 * Disable the new Zone as a drag target.
 */
static void
mnb_switcher_disable_new_workspace (MnbSwitcher     *switcher,
                                    MnbSwitcherZone *zone)
{
  MnbSwitcherPrivate *priv = switcher->priv;
  MnbSwitcherZone    *new_ws = priv->new_zone;

  g_object_set (new_ws, "enabled", FALSE, NULL);
  mnb_switcher_zone_reset_state (new_ws);
  clutter_actor_set_width (CLUTTER_ACTOR (new_ws), 22);
}

/*
 * Called by any MnbSwitcherZoneItem that implements d&d when drag starts
 */
void
mnb_switcher_dnd_started (MnbSwitcher *switcher, MnbSwitcherZone *zone)
{
  MnbSwitcherPrivate *priv = switcher->priv;

  priv->dnd_in_progress = TRUE;

  if (zone != priv->new_zone)
    mnb_switcher_enable_new_workspace (switcher, zone);
}

/*
 * Called by any MnbSwitcherZoneItem that implements d&d when drag ends
 */
void
mnb_switcher_dnd_ended (MnbSwitcher *switcher, MnbSwitcherZone *zone)
{
  MnbSwitcherPrivate *priv = switcher->priv;

  priv->dnd_in_progress = FALSE;

  if (zone != priv->new_zone)
    mnb_switcher_disable_new_workspace (switcher, zone);
}

/*
 * Implementation of ClutterActor::show vfunction.
 *
 * We construct the switcher on the fly when it is getting shown, rather than
 * going throught the hassle of maitaining it in sync with the actual desktop.
 */
static void
mnb_switcher_show (ClutterActor *self)
{
  MnbSwitcher        *switcher = MNB_SWITCHER (self);
  MnbSwitcherPrivate *priv = switcher->priv;
  MetaScreen         *screen = mutter_plugin_get_screen (priv->plugin);
  gint                ws_count, active_ws;
  gint                i, screen_width, screen_height;
  NbtkWidget         *table;
  ClutterActor       *toolbar;
  gboolean            switcher_empty = FALSE;

  moblin_netbook_unstash_window_focus (priv->plugin, CurrentTime);

  /*
   * Check the panel is visible, if not get the parent class to take care of
   * it (this will result in this function being called again once the Toolbar
   * is visible).
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

  mutter_plugin_query_screen_size (priv->plugin, &screen_width, &screen_height);

  priv->last_workspaces = g_list_copy (meta_screen_get_workspaces (screen));

  /* create the contents */
  table = nbtk_table_new ();
  priv->table = table;
  nbtk_table_set_row_spacing (NBTK_TABLE (table), 4);
  nbtk_table_set_col_spacing (NBTK_TABLE (table), 7);

  clutter_actor_set_name (CLUTTER_ACTOR (table), "switcher-table");

  mnb_drop_down_set_child (MNB_DROP_DOWN (self),
                           CLUTTER_ACTOR (table));

  ws_count    = meta_screen_get_n_workspaces (screen);
  active_ws   = meta_screen_get_active_workspace_index (screen);

  /* Handle case where no apps open.. */
  if (ws_count == 1)
    {
      GList *window_list, *l;

      window_list = mutter_plugin_get_windows (priv->plugin);
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
          label = nbtk_label_new (_("Applications you’re using will show up here. You will be able to switch and organize them to your heart's content."));
          clutter_actor_set_name ((ClutterActor *)label, "workspace-no-wins-label");

          nbtk_bin_set_child (NBTK_BIN (bin), CLUTTER_ACTOR (label));
          nbtk_bin_set_alignment (NBTK_BIN (bin), NBTK_ALIGN_START,
                                  NBTK_ALIGN_MIDDLE);
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

  for (i = 0; i < ws_count; ++i)
    {
      MnbSwitcherZoneApps *zone;
      gboolean             active = FALSE;

      if (i == active_ws)
        active = TRUE;

      zone = mnb_switcher_zone_apps_new (switcher, active, i);
      nbtk_table_add_actor (NBTK_TABLE (table), CLUTTER_ACTOR (zone), 0, i);
      mnb_switcher_zone_apps_populate (zone);

      if (active)
        {
          g_assert (!priv->selected_zone);

          priv->selected_zone = (MnbSwitcherZone*)zone;
          priv->selected_item =
            mnb_switcher_zone_get_active_item ((MnbSwitcherZone*)zone);
        }
    }

  /*
   * Now create the new workspace column.
   */
  {
    MnbSwitcherZone *new_ws = mnb_switcher_zone_new_new (MNB_SWITCHER (self),
                                                         ws_count);

    priv->new_zone = new_ws;

    /*
     * Disable the new droppable; it will get enabled if appropriate when
     * the d&d begins.
     */
    g_object_set (new_ws, "enabled", FALSE, NULL);

    nbtk_table_add_actor (NBTK_TABLE (table), CLUTTER_ACTOR (new_ws), 0,
                          ws_count);

    clutter_container_child_set (CLUTTER_CONTAINER (table),
                                 CLUTTER_ACTOR (new_ws),
                                 "y-expand", FALSE,
                                 "x-expand", FALSE, NULL);
  }

 finish_up:

  if (!switcher_empty)
    clutter_actor_set_height (CLUTTER_ACTOR (table),
                              screen_height - TOOLBAR_HEIGHT * 1.5 - 4);

  priv->constructing = FALSE;

  /*
   * We connect to the show-completed signal, and if there is something focused
   * in the switcher (should be most of the time), we try to pop up the
   * tooltip from the callback.
   */
  if (priv->show_completed_id)
    {
      priv->show_completed_id = 0;
      g_signal_handler_disconnect (self, priv->show_completed_id);
    }

  priv->show_completed_id =
    g_signal_connect (self, "show-completed",
                      G_CALLBACK (on_show_completed_cb), NULL);

  CLUTTER_ACTOR_CLASS (mnb_switcher_parent_class)->show (self);
}

/*
 * Inserts a MnbSwitcherZoneApps zone into the switcher at position indicated
 * by index.
 */
ClutterActor *
mnb_switcher_append_app_zone (MnbSwitcher *switcher, gint index)
{
  MnbSwitcherPrivate *priv  = switcher->priv;
  NbtkWidget         *table = priv->table;
  ClutterActor       *zone;

  zone = (ClutterActor*)mnb_switcher_zone_apps_new (switcher, FALSE, index);
  nbtk_table_add_actor (NBTK_TABLE (table), zone, 0, index);

  /*
   * Move the new zone 1 back
   */
  clutter_container_child_set (CLUTTER_CONTAINER (table),
                               CLUTTER_ACTOR (priv->new_zone),
                               "col", index + 1, NULL);

  return zone;
}

/*
 * Implementation of the ClutterActor::hide vfunction
 *
 * When we hide, we dispose of the internal child.
 */
static void
mnb_switcher_hide (ClutterActor *self)
{
  MnbSwitcherPrivate *priv = MNB_SWITCHER (self)->priv;

  /*
   * This should not be necessary, since every tooltip gets hidden in the
   * item dispose vfunction, but if something breaks and an item leaks, we want
   * to make sure that we at least have no left over tooltips on screen.
   */
  mnb_switcher_hide_tooltip ((MnbSwitcher*)self);

  if (priv->show_completed_id)
    {
      g_signal_handler_disconnect (self, priv->show_completed_id);
      priv->show_completed_id = 0;
    }

  /*
   * If we are in alt+Tab grab, we need to release the grab.
   */
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

  g_list_free (priv->global_tab_list);
  g_list_free (priv->last_workspaces);

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
mnb_switcher_n_workspaces_notify (MetaScreen *screen,
                                  GParamSpec *pspec,
                                  gpointer    data)
{
  MnbSwitcher        *switcher = MNB_SWITCHER (data);
  MnbSwitcherPrivate *priv = switcher->priv;
  gint                n_c_workspaces;
  GList              *c_workspaces;
  GList              *o_workspaces;
  gint                n_o_workspaces;
  gint                i;
  GList              *k, *l;
  ClutterContainer *table_cont = CLUTTER_CONTAINER (priv->table);

  if (!CLUTTER_ACTOR_IS_MAPPED (CLUTTER_ACTOR (switcher)))
    return;

  n_c_workspaces = meta_screen_get_n_workspaces (screen);
  c_workspaces   = meta_screen_get_workspaces (screen);
  o_workspaces   = priv->last_workspaces;
  n_o_workspaces = g_list_length (o_workspaces);

  if (n_c_workspaces < 8)
    g_object_set (priv->new_zone, "enabled", TRUE, NULL);
  else
    g_object_set (priv->new_zone, "enabled", FALSE, NULL);

  if (n_o_workspaces < n_c_workspaces)
    {
      /*
       * Workspace added; the relayout is handled in the dnd_dropped callback.
       */
      g_list_free (priv->last_workspaces);
      priv->last_workspaces = g_list_copy (c_workspaces);
      return;
    }

  /*
   * Work out which of the workspaces on the old list is no longer found
   * in the new list.
   */
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
              /*
               * Reset to NULL, so we know later this one is still there.
               */
              l->data = NULL;
              break;
            }

          ++i;
          l = l->next;
        }

      k = k->next;
    }

  /*
   * Now go over the old workspace list, and remove anything that is left there
   * from the Switcher table.
   */
  k = clutter_container_get_children (table_cont);
  for (l = o_workspaces, i = 0; l; l = l->next, ++i)
    {
      if (l->data)
        {
          GList *t = k;

          g_debug ("removing workspace that used to be at position %d", i);

          /* NbtkTable provides no API to get a child at given position, so we
           * have to iterate the child list, querying the child column property
           */
          while (t)
            {
              ClutterActor *a = t->data;
              gint          col;

              clutter_container_child_get (table_cont, a, "col", &col, NULL);

              if (col == i)
                {
                  clutter_container_remove_actor (table_cont, a);
                  break;
                }

              t = t->next;
            }
        }
    }

  g_list_free (k);

  /*
   * Get a fresh list of the switcher children and update their index.
   *
   * TODO -- this could be folded into the loop above.
   */
  k = clutter_container_get_children (table_cont);
  for (l = k, i = 0; l; l = l->next, ++i)
    {
      MnbSwitcherZone *zone = l->data;

      if (MNB_IS_SWITCHER_ZONE (zone))
        {
          mnb_switcher_zone_set_index (zone, i);
        }
      else
        g_warning (G_STRLOC " expected MnbSwitcherZone, got %s",
                   zone ? G_OBJECT_TYPE_NAME (zone) : "NULL");
    }

  g_list_free (k);

  g_list_free (priv->last_workspaces);
  priv->last_workspaces = g_list_copy (c_workspaces);
}

static void
mnb_switcher_constructed (GObject *self)
{
  MnbSwitcher *switcher  = MNB_SWITCHER (self);
  MutterPlugin *plugin;

  if (G_OBJECT_CLASS (mnb_switcher_parent_class)->constructed)
    G_OBJECT_CLASS (mnb_switcher_parent_class)->constructed (self);

  g_object_get (self, "mutter-plugin", &plugin, NULL);

  switcher->priv->plugin = plugin;

  g_signal_connect (mutter_plugin_get_screen (plugin),
                    "notify::keyboard-grabbed",
                    G_CALLBACK (mnb_switcher_kbd_grab_notify_cb),
                    self);

  g_signal_connect (mutter_plugin_get_screen (plugin), "notify::n-workspaces",
                    G_CALLBACK (mnb_switcher_n_workspaces_notify), self);
}

static void
mnb_switcher_class_init (MnbSwitcherClass *klass)
{
  GObjectClass      *object_class = G_OBJECT_CLASS (klass);
  ClutterActorClass *actor_class  = CLUTTER_ACTOR_CLASS (klass);

  object_class->finalize          = mnb_switcher_finalize;
  object_class->constructed       = mnb_switcher_constructed;

  actor_class->show               = mnb_switcher_show;
  actor_class->hide               = mnb_switcher_hide;

  g_type_class_add_private (klass, sizeof (MnbSwitcherPrivate));
}

/*
 * Callback for the end of the hide animation.
 */
static void
on_switcher_hide_completed_cb (ClutterActor *self, gpointer data)
{
  MnbSwitcherPrivate         *priv;
  MoblinNetbookPluginPrivate *ppriv;
  ClutterActor               *toolbar;

  g_return_if_fail (MNB_IS_SWITCHER (self));

  priv  = MNB_SWITCHER (self)->priv;
  ppriv = MOBLIN_NETBOOK_PLUGIN (priv->plugin)->priv;

  priv->table = NULL;
  priv->active_tooltip = NULL;
  priv->new_zone = NULL;
  priv->selected_zone = NULL;
  priv->selected_item = NULL;

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

/*
 * Gets zone at position n in the switcher
 */
static MnbSwitcherZone *
mnb_switcher_get_nth_zone (MnbSwitcher *switcher, gint n)
{
  MnbSwitcherPrivate *priv = switcher->priv;
  GList              *l, *o;
  ClutterContainer   *table = CLUTTER_CONTAINER (priv->table);
  MnbSwitcherZone    *zone = NULL;

  o = l = clutter_container_get_children (table);

  for (; l; l = l->next)
    {
      MnbSwitcherZone *z = l->data;

      if (!MNB_IS_SWITCHER_ZONE (z))
        {
          g_warning ("Got something else than zone (%s) in switcher !!!",
                     G_OBJECT_TYPE_NAME (z));
          continue;
        }

      if (n == mnb_switcher_zone_get_index (z))
        {
          zone = z;
          break;
        }
    }

  g_list_free (o);

  return zone;
}

/*
 * Selects given item in the switcher.
 */
static void
mnb_switcher_select_item (MnbSwitcher *switcher, MnbSwitcherItem *item)
{
  MnbSwitcherPrivate *priv = switcher->priv;
  MnbSwitcherZone    *zone;

  mnb_switcher_hide_tooltip (switcher);

  zone = mnb_switcher_item_get_zone (item);

  g_assert (zone);

  if (mnb_switcher_zone_select_item (zone, item))
    {
      MnbSwitcherZone *current_zone = priv->selected_zone;
      MnbSwitcherItem *current_item = priv->selected_item;

      if (current_zone && current_zone != zone)
        mnb_switcher_zone_set_state (current_zone, MNB_SWITCHER_ZONE_NORMAL);

      if (current_item && current_item != item)
        mnb_switcher_item_set_active (current_item, FALSE);

      priv->selected_item = item;
      priv->selected_zone = zone;
    }
}

/*
 * Selects given zone in the switcher
 */
static void
mnb_switcher_select_zone (MnbSwitcher *switcher, MnbSwitcherZone *zone)
{
  MnbSwitcherPrivate *priv = switcher->priv;

  mnb_switcher_hide_tooltip (switcher);

  g_assert (zone);

  if (mnb_switcher_zone_select (zone))
    {
      MnbSwitcherZone *current_zone = priv->selected_zone;
      MnbSwitcherItem *current_item = priv->selected_item;

      if (current_zone && current_zone != zone)
        mnb_switcher_zone_set_state (current_zone, MNB_SWITCHER_ZONE_NORMAL);

      if (current_item)
        mnb_switcher_item_set_active (current_item, FALSE);

      priv->selected_item = NULL;
      priv->selected_zone = zone;
    }
}

/*
 * Activates the window currently selected in the switcher.
 */
void
mnb_switcher_activate_selection (MnbSwitcher *switcher,
                                 gboolean     close,
                                 guint        timestamp)
{
  MnbSwitcherPrivate *priv = switcher->priv;
  MnbSwitcherItem    *item = priv->selected_item;
  MnbSwitcherZone    *zone = priv->selected_zone;

  if (item)
    mnb_switcher_item_activate (item);
  else if (zone)
    mnb_switcher_zone_activate (zone);
  else
    {
      g_warning (G_STRLOC " Nothing to activate");
      return;
    }

  if (close)
    mnb_drop_down_hide_with_toolbar (MNB_DROP_DOWN (switcher));
}

/*
 * Returns the number of zones in the switcher.
 *
 * TODO -- probably cache this number somewhere
 */
static gint
mnb_switcher_get_zone_count (MnbSwitcher *switcher)
{
  MnbSwitcherPrivate *priv  = switcher->priv;
  ClutterContainer   *table = CLUTTER_CONTAINER (priv->table);
  GList              *l;
  gint                len;

  l = clutter_container_get_children (table);

  len = g_list_length (l);

  g_list_free (l);

  return len;
}

/*
 * Returns the next zone in the switcher (or previous if backward is TRUE)
 */
static MnbSwitcherZone *
mnb_switcher_get_next_zone (MnbSwitcher     *switcher,
                            MnbSwitcherZone *current,
                            gboolean         backward)
{
  MnbSwitcherPrivate *priv = switcher->priv;
  gint                index;
  MnbSwitcherZone    *next;

  if (!priv->table)
    return NULL;

  index = mnb_switcher_zone_get_index (current);

  if (backward)
    {
      index--;

      if (index < 0)
        index = mnb_switcher_get_zone_count (switcher) - 1;
    }
  else
    {
      index++;

      if (index == mnb_switcher_get_zone_count (switcher))
        index = 0;
    }

  next = mnb_switcher_get_nth_zone (switcher, index);

  return next;
}

/*
 * Advances the switcher by a single item.
 */
void
mnb_switcher_advance (MnbSwitcher *switcher, gboolean backward)
{
  MnbSwitcherPrivate *priv         = switcher->priv;
  MnbSwitcherZone    *current_zone = priv->selected_zone;
  MnbSwitcherItem    *current_item = priv->selected_item;
  MnbSwitcherItem    *next_item    = NULL;
  MnbSwitcherZone    *next_zone    = NULL;

  if (!current_zone)
    {
      current_zone = mnb_switcher_get_nth_zone (switcher, 0);
      if (!current_zone || !MNB_IS_SWITCHER_ZONE (current_zone))
        return;
    }

  if (mnb_switcher_zone_has_items (current_zone))
    {
      if (!backward)
        next_item =
          mnb_switcher_zone_get_next_item (current_zone, current_item);
      else
        next_item =
          mnb_switcher_zone_get_prev_item (current_zone, current_item);
    }

  if (next_item)
    {
      mnb_switcher_select_item (switcher, next_item);
      return;
    }

  next_zone = current_zone;
  while ((next_zone = mnb_switcher_get_next_zone (switcher,
                                                  next_zone, backward)) &&
         next_zone != current_zone)
    {
      if (mnb_switcher_zone_has_items (next_zone))
        {
          if (!backward)
            next_item =
              mnb_switcher_zone_get_next_item (next_zone, NULL);
          else
            next_item =
              mnb_switcher_zone_get_prev_item (next_zone, NULL);

          if (next_item)
            {
              mnb_switcher_select_item (switcher, next_item);
              return;
            }
        }
      else if (mnb_switcher_zone_is_pageable (next_zone))
        {
          mnb_switcher_select_zone (switcher, next_zone);
          return;
        }
    }
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
 * Returns TRUE if handled.
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
      if ((XKeycodeToKeysym (xev->xkey.display, xev->xkey.keycode, 0)==XK_Alt_L) ||
          (XKeycodeToKeysym (xev->xkey.display, xev->xkey.keycode, 0)==XK_Alt_R))
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

/*
 * Returns TRUE if d&d is currently taking place.
 */
gboolean
mnb_switcher_get_dnd_in_progress (MnbSwitcher *switcher)
{
  MnbSwitcherPrivate *priv = switcher->priv;

  return priv->dnd_in_progress;
}

/*
 * Hides the currently showing tooltip, if any.
 */
void
mnb_switcher_hide_tooltip (MnbSwitcher *switcher)
{
  MnbSwitcherPrivate *priv = switcher->priv;

  if (priv->active_tooltip)
    {
      nbtk_tooltip_hide (NBTK_TOOLTIP (priv->active_tooltip));
      priv->active_tooltip = NULL;
    }
}

/*
 * Shows the provided tooltip, hiding any previously visible tooltips.
 */
void
mnb_switcher_show_tooltip (MnbSwitcher *switcher, NbtkTooltip *tooltip)
{
  MnbSwitcherPrivate *priv = switcher->priv;

  if (priv->active_tooltip)
    {
      nbtk_tooltip_hide (NBTK_TOOLTIP (priv->active_tooltip));
      priv->active_tooltip = NULL;
    }

  if (tooltip)
    {
      nbtk_tooltip_show (tooltip);
      priv->active_tooltip = NBTK_TOOLTIP (tooltip);
    }
}

/*
 * Returns TRUE if we are currently in the construct() vfunction.
 */
gboolean
mnb_switcher_is_constructing (MnbSwitcher *switcher)
{
  MnbSwitcherPrivate *priv = switcher->priv;

  return priv->constructing;
}

/*
 * Ends kbd grab, if one is currently in place.
 */
void
mnb_switcher_end_kbd_grab (MnbSwitcher *switcher)
{
  MnbSwitcherPrivate *priv = switcher->priv;

  if (priv->in_alt_grab)
    {
      MetaScreen  *screen  = mutter_plugin_get_screen (priv->plugin);
      MetaDisplay *display = meta_screen_get_display (screen);
      guint        timestamp;

      /*
       * Make sure our stamp is recent enough.
       */
      timestamp = meta_display_get_current_time_roundtrip (display);

      meta_display_end_grab_op (display, timestamp);
      priv->in_alt_grab = FALSE;
    }
}
