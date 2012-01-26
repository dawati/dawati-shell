/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */

/*
 * Copyright (c) 2008, 2010 Intel Corp.
 *
 * Author: Tomas Frydrych <tf@linux.intel.com>
 *         Thomas Wood <thomas@linux.intel.com>
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

#ifndef DAWATI_NETBOOK_H
#define DAWATI_NETBOOK_H

#include <gconf/gconf-client.h>
#include <meta/meta-plugin.h>
#include <mx/mx.h>

#define SN_API_NOT_YET_FROZEN 1
#include <libsn/sn.h>

#include "presence/gsm-manager.h"
#include "presence/gsm-presence.h"

#include "mnb-input-manager.h"

#define MNB_DBG_MARK() \
  g_debug (G_STRLOC ":%s", __FUNCTION__)        \

#define DAWATI_PANEL_SHORTCUT_KEY XK_Super_L

#define MAX_WORKSPACES 8

#define TOOLBAR_HEIGHT 50
#define TOOLBAR_X_PADDING 4

typedef enum
{
  MNB_OPTION_COMPOSITOR_ALWAYS_ON      = 1 << 0,
  MNB_OPTION_DISABLE_WS_CLAMP          = 1 << 1,
  MNB_OPTION_DISABLE_PANEL_RESTART     = 1 << 2,
  MNB_OPTION_COMPOSITE_FULLSCREEN_APPS = 1 << 3,
} MnbOptionFlag;

#define DAWATI_TYPE_NETBOOK_PLUGIN            (dawati_netbook_plugin_get_type ())
#define DAWATI_NETBOOK_PLUGIN(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), DAWATI_TYPE_NETBOOK_PLUGIN, DawatiNetbookPlugin))
#define DAWATI_NETBOOK_PLUGIN_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  DAWATI_TYPE_NETBOOK_PLUGIN, DawatiNetbookPluginClass))
#define MUTTER_IS_DEFAULT_PLUGIN(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), DAWATI_NETBOOK_PLUGIN_TYPE))
#define MUTTER_IS_DEFAULT_PLUGIN_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  DAWATI_TYPE_NETBOOK_PLUGIN))
#define DAWATI_NETBOOK_PLUGIN_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  DAWATI_TYPE_NETBOOK_PLUGIN, DawatiNetbookPluginClass))

#define DAWATI_NETBOOK_PLUGIN_GET_PRIVATE(obj) \
(G_TYPE_INSTANCE_GET_PRIVATE ((obj), DAWATI_TYPE_NETBOOK_PLUGIN, DawatiNetbookPluginPrivate))

typedef struct _DawatiNetbookPlugin        DawatiNetbookPlugin;
typedef struct _DawatiNetbookPluginClass   DawatiNetbookPluginClass;
typedef struct _DawatiNetbookPluginPrivate DawatiNetbookPluginPrivate;

struct _DawatiNetbookPlugin
{
  MetaPlugin parent;

  DawatiNetbookPluginPrivate *priv;
};

struct _DawatiNetbookPluginClass
{
  MetaPluginClass parent_class;
};


#define MN_PADDING(a, b, c, d) {CLUTTER_UNITS_FROM_INT (a), CLUTTER_UNITS_FROM_INT (b), CLUTTER_UNITS_FROM_INT (c), CLUTTER_UNITS_FROM_INT (d)}

typedef struct ActorPrivate  ActorPrivate;

/*
 * Plugin private data that we store in the .plugin_private member.
 */
struct _DawatiNetbookPluginPrivate
{
  ClutterActor          *statusbar;
  ClutterActor          *toolbar;
  ClutterActor          *switcher_overlay;
  MetaWindow            *last_focused;

  GList                 *fullscreen_wins;

  gboolean               holding_focus       : 1;
  gboolean               compositor_disabled : 1;
  gboolean               screen_saver_dpms   : 1;
  gboolean               scaled_background   : 1;

  /* Background desktop texture */
  ClutterActor          *desktop_tex;

  MetaPluginInfo         info;

  Window                 focus_xwin;

  /* Desktop background stuff */
  GConfClient           *gconf_client;
  GSettings             *settings;

  MetaWindowActor       *screen_saver_mcw;

  /* Presence manager */
  GsmManager            *manager;
  GsmPresence           *presence;

  int                    saver_base;
  int                    saver_error;

  /*  */
  gboolean               workspaces_ready : 1;
};

GType dawati_netbook_plugin_get_type (void);

/*
 * Per actor private data we attach to each actor.
 */
struct ActorPrivate
{
  ClutterActor *orig_parent;
  gint          orig_x;
  gint          orig_y;

  ClutterTimeline *tml_minimize;
  ClutterTimeline *tml_maximize;
  ClutterTimeline *tml_map;

  gboolean      is_minimized   : 1;
  gboolean      is_maximized   : 1;
  gboolean      sn_in_progress : 1;
};

ActorPrivate * get_actor_private (MetaWindowActor *actor);
void           dawati_netbook_focus_stage (MetaPlugin *plugin,
                                          guint32       timestamp);

void           dawati_netbook_unfocus_stage (MetaPlugin *plugin,
                                            guint32 timestamp);

void dawati_netbook_notify_init (MetaPlugin *plugin);

void
dawati_netbook_stash_window_focus (MetaPlugin *plugin, guint32 timestamp);

void
dawati_netbook_unstash_window_focus (MetaPlugin *plugin, guint32 timestamp);

void
dawati_netbook_setup_kbd_grabs (MetaPlugin *plugin);

gboolean
dawati_netbook_fullscreen_apps_present (MetaPlugin *plugin);

MetaPlugin *
dawati_netbook_get_plugin_singleton (void);

gboolean
dawati_netbook_modal_windows_present (MetaPlugin *plugin, gint workspace);

gboolean
dawati_netbook_compositor_disabled (MetaPlugin *plugin);

void
dawati_netbook_activate_window (MetaWindow *window);

ClutterActor *
dawati_netbook_get_toolbar (MetaPlugin *plugin);

gboolean
dawati_netbook_activate_mutter_window (MetaWindowActor *mcw);

guint32
dawati_netbook_get_compositor_option_flags (void);

gboolean
dawati_netbook_urgent_notification_present (MetaPlugin *plugin);

void
dawati_netbook_set_struts (MetaPlugin *plugin,
                          gint        left,
                          gint        right,
                          gint        top,
                          gint        bottom);

#endif
