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

/*
 * Alt_tab machinery.
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

#include <clutter/x11/clutter-x11.h>
#include <string.h>

#include "../mnb-toolbar.h"
#include "mnb-switcher.h"
#include "mnb-switcher-item.h"
#include "mnb-switcher-alttab.h"
#include "mnb-switcher-private.h"

/*
 * Activates the given MetaWindow, taking care of the quirks in Meta API.
 */
static void
activate_window (MnbSwitcher *switcher, MetaWindow *next, guint timestamp)
{
  MnbSwitcherPrivate *priv = switcher->priv;
  MetaWorkspace      *workspace;
  MetaWorkspace      *active_workspace;
  MetaScreen         *screen;

  g_assert (next);

  priv->in_alt_grab = FALSE;

  screen           = meta_window_get_screen (next);
  workspace        = meta_window_get_workspace (next);
  active_workspace = meta_screen_get_active_workspace (screen);

  mnb_drop_down_hide_with_toolbar (MNB_DROP_DOWN (switcher));

  if (!active_workspace || (active_workspace == workspace))
    {
      meta_window_activate_with_workspace (next, timestamp, workspace);
    }
  else
    {
      meta_workspace_activate_with_focus (workspace, next, timestamp);
    }
}

/*
 * Establishes an active kbd grab for us via the Meta API.
 */
static gboolean
establish_keyboard_grab (MnbSwitcher  *switcher,
                         MetaDisplay  *display,
                         MetaScreen   *screen,
                         gulong        mask,
                         guint         timestamp)
{
  MnbSwitcherPrivate *priv = switcher->priv;

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
      priv->in_alt_grab = TRUE;

      return TRUE;
    }

  g_warning ("Failed to grab keyboard");

  return FALSE;
}

/*
 * Finds the next window that we should move to when not showing the switcher
 */
static MetaWindow *
find_next_window (MnbSwitcher *switcher, gboolean backward)
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

  /*
   * When we cannot get a window from the switcher (i.e., the switcher is not
   * visible), get the next suitable window from the global tab list.
   */
  if (priv->global_tab_list)
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
  if (current && (!next || (next == current)))
    {
      MetaWorkspace *ws = meta_window_get_workspace (current);

      next = meta_display_get_tab_next (display,
                                        META_TAB_LIST_NORMAL,
                                        screen,
                                        ws,
                                        current,
                                        backward);
    }

  if (next == current)
    return NULL;

  return next;
}

/*
 * Activates the next window in the Alt+Tab list
 */
static void
activate_next_window (MnbSwitcher *switcher, guint timestamp, gboolean backward)
{
  MetaWindow *next = find_next_window (switcher, backward);

  if (next)
    {
      activate_window (switcher, next, timestamp);
    }
  else
    g_warning ("Could not locate next window");
}

/*
 * Data we need to pass through via our timeout and show callbacks.
 */
struct alt_tab_show_complete_data
{
  MnbSwitcher    *switcher;
  MetaDisplay    *display;
  MetaScreen     *screen;
  MetaWindow     *window;
  MetaKeyBinding *binding;
  XEvent          xevent;
};

/*
 * Callback for when the switcher is shown due to Alt+Tab press.
 */
static void
alt_tab_switcher_show_completed_cb (ClutterActor *switcher, gpointer data)
{
  struct alt_tab_show_complete_data *alt_data = data;
  gboolean backward = FALSE;

  if (alt_data->xevent.xkey.state & ShiftMask)
    backward = !backward;

  mnb_switcher_advance (MNB_SWITCHER (switcher), backward);

  /*
   * This is a one-off, disconnect ourselves.
   */
  g_signal_handlers_disconnect_by_func (switcher,
                                        alt_tab_switcher_show_completed_cb,
                                        data);

  g_free (data);
}

/*
 * Callback for the timeout we use to test whether to show switcher or directly
 * move to next window.
 */
static gboolean
alt_tab_timeout_cb (gpointer data)
{
  struct alt_tab_show_complete_data *alt_data = data;
  ClutterActor                      *stage;
  Window                             xwin;
  MnbSwitcherPrivate                *priv = alt_data->switcher->priv;

  stage = mutter_get_stage_for_screen (alt_data->screen);
  xwin  = clutter_x11_get_stage_window (CLUTTER_STAGE (stage));

  priv->waiting_for_timeout = FALSE;

  /*
   * Check wether the Alt key is still down; if so, show the Switcher, and
   * wait for the show-completed signal to process the Alt+Tab.
   */
  if (priv->alt_tab_down)
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

      activate_next_window (alt_data->switcher,
                            alt_data->xevent.xkey.time, backward);

      mnb_switcher_end_kbd_grab (alt_data->switcher);
      g_free (data);
    }

  /* One off */
  return FALSE;
}

/*
 * The handler for Alt+Tab that we register with metacity.
 *
 * NB: this is the only function we expose from this file for the switcher
 *     object.
 */
void
mnb_switcher_alt_tab_key_handler (MetaDisplay    *display,
                                  MetaScreen     *screen,
                                  MetaWindow     *window,
                                  XEvent         *event,
                                  MetaKeyBinding *binding,
                                  gpointer        data)
{
  MnbSwitcher        *switcher = MNB_SWITCHER (data);
  MnbSwitcherPrivate *priv     = switcher->priv;

  if (!priv->in_alt_grab)
    establish_keyboard_grab (switcher, display, screen,
                             binding->mask, event->xkey.time);

  priv->alt_tab_down = TRUE;

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
      priv->waiting_for_timeout = TRUE;
      return;
    }
  else
    {
      gboolean backward = FALSE;

      priv->waiting_for_timeout = FALSE;

      if (event->xkey.state & ShiftMask)
        backward = !backward;

      mnb_switcher_advance (switcher, backward);
    }
}

void
mnb_switcher_alt_tab_select_handler (MetaDisplay    *display,
                                     MetaScreen     *screen,
                                     MetaWindow     *window,
                                     XEvent         *event,
                                     MetaKeyBinding *binding,
                                     gpointer        data)
{
  MnbSwitcher        *switcher = MNB_SWITCHER (data);
  MnbSwitcherPrivate *priv     = switcher->priv;

  mnb_switcher_end_kbd_grab (switcher);

  priv->alt_tab_down = FALSE;

  if (!switcher->priv->waiting_for_timeout)
    mnb_switcher_activate_selection (switcher, TRUE, event->xkey.time);
}

void
mnb_switcher_alt_tab_cancel_handler (MetaDisplay    *display,
                                     MetaScreen     *screen,
                                     MetaWindow     *window,
                                     XEvent         *event,
                                     MetaKeyBinding *binding,
                                     gpointer        data)
{
  MnbSwitcher        *switcher = MNB_SWITCHER (data);
  MnbSwitcherPrivate *priv     = switcher->priv;

  mnb_switcher_end_kbd_grab (switcher);

  priv->alt_tab_down = FALSE;
}

