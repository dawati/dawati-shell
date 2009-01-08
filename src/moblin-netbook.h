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

#include <mutter-plugin.h>
#include <nbtk.h>

#define SN_API_NOT_YET_FROZEN 1
#include <libsn/sn.h>

#include <libintl.h>
#define _(x) dgettext (GETTEXT_PACKAGE, x)
#define N_(x) x

#include "moblin-netbook-tray-manager.h"
#include "moblin-netbook-notify-store.h"

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

typedef struct ActorPrivate  ActorPrivate;

/*
 * Plugin private data that we store in the .plugin_private member.
 */
struct _MoblinNetbookPluginPrivate
{
  ClutterEffectTemplate *destroy_effect;
  ClutterEffectTemplate *minimize_effect;
  ClutterEffectTemplate *maximize_effect;
  ClutterEffectTemplate *map_effect;
  ClutterEffectTemplate *switch_workspace_effect;
  ClutterEffectTemplate *switch_workspace_arrow_effect;
  ClutterEffectTemplate *panel_slide_effect;
  ClutterEffectTemplate *ws_switcher_slide_effect;

  /* Valid only when switch_workspace effect is in progress */
  ClutterTimeline       *tml_switch_workspace0;
  ClutterTimeline       *tml_switch_workspace1;
  GList                **actors;
  ClutterActor          *desktop1;
  ClutterActor          *desktop2;

  ClutterActor          *d_overlay ; /* arrow indicator */
  ClutterActor          *panel;
  ClutterActor          *panel_shadow;
  ClutterActor          *tray;

  ClutterActor          *switcher;
  ClutterActor          *workspace_switcher;
  ClutterActor          *workspace_chooser;
  ClutterActor          *launcher;
  ClutterActor          *lowlight;

  ShellTrayManager      *tray_manager;

  XserverRegion          input_region;
  XserverRegion          screen_region;

  gboolean               debug_mode                 : 1;
  gboolean               panel_out                  : 1;
  gboolean               panel_out_in_progress      : 1;
  gboolean               panel_back_in_progress     : 1;
  gboolean               desktop_switch_in_progress : 1;
  gboolean               keyboard_grab              : 1;

  guint                  workspace_chooser_timeout;

  ClutterActor          *panel_buttons[8];
  NbtkWidget            *panel_time;
  NbtkWidget            *panel_date;

  /* Startup Notification */
  SnDisplay             *sn_display;
  SnMonitorContext      *sn_context;
  GHashTable            *sn_hash;

  /* Application notification, ala libnotify */
  MoblinNetbookNotifyStore *notify_store;

  /* Background parallax texture */
  gint                   parallax_paint_offset;
  ClutterActor          *parallax_tex;

  MutterPluginInfo       info;
};

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
  ClutterTimeline *tml_destroy;
  ClutterTimeline *tml_map;

  gboolean      is_minimized   : 1;
  gboolean      is_maximized   : 1;
  gboolean      sn_in_progress : 1;
};

ActorPrivate * get_actor_private (MutterWindow *actor);
void           disable_stage     (MutterPlugin *plugin, guint32 timestamp);
void           enable_stage      (MutterPlugin *plugin, guint32 timestamp);

void moblin_netbook_notify_init (MutterPlugin *plugin);

#endif
