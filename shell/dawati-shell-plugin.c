/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */

/*
 * Copyright (c) 2008       Red Hat, Inc.
 * Copyright (c) 2008, 2012 Intel Corp.
 *
 * Based on plugin skeleton by:
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

#include "config.h"

#include <stdlib.h>
#include <string.h>

#include <clutter/clutter.h>
#include <clutter/x11/clutter-x11.h>
/* #include <GL/glx.h> */
/* #include <GL/glxext.h> */
#include <gjs/gjs.h>
#include <meta/display.h>
#include <meta/meta-plugin.h>

#include "shell-global-private.h"
#include "shell-perf-log.h"
#include "shell-wm-private.h"

static void dawati_shell_plugin_dispose     (GObject *object);
static void dawati_shell_plugin_finalize    (GObject *object);

static void dawati_shell_plugin_start            (MetaPlugin          *plugin);
static void dawati_shell_plugin_minimize         (MetaPlugin          *plugin,
                                                 MetaWindowActor     *actor);
static void dawati_shell_plugin_maximize         (MetaPlugin          *plugin,
                                                 MetaWindowActor     *actor,
                                                 gint                 x,
                                                 gint                 y,
                                                 gint                 width,
                                                 gint                 height);
static void dawati_shell_plugin_unmaximize       (MetaPlugin          *plugin,
                                                 MetaWindowActor     *actor,
                                                 gint                 x,
                                                 gint                 y,
                                                 gint                 width,
                                                 gint                 height);
static void dawati_shell_plugin_map              (MetaPlugin          *plugin,
                                                 MetaWindowActor     *actor);
static void dawati_shell_plugin_destroy          (MetaPlugin          *plugin,
                                                 MetaWindowActor     *actor);

static void dawati_shell_plugin_switch_workspace (MetaPlugin          *plugin,
                                                 gint                 from,
                                                 gint                 to,
                                                 MetaMotionDirection  direction);

static void dawati_shell_plugin_kill_window_effects   (MetaPlugin      *plugin,
                                                      MetaWindowActor *actor);
static void dawati_shell_plugin_kill_switch_workspace (MetaPlugin      *plugin);


static gboolean              dawati_shell_plugin_xevent_filter (MetaPlugin *plugin,
                                                               XEvent     *event);
static const MetaPluginInfo *dawati_shell_plugin_plugin_info   (MetaPlugin *plugin);


#define DAWATI_TYPE_SHELL_PLUGIN            (dawati_shell_plugin_get_type ())
#define DAWATI_SHELL_PLUGIN(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), DAWATI_TYPE_SHELL_PLUGIN, DawatiShellPlugin))
#define DAWATI_SHELL_PLUGIN_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  DAWATI_TYPE_SHELL_PLUGIN, DawatiShellPluginClass))
#define DAWATI_IS_SHELL_PLUGIN(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), DAWATI_SHELL_PLUGIN_TYPE))
#define DAWATI_IS_SHELL_PLUGIN_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  DAWATI_TYPE_SHELL_PLUGIN))
#define DAWATI_SHELL_PLUGIN_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  DAWATI_TYPE_SHELL_PLUGIN, DawatiShellPluginClass))

typedef struct _DawatiShellPlugin        DawatiShellPlugin;
typedef struct _DawatiShellPluginClass   DawatiShellPluginClass;

struct _DawatiShellPlugin
{
  MetaPlugin parent;

  Atom panel_action;
  Atom panel_action_run_dialog;
  Atom panel_action_main_menu;

  int glx_error_base;
  int glx_event_base;
  guint have_swap_event : 1;

  ShellGlobal *global;
};

struct _DawatiShellPluginClass
{
  MetaPluginClass parent_class;
};

GType dawati_shell_plugin_get_type (void);

G_DEFINE_TYPE (DawatiShellPlugin, dawati_shell_plugin, META_TYPE_PLUGIN)

static void
dawati_shell_plugin_class_init (DawatiShellPluginClass *klass)
{
  GObjectClass      *gobject_class = G_OBJECT_CLASS (klass);
  MetaPluginClass *plugin_class  = META_PLUGIN_CLASS (klass);

  gobject_class->dispose         = dawati_shell_plugin_dispose;
  gobject_class->finalize        = dawati_shell_plugin_finalize;

  plugin_class->start            = dawati_shell_plugin_start;
  plugin_class->map              = dawati_shell_plugin_map;
  plugin_class->minimize         = dawati_shell_plugin_minimize;
  plugin_class->maximize         = dawati_shell_plugin_maximize;
  plugin_class->unmaximize       = dawati_shell_plugin_unmaximize;
  plugin_class->destroy          = dawati_shell_plugin_destroy;

  plugin_class->switch_workspace = dawati_shell_plugin_switch_workspace;

  plugin_class->kill_window_effects   = dawati_shell_plugin_kill_window_effects;
  plugin_class->kill_switch_workspace = dawati_shell_plugin_kill_switch_workspace;

  plugin_class->xevent_filter    = dawati_shell_plugin_xevent_filter;
  plugin_class->plugin_info      = dawati_shell_plugin_plugin_info;
}

static void
dawati_shell_plugin_init (DawatiShellPlugin *shell_plugin)
{
}

static void
dawati_shell_plugin_start (MetaPlugin *plugin)
{
  DawatiShellPlugin *shell_plugin = DAWATI_SHELL_PLUGIN (plugin);
  /* MetaScreen *screen; */
  /* MetaDisplay *display; */
  /* Display *xdisplay; */
  GError *error = NULL;
  int status;
  /* const char *glx_extensions; */
  GjsContext *gjs_context;

  /* screen = meta_plugin_get_screen (plugin); */
  /* display = meta_screen_get_display (screen); */

  /* xdisplay = meta_display_get_xdisplay (display); */

  /* glXQueryExtension (xdisplay, */
  /*                    &shell_plugin->glx_error_base, */
  /*                    &shell_plugin->glx_event_base); */

  /* glx_extensions = glXQueryExtensionsString (xdisplay, */
  /*                                            meta_screen_get_screen_number (screen)); */
  /* shell_plugin->have_swap_event = strstr (glx_extensions, "GLX_INTEL_swap_event") != NULL; */

  shell_perf_log_define_event (shell_perf_log_get_default (),
                               "glx.swapComplete",
                               "GL buffer swap complete event received (with timestamp of completion)",
                               "x");

  shell_plugin->global = shell_global_get ();
  _shell_global_set_plugin (shell_plugin->global, META_PLUGIN (shell_plugin));

  gjs_context = _shell_global_get_gjs_context (shell_plugin->global);

  if (!gjs_context_eval (gjs_context,
                         "imports.ui.environment.init();"
                         "imports.ui.main.start();",
                         -1,
                         "<main>",
                         &status,
                         &error))
    {
      g_message ("Execution of main.js threw exception: %s", error->message);
      g_error_free (error);
      /* We just exit() here, since in a development environment you'll get the
       * error in your shell output, and it's way better than a busted WM,
       * which typically manifests as a white screen.
       *
       * In production, we shouldn't crash =)  But if we do, we should get
       * restarted by the session infrastructure, which is likely going
       * to be better than some undefined state.
       *
       * If there was a generic "hook into bug-buddy for non-C crashes"
       * infrastructure, here would be the place to put it.
       */
      exit (1);
    }
}

static void
dawati_shell_plugin_dispose (GObject *object)
{
  G_OBJECT_CLASS(dawati_shell_plugin_parent_class)->dispose (object);
}

static void
dawati_shell_plugin_finalize (GObject *object)
{
  G_OBJECT_CLASS(dawati_shell_plugin_parent_class)->finalize (object);
}

static ShellWM *
get_shell_wm (void)
{
  ShellWM *wm;

  g_object_get (shell_global_get (),
                "window-manager", &wm,
                NULL);
  /* drop extra ref added by g_object_get */
  g_object_unref (wm);

  return wm;
}

static void
dawati_shell_plugin_minimize (MetaPlugin         *plugin,
			     MetaWindowActor    *actor)
{
  _shell_wm_minimize (get_shell_wm (),
                      actor);

}

static void
dawati_shell_plugin_maximize (MetaPlugin         *plugin,
                             MetaWindowActor    *actor,
                             gint                x,
                             gint                y,
                             gint                width,
                             gint                height)
{
  _shell_wm_maximize (get_shell_wm (),
                      actor, x, y, width, height);
}

static void
dawati_shell_plugin_unmaximize (MetaPlugin         *plugin,
                               MetaWindowActor    *actor,
                               gint                x,
                               gint                y,
                               gint                width,
                               gint                height)
{
  _shell_wm_unmaximize (get_shell_wm (),
                        actor, x, y, width, height);
}

static void
dawati_shell_plugin_map (MetaPlugin         *plugin,
                        MetaWindowActor    *actor)
{
  _shell_wm_map (get_shell_wm (),
                 actor);
}

static void
dawati_shell_plugin_destroy (MetaPlugin         *plugin,
                            MetaWindowActor    *actor)
{
  _shell_wm_destroy (get_shell_wm (),
                     actor);
}

static void
dawati_shell_plugin_switch_workspace (MetaPlugin         *plugin,
                                     gint                from,
                                     gint                to,
                                     MetaMotionDirection direction)
{
  _shell_wm_switch_workspace (get_shell_wm(), from, to, direction);
}

static void
dawati_shell_plugin_kill_window_effects (MetaPlugin         *plugin,
                                        MetaWindowActor    *actor)
{
  _shell_wm_kill_window_effects (get_shell_wm(), actor);
}

static void
dawati_shell_plugin_kill_switch_workspace (MetaPlugin         *plugin)
{
  _shell_wm_kill_switch_workspace (get_shell_wm());
}

static gboolean
dawati_shell_plugin_xevent_filter (MetaPlugin *plugin,
                                  XEvent     *xev)
{
  MetaScreen *screen = meta_plugin_get_screen (plugin);
  ClutterStage *stage = CLUTTER_STAGE (meta_get_stage_for_screen (screen));

  DawatiShellPlugin *shell_plugin = DAWATI_SHELL_PLUGIN (plugin);
#ifdef GLX_INTEL_swap_event
  if (shell_plugin->have_swap_event &&
      xev->type == (shell_plugin->glx_event_base + GLX_BufferSwapComplete))
    {
      GLXBufferSwapComplete *swap_complete_event;
      swap_complete_event = (GLXBufferSwapComplete *)xev;

      /* Buggy early versions of the INTEL_swap_event implementation in Mesa
       * can send this with a ust of 0. Simplify life for consumers
       * by ignoring such events */
      if (swap_complete_event->ust != 0)
        shell_perf_log_event_x (shell_perf_log_get_default (),
                                "glx.swapComplete",
                                swap_complete_event->ust);
    }
#endif

  if ((xev->xany.type == EnterNotify || xev->xany.type == LeaveNotify)
      && xev->xcrossing.window == clutter_x11_get_stage_window (stage))
    {
      /* If the pointer enters a child of the stage window (eg, a
       * trayicon), we want to consider it to still be in the stage,
       * so don't let Clutter see the event.
       */
      if (xev->xcrossing.detail == NotifyInferior)
        return TRUE;

      /* If the pointer is grabbed by a window it is not currently in,
       * filter that out as well. In particular, if a trayicon grabs
       * the pointer after a click on its label, we don't want to hide
       * the message tray. Filtering out this event will leave Clutter
       * out of sync, but that happens fairly often with grabs, and we
       * can work around it. (Eg, shell_global_sync_pointer().)
       */
      if (xev->xcrossing.mode == NotifyGrab &&
          (xev->xcrossing.detail == NotifyNonlinear ||
           xev->xcrossing.detail == NotifyNonlinearVirtual))
        return TRUE;
    }

  /*
   * Pass the event to shell-global
   */
  if (_shell_global_check_xdnd_event (shell_plugin->global, xev))
    return TRUE;

  return clutter_x11_handle_event (xev) != CLUTTER_X11_FILTER_CONTINUE;
}

static const
MetaPluginInfo *dawati_shell_plugin_plugin_info (MetaPlugin *plugin)
{
  static const MetaPluginInfo info = {
    .name = "DAWATI Shell",
    .version = "0.1",
    .author = "Various",
    .license = "GPLv2+",
    .description = "Provides DAWATI Shell core functionality"
  };

  return &info;
}

#if HAVE_BLUETOOTH
/* HACK:
   Add a non-static function that calls into libgnome-bluetooth-applet.so,
   to avoid the linker being too smart and removing the dependency.
   This function is never actually called.
*/
extern GType bluetooth_applet_get_type(void);
void _shell_link_to_bluetooth(void);

void _shell_link_to_bluetooth(void) {
  bluetooth_applet_get_type();
}
#endif
