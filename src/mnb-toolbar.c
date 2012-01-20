/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */

/* mnb-toolbar.c */
/*
 * Copyright (c) 2009, 2010 Intel Corp.
 *
 * Authors: Matthew Allum <matthew.allum@intel.com>
 *          Tomas Frydrych <tf@linux.intel.com>
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-bindings.h>
#include <dbus/dbus-glib-lowlevel.h>
#include <dbus/dbus.h>
#include <gconf/gconf-client.h>
#include <dawati-panel/mpl-panel-common.h>
#include <meta/display.h>
#include <meta/keybindings.h>
#include <meta/errors.h>

/*
 * Including mutter's errors.h defines the i18n macros, so undefine them before
 * including the glib i18n header.
 */
#undef _
#undef N_
#include <glib/gi18n.h>


#include "dawati-netbook.h"

#include "mnb-toolbar.h"
#include "mnb-toolbar-applet.h"
#include "mnb-toolbar-background.h"
#include "mnb-toolbar-button.h"
#include "mnb-toolbar-clock.h"
#include "mnb-toolbar-icon.h"
#include "mnb-toolbar-shadow.h"
#include "mnb-panel-oop.h"
#include "mnb-spinner.h"
#include "mnb-statusbar.h"

/* For systray windows stuff */
#include <gdk/gdkx.h>

#include <clutter/x11/clutter-x11.h>

#define TOOLBAR_DROPDOWN_DURATION 150
#define SLIDE_DURATION 150

#define KEY_DIR "/desktop/dawati/toolbar/panels"
#define KEY_ORDER KEY_DIR "/order"

#define BUTTON_SPACING 6
#define BUTTON_SPACING_INITIAL (0+TOOLBAR_X_PADDING)
#define BUTTON_Y 0
/*
 * According to the latest MeeGo 1.0 designs, on the bigger screen the Toolbar
 * does not have any shadow on its sides, nor the rounded corners. This is done
 * by making the Toolbar wider than the screen by 2 * BIG_SCREEN_PAD and moving
 * it to the left by BIG_SCREEN_PAD. The BIG_SCREEN_BUTTON_SHIFT is the amount
 * by which the panel buttons have to be moved left so that the myzone button
 * fits into the middle of the space which is embossed into the Toolbar asset.
 */
#define BIG_SCREEN_PAD 10
#define BIG_SCREEN_BUTTON_SHIFT 0

#define MNB_TOOLBAR_MAX_APPLETS 4
#define TRAY_WIDTH (3 * TRAY_BUTTON_WIDTH + CLOCK_WIDTH + 4 * TRAY_PADDING - TRAY_BUTTON_INTERNAL_PADDING / 2)
#define TRAY_PADDING   6

#define TOOLBAR_SELECTOR_ANIMATION_DELAY (300)

#define TOOLBAR_TRIGGER_THRESHOLD       1
#define TOOLBAR_TRIGGER_ADJUSTMENT      2
#define TOOLBAR_HIDE_TIMEOUT           (1500)
#define TOOLBAR_LOWLIGHT_FADE_DURATION 300
#define TOOLBAR_AUTOSTART_DELAY 15
#define TOOLBAR_AUTOSTART_ATTEMPTS 10
#define TOOLBAR_WAITING_FOR_PANEL_TIMEOUT 1 /* in seconds */
#define TOOLBAR_PANEL_STUB_TIMEOUT 6        /* in seconds */
#define DAWATI_BOOT_COUNT_KEY "/desktop/dawati/myzone/boot_count"

#define CLOSE_BUTTON_GUARD_WIDTH 35

#define MAX_PANELS(screen_width)                \
  ((screen_width - TRAY_WIDTH - TOOLBAR_X_PADDING - BUTTON_SPACING_INITIAL) / \
   (BUTTON_WIDTH + BUTTON_SPACING))

#if 0
/*
 * TODO
 * This is currently define in dawati-netbook.h, as it is needed by the
 * tray manager and MnbDropDown -- this should not be hardcoded, and we need
 * a way for the drop down to query it from the panel.
 */
#define TOOLBAR_HEIGHT 50
#endif

/*
 * The toolbar shadow extends by the TOOLBAR_SHADOW_EXTRA below the toolbar.
 * In addition, the lowlight actor is extended by the TOOLBAR_SHADOW_HEIGHT so
 * that it does not roll above the edge of the screen during the toolbar hide
 * animation.
 */
#define TOOLBAR_SHADOW_EXTRA  47
#define TOOLBAR_SHADOW_HEIGHT (TOOLBAR_HEIGHT + TOOLBAR_SHADOW_EXTRA)

#define TOOLBAR_CUT_OUT_SHIFT (10.0)

G_DEFINE_TYPE (MnbToolbar, mnb_toolbar, CLUTTER_TYPE_GROUP)

#define MNB_TOOLBAR_GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MNB_TYPE_TOOLBAR, MnbToolbarPrivate))

typedef struct _MnbToolbarPanel MnbToolbarPanel;

static void mnb_toolbar_constructed (GObject *self);
static void mnb_toolbar_real_hide (ClutterActor *actor);
static void mnb_toolbar_real_show (ClutterActor *actor);
static gboolean mnb_toolbar_event_cb (MnbToolbar *toolbar,
                                      ClutterEvent *event,
                                      gpointer      data);
static gboolean mnb_toolbar_stage_input_cb (ClutterActor *stage,
                                            ClutterEvent *event,
                                            gpointer      data);
static void mnb_toolbar_stage_show_cb (ClutterActor *stage,
                                       MnbToolbar *toolbar);
static void mnb_toolbar_handle_dbus_name (MnbToolbar *, const gchar *);
static MnbPanel * mnb_toolbar_panel_name_to_panel (MnbToolbar  *toolbar,
                                                   const gchar *name);
static MnbToolbarPanel * mnb_toolbar_panel_name_to_panel_internal (MnbToolbar  *toolbar,
                                                                   const gchar *name);
static MnbToolbarPanel * mnb_toolbar_panel_to_toolbar_panel (MnbToolbar *toolbar,
                                                             MnbPanel *panel);
static void mnb_toolbar_activate_panel_internal (MnbToolbar *toolbar,
                                                 MnbToolbarPanel *tp,
                                                 MnbShowHideReason reason);
static void mnb_toolbar_setup_gconf (MnbToolbar *toolbar);
static gboolean mnb_toolbar_start_panel_service (MnbToolbar *toolbar,
                                                 MnbToolbarPanel *tp);
static void mnb_toolbar_workarea_changed_cb (MetaScreen *screen,
                                             MnbToolbar *toolbar);


enum {
  PROP_0,

  PROP_MUTTER_PLUGIN,
};

enum
{
  SHOW_COMPLETED,
  HIDE_BEGIN,
  HIDE_COMPLETED,

  LAST_SIGNAL
};

static guint toolbar_signals[LAST_SIGNAL] = { 0 };

typedef enum
{
  MNB_TOOLBAR_PANEL_NORMAL = 0,
  MNB_TOOLBAR_PANEL_APPLET,
  MNB_TOOLBAR_PANEL_CLOCK
} MnbToolbarPanelType;

struct _MnbToolbarPanel
{
  gchar      *name;
  gchar      *service;
  gchar      *button_stylesheet;
  gchar      *button_style;
  gchar      *tooltip;

  ClutterActor *button;
  MnbPanel   *panel;

  MnbToolbarPanelType type;

  gboolean    windowless : 1; /* button-only panel */
  gboolean    unloaded   : 1;
  gboolean    current    : 1;
  gboolean    pinged     : 1;
  gboolean    required   : 1;
  gboolean    failed     : 1;
};

static void
mnb_toolbar_panel_destroy (MnbToolbarPanel *tp)
{
  g_free (tp->name);
  g_free (tp->service);
  g_free (tp->button_stylesheet);
  g_free (tp->button_style);
  g_free (tp->tooltip);

  if (tp->button)
    g_critical (G_STRLOC ": button leaked");

  if (tp->panel)
    g_critical (G_STRLOC ": panel leaked");
}

struct _MnbToolbarPrivate
{
  MetaPlugin *plugin;

  ClutterActor *hbox_main;    /* Contains the following 2 h-boxes */
  ClutterActor *hbox_buttons; /* This is where all the contents are placed */
  ClutterActor *hbox_applets; /* This is where all the contents are placed */

  CoglHandle selector_texture;
  ClutterActor *light_spot;
  ClutterActorBox next_selector_pos;

  ClutterActor *lowlight;
  ClutterActor *panel_stub;
  ClutterActor *spinner;
  ClutterActor *shadow;

  /*
   * TODO -- The proper clock is a toolbar button associated with a panel,
   * like any other button, and can be removed/added via the Toolbar capplet.
   * As such, we rely on a desktop file, and expect an associated dbus service,
   * etc. So, while we are waiting for the Date & Time panel to be ready for
   * release, we use a dummy clock object in its place; once the panel is ready
   * we should remove this.
   */
  ClutterActor *dummy_clock;

  GList        *panels;         /* Panels (the dropdowns) */

  guint         max_panels;
  guint         n_applets;

  MnbToolbarPanel *stubbed_panel; /* The panel for which we are showing stub */

  gboolean button_click      : 1; /* Used to differentiate programmatic and
                                   * manual button toggling.
                                   */
  gboolean no_autoloading    : 1;
  gboolean shown             : 1;
  gboolean shown_myzone      : 1;
  gboolean disabled          : 1;
  gboolean in_show_animation : 1; /* Animation tracking */
  gboolean in_hide_animation : 1;
  gboolean is_visible        : 1;
  gboolean waiting_for_panel_show : 1; /* Set between button click and panel
                                        * show */
  gboolean waiting_for_panel_hide : 1;

  gboolean dont_autohide     : 1; /* Whether the panel should hide when the
                                   * pointer goes south
                                   */
  gboolean panel_input_only  : 1; /* Set when the region below panels should not
                                   * be included in the panel input region.
                                   */
  gboolean struts_set        : 1;
  gboolean have_clock        : 1;

  MnbShowHideReason reason_for_show; /* Reason for pending Toolbar show */
  MnbShowHideReason reason_for_hide; /* Reason for pending Toolbar hide */

  MnbInputRegion *trigger_region;  /* The show panel trigger region */
  MnbInputRegion *input_region;    /* The panel input region on the region
                                   * stack.
                                   */

  guint hide_timeout_id;

  DBusGConnection *dbus_conn;
  DBusGProxy      *dbus_proxy;

  GSList          *pending_panels;
  MnbToolbarPanel *tp_to_activate;

  gint             old_screen_width;
  gint             old_screen_height;

  guint            waiting_for_panel_show_cb_id;
  guint            waiting_for_panel_hide_cb_id;
  guint            panel_stub_timeout_id;
  guint            trigger_cb_id;
};

static GSList *
mnb_toolbar_get_default_panel_list (void)
{
  const gchar *panels_required[] = {"dawati-panel-myzone",
                                    "dawati-panel-zones",
                                    "dawati-panel-applications",
                                    "dawati-panel-status",
                                    "dawati-panel-people",
                                    "dawati-panel-internet",
                                    "dawati-panel-music",
                                    "dawati-panel-devices",
                                    "dawati-panel-datetime",
                                    "dawati-panel-bluetooth",
                                    "carrick",
                                    "dawati-power-icon" };
  gint i;
  GSList *order = NULL;

  for (i = 0; i < G_N_ELEMENTS (panels_required); i++)
    order = g_slist_append (order, (gchar *) panels_required[i]);

  return order;
}



static void
mnb_toolbar_get_property (GObject    *object,
                          guint       property_id,
                          GValue     *value,
                          GParamSpec *pspec)
{
  MnbToolbar *self = MNB_TOOLBAR (object);

  switch (property_id)
    {
    case PROP_MUTTER_PLUGIN:
      g_value_set_object (value, self->priv->plugin);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
mnb_toolbar_set_property (GObject *object, guint property_id,
                          const GValue *value, GParamSpec *pspec)
{
  MnbToolbar *self = MNB_TOOLBAR (object);

  switch (property_id)
    {
    case PROP_MUTTER_PLUGIN:
      self->priv->plugin = g_value_get_object (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
mnb_toolbar_dispose (GObject *object)
{
  MnbToolbarPrivate *priv = MNB_TOOLBAR (object)->priv;

  if (priv->light_spot)
    {
      clutter_actor_destroy (priv->light_spot);
      priv->light_spot = NULL;
    }

  if (priv->dbus_conn)
    {
      g_object_unref (priv->dbus_conn);
      priv->dbus_conn = NULL;
    }

  if (priv->input_region)
    {
      mnb_input_manager_remove_region (priv->input_region);
      priv->input_region = NULL;
    }

  if (priv->trigger_region)
    {
      mnb_input_manager_remove_region (priv->trigger_region);
      priv->trigger_region = NULL;
    }

  G_OBJECT_CLASS (mnb_toolbar_parent_class)->dispose (object);
}

static void
mnb_toolbar_finalize (GObject *object)
{
  MnbToolbarPrivate *priv = MNB_TOOLBAR (object)->priv;
  GSList            *l;

  l = priv->pending_panels;
  while (l)
    {
      gchar *n = l->data;
      g_free (n);

      l = l->next;
    }
  g_slist_free (priv->pending_panels);
  priv->pending_panels = NULL;

  G_OBJECT_CLASS (mnb_toolbar_parent_class)->finalize (object);
}

/*
 * show/hide machinery
 */
static void
mnb_toolbar_show_completed_cb (ClutterAnimation *animation, ClutterActor *actor)
{
  MnbToolbarPrivate *priv = MNB_TOOLBAR (actor)->priv;
  GList             *l = priv->panels;

  if (!priv->in_hide_animation && CLUTTER_ACTOR_IS_VISIBLE (actor))
    {
      for (; l; l = l->next)
        {
          MnbToolbarPanel *panel = l->data;

          if (panel && panel->button)
            clutter_actor_set_reactive (CLUTTER_ACTOR (panel->button), TRUE);
        }

      clutter_actor_show (priv->shadow);

      g_signal_emit (actor, toolbar_signals[SHOW_COMPLETED], 0);
    }

  priv->in_show_animation = FALSE;
  priv->is_visible = TRUE;

  if (!priv->waiting_for_panel_show)
    priv->reason_for_show = _MNB_SHOW_HIDE_UNSET;

  g_object_unref (actor);

  if (priv->tp_to_activate)
    {
      MnbToolbarPanel *tp = priv->tp_to_activate;

      priv->tp_to_activate = NULL;

      if (!mx_button_get_toggled (MX_BUTTON (tp->button)))
        mx_button_set_toggled (MX_BUTTON (tp->button), TRUE);
    }

}

ClutterActor *
mnb_toolbar_get_lowlight (MnbToolbar *toolbar)
{
  MnbToolbarPrivate *priv = toolbar->priv;

  g_return_val_if_fail (MNB_IS_TOOLBAR (toolbar), NULL);

  return priv->lowlight;
}


static void
mnb_toolbar_show_lowlight (MnbToolbar *toolbar)
{
  ClutterActor *lowlight = toolbar->priv->lowlight;

  if (CLUTTER_ACTOR_IS_VISIBLE (lowlight))
    return;

  clutter_actor_set_opacity (lowlight, 0);
  clutter_actor_show (lowlight);

  clutter_actor_animate (CLUTTER_ACTOR(lowlight),
                         CLUTTER_EASE_IN_SINE,
                         TOOLBAR_LOWLIGHT_FADE_DURATION,
                         "opacity", 0x7f,
                         NULL);

}

static void
mnb_toolbar_hide_lowlight (MnbToolbar *toolbar)
{
  ClutterActor     *lowlight = toolbar->priv->lowlight;
  ClutterAnimation *anim;

  if (!CLUTTER_ACTOR_IS_VISIBLE (lowlight))
    return;

  anim = clutter_actor_animate (CLUTTER_ACTOR(lowlight),
                                CLUTTER_EASE_IN_SINE,
                                TOOLBAR_LOWLIGHT_FADE_DURATION,
                                "opacity", 0,
                                NULL);

  g_signal_connect_swapped (anim,
                            "completed",
                            G_CALLBACK (clutter_actor_hide),
                            lowlight);
}

/*i
 * Returns TRUE if currently focused window is system-modal.
 */
static gboolean
mnb_toolbar_system_modal_state (MnbToolbar *toolbar)
{
  MnbToolbarPrivate          *priv   = toolbar->priv;
  MetaPlugin                 *plugin = priv->plugin;
  DawatiNetbookPluginPrivate *ppriv  = DAWATI_NETBOOK_PLUGIN (plugin)->priv;
  MetaWindow                 *focus  = ppriv->last_focused;

  if (focus && meta_window_is_modal (focus))
    {
      MetaWindow *p = meta_window_find_root_ancestor (focus);

      if (p == focus)
        return TRUE;
    }

  return FALSE;
}

static void
mnb_toolbar_wipe_input (MnbToolbar *toolbar)
{
  MnbToolbarPrivate *priv = toolbar->priv;

  if (priv->input_region)
    {
      mnb_input_manager_remove_region (priv->input_region);
      priv->input_region = NULL;
    }
}

static void
mnb_toolbar_setup_input (MnbToolbar *toolbar)
{
  MnbToolbarPrivate *priv = toolbar->priv;

  if (priv->input_region)
    mnb_toolbar_wipe_input (toolbar);

  priv->input_region =
    mnb_input_manager_push_region (0, 0,
                                   priv->old_screen_width,
                                   TOOLBAR_HEIGHT + 10,
                                   FALSE, MNB_INPUT_LAYER_PANEL);
}

static void
mnb_toolbar_update_input (MnbToolbar *toolbar)
{
  MnbToolbarPrivate *priv = toolbar->priv;

  if (priv->input_region)
    mnb_input_manager_remove_region_without_update (priv->input_region);

  priv->input_region =
    mnb_input_manager_push_region (0, 0,
                                   priv->old_screen_width,
                                   TOOLBAR_HEIGHT + 10,
                                   FALSE, MNB_INPUT_LAYER_PANEL);
}

void
mnb_toolbar_show (MnbToolbar *toolbar, MnbShowHideReason reason)
{
  MnbToolbarPrivate  *priv = MNB_TOOLBAR (toolbar)->priv;
  ClutterActor       *actor = (ClutterActor*)toolbar;
  ClutterAnimation   *animation;

  if (priv->in_show_animation)
    {
      g_signal_stop_emission_by_name (toolbar, "show");
      return;
    }

  if (mnb_toolbar_system_modal_state (toolbar))
    return;

  priv->reason_for_show = reason;

  clutter_actor_show (actor);

  mnb_toolbar_setup_input (MNB_TOOLBAR (actor));

  /* set initial width and height */
  clutter_actor_set_y (actor, -(clutter_actor_get_height (actor)));

  g_object_ref (actor);

  priv->in_show_animation = TRUE;

  /*
   * Start animation and wait for it to complete.
   */
  animation = clutter_actor_animate (actor,
                                     CLUTTER_LINEAR, TOOLBAR_DROPDOWN_DURATION,
                                     "y", 0.0, NULL);

  g_signal_connect (animation,
                    "completed",
                    G_CALLBACK (mnb_toolbar_show_completed_cb),
                    actor);
}

static void
mnb_toolbar_real_show (ClutterActor *actor)
{
  MnbToolbarPrivate  *priv = MNB_TOOLBAR (actor)->priv;
  GList              *l;

  if (priv->in_show_animation)
    {
      g_signal_stop_emission_by_name (actor, "show");
      return;
    }

  mnb_toolbar_show_lowlight (MNB_TOOLBAR (actor));

  /*
   * Show all of the buttons -- see comments in _hide_completed_cb() on why we
   * do this.
   */
  for (l = priv->panels; l; l = l->next)
    {
      MnbToolbarPanel *panel = l->data;

      if (panel && panel->button)
      {
        clutter_actor_show (CLUTTER_ACTOR (panel->button));
        clutter_actor_set_reactive (CLUTTER_ACTOR (panel->button), FALSE);
      }
    }

  /*
   * Call the parent show(); this must be done before we do anything else.
   */
  CLUTTER_ACTOR_CLASS (mnb_toolbar_parent_class)->show (actor);

  dawati_netbook_stash_window_focus (priv->plugin, CurrentTime);
}

static void
mnb_toolbar_real_hide (ClutterActor *actor)
{
  MnbToolbarPrivate *priv = MNB_TOOLBAR (actor)->priv;
  GList             *l;

  /* the hide animation has finished, so now really hide the actor */
  CLUTTER_ACTOR_CLASS (mnb_toolbar_parent_class)->hide (actor);

  /*
   * We need to explicitely hide all the individual buttons, otherwise the
   * button tooltips will stay on screen.
   */
  for (l = priv->panels; l; l = l->next)
    {
      MnbToolbarPanel *panel = l->data;

      if (panel && panel->button)
        {
          clutter_actor_hide (CLUTTER_ACTOR (panel->button));

          if (mx_button_get_toggled (MX_BUTTON (panel->button)))
            mx_button_set_toggled (MX_BUTTON (panel->button), FALSE);
        }
    }
}

static void
mnb_toolbar_hide_transition_completed_cb (ClutterAnimation *animation,
					  ClutterActor     *actor)
{
  MnbToolbarPrivate *priv = MNB_TOOLBAR (actor)->priv;

  priv->in_hide_animation = FALSE;
  priv->dont_autohide = FALSE;
  priv->panel_input_only = FALSE;
  priv->reason_for_hide = _MNB_SHOW_HIDE_UNSET;
  priv->is_visible = FALSE;

  dawati_netbook_unstash_window_focus (priv->plugin, CurrentTime);

  g_signal_emit (actor, toolbar_signals[HIDE_COMPLETED], 0);

  /* clutter_actor_hide (actor); */

  g_object_unref (actor);
}

/*
 * Returns TRUE if windows other than docks and ORs are present.
 */
static gboolean
mnb_toolbar_check_for_windows (MnbToolbar *toolbar)
{
  MnbToolbarPrivate *priv   = toolbar->priv;
  MetaScreen        *screen = meta_plugin_get_screen (priv->plugin);
  GList             *l;
  gint               count = 0;
  gint               max_allowed = 0;

  if (meta_plugin_debug_mode (priv->plugin))
    {
      /*
       * The don't hide panel/toolbar feature is an absolute PITA to debug
       * so if we are in debugging mode, increase the threshold to 1.
       */

      max_allowed = 1;
    }

  l = meta_get_window_actors (screen);

  while (l)
    {
      MetaWindowActor    *m    = l->data;
      MetaWindow         *mw   = meta_window_actor_get_meta_window (m);
      MetaWindowType      type = meta_window_get_window_type (mw);

      if (!(type == META_WINDOW_DOCK    ||
            type == META_WINDOW_DESKTOP ||
            meta_window_actor_is_override_redirect (m)))
        {
          if (++count > max_allowed)
            return TRUE;
        }

      l = l->next;
    }

  return FALSE;
}

gboolean
mnb_toolbar_is_visible (MnbToolbar *toolbar)
{
  MnbToolbarPrivate *priv;

  g_return_val_if_fail (MNB_IS_TOOLBAR (toolbar), FALSE);

  priv = toolbar->priv;

  return priv->is_visible;
}

void
mnb_toolbar_hide (MnbToolbar *toolbar, MnbShowHideReason reason)
{
  ClutterActor      *actor = CLUTTER_ACTOR (toolbar);
  MnbToolbarPrivate *priv = toolbar->priv;
  ClutterAnimation  *animation;
  gfloat             height;
  GList             *l;

  if (priv->in_hide_animation)
    return;

  /*
   * If the reason for us hiding is of a lesser priority than the reason for
   * any pending show, we do not hide.
   */
  if (priv->reason_for_show > reason)
    {
      g_debug ("Not hiding Toolbar, reasons show %d, hide %d",
               priv->reason_for_show, reason);

        return;
    }

  priv->reason_for_hide = reason;

  clutter_actor_hide (priv->shadow);

  mnb_toolbar_hide_lowlight (MNB_TOOLBAR (actor));

  for (l = priv->panels; l; l = l->next)
    {
      MnbToolbarPanel *panel = l->data;

      if (panel->button)
        clutter_actor_set_reactive (CLUTTER_ACTOR (panel->button), FALSE);
    }

  g_signal_emit (actor, toolbar_signals[HIDE_BEGIN], 0);

  mnb_toolbar_wipe_input (toolbar);

  priv->in_hide_animation = TRUE;

  g_object_ref (actor);

  height = clutter_actor_get_height (actor);

  /*
   * Start animation and wait for it to complete.
   */
  animation = clutter_actor_animate (actor,
                                     CLUTTER_LINEAR, TOOLBAR_DROPDOWN_DURATION,
                                     "y", -height, NULL);

  g_signal_connect (animation,
                    "completed",
                    G_CALLBACK (mnb_toolbar_hide_transition_completed_cb),
                    actor);
}

static gboolean
mnb_toolbar_dbus_show_toolbar (MnbToolbar *self, GError **error)
{
  mnb_toolbar_show (self, MNB_SHOW_HIDE_BY_DBUS);
  return TRUE;
}

static gboolean
mnb_toolbar_dbus_hide_toolbar (MnbToolbar *self, GError **error)
{
  mnb_toolbar_hide (self, MNB_SHOW_HIDE_BY_DBUS);
  return TRUE;
}

static gboolean
mnb_toolbar_dbus_show_panel (MnbToolbar *self, gchar *name, GError **error)
{
  MnbToolbarPanel *tp;

  tp = mnb_toolbar_panel_name_to_panel_internal (self, name);

  if (!tp)
    return FALSE;

  mnb_toolbar_activate_panel_internal (self, tp, MNB_SHOW_HIDE_BY_DBUS);

  return TRUE;
}

static gboolean
mnb_toolbar_dbus_hide_panel (MnbToolbar  *self,
                             gchar       *name,
                             gboolean     hide_toolbar,
                             GError     **error)
{
  MnbPanel *panel = mnb_toolbar_panel_name_to_panel (self, name);

  if (!panel)
    return FALSE;

  if (!mnb_panel_is_mapped (panel))
    {
      if (hide_toolbar && CLUTTER_ACTOR_IS_MAPPED (self))
        mnb_toolbar_hide (self, MNB_SHOW_HIDE_BY_DBUS);
    }
  else/*  if (hide_toolbar) */
  /*   mnb_panel_hide_with_toolbar (panel, MNB_SHOW_HIDE_BY_DBUS); */
  /* else */
    mnb_panel_hide (panel);

  return TRUE;
}

#include "../src/mnb-toolbar-dbus-glue.h"

static void
mnb_toolbar_allocate (ClutterActor          *actor,
                      const ClutterActorBox *box,
                      ClutterAllocationFlags flags)
{
  /*
   * If the drop down is not visible, we just return; this insures that the
   * needs_allocation flag in ClutterActor remains set, and the actor will get
   * reallocated when we show it.
   */
  if (!CLUTTER_ACTOR_IS_VISIBLE (actor))
    return;

  CLUTTER_ACTOR_CLASS (mnb_toolbar_parent_class)->allocate (actor, box, flags);
}

static gboolean
mnb_toolbar_button_press_event (ClutterActor         *actor,
                                ClutterButtonEvent   *event)
{
  /*
   * We never ever want press events propagated
   */
  return TRUE;
}


static void
mnb_toolbar_get_preferred_width (ClutterActor *self,
                                 gfloat        for_height,
                                 gfloat       *min_width_p,
                                 gfloat       *nat_width_p)
{
  MnbToolbarPrivate *priv = ((MnbToolbar *)self)->priv;
  gint screen_width, screen_height;

  meta_plugin_query_screen_size (priv->plugin, &screen_width, &screen_height);

  if (min_width_p)
    *min_width_p = screen_width;
  if (nat_width_p)
    *nat_width_p = screen_width;
}

static void
mnb_toolbar_get_preferred_height (ClutterActor *self,
                                  gfloat        for_width,
                                  gfloat       *min_height_p,
                                  gfloat       *nat_height_p)
{
  if (min_height_p)
    *min_height_p = TOOLBAR_HEIGHT;
  if (nat_height_p)
    *nat_height_p = TOOLBAR_HEIGHT;
}

static void
mnb_toolbar_class_init (MnbToolbarClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  ClutterActorClass *clutter_class = CLUTTER_ACTOR_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MnbToolbarPrivate));

  object_class->get_property = mnb_toolbar_get_property;
  object_class->set_property = mnb_toolbar_set_property;
  object_class->dispose = mnb_toolbar_dispose;
  object_class->finalize = mnb_toolbar_finalize;
  object_class->constructed = mnb_toolbar_constructed;

  clutter_class->show = mnb_toolbar_real_show;
  clutter_class->hide = mnb_toolbar_real_hide;
  clutter_class->allocate = mnb_toolbar_allocate;
  clutter_class->button_press_event = mnb_toolbar_button_press_event;
  clutter_class->get_preferred_width = mnb_toolbar_get_preferred_width;
  clutter_class->get_preferred_height = mnb_toolbar_get_preferred_height;

  dbus_g_object_type_install_info (G_TYPE_FROM_CLASS (klass),
                                   &dbus_glib_mnb_toolbar_dbus_object_info);

  g_object_class_install_property (object_class,
                                   PROP_MUTTER_PLUGIN,
                                   g_param_spec_object ("mutter-plugin",
                                                        "Mutter Plugin",
                                                        "Mutter Plugin",
                                                        META_TYPE_PLUGIN,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));


  /*
   * MnbToolbar::show-completed, emitted when show animation completes.
   */
  toolbar_signals[SHOW_COMPLETED] =
    g_signal_new ("show-completed",
                  G_TYPE_FROM_CLASS (object_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (MnbToolbarClass, show_completed),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  /*
   * MnbToolbar::hide-begin, emitted before hide animation is started.
   */
  toolbar_signals[HIDE_BEGIN] =
    g_signal_new ("hide-begin",
                  G_TYPE_FROM_CLASS (object_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (MnbToolbarClass, hide_begin),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  /*
   * MnbToolbar::hide-completed, emitted when hide animation completes.
   */
  toolbar_signals[HIDE_COMPLETED] =
    g_signal_new ("hide-completed",
                  G_TYPE_FROM_CLASS (object_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (MnbToolbarClass, hide_completed),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);
}

/*
 * We need a safety clearing mechanism for the waiting_for_panel flags (so that
 * if a panel fails to complete the show/hide process, we do not block the
 * various depenedent UI ops indefinitely).
 */
static gboolean
mnb_toolbar_waiting_for_panel_show_cb (gpointer data)
{
  MnbToolbarPrivate *priv = MNB_TOOLBAR (data)->priv;

  priv->waiting_for_panel_show       = FALSE;
  priv->waiting_for_panel_show_cb_id = 0;
  priv->reason_for_show              = _MNB_SHOW_HIDE_UNSET;

  return FALSE;
}

static gboolean
mnb_toolbar_waiting_for_panel_hide_cb (gpointer data)
{
  MnbToolbarPrivate *priv = MNB_TOOLBAR (data)->priv;

  priv->waiting_for_panel_hide       = FALSE;
  priv->waiting_for_panel_hide_cb_id = 0;

  return FALSE;
}

static void
mnb_toolbar_set_waiting_for_panel_show (MnbToolbar *toolbar,
                                        gboolean    whether,
                                        gboolean    with_timeout)
{
  MnbToolbarPrivate *priv = toolbar->priv;

  /*
   * Remove any existing timeout (if whether is TRUE, we need to restart it)
   */
  if (priv->waiting_for_panel_show_cb_id)
    {
      g_source_remove (priv->waiting_for_panel_show_cb_id);
      priv->waiting_for_panel_show_cb_id = 0;
    }

  if (whether && with_timeout)
    priv->waiting_for_panel_show_cb_id =
      g_timeout_add_seconds (TOOLBAR_WAITING_FOR_PANEL_TIMEOUT,
                             mnb_toolbar_waiting_for_panel_show_cb, toolbar);

  priv->waiting_for_panel_show = whether;

  if (!whether)
    priv->reason_for_show = _MNB_SHOW_HIDE_UNSET;
}

static void
mnb_toolbar_set_waiting_for_panel_hide (MnbToolbar *toolbar, gboolean whether)
{
  MnbToolbarPrivate *priv = toolbar->priv;

  /*
   * Remove any existing timeout (if whether is TRUE, we need to restart it)
   */
  if (priv->waiting_for_panel_hide_cb_id)
    {
      g_source_remove (priv->waiting_for_panel_hide_cb_id);
      priv->waiting_for_panel_hide_cb_id = 0;
    }

  if (whether)
    priv->waiting_for_panel_hide_cb_id =
      g_timeout_add_seconds (TOOLBAR_WAITING_FOR_PANEL_TIMEOUT,
                             mnb_toolbar_waiting_for_panel_hide_cb, toolbar);

  priv->waiting_for_panel_hide = whether;
}

static gboolean
mnb_toolbar_panel_stub_timeout_cb (gpointer data)
{
  MnbToolbar        *toolbar = data;
  MnbToolbarPrivate *priv    = toolbar->priv;
  MnbToolbarPanel   *stubbed = priv->stubbed_panel;
  GList             *l;

  mnb_toolbar_set_waiting_for_panel_show (toolbar, FALSE, FALSE);
  clutter_actor_hide (priv->panel_stub);
  mnb_spinner_stop ((MnbSpinner*)priv->spinner);

  priv->stubbed_panel = NULL;

  for (l = priv->panels; l; l = l->next)
    {
      MnbToolbarPanel *tp = l->data;

      if (!tp || tp != stubbed)
        continue;

      if (mx_button_get_toggled (MX_BUTTON (tp->button)))
        {
          gchar *tooltip;

          tp->pinged = FALSE;
          tp->failed = TRUE;

          tooltip = g_strdup_printf (_("Sorry, %s is broken"),
                                     tp->tooltip);

          mx_widget_set_tooltip_text (MX_WIDGET (tp->button), tooltip);
          mx_widget_show_tooltip (MX_WIDGET (tp->button));

          g_free (tooltip);

          mx_button_set_toggled (MX_BUTTON (tp->button), FALSE);
        }

      break;
    }

  return FALSE;
}

static void
mnb_toolbar_show_pending_panel (MnbToolbar *toolbar, MnbToolbarPanel *tp)
{
  MnbToolbarPrivate *priv = toolbar->priv;
  gint               screen_width, screen_height;

  meta_plugin_query_screen_size (priv->plugin,
                                   &screen_width, &screen_height);

  clutter_actor_set_size (priv->panel_stub, screen_width, screen_height / 3);

  /*
   * Set the waiting_for_panel_show flag, but without the timeout (since we
   * have a stub timeout of our own, which needs to be considerably longer).
   */
  mnb_toolbar_set_waiting_for_panel_show (toolbar, TRUE, FALSE);
  clutter_actor_set_opacity (priv->panel_stub, 0xff);
  clutter_actor_show (priv->panel_stub);
  clutter_actor_raise_top (priv->panel_stub);
  mnb_spinner_start ((MnbSpinner*)priv->spinner);
  priv->stubbed_panel = tp;

  if (priv->panel_stub_timeout_id)
    {
      g_source_remove (priv->panel_stub_timeout_id);
    }

  priv->panel_stub_timeout_id =
    g_timeout_add_seconds (TOOLBAR_PANEL_STUB_TIMEOUT,
                           mnb_toolbar_panel_stub_timeout_cb,
                           toolbar);

  tp->pinged = TRUE;
  mnb_toolbar_start_panel_service (toolbar, tp);
}

static gboolean
mnb_toolbar_button_button_release_cb (MnbToolbarButton   *button,
                                      ClutterButtonEvent *event,
                                      MnbToolbar         *toolbar)
{
  MnbToolbarPrivate *priv = toolbar->priv;

  priv->button_click = TRUE;

  return FALSE;
}

static void
mnb_toolbar_hide_selector (MnbToolbar *self)
{
#if TOOLBAR_CUT_OUT
  MnbToolbarPrivate *priv = self->priv;

  clutter_actor_animate (priv->light_spot,
                         CLUTTER_LINEAR, TOOLBAR_SELECTOR_ANIMATION_DELAY,
                         "y", TOOLBAR_CUT_OUT_SHIFT,
                         NULL);
#endif /* TOOLBAR_CUT_OUT */
}

static void
mnb_toolbar_show_selector (MnbToolbar *self)
{
#if TOOLBAR_CUT_OUT
  MnbToolbarPrivate *priv = self->priv;

  clutter_actor_set_x (priv->light_spot,
                       priv->next_selector_pos.x1);
  clutter_actor_animate (priv->light_spot,
                         CLUTTER_LINEAR, TOOLBAR_SELECTOR_ANIMATION_DELAY,
                         "y", 0.0,
                         NULL);
#endif /* TOOLBAR_CUT_OUT */
}

static void
mnb_toolbar_selector_animation_completed_cb (ClutterAnimation *anim,
                                             MnbToolbar       *toolbar)
{
  mnb_toolbar_show_selector (toolbar);
}

static void
mnb_toolbar_move_selector (MnbToolbar *self)
{
#if TOOLBAR_CUT_OUT
  MnbToolbarPrivate *priv = self->priv;
  ClutterAnimation  *animation;

  animation =
    clutter_actor_animate (priv->light_spot,
                           CLUTTER_LINEAR, TOOLBAR_SELECTOR_ANIMATION_DELAY,
                           "y", 10.0,
                           NULL);

  g_signal_connect_after (animation,
                          "completed",
                          G_CALLBACK (mnb_toolbar_selector_animation_completed_cb),
                          self);
#endif /* TOOLBAR_CUT_OUT */
}

static void
mnb_toolbar_lightspot_moved_cb (ClutterActor           *actor,
                                ClutterActorBox        *box,
                                ClutterAllocationFlags  flags,
                                MnbToolbar             *toolbar)
{
  MnbToolbarPrivate *priv = toolbar->priv;

  clutter_actor_queue_redraw (priv->shadow);
  clutter_actor_queue_redraw (CLUTTER_ACTOR (toolbar));
}

/*
 * Toolbar button click handler.
 *
 * If the new button stage is 'checked' we show the asociated panel and hide
 * all others; in the oposite case, we hide the associated panel.
 */
static void
mnb_toolbar_button_toggled_cb (MxButton *button,
                               GParamSpec *pspec,
                               MnbToolbar *toolbar)
{
  MnbToolbarPrivate *priv    = toolbar->priv;
  gboolean           checked;
  GList             *l;
  gboolean           button_click;
  gboolean           previous_clicked = FALSE;

  static gboolean    recursion = FALSE;

  if (recursion)
    return;

  recursion = TRUE;

  button_click = priv->button_click;
  priv->button_click = FALSE;

  checked = mx_button_get_toggled (button);

  /*
   * If the user clicked on a button to show panel, but currently focused
   * window is system-modal, ignore the click.
   */
  if (button_click && checked && mnb_toolbar_system_modal_state (toolbar))
    {
      mx_button_set_toggled (button, FALSE);
      recursion = FALSE;
      return;
    }

  /*
   * If the user clicked on a button to hide a panel, and there are no
   * meaningful windows present, ignore the click.
   *
   * We differentiate here between a human click and a programmatic toggle,
   * otherwise the button is left in wrong state, for example, when launching
   * the first application.
   */
  if (button_click && !checked && !mnb_toolbar_check_for_windows (toolbar))
    {
      mx_button_set_toggled (button, TRUE);
      recursion = FALSE;
      return;
    }

  /* Update position of selected button */
  if (checked)
    {
      ClutterActor *parent_box =
        clutter_actor_get_parent (CLUTTER_ACTOR (button));
      ClutterActorBox parent_alloc;

      clutter_actor_get_allocation_box (parent_box, &parent_alloc);
      clutter_actor_get_allocation_box (CLUTTER_ACTOR (button),
                                        &priv->next_selector_pos);
      priv->next_selector_pos.x1 += parent_alloc.x1;
      priv->next_selector_pos.y1 += parent_alloc.y1;
    }

  /*
   * Clear the autohiding flag -- if the user is clicking on the panel buttons
   * then we are back to normal mode.
   */
  priv->dont_autohide = FALSE;

  for (l = priv->panels; l; l = l->next)
    {
      MnbToolbarPanel *tp = l->data;

      if (!tp)
        continue;

      if (tp->button != (ClutterActor*)button)
        {
          if (tp->button && mx_button_get_toggled (MX_BUTTON (tp->button)))
            {
              previous_clicked = TRUE;
              mx_button_set_toggled (MX_BUTTON (tp->button), FALSE);
              mnb_toolbar_move_selector (toolbar);
            }

          if (tp->panel)
            {
            if (mnb_panel_is_mapped (tp->panel))
              {
                mnb_panel_hide (tp->panel);
              }
            }

          /*
           * Ensure the pinged flag is cleared (the user seems to have clicked
           * some other buttons since they clicked on this one)
           */
          tp->pinged = FALSE;
        }
    else
      {
        /*
         * When showing/hiding this panels, set the waiting_for_panel flag; this
         * serves two purposes:
         *
         *   a) forces LEAVE events to be ignored until the panel is shown
         *      (see bug 3531).
         *
         *   b) Prevents race conditions when the user starts clicking fast at
         *      the button (see bug 5020)
         */

        if (tp->panel)
          {
            if (checked && !mnb_panel_is_mapped (tp->panel))
              {
                mnb_toolbar_set_waiting_for_panel_show (toolbar, TRUE, TRUE);
                mnb_panel_show (tp->panel);

                if (priv->panel_stub_timeout_id)
                  {
                    g_source_remove (priv->panel_stub_timeout_id);
                    priv->panel_stub_timeout_id = 0;
                    clutter_actor_hide (priv->panel_stub);
                    mnb_spinner_stop ((MnbSpinner*)priv->spinner);
                    priv->stubbed_panel = NULL;
                  }
              }
            else if (!checked && mnb_panel_is_mapped (tp->panel))
              {
                mnb_toolbar_set_waiting_for_panel_hide (toolbar, TRUE);
                mnb_panel_hide (tp->panel);
              }
          }
        else
          {
            if (checked)
              {
                if (!tp->failed)
                  mnb_toolbar_show_pending_panel (toolbar, tp);
                else
                  {
                    g_warning ("Panel %s preiviously failed to load, ingoring",
                               tp->name);

                    mx_button_set_toggled (MX_BUTTON (tp->button), FALSE);
                  }
              }
            else
              {
                /*
                 * We are waiting for the panel to load; just ignore the click.
                 * This is not very nice to the user maybe, but simple and
                 * clean.
                 */
                if (button_click && !tp->failed)
                  mx_button_set_toggled (MX_BUTTON (tp->button), TRUE);
              }
          }

        if (!checked)
          {
            previous_clicked = TRUE;
            mnb_toolbar_hide_selector (toolbar);
          }
      }
    }

  if (!previous_clicked)
    mnb_toolbar_show_selector (toolbar);


  recursion = FALSE;
}

static MnbToolbarPanel *
mnb_toolbar_panel_to_toolbar_panel (MnbToolbar *toolbar, MnbPanel *panel)
{
  MnbToolbarPrivate *priv = toolbar->priv;
  GList             *l;

  g_return_val_if_fail (panel, NULL);

  for (l = priv->panels; l; l = l->next)
    {
      MnbToolbarPanel *tp = l->data;

      if (tp && tp->panel == panel)
        return tp;
    }

  return NULL;
}

MnbToolbarPanel *
mnb_toolbar_panel_name_to_panel_internal (MnbToolbar  *toolbar,
                                          const gchar *name)
{
  MnbToolbarPrivate *priv = toolbar->priv;
  GList             *l = priv->panels;

  g_return_val_if_fail (name, NULL);

  for (; l; l = l->next)
    {
      MnbToolbarPanel *tp = l->data;

      if (tp && tp->name)
        if (!strcmp (name, tp->name))
          return tp;
    }

  return NULL;
}

MnbToolbarPanel *
mnb_toolbar_panel_service_to_panel_internal (MnbToolbar  *toolbar,
                                             const gchar *service)
{
  MnbToolbarPrivate *priv = toolbar->priv;
  GList             *l = priv->panels;

  g_return_val_if_fail (service, NULL);

  for (; l; l = l->next)
    {
      MnbToolbarPanel *tp = l->data;

      if (tp && tp->service)
        {
          if (!strcmp (service, tp->service))
            return tp;
        }
    }

  return NULL;
}

MnbPanel *
mnb_toolbar_panel_name_to_panel (MnbToolbar *toolbar, const gchar *name)
{
  MnbToolbarPanel *tp;

  tp = mnb_toolbar_panel_name_to_panel_internal (toolbar, name);

  if (!tp)
    return NULL;

  return tp->panel;
}

/*
 * Helper function to manage lowlight stacking when showing a panel.
 */
static void
mnb_toolbar_raise_lowlight_for_panel (MnbToolbar *toolbar, MnbPanel *panel)
{
  MnbToolbarPrivate *priv = toolbar->priv;

  if (CLUTTER_IS_ACTOR (panel))
    clutter_actor_raise (CLUTTER_ACTOR (panel), priv->lowlight);
  else if (MNB_IS_PANEL_OOP (panel))
    {
      MnbPanelOop  *opanel = (MnbPanelOop*) panel;
      ClutterActor *actor;

      actor = (ClutterActor*) mnb_panel_oop_get_mutter_window (opanel);

      clutter_actor_raise (actor, priv->lowlight);

      if (CLUTTER_ACTOR_IS_VISIBLE (priv->panel_stub))
        clutter_actor_raise (priv->panel_stub, actor);
    }
}

static void
mnb_toolbar_panel_show_begin_cb (MnbPanel *panel, MnbToolbar *toolbar)
{
  MnbToolbarPrivate *priv = toolbar->priv;

  if (CLUTTER_ACTOR_IS_VISIBLE (priv->panel_stub))
    {
      guint  w, h;
      gfloat wf, hf;

      mnb_panel_get_size (panel, &w, &h);

      wf = w;
      hf = h;

      clutter_actor_animate (priv->panel_stub, CLUTTER_EASE_IN_SINE,
                             SLIDE_DURATION,
                             "opacity", 0,
                             "width", wf,
                             "height", hf,
                             NULL);
    }

  mnb_toolbar_raise_lowlight_for_panel (toolbar, panel);
}

#if 0
/*
 * Use for any built-in panels, which require the input region to cover the
 * whole panel area.
 */
static void
mnb_toolbar_dropdown_show_completed_full_cb (MnbPanel   *panel,
                                             MnbToolbar *toolbar)
{
  g_assert (CLUTTER_IS_ACTOR (panel));

  mnb_input_manager_push_actor (CLUTTER_ACTOR (panel), MNB_INPUT_LAYER_PANEL);

  mnb_toolbar_set_waiting_for_panel_show (toolbar, FALSE, FALSE);
}
#endif

static void
mnb_toolbar_dropdown_show_completed_partial_cb (MnbPanel    *panel,
                                                MnbToolbar  *toolbar)
{
  MnbToolbarPrivate *priv = toolbar->priv;
  MetaWindowActor   *mcw;

  g_assert (MNB_IS_PANEL_OOP (panel));

  mcw = mnb_panel_oop_get_mutter_window ((MnbPanelOop*)panel);
  mnb_panel_oop_set_delayed_show ((MnbPanelOop*)panel, FALSE);

  if (!mcw)
    g_warning ("Completed show on panel with no window ?!");
  else
    mnb_input_manager_push_oop_panel (mcw);

  clutter_actor_hide (priv->panel_stub);
  mnb_spinner_stop ((MnbSpinner*)priv->spinner);
  priv->stubbed_panel = NULL;

  mnb_toolbar_show_lowlight (toolbar);

  mnb_toolbar_raise_lowlight_for_panel (toolbar, panel);
  mnb_toolbar_set_waiting_for_panel_show (toolbar, FALSE, FALSE);
}

static void
mnb_toolbar_dropdown_hide_completed_cb (MnbPanel *panel, MnbToolbar  *toolbar)
{
  MnbToolbarPrivate *priv = toolbar->priv;
  MetaPlugin        *plugin = priv->plugin;
  MnbPanel          *active;

  dawati_netbook_stash_window_focus (plugin, CurrentTime);

  if (!priv->waiting_for_panel_show &&
      (!(active = mnb_toolbar_get_active_panel (toolbar)) ||
       active == panel))
    {
      mnb_toolbar_hide_lowlight (toolbar);
    }

  priv->panel_input_only = FALSE;
  mnb_toolbar_set_waiting_for_panel_hide (toolbar, FALSE);
}

#if 0
static void
mnb_toolbar_append_panel_builtin_internal (MnbToolbar      *toolbar,
                                           MnbToolbarPanel *tp)
{
  MnbToolbarPrivate *priv = toolbar->priv;
  MutterPlugin      *plugin = priv->plugin;
  MnbPanel          *panel = NULL;
  gint               screen_width, screen_height;

  if (!tp)
    return;

  if (tp->panel)
    {
      /*
       * BTW -- this code should not be reached; we should have exited
       * already in the button test.
       */
      g_warning ("The Zones Panel cannot be replaced\n");
      return;
    }

  meta_plugin_query_screen_size (plugin, &screen_width, &screen_height);

  {
#if 0
      MetaScreen  *screen  = meta_plugin_get_screen (plugin);
      MetaDisplay *display = meta_screen_get_display (screen);

      panel = tp->panel = MNB_PANEL (mnb_switcher_new (plugin));

      g_signal_connect (panel, "show-completed",
                        G_CALLBACK(mnb_toolbar_dropdown_show_completed_full_cb),
                        toolbar);
      g_signal_connect (panel, "show-begin",
                        G_CALLBACK(mnb_toolbar_panel_show_begin_cb),
                        toolbar);

      g_signal_connect (display, "notify::focus-window",
                        G_CALLBACK (mnb_switcher_focus_window_cb),
                        panel);
#endif
    }

  if (!panel)
    {
      g_warning ("Builtin panel %s is not available", tp->name);
      return;
    }

  g_signal_connect (panel, "hide-completed",
                    G_CALLBACK(mnb_toolbar_dropdown_hide_completed_cb),
                    toolbar);

  /* This is safe, because the Switcher is an actor */
  clutter_container_add_actor (CLUTTER_CONTAINER (priv->hbox_buttons),
                               CLUTTER_ACTOR (panel));
  clutter_actor_set_width (CLUTTER_ACTOR (panel),
                           screen_width);

  if (tp->button)
    mnb_panel_set_button (panel, MX_BUTTON (tp->button));
  mnb_panel_set_position (panel, 0, STATUSBAR_HEIGHT);
}
#endif

static void
mnb_toolbar_panel_request_button_style_cb (MnbPanel    *panel,
                                           const gchar *style_id,
                                           MnbToolbar  *toolbar)
{
  MnbToolbarPanel *tp;

  tp = mnb_toolbar_panel_to_toolbar_panel (toolbar, panel);

  if (!tp || !tp->button)
    return;

  clutter_actor_set_name (CLUTTER_ACTOR (tp->button), style_id);
}

static void
mnb_toolbar_panel_request_button_state_cb (MnbPanel       *panel,
                                           MnbButtonState  state,
                                           MnbToolbar     *toolbar)
{
  ClutterActor    *actor;
  MnbToolbarPanel *tp;

  tp = mnb_toolbar_panel_to_toolbar_panel (toolbar, panel);

  if (!tp || !tp->button)
    return;

  actor = CLUTTER_ACTOR (tp->button);

  if (CLUTTER_ACTOR_IS_MAPPED (actor) && (state & MNB_BUTTON_HIDDEN))
    clutter_actor_hide (actor);
  else if (!CLUTTER_ACTOR_IS_MAPPED (actor) && !(state & MNB_BUTTON_HIDDEN))
    clutter_actor_show (actor);

  if ((state & MNB_BUTTON_INSENSITIVE))
    g_warning (G_STRLOC " Insensitive state is not yet implemented.");
}

static void
mnb_toolbar_panel_notify_modal_cb (MnbPanel   *panel,
                                   GParamSpec *pspec,
                                   MnbToolbar *toolbar)
{
  MnbToolbarPanel *tp;

  tp = mnb_toolbar_panel_to_toolbar_panel (toolbar, panel);

  if (!tp || !tp->panel || !mnb_panel_is_mapped (tp->panel))
    return;

  mnb_toolbar_raise_lowlight_for_panel (toolbar, panel);
}

/* static void */
/* mnb_toolbar_panel_request_tooltip_cb (MnbPanel    *panel, */
/*                                       const gchar *tooltip, */
/*                                       MnbToolbar  *toolbar) */
/* { */
/*   MnbToolbarPanel *tp; */

/*   tp = mnb_toolbar_panel_to_toolbar_panel (toolbar, panel); */

/*   if (!tp || !tp->button) */
/*     return; */

/*   if (tp->button && (tp->windowless || */
/*                      tp->type == MNB_TOOLBAR_PANEL_CLOCK)) */
/*     { */
/*       mx_widget_set_tooltip_text (MX_WIDGET (tp->button), tooltip); */
/*       mx_widget_show_tooltip (MX_WIDGET (tp->button)); */
/*     } */
/* } */

/*
 * Removes the panel from the pending_panels list
 */
static void
mnb_toolbar_remove_panel_from_pending (MnbToolbar *toolbar, MnbPanelOop *panel)
{
  MnbToolbarPrivate *priv      = toolbar->priv;
  const gchar       *dbus_name = mnb_panel_oop_get_dbus_name (panel);

  if (dbus_name)
    {
      GSList *l = priv->pending_panels;

      while (l)
        {
          gchar *n = l->data;

          if (!strcmp (n, dbus_name))
            {
              g_free (n);
              priv->pending_panels =
                g_slist_delete_link (priv->pending_panels, l);
              break;
            }

          l = l->next;
        }
    }
}

/*
 * Removes the panel from the toolbar, avoiding any recursion
 * due to "destroy" signal handler.
 *
 * The panel_destroyed parameter should be set to TRUE if the panel is known
 * to be in the destroy sequence.
 */
static void
mnb_toolbar_dispose_of_button (MnbToolbar      *toolbar,
                               MnbToolbarPanel *tp)
{
  ClutterActor *button;

  if (!tp)
    return;

  button = tp->button;

  if (!button)
    return;

  tp->button = NULL;

  g_signal_handlers_disconnect_matched (button,
                                        G_SIGNAL_MATCH_DATA,
                                        0, 0, NULL, NULL,
                                        toolbar);

  clutter_actor_destroy (button);
}

/*
 * Removes the panel from the toolbar, avoiding any recursion
 * due to "destroy" signal handler.
 *
 * The panel_destroyed parameter should be set to TRUE if the panel is known
 * to be in the destroy sequence.
 */
static void
mnb_toolbar_dispose_of_panel (MnbToolbar      *toolbar,
                              MnbToolbarPanel *tp,
                              gboolean         panel_destroyed)
{
  MnbToolbarPrivate *priv = toolbar->priv;
  MnbPanel          *panel;

  if (!tp)
    return;

  panel  = tp->panel;

  if (!tp->panel)
    return;

  tp->panel  = NULL;

  g_signal_handlers_disconnect_matched (panel,
                                        G_SIGNAL_MATCH_DATA,
                                        0, 0, NULL, NULL,
                                        toolbar);

  if (MNB_IS_PANEL_OOP (panel))
    mnb_toolbar_remove_panel_from_pending (toolbar, (MnbPanelOop*)panel);

  if (!panel_destroyed && CLUTTER_IS_ACTOR (panel))
    clutter_container_remove_actor (CLUTTER_CONTAINER (priv->hbox_buttons),
                                    CLUTTER_ACTOR (panel));
}

#if 0
static void
mnb_toolbar_update_dropdown_input_region (MnbToolbar  *toolbar,
                                          MnbPanel    *panel)
{
  MnbToolbarPrivate *priv;
  MutterPlugin      *plugin;
  gfloat             x, y,w, h;
  MnbInputRegion    *region;

  /*
   * If this panel is visible, we need to update the input region to match
   * the new geometry.
   */
  if (!mnb_panel_is_mapped (panel))
    return;

  priv   = toolbar->priv;
  plugin = priv->plugin;

  mnb_drop_down_get_footer_geometry (panel, &x, &y, &w, &h);

  region = mnb_panel_get_input_region (panel);

  if (region)
    mnb_input_manager_remove_region_without_update (region);

  if (priv->panel_input_only)
    region = mnb_input_manager_push_region ((gint)x, STATUSBAR_HEIGHT + (gint)y,
                                            (guint)w, (guint)h,
                                            FALSE, MNB_INPUT_LAYER_PANEL);
  else
    region = mnb_input_manager_push_region ((gint)x, STATUSBAR_HEIGHT + (gint)y,
                                            (guint)w,
                                            priv->old_screen_height -
                                            (STATUSBAR_HEIGHT + (gint)y),
                                            FALSE, MNB_INPUT_LAYER_PANEL);

  mnb_panel_set_input_region (panel, region);
}
#endif

static void
mnb_toolbar_panel_died_cb (MnbPanel *panel, MnbToolbar *toolbar)
{
  MnbToolbarPrivate *priv = toolbar->priv;
  MnbToolbarPanel   *tp;

  tp = mnb_toolbar_panel_to_toolbar_panel (toolbar, panel);

  if (!tp)
    return;

  mnb_toolbar_dispose_of_panel (toolbar, tp, FALSE);

  /*
   * If the panel went away because we unloaded it, we are done.
   */
  if (tp->unloaded)
    {
      mnb_toolbar_dispose_of_button (toolbar, tp);
      priv->panels = g_list_remove (priv->panels, tp);
      mnb_toolbar_panel_destroy (tp);
      return;
    }

  /*
   * Try to restart the service
   */
  if (!toolbar->priv->no_autoloading && tp->service)
    {
      mnb_toolbar_handle_dbus_name (toolbar, tp->service);
    }
}

static void
mnb_toolbar_panel_ready_cb (MnbPanel *panel, MnbToolbar *toolbar)
{
  if (MNB_IS_PANEL (panel))
    {
      MnbToolbarPrivate *priv   = toolbar->priv;
      ClutterActor      *button;
      const gchar       *style_id;
      const gchar       *stylesheet;
      MnbToolbarPanel   *tp;

      tp = mnb_toolbar_panel_to_toolbar_panel (toolbar, panel);

      if (!tp)
        return;

      button = tp->button;

      stylesheet = mnb_panel_get_stylesheet (panel);
      style_id   = mnb_panel_get_button_style (panel);

      if (button)
        {
          gchar *button_style = NULL;

          if (stylesheet && *stylesheet &&
              (!tp->button_stylesheet ||
               strcmp (stylesheet, tp->button_stylesheet)))
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
              else
                {
                  g_free (tp->button_stylesheet);
                  tp->button_stylesheet = g_strdup (stylesheet);
                }
            }

          if (!style_id || !*style_id)
            {
              const gchar *name = mnb_panel_get_name (panel);

              if (tp->button_style)
                style_id = tp->button_style;
              else
                button_style = g_strdup_printf ("%s-button", name);
            }

          clutter_actor_set_name (CLUTTER_ACTOR (button),
                                  button_style ? button_style : style_id);

          g_free (button_style);
        }

      if (tp->pinged)
        {
          tp->pinged = FALSE;

          if (MNB_IS_PANEL_OOP (panel))
            mnb_panel_oop_set_delayed_show ((MnbPanelOop*)panel, TRUE);

          if (priv->panel_stub_timeout_id)
            {
              g_source_remove (priv->panel_stub_timeout_id);
              priv->panel_stub_timeout_id = 0;
            }

          mnb_panel_show (panel);
        }
      else if (!priv->shown_myzone && priv->shown)
        {
          if (tp->name && !strcmp (tp->name, "dawati-panel-myzone"))
            {
              mnb_panel_show (panel);
              priv->shown_myzone = TRUE;
            }
        }
    }
}

static void
mnb_toolbar_panel_destroy_cb (MnbPanel *panel, MnbToolbar *toolbar)
{
  MnbToolbarPanel *tp;

  if (!MNB_IS_PANEL_OOP (panel))
    {
      g_warning ("Unhandled panel type: %s", G_OBJECT_TYPE_NAME (panel));
      return;
    }

  tp = mnb_toolbar_panel_to_toolbar_panel (toolbar, panel);

  if (!tp)
    {
      /*
       * This is the case when the panel initialization failed, and the panel
       * was never added to the Toolbar. We need to release any floating
       * reference.
       */
      if (MNB_IS_PANEL_OOP (panel))
        mnb_toolbar_remove_panel_from_pending (toolbar, (MnbPanelOop*)panel);

      if (g_object_is_floating (panel))
        {
          g_object_ref_sink (panel);
          g_object_unref (panel);
        }

      return;
    }

  mnb_toolbar_dispose_of_panel (toolbar, tp, TRUE);
}

static void
mnb_toolbar_ensure_button_position (MnbToolbar *toolbar, MnbToolbarPanel *tp)
{
  MnbToolbarPrivate *priv   = toolbar->priv;
  ClutterActor      *button;

  if (!tp || !tp->button)
    return;

  button = tp->button;

  /*
   * The button size and positioning depends on whether this is a regular
   * zone button, but one of the applet buttons.
   */
  if (tp->type == MNB_TOOLBAR_PANEL_NORMAL)
    {
      if (!clutter_actor_get_parent (button))
        clutter_container_add_actor (CLUTTER_CONTAINER (priv->hbox_buttons),
                                     button);

      clutter_container_raise_child (CLUTTER_CONTAINER (priv->hbox_buttons),
                                         button, NULL);
    }
  else
    {
      if (!clutter_actor_get_parent (button))
        clutter_container_add_actor (CLUTTER_CONTAINER (priv->hbox_applets),
                                     button);

      clutter_container_raise_child (CLUTTER_CONTAINER (priv->hbox_applets),
                                     button, NULL);

    }
}

static void
mnb_toolbar_windowless_button_toggled_cb (MxButton *button,
                                          GParamSpec *pspec,
                                          MnbToolbar *toolbar)
{
  /*
   * Windowless buttons cannot be checked.
   */
  if (mx_button_get_toggled (button))
    mx_button_set_toggled (button, FALSE);
}

/*
 * Appends a panel
 */
static void
mnb_toolbar_append_button (MnbToolbar  *toolbar, MnbToolbarPanel *tp)
{
  MnbToolbarPrivate *priv   = toolbar->priv;
  ClutterActor      *button;
  gchar             *button_style = NULL;
  const gchar       *name;
  const gchar       *stylesheet = NULL;
  const gchar       *style_id = NULL;

  if (!tp)
    return;

  name       = tp->name;
  stylesheet = tp->button_stylesheet;
  style_id   = tp->button_style;

  if (!style_id || !*style_id)
    {
      if (tp->button_style)
        style_id = tp->button_style;
      else if (tp->type != MNB_TOOLBAR_PANEL_CLOCK)
        button_style = g_strdup_printf ("%s-button", name);
    }

  if (tp->button)
    clutter_actor_destroy (CLUTTER_ACTOR (tp->button));

  if (tp->type == MNB_TOOLBAR_PANEL_CLOCK)
    {
      button = tp->button = mnb_toolbar_clock_new ();
      priv->have_clock = TRUE;

      if (priv->dummy_clock)
        {
          clutter_actor_destroy (priv->dummy_clock);
          priv->dummy_clock = NULL;
        }
    }
  else if (tp->windowless)
    {
      button = tp->button = mnb_toolbar_icon_new ();
      /* mx_widget_set_tooltip_text (MX_WIDGET (tp->button), */
      /*                             tp->tooltip); */
    }
  else if (tp->type == MNB_TOOLBAR_PANEL_APPLET)
    {
      button = tp->button = mnb_toolbar_applet_new ();
    }
  else
    {
      button = tp->button = mx_button_new ();
      mx_stylable_set_style_class (MX_STYLABLE (button),
                                   "ToolbarButton");
      mx_button_set_label (MX_BUTTON (button), tp->tooltip);
      mx_button_set_icon_name (MX_BUTTON (button), "player_play");
      mx_button_set_icon_position (MX_BUTTON (button), MX_POSITION_LEFT);
      mx_widget_hide_tooltip (MX_WIDGET (button));
    }

  if (stylesheet && *stylesheet)
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

              g_free (tp->button_stylesheet);
              tp->button_stylesheet = NULL;
            }
        }
    }

  mx_button_set_is_toggle (MX_BUTTON (button), TRUE);

  clutter_actor_set_name (button, button_style ? button_style : style_id);

  g_free (button_style);

  mnb_toolbar_ensure_button_position (toolbar, tp);

  if (!tp->windowless)
    {
      g_signal_connect (button, "notify::toggled",
                        G_CALLBACK (mnb_toolbar_button_toggled_cb),
                        toolbar);

      g_signal_connect (button, "button-release-event",
                        G_CALLBACK (mnb_toolbar_button_button_release_cb),
                        toolbar);
    }
  else
    g_signal_connect (button, "notify::toggled",
                      G_CALLBACK (mnb_toolbar_windowless_button_toggled_cb),
                      toolbar);
}

/*
 * Appends a panel
 */
static void
mnb_toolbar_append_panel (MnbToolbar  *toolbar, MnbPanel *panel)
{
  const gchar       *name;
  const gchar       *service = NULL;
  MnbToolbarPanel   *tp;

  if (MNB_IS_PANEL (panel))
    {
      name = mnb_panel_get_name (panel);

      /*
       * Remove this panel from the pending list.
       */
      if (MNB_IS_PANEL_OOP (panel))
        {
          service = mnb_panel_oop_get_dbus_name ((MnbPanelOop*)panel);
          mnb_toolbar_remove_panel_from_pending (toolbar, (MnbPanelOop*)panel);
        }
    }
   else
    {
      g_warning ("Unhandled panel type: %s", G_OBJECT_TYPE_NAME (panel));
      return;
    }

  tp = mnb_toolbar_panel_name_to_panel_internal (toolbar, name);

  if (!tp)
    {
      if (service)
          tp = mnb_toolbar_panel_service_to_panel_internal (toolbar, service);

      if (!tp)
        {
          g_warning (G_STRLOC ": Unknown panel %s", name);
          return;
        }
    }

  /*
   * Make sure the failed flag is unset, in case a panel just took too long to
   * start.
   */
  if (tp->failed)
    {
      g_message (G_STRLOC ": Previously failed panel '%s' appeared", name);
      tp->failed = FALSE;
    }

  if (panel == tp->panel)
    return;

  /*
   * Disconnect this function from the "ready" signal. Instead, we connect a
   * handler later on that updates things if this signal is issued again.
   */
  g_signal_handlers_disconnect_by_func (panel,
                                        mnb_toolbar_append_panel, toolbar);

  /*
   * If the respective slot is already occupied, remove the old objects.
   */
  mnb_toolbar_dispose_of_panel (toolbar, tp, FALSE);

  g_signal_connect (panel, "show-completed",
                    G_CALLBACK(mnb_toolbar_dropdown_show_completed_partial_cb),
                    toolbar);
  g_signal_connect (panel, "show-begin",
                    G_CALLBACK(mnb_toolbar_panel_show_begin_cb),
                    toolbar);

  g_signal_connect (panel, "hide-completed",
                    G_CALLBACK (mnb_toolbar_dropdown_hide_completed_cb), toolbar);

  g_signal_connect (panel, "request-button-style",
                    G_CALLBACK (mnb_toolbar_panel_request_button_style_cb),
                    toolbar);

  g_signal_connect (panel, "request-button-state",
                    G_CALLBACK (mnb_toolbar_panel_request_button_state_cb),
                    toolbar);

  /* g_signal_connect (panel, "request-tooltip", */
  /*                   G_CALLBACK (mnb_toolbar_panel_request_tooltip_cb), */
  /*                   toolbar); */

  g_signal_connect (panel, "notify::modal",
                    G_CALLBACK (mnb_toolbar_panel_notify_modal_cb),
                    toolbar);

  g_signal_connect (panel, "remote-process-died",
                    G_CALLBACK (mnb_toolbar_panel_died_cb), toolbar);

  tp->panel = panel;

  if (tp->button)
    mnb_panel_set_button (panel, MX_BUTTON (tp->button));

  if (mnb_panel_oop_is_ready (MNB_PANEL_OOP (panel)))
    mnb_toolbar_panel_ready_cb (panel, toolbar);
  else
    g_signal_connect (panel, "ready",
                      G_CALLBACK (mnb_toolbar_panel_ready_cb), toolbar);
}

static void
mnb_toolbar_init (MnbToolbar *self)
{
  MnbToolbarPrivate *priv;
  guint32            flags = dawati_netbook_get_compositor_option_flags ();

  priv = self->priv = MNB_TOOLBAR_GET_PRIVATE (self);

  if (flags & MNB_OPTION_DISABLE_PANEL_RESTART)
    priv->no_autoloading = TRUE;
}

static DBusGConnection *
mnb_toolbar_connect_to_dbus (MnbToolbar *self)
{
  MnbToolbarPrivate *priv = self->priv;
  DBusGConnection   *conn;
  DBusGProxy        *proxy;
  GError            *error = NULL;
  guint              status;

  conn = dbus_g_bus_get (DBUS_BUS_SESSION, &error);

  if (!conn)
    {
      g_warning ("Cannot connect to DBus: %s", error->message);
      g_error_free (error);
      return NULL;
    }

  proxy = dbus_g_proxy_new_for_name (conn,
                                     DBUS_SERVICE_DBUS,
                                     DBUS_PATH_DBUS,
                                     DBUS_INTERFACE_DBUS);

  if (!proxy)
    {
      g_object_unref (conn);
      return NULL;
    }

  if (!org_freedesktop_DBus_request_name (proxy,
                                          MPL_TOOLBAR_DBUS_NAME,
                                          DBUS_NAME_FLAG_DO_NOT_QUEUE,
                                          &status, &error))
    {
      if (error)
        {
          g_warning ("%s: %s", __FUNCTION__, error->message);
          g_error_free (error);
        }
      else
        {
          g_warning ("%s: Unknown error", __FUNCTION__);
        }

      g_object_unref (conn);
      conn = NULL;
    }

  priv->dbus_proxy = proxy;

  dbus_g_proxy_add_signal (proxy, "NameOwnerChanged",
                           G_TYPE_STRING,
                           G_TYPE_STRING,
                           G_TYPE_STRING,
                           G_TYPE_INVALID);

  return conn;
}

/*
 * Creates OOP panel for the panel service as it appears on the bus
 */
static void
mnb_toolbar_handle_dbus_name (MnbToolbar *toolbar, const gchar *name)
{
  MnbToolbarPrivate *priv = toolbar->priv;
  MnbPanelOop       *panel;

  panel = mnb_panel_oop_new (name,
                             0,
                             STATUSBAR_HEIGHT,
                             priv->old_screen_width,
                             priv->old_screen_height);

  if (panel)
    {
      g_signal_connect (panel, "destroy",
                        G_CALLBACK (mnb_toolbar_panel_destroy_cb), toolbar);

      if (mnb_panel_oop_is_ready (panel))
        {
          mnb_toolbar_append_panel (toolbar, (MnbPanel*)panel);
        }
      else
        {
          priv->pending_panels =
            g_slist_prepend (priv->pending_panels, g_strdup (name));
          g_signal_connect_swapped (panel, "ready",
                                    G_CALLBACK (mnb_toolbar_append_panel),
                                    toolbar);
        }
    }
}

static void
mnb_toolbar_noc_cb (DBusGProxy  *proxy,
                    const gchar *name,
                    const gchar *old_owner,
                    const gchar *new_owner,
                    MnbToolbar  *toolbar)
{
  MnbToolbarPrivate *priv;
  GSList            *l;

  /*
   * Unfortunately, we get this for all name owner changes on the bus, so
   * return early.
   */
  if (!name || strncmp (name, MPL_PANEL_DBUS_NAME_PREFIX,
                        strlen (MPL_PANEL_DBUS_NAME_PREFIX)))
    return;

  priv = MNB_TOOLBAR (toolbar)->priv;

  if (!new_owner || !*new_owner)
    {
      /*
       * This is the case where a panel gone away; we can ignore it here,
       * as this gets handled nicely elsewhere.
       */
      return;
    }

  l = priv->pending_panels;
  while (l)
    {
      const gchar *my_name = l->data;

      if (!strcmp (my_name, name))
        {
          /* We are already handling this one */
          return;
        }

      l = l->next;
    }

  mnb_toolbar_handle_dbus_name (toolbar, name);
}

/*
 * Start panel service for the given panel, if necessary
 *
 * Returns FALSE if the panel is already present.
 */
static gboolean
mnb_toolbar_start_panel_service (MnbToolbar *toolbar, MnbToolbarPanel *tp)
{
  MnbToolbarPrivate *priv = toolbar->priv;

  if (tp->panel)
    return FALSE;

  if (!tp->service)
    {
      g_warning ("Panel %s does not provide service", tp->name);
      return FALSE;
    }

  mnb_toolbar_ping_panel_oop (priv->dbus_conn, tp->service);

  return TRUE;
}

#if 0
static gboolean
mnb_toolbar_autostart_panels_cb (gpointer toolbar)
{
  static gint count = 0;

  MnbToolbarPrivate  *priv = MNB_TOOLBAR (toolbar)->priv;
  gboolean            missing = FALSE;
  GList              *l;

  for (l = priv->panels; l; l = l->next)
    {
      MnbToolbarPanel *tp = l->data;

      if (!tp || tp->unloaded)
        continue;

      if (!tp->panel && tp->service)
        {
          missing = mnb_toolbar_start_panel_service (toolbar, tp);

          if (count > TOOLBAR_AUTOSTART_ATTEMPTS)
            {
              g_warning ("Panel %s is still not running after %d "
                         "attempts to start it, last attempt.",
                         tp->name, count);
            }
        }
    }

  if (!missing || count > TOOLBAR_AUTOSTART_ATTEMPTS)
    return FALSE;

  count++;

  return TRUE;
}
#endif

static void
mnb_toolbar_dbus_list_names_cb (DBusGProxy  *proxy,
                                char       **names,
                                GError      *error,
                                gpointer     data)
{
  MnbToolbar         *toolbar = MNB_TOOLBAR (data);
  MnbToolbarPrivate  *priv    = toolbar->priv;

  if (!priv->dbus_conn || !priv->dbus_proxy)
    {
      g_warning ("DBus connection not available, cannot start panels !!!");
      return;
    }

  /*
   * Insert panels for any services already running.
   */
  if (!error)
    {
      gchar **p = names;
      while (*p)
        {
          if (!strncmp (*p, MPL_PANEL_DBUS_NAME_PREFIX,
                        strlen (MPL_PANEL_DBUS_NAME_PREFIX)))
            {
              gboolean  has_owner = FALSE;

              if (org_freedesktop_DBus_name_has_owner (priv->dbus_proxy,
                                                       *p, &has_owner, NULL) &&
                  has_owner)
                {
                  MnbToolbarPanel *tp;

                  tp = mnb_toolbar_panel_service_to_panel_internal (toolbar,*p);

                  if (tp)
                    mnb_toolbar_handle_dbus_name (toolbar, *p);
                }
            }

          p++;
        }
    }
  else
    {
      g_warning (G_STRLOC " Initial panel setup failed: %s",
                 error->message);
      g_error_free (error);
    }

  if (names)
    dbus_free_string_array (names);

  dbus_g_proxy_connect_signal (priv->dbus_proxy, "NameOwnerChanged",
                               G_CALLBACK (mnb_toolbar_noc_cb),
                               toolbar, NULL);
}


/*
 * Create panels for any of our services that are already up.
 */
static void
mnb_toolbar_dbus_setup_panels (MnbToolbar *toolbar)
{
  MnbToolbarPrivate  *priv = toolbar->priv;

  if (!priv->dbus_conn || !priv->dbus_proxy)
    {
      g_warning ("DBus connection not available, cannot start panels !!!");
      return;
    }

  /*
   * Insert panels for any services already running. Like everything else,
   * do this asynchronously to avoid blocking the WM.
   */
  org_freedesktop_DBus_list_names_async (priv->dbus_proxy,
                                         mnb_toolbar_dbus_list_names_cb,
                                         toolbar);
}

/*
 * If the compositor restacks, and we are showing an OOP panel, we need to
 * lower the shadow below the panel.
 */
static void
mnb_toolbar_screen_restacked_cb (MetaScreen *screen, MnbToolbar *toolbar)
{
  MnbPanel *panel;

  panel = mnb_toolbar_get_active_panel (toolbar);

  if (!panel || !MNB_IS_PANEL_OOP (panel))
    return;

  mnb_toolbar_raise_lowlight_for_panel (toolbar, panel);
}

static void
mnb_toolbar_setup_panels (MnbToolbar *toolbar)
{
  /*
   * Set up panels from gconf watch for changes
   */
  mnb_toolbar_setup_gconf (toolbar);

  mnb_toolbar_dbus_setup_panels (toolbar);
}

static gboolean
mnb_toolbar_lowlight_button_press_cb (ClutterActor *lowlight,
                                      ClutterEvent *event,
                                      MnbToolbar   *toolbar)
{
  if (CLUTTER_ACTOR_IS_MAPPED (toolbar))
    {
      MnbPanel *panel = mnb_toolbar_get_active_panel (toolbar);

      if (!panel || !MNB_IS_PANEL_OOP (panel))
        return FALSE;

      /*
       * If there are no singificant windows, we do not hide the toolbar (we
       * do not hide the panel either, the user must explicitely switch to a
       * different panel).
       */
      /* if (mnb_toolbar_check_for_windows (toolbar)) */
      /*   mnb_panel_hide_with_toolbar (panel, MNB_SHOW_HIDE_BY_MOUSE); */

      return TRUE;
    }

  return FALSE;
}

/* static void */
/* mnb_toolbar_set_struts (MnbToolbar *toolbar) */
/* { */
/*   MnbToolbarPrivate *priv         = toolbar->priv; */
/*   MetaPlugin        *plugin       = priv->plugin; */
/*   gboolean           netbook_mode = dawati_netbook_use_netbook_mode (plugin); */

/*   /\* */
/*    * When not in netbook mode, we need to reserve space for the toolbar. */
/*    *\/ */
/*   if (!netbook_mode && !priv->struts_set) */
/*     { */
/*       dawati_netbook_set_struts (plugin, -1, -1, TOOLBAR_HEIGHT, -1); */
/*       priv->struts_set = TRUE; */
/*     } */
/*   else if (netbook_mode && priv->struts_set) */
/*     { */
/*       dawati_netbook_set_struts (plugin, -1, -1, 0, -1); */
/*       priv->struts_set = FALSE; */
/*     } */
/* } */

static void
mnb_toolbar_constructed (GObject *self)
{
  MnbToolbarPrivate *priv = MNB_TOOLBAR (self)->priv;
  MetaPlugin        *plugin = priv->plugin;
  ClutterActor      *actor = CLUTTER_ACTOR (self);
  ClutterActor      *lowlight, *panel_stub;
  ClutterTexture    *sh_texture;
  gint               screen_width, screen_height;
  ClutterColor       low_clr = { 0, 0, 0, 0x7f };
  DBusGConnection   *conn;
  MetaScreen        *screen = meta_plugin_get_screen (plugin);
  ClutterActor      *wgroup = meta_get_window_group_for_screen (screen);

  /*
   * Make sure our parent gets chance to do what it needs to.
   */
  if (G_OBJECT_CLASS (mnb_toolbar_parent_class)->constructed)
    G_OBJECT_CLASS (mnb_toolbar_parent_class)->constructed (self);

  if ((conn = mnb_toolbar_connect_to_dbus (MNB_TOOLBAR (self))))
    {
      priv->dbus_conn = conn;
      dbus_g_connection_register_g_object (conn, MPL_TOOLBAR_DBUS_PATH, self);
    }
  else
    {
      g_warning (G_STRLOC " DBus connection not available !!!");
    }

  meta_plugin_query_screen_size (plugin,
                                   &priv->old_screen_width,
                                   &priv->old_screen_height);

  clutter_actor_set_reactive (actor, TRUE);

  priv->hbox_main = mnb_toolbar_background_new (MNB_TOOLBAR (self));
  clutter_actor_set_name (priv->hbox_main, "toolbar-main-box");
  mx_box_layout_set_orientation (MX_BOX_LAYOUT (priv->hbox_main),
                                 MX_ORIENTATION_HORIZONTAL);
  clutter_container_add_actor (CLUTTER_CONTAINER (self), priv->hbox_main);

  priv->hbox_buttons = mx_box_layout_new ();
  clutter_actor_set_name (priv->hbox_buttons, "toolbar-left-box");
  mx_box_layout_set_orientation (MX_BOX_LAYOUT (priv->hbox_buttons),
                                 MX_ORIENTATION_HORIZONTAL);

  priv->hbox_applets = mx_box_layout_new ();
  clutter_actor_set_name (priv->hbox_applets, "toolbar-right-box");
  mx_box_layout_set_orientation (MX_BOX_LAYOUT (priv->hbox_applets),
                                 MX_ORIENTATION_HORIZONTAL);

  g_object_set (self,
                "show-on-set-parent", FALSE,
                NULL);

  {
    MetaWorkspace *workspace = meta_screen_get_active_workspace (screen);

    meta_plugin_query_screen_size (plugin, &screen_width, &screen_height);

    if (workspace)
      {
        MetaRectangle  r;

        meta_workspace_get_work_area_all_monitors (workspace, &r);

        screen_height = r.y + r.height;
      }
  }

  priv->old_screen_width  = screen_width;
  priv->old_screen_height = screen_height;

  priv->max_panels = MAX_PANELS (screen_width);

  clutter_actor_set_size (priv->hbox_main, screen_width, TOOLBAR_HEIGHT);

  lowlight = clutter_rectangle_new_with_color (&low_clr);

  clutter_actor_set_size (lowlight, screen_width, screen_height);
  clutter_container_add_actor (CLUTTER_CONTAINER (wgroup), lowlight);
  clutter_actor_hide (lowlight);
  clutter_actor_set_reactive (lowlight, TRUE);

  g_signal_connect (lowlight, "button-press-event",
                    G_CALLBACK (mnb_toolbar_lowlight_button_press_cb),
                    self);

  priv->lowlight = lowlight;

  {
    ClutterActor *spinner = priv->spinner = mnb_spinner_new ();

    panel_stub = (ClutterActor*)mx_frame_new ();
    mx_bin_set_child (MX_BIN (panel_stub), spinner);

    clutter_actor_set_size (panel_stub,
                            screen_width, screen_height / 3);
    clutter_actor_set_position (panel_stub, 0, STATUSBAR_HEIGHT);
    clutter_actor_set_name (panel_stub, "panel-stub");
    clutter_container_add_actor (CLUTTER_CONTAINER (wgroup), panel_stub);
    clutter_actor_hide (panel_stub);
    mnb_spinner_stop ((MnbSpinner*)spinner);
    priv->panel_stub = panel_stub;
  }

  /*
   * The shadow needs to go into the window group, like the lowlight.
   */
  sh_texture = mx_texture_cache_get_texture (mx_texture_cache_get_default (),
                                             THEMEDIR
                                             "/toolbar/toolbar-shadow.png");
  if (sh_texture)
    {
      priv->shadow = mnb_toolbar_shadow_new (MNB_TOOLBAR (self),
                                             sh_texture,
                                             0, /* top */
                                             0, /* right */
                                             0, /* bottom */
                                             0  /* left */);
      clutter_actor_set_size (priv->shadow, screen_width, TOOLBAR_SHADOW_EXTRA);
      clutter_container_add_actor (CLUTTER_CONTAINER (self), priv->shadow);
      clutter_actor_lower (priv->shadow, priv->hbox_main);
      clutter_actor_set_y (priv->shadow, TOOLBAR_HEIGHT - 10);
      clutter_actor_hide (priv->shadow);
    }

  mx_box_layout_add_actor_with_properties (MX_BOX_LAYOUT (priv->hbox_main),
                                           priv->hbox_buttons,
                                           0,
                                           "expand", TRUE,
                                           "x-fill", FALSE,
                                           "x-align", MX_ALIGN_START,
                                           NULL);
  mx_box_layout_add_actor_with_properties (MX_BOX_LAYOUT (priv->hbox_main),
                                           priv->hbox_applets,
                                           1,
                                           "expand", FALSE,
                                           "x-fill", FALSE,
                                           "x-align", MX_ALIGN_END,
                                           NULL);

  /* mx_bin_set_alignment (MX_BIN (self), MX_ALIGN_START, MX_ALIGN_START); */
  /* mx_bin_set_child (MX_BIN (self), hbox); */

  /*
   * In netbook mode, we need hook to the captured signal to show/hide the
   * toolbar as required.
   */

  g_signal_connect (self, "event",
                    G_CALLBACK (mnb_toolbar_event_cb),
                    NULL);

  g_signal_connect (meta_plugin_get_stage (plugin),
                    "button-press-event",
                    G_CALLBACK (mnb_toolbar_stage_input_cb),
                    self);

  /* mnb_toolbar_set_struts (MNB_TOOLBAR (self)); */

  /*
   * Hook into "show" signal on stage, to set up input regions.
   * (We cannot set up the stage here, because the overlay window, etc.,
   * is not in place until the stage is shown.)
   */
  g_signal_connect (meta_plugin_get_stage (META_PLUGIN (plugin)),
                    "show", G_CALLBACK (mnb_toolbar_stage_show_cb),
                    self);

  mnb_toolbar_setup_panels (MNB_TOOLBAR (self));

  g_signal_connect (screen, "restacked",
                    G_CALLBACK (mnb_toolbar_screen_restacked_cb),
                    self);


#if TOOLBAR_CUT_OUT
  priv->selector_texture =
    cogl_texture_new_from_file (THEMEDIR "/toolbar/toolbar-selection.png",
                                COGL_TEXTURE_NO_SLICING, COGL_PIXEL_FORMAT_ANY,
                                NULL);

  low_clr.red = 0xff;
  low_clr.green = 0;
  low_clr.blue = 0;
  low_clr.alpha = 0x0;
  priv->light_spot = clutter_rectangle_new_with_color (&low_clr);
  clutter_actor_set_size (priv->light_spot,
                          cogl_texture_get_width (priv->selector_texture),
                          cogl_texture_get_height (priv->selector_texture));
  clutter_container_add_actor (CLUTTER_CONTAINER (self), priv->light_spot);
  clutter_actor_set_position (priv->light_spot,
                              -clutter_actor_get_width (priv->light_spot),
                              0);
  clutter_actor_show (priv->light_spot);

  g_signal_connect (priv->light_spot, "allocation-changed",
                    G_CALLBACK (mnb_toolbar_lightspot_moved_cb),
                    self);
#endif /* TOOLBAR_CUT_OUT */
}

ClutterActor*
mnb_toolbar_new (MetaPlugin *plugin)
{
  return g_object_new (MNB_TYPE_TOOLBAR,
                       "mutter-plugin", plugin, NULL);
}

static void
mnb_toolbar_activate_panel_internal (MnbToolbar        *toolbar,
                                     MnbToolbarPanel   *tp,
                                     MnbShowHideReason  reason)
{
  MnbToolbarPrivate *priv  = toolbar->priv;
  GList             *l;
  MnbPanel          *panel = tp->panel;

  g_return_if_fail (tp);

  if (panel && mnb_panel_is_mapped (panel))
    return;

  for (l = priv->panels; l; l = l->next)
    {
      MnbToolbarPanel *t = l->data;

      if (!t)
        continue;

      if (t->panel)
        {
          if (t != tp)
            {
              if (mnb_panel_is_mapped (t->panel))
                {
                  mnb_toolbar_set_waiting_for_panel_hide (toolbar, TRUE);
                  mnb_panel_hide (t->panel);
                }
            }
          else
            {
              mnb_toolbar_set_waiting_for_panel_show (toolbar, TRUE, TRUE);
              mnb_panel_show (t->panel);
            }
        }
      else if (t == tp && tp->button)
        {
          /*
           * Supposed to start panel that is not yet running.
           *
           * Toggle the Toolbar button.
           */
          if (!CLUTTER_ACTOR_IS_VISIBLE (toolbar))
            {
              priv->tp_to_activate = tp;
              mnb_toolbar_show (toolbar, reason);
            }
          else if (!mx_button_get_toggled (MX_BUTTON (tp->button)))
            {
              /*
               * We are not calling mnb_toolbar_show() since it is already
               * visible, but we need to set the reason for the show
               * to prevent the toolbar from hiding.
               */
              priv->reason_for_show = reason;
              mx_button_set_toggled (MX_BUTTON (tp->button), TRUE);
            }
        }
    }
}

void
mnb_toolbar_activate_panel (MnbToolbar        *toolbar,
                            const gchar       *panel_name,
                            MnbShowHideReason  reason)
{
  MnbToolbarPanel *tp = mnb_toolbar_panel_name_to_panel_internal (toolbar,
                                                                  panel_name);

  if (tp)
    mnb_toolbar_activate_panel_internal (toolbar, tp, reason);
}

void
mnb_toolbar_deactivate_panel (MnbToolbar *toolbar, const gchar *panel_name)
{
  MnbPanel        *panel;
  MnbToolbarPanel *tp = mnb_toolbar_panel_name_to_panel_internal (toolbar,
                                                                  panel_name);

  if (!tp || !tp->panel)
    return;

  panel = tp->panel;

  if (!mnb_panel_is_mapped (panel))
    return;

  mnb_panel_hide (panel);
}

void
mnb_toolbar_unload_panel (MnbToolbar *toolbar, const gchar *panel_name)
{
  MnbPanel        *panel;
  MnbToolbarPanel *tp = mnb_toolbar_panel_name_to_panel_internal (toolbar,
                                                                  panel_name);

  if (!tp || !tp->panel)
    return;

  panel = tp->panel;

  if (!MNB_IS_PANEL_OOP (panel))
    {
      g_warning ("Panel %s cannot be unloaded (only OOP panels can be).",
                 panel_name);
      return;
    }

  tp->unloaded = TRUE;

  mnb_panel_oop_unload ((MnbPanelOop*)panel);
}

void
mnb_toolbar_load_panel (MnbToolbar *toolbar, const gchar *panel_name)
{
  MnbToolbarPrivate *priv  = toolbar->priv;
  MnbToolbarPanel   *tp;
  gchar             *dbus_name;

  tp = mnb_toolbar_panel_name_to_panel_internal (toolbar, panel_name);

  if (!tp || !tp->panel)
    return;

  tp->unloaded = FALSE;

  dbus_name = g_strconcat (MPL_PANEL_DBUS_NAME_PREFIX, panel_name, NULL);

  mnb_toolbar_ping_panel_oop (priv->dbus_conn, dbus_name);

  g_free (dbus_name);
}

/* returns NULL if no panel active */
const gchar *
mnb_toolbar_get_active_panel_name (MnbToolbar *toolbar)
{
  MnbToolbarPrivate *priv  = toolbar->priv;
  GList             *l;

  for (l = priv->panels; l; l = l->next)
    {
      MnbToolbarPanel *tp = l->data;

      if (tp && tp->panel && mnb_panel_is_mapped (tp->panel))
        return tp->name;
    }

  return NULL;
}

/* Are we animating in */
gboolean
mnb_toolbar_in_show_transition (MnbToolbar *toolbar)
{
  MnbToolbarPrivate *priv = toolbar->priv;

  return priv->in_show_animation;
}

/* Are we animating in or out */
gboolean
mnb_toolbar_in_transition (MnbToolbar *toolbar)
{
  MnbToolbarPrivate *priv = toolbar->priv;

  return (priv->in_show_animation || priv->in_hide_animation);
}

/*
 * Sets a flag indicating whether the toolbar should hide when the pointer is
 * outside of the toolbar input zone -- this is the normal behaviour, but
 * needs to be disabled, for example, when the toolbar is opened using the
 * kbd shortcut.
 *
 * This flag gets cleared automatically when the panel is hidden.
 */
void
mnb_toolbar_set_dont_autohide (MnbToolbar *toolbar, gboolean dont)
{
  MnbToolbarPrivate *priv = toolbar->priv;

  priv->dont_autohide = dont;
}

/*
 * Machinery for showing and hiding the panel in response to pointer position.
 */

/*
 * The timeout callback that hide the toolbar if the pointer has left
 * the toolbar.
 */
static gboolean
mnb_toolbar_hide_timeout_cb (gpointer data)
{
  MnbToolbar *toolbar = MNB_TOOLBAR (data);

  mnb_toolbar_hide (toolbar, MNB_SHOW_HIDE_BY_MOUSE);
  toolbar->priv->hide_timeout_id = 0;

  return FALSE;
}

/*
 * Callback for ClutterStage::captured-event singal.
 *
 * Processes CLUTTER_ENTER and CLUTTER_LEAVE events and shows/hides the
 * panel as required.
 */
static gboolean
mnb_toolbar_event_cb (MnbToolbar *toolbar,
                      ClutterEvent *event,
                      gpointer      data)
{
  MnbToolbarPrivate *priv    = toolbar->priv;

  if (event->type == CLUTTER_ENTER)
    {
      if (priv->hide_timeout_id)
        {
          g_source_remove (priv->hide_timeout_id);
          priv->hide_timeout_id = 0;
        }
    }
  else if (event->type == CLUTTER_LEAVE)
    {
      if (priv->hide_timeout_id)
        {
          g_source_remove (priv->hide_timeout_id);
          priv->hide_timeout_id = 0;
        }

      priv->hide_timeout_id =
        g_timeout_add (TOOLBAR_HIDE_TIMEOUT,
                       mnb_toolbar_hide_timeout_cb, toolbar);
    }

  return FALSE;
}

/*
 * Handles ButtonPress events on stage.
 *
 * Used to hide the toolbar if the user clicks directly on stage.
 */
static gboolean
mnb_toolbar_stage_input_cb (ClutterActor *stage,
                            ClutterEvent *event,
                            gpointer      data)
{
  MnbToolbar *toolbar = MNB_TOOLBAR (data);

  if (event->type == CLUTTER_BUTTON_PRESS)
    {
      if (mnb_toolbar_in_transition (toolbar))
        return FALSE;

      if (!CLUTTER_ACTOR_IS_MAPPED (toolbar))
        return FALSE;

      /*
       * If there are no singificant windows, we do not hide the toolbar (we
       * do not hide the panel either, the user must explicitely switch to a
       * different panel).
       */
      if (mnb_toolbar_check_for_windows (toolbar))
        mnb_toolbar_hide (toolbar, MNB_SHOW_HIDE_BY_MOUSE);
    }

  return FALSE;
}

static void
mnb_toolbar_ensure_size_for_screen (MnbToolbar *toolbar)
{
  MnbToolbarPrivate *priv      = toolbar->priv;
  MetaScreen        *screen    = meta_plugin_get_screen (priv->plugin);
  MetaWorkspace     *workspace = meta_screen_get_active_workspace (screen);
  gint               screen_width, screen_height;
  gboolean           netbook_mode;
  gint               width = priv->old_screen_width;

  netbook_mode = dawati_netbook_use_netbook_mode (priv->plugin);

  meta_plugin_query_screen_size (priv->plugin, &screen_width, &screen_height);

  if (workspace)
    {
      MetaRectangle  r;

      meta_workspace_get_work_area_all_monitors (workspace, &r);

      screen_height = r.y + r.height;
    }

  if (priv->old_screen_width  == screen_width &&
      priv->old_screen_height == screen_height)
    {
      return;
    }

  /*
   * Ensure that the handler for showing Toolbar in small-screen mode is
   * (dis)connected as appropriate.
   */
  if (netbook_mode && !priv->trigger_cb_id)
    {
      /* priv->trigger_cb_id = */
      /*   g_signal_connect (meta_plugin_get_stage (priv->plugin), */
      /*                     "captured-event", */
      /*                     G_CALLBACK (mnb_toolbar_stage_captured_cb), */
      /*                     toolbar); */
    }
  else if (!netbook_mode && priv->trigger_cb_id)
    {
      g_signal_handler_disconnect (meta_plugin_get_stage (priv->plugin),
                                   priv->trigger_cb_id);
      priv->trigger_cb_id = 0;
    }

  /*
   * NB: This might trigger another change to the workspace area, but that
   *     one will become a no-op.
   */
  /* mnb_toolbar_set_struts (toolbar); */

  /*
   * First, handle only stuff that depends only on width changes.
   */
  if (priv->old_screen_width != screen_width)
    {
      MetaPlugin                *plugin = priv->plugin;
      DawatiNetbookPluginPrivate *ppriv = DAWATI_NETBOOK_PLUGIN (plugin)->priv;

      width = screen_width;

      priv->max_panels = MAX_PANELS (screen_width);

      /*
       * First of all, sort out the size of the Toolbar assets to match the
       * new screen.
       */
      clutter_actor_set_width ((ClutterActor*)toolbar, width);
      clutter_actor_set_width (priv->shadow, screen_width);

      /*
       * Now re-read the gconf key holding the panel order; this ensures that
       * all the panels that can be loaded are. This will also take care of
       * the panel positions.
       */
      gconf_client_notify (ppriv->gconf_client, KEY_ORDER);
    }

  clutter_actor_set_width (priv->hbox_main, screen_width);
  clutter_actor_set_width (priv->shadow, screen_width);
  clutter_actor_set_size (priv->lowlight, screen_width, screen_height);

  if (priv->input_region)
    mnb_toolbar_update_input (toolbar);

  priv->old_screen_width  = screen_width;
  priv->old_screen_height = screen_height;
}

static void
mnb_toolbar_workarea_changed_cb (MetaScreen *screen, MnbToolbar *toolbar)
{
  mnb_toolbar_ensure_size_for_screen (toolbar);
}

static void
mnb_toolbar_stage_allocation_cb (ClutterActor *stage,
                                 GParamSpec   *pspec,
                                 MnbToolbar   *toolbar)
{
  mnb_toolbar_ensure_size_for_screen (toolbar);

  /*
   * This is a one-off to deal with the inital stage allocation; screen size
   * changes are handled by the workarea_changed_cb.
   */
  g_signal_handlers_disconnect_by_func (stage,
                                        mnb_toolbar_stage_allocation_cb,
                                        toolbar);
}

static void
mnb_toolbar_alt_f2_key_handler (MetaDisplay    *display,
                                MetaScreen     *screen,
                                MetaWindow     *window,
                                XEvent         *event,
                                MetaKeyBinding *binding,
                                gpointer        data)
{
  MnbToolbar      *toolbar = MNB_TOOLBAR (data);
  MnbToolbarPanel *tp;

  if (dawati_netbook_urgent_notification_present (toolbar->priv->plugin))
    return;

  tp = mnb_toolbar_panel_name_to_panel_internal (toolbar,
                                                 "dawati-panel-applications");

  if (tp)
    mnb_toolbar_activate_panel_internal (toolbar, tp, MNB_SHOW_HIDE_BY_KEY);
}

/*
 * Callback for ClutterStage::show() signal.
 *
 * Carries out set up that can only be done once the stage is shown and the
 * associated X resources are in place.
 */
static void
mnb_toolbar_stage_show_cb (ClutterActor *stage, MnbToolbar *toolbar)
{
  MnbToolbarPrivate *priv = toolbar->priv;
  MetaPlugin        *plugin = priv->plugin;
  XWindowAttributes  attr;
  long               event_mask;
  Window             xwin;
  MetaScreen        *screen;
  Display           *xdpy;
  ClutterStage      *stg;
  MnbPanel          *myzone;

  xdpy   = meta_plugin_get_xdisplay (plugin);
  stg    = CLUTTER_STAGE (meta_plugin_get_stage (plugin));
  screen = meta_plugin_get_screen (plugin);

  /*
   * Set up the stage input region
   */

  /*
   * Make sure we are getting enter and leave events for stage (set up both
   * stage and overlay windows).
   */
  xwin       = clutter_x11_get_stage_window (stg);
  event_mask = EnterWindowMask | LeaveWindowMask;

  if (XGetWindowAttributes (xdpy, xwin, &attr))
    {
      event_mask |= attr.your_event_mask;
    }

  XSelectInput (xdpy, xwin, event_mask);

  xwin = meta_get_overlay_window (screen);
  event_mask = EnterWindowMask | LeaveWindowMask;

  if (XGetWindowAttributes (xdpy, xwin, &attr))
    {
      event_mask |= attr.your_event_mask;
    }

  XSelectInput (xdpy, xwin, event_mask);

  priv->shown = TRUE;

  /*
   * Show Myzone
   */
  if (!priv->shown_myzone &&
      (myzone = mnb_toolbar_panel_name_to_panel (toolbar, MPL_PANEL_MYZONE)))
    {
      /*
       * We can only do this if there are no modal windows showing otherwise
       * the zone will cover up the modal window; the user can then open another
       * panel, such as the applications, launch a new application, at which
       * point we switch to the new zone with the new application, but the modal
       * window on the original zone will still have focus, and the user will
       * have no idea why her kbd is not working. (Other than on startup this
       * should not be an issue, since will automatically hide the Shell when
       * the modal window pops up.
       */
      if (!dawati_netbook_modal_windows_present (plugin, -1))
        {
          priv->shown_myzone = TRUE;
          mnb_panel_show (myzone);
        }
    }

#if 0
  g_timeout_add_seconds (TOOLBAR_AUTOSTART_DELAY,
                         mnb_toolbar_autostart_panels_cb,
                         toolbar);
#endif

  g_signal_connect (stage, "notify::allocation",
                    G_CALLBACK (mnb_toolbar_stage_allocation_cb),
                    toolbar);

  /*
   * We need to be called last, after the handler in dawati-netbook.c has
   * fixed up the screen mode to either small-screen or bigger-screen.
   */
  g_signal_connect_after (screen,
                          "workareas-changed",
                          G_CALLBACK (mnb_toolbar_workarea_changed_cb),
                          toolbar);

  meta_keybindings_set_custom_handler ("panel_run_dialog",
                                       mnb_toolbar_alt_f2_key_handler,
                                       toolbar, NULL);
}

/*
 * Sets the disabled flag which indicates that the toolbar should not be
 * shown.
 *
 * Unlike the dont_autohide flag, this setting remains in force until
 * explicitely cleared.
 */
void
mnb_toolbar_set_disabled (MnbToolbar *toolbar, gboolean disabled)
{
  MnbToolbarPrivate *priv = toolbar->priv;

  priv->disabled = disabled;
}

MnbPanel *
mnb_toolbar_find_panel_for_xid (MnbToolbar *toolbar, guint xid)
{
  MnbToolbarPrivate *priv = toolbar->priv;
  GList             *l;

  for (l = priv->panels; l; l = l->next)
    {
      MnbToolbarPanel *tp = l->data;
      MnbPanel        *panel;

      if (!tp || !tp->panel || !MNB_IS_PANEL_OOP (tp->panel))
        continue;

      panel = tp->panel;

      if (xid == mnb_panel_oop_get_xid ((MnbPanelOop*)panel))
        {
          return panel;
        }
    }

  return NULL;
}

/*
 * Returns the active panel, or NULL, if no panel is active.
 */
MnbPanel *
mnb_toolbar_get_active_panel (MnbToolbar *toolbar)
{
  MnbToolbarPrivate *priv = toolbar->priv;
  GList             *l;

  if (!CLUTTER_ACTOR_IS_MAPPED (toolbar))
    return NULL;

  for (l = priv->panels; l; l = l->next)
    {
      MnbToolbarPanel *tp = l->data;

      if (tp && tp->panel && mnb_panel_is_mapped (tp->panel))
        return tp->panel;
    }

  return NULL;
}

gboolean
mnb_toolbar_is_waiting_for_panel (MnbToolbar *toolbar)
{
  MnbToolbarPrivate *priv = toolbar->priv;

  return (priv->waiting_for_panel_show || priv->waiting_for_panel_hide);
}

void
mnb_toolbar_foreach_panel (MnbToolbar        *toolbar,
                           MnbToolbarCallback callback,
                           gpointer           data)
{
  MnbToolbarPrivate *priv  = toolbar->priv;
  GList             *l;

  for (l = priv->panels; l; l = l->next)
    {
      MnbToolbarPanel *tp = l->data;

      if (tp && tp->panel)
        callback (tp->panel, data);
    }
}

gboolean
mnb_toolbar_owns_window (MnbToolbar *toolbar, MetaWindowActor *mcw)
{
  MnbToolbarPanel   *tp   =
    (MnbToolbarPanel *) mnb_toolbar_get_active_panel (toolbar);

  if (!mcw)
    return FALSE;

  if (tp && tp->panel && MNB_IS_PANEL_OOP (tp->panel))
    if (mnb_panel_oop_owns_window ((MnbPanelOop*)tp->panel, mcw))
      return TRUE;

  return FALSE;
}

/*
 * Creates MnbToolbarPanel object from a desktop file.
 *
 * desktop is the desktop file name without the .desktop prefix, which will be
 * loaded from standard location.
 */
static MnbToolbarPanel *
mnb_toolbar_make_panel_from_desktop (MnbToolbar *toolbar, const gchar *desktop)
{
  gchar           *path;
  GKeyFile        *kfile;
  GError          *error = NULL;
  MnbToolbarPanel *tp = NULL;

  path = g_strconcat (PANELSDIR, "/", desktop, ".desktop", NULL);

  kfile = g_key_file_new ();

  if (!g_key_file_load_from_file (kfile, path, G_KEY_FILE_NONE, &error))
    {
      g_warning ("Failed to load %s: %s", path, error->message);
      g_clear_error (&error);
    }
  else
    {
      gchar    *s;
      gboolean  b;
      gboolean  builtin = FALSE;

      tp = g_new0 (MnbToolbarPanel, 1);

      tp->name = g_strdup (desktop);

      b = g_key_file_get_boolean (kfile,
                                  G_KEY_FILE_DESKTOP_GROUP,
                                  "X-Dawati-Panel-Optional",
                                  &error);

      /*
       * If the key does not exist (i.e., there is an error), the panel is
       * not required
       */
      if (!error)
        tp->required = !b;
      else
        g_clear_error (&error);

      b = g_key_file_get_boolean (kfile,
                                  G_KEY_FILE_DESKTOP_GROUP,
                                  "X-Dawati-Panel-Windowless",
                                  &error);

      /*
       * If the key does not exist (i.e., there is an error), the panel is
       * not windowless
       */
      if (!error)
        tp->windowless = b;
      else
        g_clear_error (&error);

      s = g_key_file_get_locale_string (kfile,
                                        G_KEY_FILE_DESKTOP_GROUP,
                                        G_KEY_FILE_DESKTOP_KEY_NAME,
                                        NULL,
                                        NULL);

      tp->tooltip = s;

      s = g_key_file_get_string (kfile,
                                 G_KEY_FILE_DESKTOP_GROUP,
                                 "X-Dawati-Panel-Button-Style",
                                        NULL);

      if (!s)
        {
          /*
           * FIXME -- temporary hack
           */
          if (!strcmp (desktop, "dawati-panel-myzone"))
            tp->button_style = g_strdup ("myzone-button");
          else
            tp->button_style = g_strdup_printf ("%s-button", desktop);
        }
      else
        tp->button_style = s;

      s = g_key_file_get_string (kfile,
                                 G_KEY_FILE_DESKTOP_GROUP,
                                 "X-Dawati-Panel-Type",
                                 NULL);

      if (s)
        {
          if (!strcmp (s, "applet"))
            tp->type = MNB_TOOLBAR_PANEL_APPLET;
          else if (!strcmp (s, "clock"))
            tp->type = MNB_TOOLBAR_PANEL_CLOCK;

          g_free (s);
        }

      if (!builtin)
        {
          s = g_key_file_get_string (kfile,
                                     G_KEY_FILE_DESKTOP_GROUP,
                                     "X-Dawati-Panel-Stylesheet",
                                     NULL);

          tp->button_stylesheet = s;

          s = g_key_file_get_string (kfile,
                                     G_KEY_FILE_DESKTOP_GROUP,
                                     "X-Dawati-Service",
                                     NULL);

          tp->service = s;
        }

      /*
       * Sanity check
       */
      if (!builtin && !tp->service)
        {
          g_warning ("Panel desktop file %s did not contain dbus service",
                     desktop);

          mnb_toolbar_panel_destroy (tp);
          tp = NULL;
        }
    }

  g_key_file_free (kfile);

  return tp;
}

static gint
mnb_toolbar_find_panel_by_name (const MnbToolbarPanel *tp, const gchar *name)
{
  if (!tp || !tp->name)
    return -1;

  return strcmp (tp->name, name);
}

/*
 * Sorts out panels to match the new order, removing / adding any panels
 * in the process.
 *
 * the strings parameter identifies the list type; if TRUE, the list data are
 * strings, if FALSE they are GConfValues holding a string.
 */
static void
mnb_toolbar_fixup_panels (MnbToolbar *toolbar, GSList *order, gboolean strings)
{
  MnbToolbarPrivate *priv  = toolbar->priv;
  GSList            *l;
  GList             *panels = priv->panels;
  gint               n_panels = 0;

  for (l = order; l; l = l->next)
    {
      const gchar     *name;
      GList           *k;
      MnbToolbarPanel *tp;

      if (!strings)
        {
          GConfValue *value = l->data;

          name = gconf_value_get_string (value);
        }
      else
        name = l->data;

      k = g_list_find_custom (panels, name,
                              (GCompareFunc) mnb_toolbar_find_panel_by_name);

      if (!k)
        tp = mnb_toolbar_make_panel_from_desktop (toolbar, name);
      else
        tp = k->data;

      if (!tp)
        continue;

      tp->current = TRUE;

      /*
       * Move to the start of the list.
       */
      if (k)
        panels = g_list_delete_link (panels, k);

      panels = g_list_prepend (panels, tp);
    }

  priv->panels = panels;

  /*
   * Count how many panels we are trying to put on the toolbar.
   */
  while (panels)
    {
      MnbToolbarPanel *tp = panels->data;

      if (tp->type == MNB_TOOLBAR_PANEL_NORMAL && (tp->current || tp->required))
        ++n_panels;

      panels = panels->next;
    }

  panels = priv->panels;

  /*
   * Destroy any panels that are no longer current, or panels that do not
   * fit on the Toolbar, making sure we keep any required panels.
   */
  while (panels)
    {
      MnbToolbarPanel *tp = panels->data;
      MnbPanel        *panel;
      gboolean         regular;

      if (!tp)
        {
          panels = panels->next;
          continue;
        }

      /*
       * if the panel is either not a regular panel, or the current number of
       * panels we are trying to put on the toolbar is below the max thershold,
       * move on.
       */
      if ((tp->current || tp->required) &&
          (n_panels <= priv->max_panels ||
           tp->type != MNB_TOOLBAR_PANEL_NORMAL))
        {
          panels = panels->next;
          continue;
        }

      /*
       * If this is a regular panel that is required and we are over the max
       * threshold, we need to find some other panel to remove in its stead.
       */
      if (tp->type == MNB_TOOLBAR_PANEL_NORMAL &&
          tp->required && n_panels > priv->max_panels)
        {
          /*
           * Find some other panel to remove instead ... the panel list is
           * at this point in reverse order, so we look for the panel forward.
           */
          GList *k = panels->next;

          while (k)
            {
              MnbToolbarPanel *t = k->data;

              if (t->type == MNB_TOOLBAR_PANEL_NORMAL &&
                  !t->required && t->current)
                {
                  tp = t;
                  break;
                }

              k = k->next;
            }
        }

      panel = tp->panel;

      regular = (tp->type == MNB_TOOLBAR_PANEL_NORMAL);

      mnb_toolbar_dispose_of_button (toolbar, tp);
      mnb_toolbar_dispose_of_panel  (toolbar, tp, FALSE);

      priv->panels = g_list_remove (priv->panels, tp);
      mnb_toolbar_panel_destroy (tp);

      if (MNB_IS_PANEL_OOP (panel))
        mnb_panel_oop_unload ((MnbPanelOop*)panel);

      /*
       * Start from the beginning.
       */
      panels = priv->panels;

      if (regular)
        --n_panels;
    }

  panels = priv->panels;

  /*
   * Now move any required panels that were not on the list to the start.
   */
  while (panels)
    {
      MnbToolbarPanel *tp   = panels->data;
      GList           *next = panels->next;

      if (!tp->current)
        {
          tp->current = TRUE;

          priv->panels = g_list_remove (priv->panels, tp);
          priv->panels = g_list_prepend (priv->panels, tp);
        }

      panels = next;
    }

  priv->panels = g_list_reverse (priv->panels);

  /*
   * Now fix up the button positions; also clear the current flag on the
   * remaining panels
   */
  for (panels = priv->panels; panels; panels = panels->next)
    {
      MnbToolbarPanel *tp = panels->data;

      if (!tp)
        continue;

      if (tp->current && !tp->button)
        mnb_toolbar_append_button (toolbar, tp);

      tp->current = FALSE;

      mnb_toolbar_ensure_button_position (toolbar, tp);
    }
}

static void
mnb_toolbar_panel_gconf_key_changed_cb (GConfClient *client,
                                        guint        cnxn_id,
                                        GConfEntry  *entry,
                                        gpointer     data)
{
  MnbToolbar  *toolbar = MNB_TOOLBAR (data);
  GConfValue  *value;
  const gchar *key;

  key = gconf_entry_get_key (entry);

  if (!key)
    {
      g_warning (G_STRLOC ": no key!");
      return;
    }

  if (!strcmp (key, KEY_ORDER))
    {
      GSList *order;

      value = gconf_entry_get_value (entry);

      if (value)
        {
          if (value->type != GCONF_VALUE_LIST)
            {
              g_warning (G_STRLOC ": %s does not contain a list!", KEY_ORDER);
              return;
            }

          if (gconf_value_get_list_type (value) != GCONF_VALUE_STRING)
            {
              g_warning (G_STRLOC ": %s list does not contain strings!", KEY_ORDER);
              return;
            }

          order = gconf_value_get_list (value);

          mnb_toolbar_fixup_panels (toolbar, order, FALSE);
        }
      else
        {
          GSList *order;

          g_warning (G_STRLOC ": no value for key %s !", key);

          order = mnb_toolbar_get_default_panel_list ();

          mnb_toolbar_fixup_panels (toolbar, order, TRUE);

          g_slist_free (order);
        }
      return;
    }

  g_warning (G_STRLOC ": Unknown key %s", key);
}

static void
mnb_toolbar_load_gconf_settings (MnbToolbar *toolbar)
{
  MnbToolbarPrivate          *priv   = toolbar->priv;
  MetaPlugin                 *plugin = priv->plugin;
  DawatiNetbookPluginPrivate *ppriv  = DAWATI_NETBOOK_PLUGIN (plugin)->priv;
  GConfClient                *client = ppriv->gconf_client;
  GSList                     *order;
  GSList                     *l, *defaults;

  order = gconf_client_get_list (client, KEY_ORDER, GCONF_VALUE_STRING, NULL);

  defaults = mnb_toolbar_get_default_panel_list ();

  l = defaults;
  while (l)
    {
      GSList *e = g_slist_find_custom (order,
                                       l->data,
                                       (GCompareFunc) g_strcmp0);

      if (!e)
        order = g_slist_append (order, g_strdup (l->data));

      l = l->next;
    }

  mnb_toolbar_fixup_panels (toolbar, order, TRUE);

  /*
   * Clean up
   */
  g_slist_free (defaults);
  g_slist_foreach (order, (GFunc) g_free, NULL);
  g_slist_free (order);
}

static void
mnb_toolbar_setup_gconf (MnbToolbar *toolbar)
{
  MnbToolbarPrivate          *priv   = toolbar->priv;
  MetaPlugin                 *plugin = priv->plugin;
  DawatiNetbookPluginPrivate  *ppriv  = DAWATI_NETBOOK_PLUGIN (plugin)->priv;
  GConfClient                *client = ppriv->gconf_client;
  GError                     *error  = NULL;

  gconf_client_add_dir (client,
                        KEY_DIR,
                        GCONF_CLIENT_PRELOAD_NONE,
                        &error);

  if (error)
    {
      g_warning (G_STRLOC ": Error when adding directory for notification: %s",
                 error->message);
      g_clear_error (&error);
    }

  gconf_client_notify_add (client,
                           KEY_DIR,
                           mnb_toolbar_panel_gconf_key_changed_cb,
                           toolbar,
                           NULL,
                           &error);

  mnb_toolbar_load_gconf_settings (toolbar);
}

CoglHandle
mnb_toolbar_get_selector_texture (MnbToolbar *toolbar)
{
  g_return_val_if_fail (MNB_IS_TOOLBAR (toolbar), COGL_INVALID_HANDLE);

  return toolbar->priv->selector_texture;
}

void
mnb_toolbar_get_selector_allocation_box (MnbToolbar      *toolbar,
                                         ClutterActorBox *box)
{
  g_return_if_fail (MNB_IS_TOOLBAR (toolbar));
  g_return_if_fail (box != NULL);

  clutter_actor_get_allocation_box (toolbar->priv->light_spot, box);
}
