/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */

/*
 * Copyright (c) 2008 Intel Corp.
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

#ifndef MOBLIN_NETBOOK_H
#define MOBLIN_NETBOOK_H

#include <gconf/gconf-client.h>
#include <mutter-plugin.h>
#include <nbtk/nbtk.h>

#define SN_API_NOT_YET_FROZEN 1
#include <libsn/sn.h>

#include "notifications/moblin-netbook-notify-store.h"
#include "notifications/mnb-notification-cluster.h"
#include "notifications/mnb-notification-urgent.h"

#include "mnb-input-manager.h"

#define MOBLIN_PANEL_SHORTCUT_KEY XK_Super_L

#define MAX_WORKSPACES 8

#define TOOLBAR_HEIGHT 64
#define TOOLBAR_X_PADDING 4

#define MOBLIN_TYPE_NETBOOK_PLUGIN            (moblin_netbook_plugin_get_type ())
#define MOBLIN_NETBOOK_PLUGIN(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), MOBLIN_TYPE_NETBOOK_PLUGIN, MoblinNetbookPlugin))
#define MOBLIN_NETBOOK_PLUGIN_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  MOBLIN_TYPE_NETBOOK_PLUGIN, MoblinNetbookPluginClass))
#define MUTTER_IS_DEFAULT_PLUGIN(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MOBLIN_NETBOOK_PLUGIN_TYPE))
#define MUTTER_IS_DEFAULT_PLUGIN_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  MOBLIN_TYPE_NETBOOK_PLUGIN))
#define MOBLIN_NETBOOK_PLUGIN_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  MOBLIN_TYPE_NETBOOK_PLUGIN, MoblinNetbookPluginClass))

#define MOBLIN_NETBOOK_PLUGIN_GET_PRIVATE(obj) \
(G_TYPE_INSTANCE_GET_PRIVATE ((obj), MOBLIN_TYPE_NETBOOK_PLUGIN, MoblinNetbookPluginPrivate))

typedef struct _MoblinNetbookPlugin        MoblinNetbookPlugin;
typedef struct _MoblinNetbookPluginClass   MoblinNetbookPluginClass;
typedef struct _MoblinNetbookPluginPrivate MoblinNetbookPluginPrivate;

struct _MoblinNetbookPlugin
{
  MutterPlugin parent;

  MoblinNetbookPluginPrivate *priv;
};

struct _MoblinNetbookPluginClass
{
  MutterPluginClass parent_class;
};


#define MN_PADDING(a, b, c, d) {CLUTTER_UNITS_FROM_INT (a), CLUTTER_UNITS_FROM_INT (b), CLUTTER_UNITS_FROM_INT (c), CLUTTER_UNITS_FROM_INT (d)}

typedef struct ActorPrivate  ActorPrivate;

/*
 * Plugin private data that we store in the .plugin_private member.
 */
struct _MoblinNetbookPluginPrivate
{
  ClutterActor          *toolbar;
  ClutterActor          *lowlight;

  MetaWindow            *last_focused;

  GList                 *fullscreen_wins;

  gboolean               holding_focus       : 1;
  gboolean               compositor_disabled : 1;

  /* Background desktop texture */
  ClutterActor          *desktop_tex;

  MutterPluginInfo       info;

  /* Notification 'widget' */
  ClutterActor          *notification_cluster;
  ClutterActor          *notification_urgent;
  MnbInputRegion        *notification_cluster_input_region;
  MnbInputRegion        *notification_urgent_input_region;

  Window                 focus_xwin;

  /* Desktop background stuff */
  GConfClient           *gconf_client;
};

GType moblin_netbook_plugin_get_type (void);

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

ActorPrivate * get_actor_private (MutterWindow *actor);
void           moblin_netbook_focus_stage (MutterPlugin *plugin,
                                           guint32       timestamp);

void           moblin_netbook_unfocus_stage (MutterPlugin *plugin,
                                             guint32 timestamp);

void moblin_netbook_notify_init (MutterPlugin *plugin);

void
moblin_netbook_set_lowlight (MutterPlugin *plugin, gboolean on);

void
moblin_netbook_stash_window_focus (MutterPlugin *plugin, guint32 timestamp);

void
moblin_netbook_unstash_window_focus (MutterPlugin *plugin, guint32 timestamp);

void
moblin_netbook_setup_kbd_grabs (MutterPlugin *plugin);

gboolean
moblin_netbook_fullscreen_apps_present (MutterPlugin *plugin);

MutterPlugin *
moblin_netbook_get_plugin_singleton (void);

gboolean
moblin_netbook_modal_windows_present (MutterPlugin *plugin, gint workspace);

gboolean
moblin_netbook_compositor_disabled (MutterPlugin *plugin);

void
moblin_netbook_activate_window (MetaWindow *window);

#endif
