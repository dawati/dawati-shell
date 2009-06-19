/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */

/* mnb-toolbar.c */
/*
 * Copyright (c) 2009 Intel Corp.
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

#include <glib/gi18n.h>

#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-bindings.h>
#include <dbus/dbus-glib-lowlevel.h>
#include <dbus/dbus.h>
#include <moblin-panel/mpl-panel-common.h>

#include "moblin-netbook.h"

#include "mnb-toolbar.h"
#include "mnb-toolbar-button.h"
#include "mnb-drop-down.h"
#include "mnb-switcher.h"

/* for the launching API;
 * TODO -- refactor & namespace that
 */
#include "moblin-netbook-chooser.h"

/* TODO -- remove these after multiprocing */
#include "penge/penge-grid-view.h"
#include "moblin-netbook-status.h"
#include "moblin-netbook-netpanel.h"
#include "moblin-netbook-pasteboard.h"

#ifdef USE_AHOGHILL
#include "ahoghill/ahoghill-grid-view.h"
#endif

#define BUTTON_WIDTH 66
#define BUTTON_HEIGHT 55
#define BUTTON_SPACING 10

#define TRAY_PADDING   3
#define TRAY_BUTTON_HEIGHT 55
#define TRAY_BUTTON_WIDTH 44

#define TOOLBAR_TRIGGER_THRESHOLD       1
#define TOOLBAR_TRIGGER_THRESHOLD_TIMEOUT 300
#define TOOLBAR_LOWLIGHT_FADE_DURATION 300

#if 0
/*
 * TODO
 * This is currently define in moblin-netbook.h, as it is needed by the
 * tray manager and MnbDropDown -- this should not be hardcoded, and we need
 * a way for the drop down to query it from the panel.
 */
#define TOOLBAR_HEIGHT 64
#endif

#define TOOLBAR_SHADOW_HEIGHT (TOOLBAR_HEIGHT + 37)

#define TOOLBAR_X_PADDING 4

G_DEFINE_TYPE (MnbToolbar, mnb_toolbar, NBTK_TYPE_BIN)

#define MNB_TOOLBAR_GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MNB_TYPE_TOOLBAR, MnbToolbarPrivate))

static void mnb_toolbar_constructed (GObject *self);
static void mnb_toolbar_hide (ClutterActor *actor);
static void mnb_toolbar_show (ClutterActor *actor);
static gboolean mnb_toolbar_stage_captured_cb (ClutterActor *stage,
                                               ClutterEvent *event,
                                               gpointer      data);
static gboolean mnb_toolbar_stage_input_cb (ClutterActor *stage,
                                            ClutterEvent *event,
                                            gpointer      data);
static void mnb_toolbar_stage_show_cb (ClutterActor *stage,
                                       MnbToolbar *toolbar);
static void mnb_toolbar_setup_kbd_grabs (MnbToolbar *toolbar);
static void mnb_toolbar_handle_dbus_name (MnbToolbar *, const gchar *);

enum {
    MYZONE = 0,
    STATUS_ZONE,
    PEOPLE_ZONE,
    INTERNET_ZONE,
    MEDIA_ZONE,
    PASTEBOARD_ZONE,
    APPS_ZONE,
    SPACES_ZONE,

    APPLETS_START,

    /* Below here are the applets -- with the new dbus API, these are
     * just extra panels, only the buttons are slightly different in size.
     */
    WIFI_APPLET = APPLETS_START,
    VOLUME_APPLET,
    BATTERY_APPLET,
    BT_APPLET,
    TEST_APPLET,
    /* LAST */
    NUM_ZONES
};

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

struct _MnbToolbarPrivate
{
  MutterPlugin *plugin;

  ClutterActor *hbox; /* This is where all the contents are placed */
  ClutterActor *hint;
  ClutterActor *lowlight;

  NbtkWidget   *time; /* The time and date fields, needed for the updates */
  NbtkWidget   *date;

  NbtkWidget   *buttons[NUM_ZONES]; /* Buttons, one per zone & applet */
  NbtkWidget   *panels[NUM_ZONES];  /* Panels (the dropdowns) */

  ShellTrayManager *tray_manager;

  gboolean shown             : 1;
  gboolean shown_myzone      : 1;
  gboolean disabled          : 1;
  gboolean in_show_animation : 1; /* Animation tracking */
  gboolean in_hide_animation : 1;
  gboolean dont_autohide     : 1; /* Whether the panel should hide when the
                                   * pointer goes south
                                   */

  MnbInputRegion dropdown_region;
  MnbInputRegion trigger_region;  /* The show panel trigger region */
  MnbInputRegion input_region;    /* The panel input region on the region
                                   * stack.
                                   */

  guint trigger_timeout_id;

  gint panels_showing;

#if 1
  /* TODO remove */
  gboolean systray_window_showing;

  guint tray_xids [NUM_ZONES];
#endif

  DBusGConnection *dbus_conn;
  DBusGProxy      *dbus_proxy;

  GSList          *pending_panels;
};

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

  if (priv->dbus_conn)
    {
      g_object_unref (priv->dbus_conn);
      priv->dbus_conn = NULL;
    }

  if (priv->dropdown_region)
    {
      moblin_netbook_input_region_remove (priv->plugin, priv->dropdown_region);
      priv->dropdown_region = NULL;
    }

  if (priv->input_region)
    {
      moblin_netbook_input_region_remove (priv->plugin, priv->input_region);
      priv->input_region = NULL;
    }

  if (priv->trigger_region)
    {
      moblin_netbook_input_region_remove (priv->plugin, priv->trigger_region);
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
mnb_toolbar_show_completed_cb (ClutterTimeline *timeline, ClutterActor *actor)
{
  MnbToolbarPrivate *priv = MNB_TOOLBAR (actor)->priv;
  gint               i;

  for (i = 0; i < NUM_ZONES; ++i)
    if (priv->buttons[i])
      clutter_actor_set_reactive (CLUTTER_ACTOR (priv->buttons[i]), TRUE);

  priv->in_show_animation = FALSE;
  g_signal_emit (actor, toolbar_signals[SHOW_COMPLETED], 0);
  g_object_unref (actor);
}

static void
mnb_toolbar_show_lowlight (MnbToolbar *toolbar)
{
  ClutterActor *lowlight = toolbar->priv->lowlight;

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

static void
mnb_toolbar_show (ClutterActor *actor)
{
  MnbToolbarPrivate          *priv = MNB_TOOLBAR (actor)->priv;
  gint                        screen_width, screen_height;
  gint                        i;
  ClutterAnimation           *animation;

  if (priv->in_show_animation)
    {
      g_signal_stop_emission_by_name (actor, "show");
      return;
    }

  mnb_toolbar_show_lowlight (MNB_TOOLBAR (actor));

  mutter_plugin_query_screen_size (priv->plugin, &screen_width, &screen_height);

  /*
   * Show all of the buttons -- see comments in _hide_completed_cb() on why we
   * do this.
   */
  for (i = 0; i < NUM_ZONES; ++i)
    if (priv->buttons[i])
      {
        clutter_actor_show (CLUTTER_ACTOR (priv->buttons[i]));
        clutter_actor_set_reactive (CLUTTER_ACTOR (priv->buttons[i]), FALSE);
      }

  /*
   * Call the parent show(); this must be done before we do anything else.
   */
  CLUTTER_ACTOR_CLASS (mnb_toolbar_parent_class)->show (actor);

  /* set initial width and height */
  clutter_actor_set_position (actor, 0, -(clutter_actor_get_height (actor)));

  if (priv->input_region)
    moblin_netbook_input_region_remove_without_update (priv->plugin,
                                                       priv->input_region);

  priv->input_region =
    moblin_netbook_input_region_push (priv->plugin, 0, 0,
                                      screen_width, TOOLBAR_HEIGHT + 10);


  moblin_netbook_stash_window_focus (priv->plugin, CurrentTime);

  priv->in_show_animation = TRUE;

  /*
   * Start animation and wait for it to complete.
   */
  animation = clutter_actor_animate (actor, CLUTTER_LINEAR, 150, "y", 0.0, NULL);

  g_object_ref (actor);

  g_signal_connect (clutter_animation_get_timeline (animation),
                    "completed",
                    G_CALLBACK (mnb_toolbar_show_completed_cb),
                    actor);
}

static void
mnb_toolbar_hide_completed_cb (ClutterTimeline *timeline, ClutterActor *actor)
{
  MnbToolbarPrivate *priv = MNB_TOOLBAR (actor)->priv;
  gint i;

  /* the hide animation has finished, so now really hide the actor */
  CLUTTER_ACTOR_CLASS (mnb_toolbar_parent_class)->hide (actor);

  /*
   * We need to explicitely hide all the individual buttons, otherwise the
   * button tooltips will stay on screen.
   */
  for (i = 0; i < NUM_ZONES; ++i)
    if (priv->buttons[i])
      {
        clutter_actor_hide (CLUTTER_ACTOR (priv->buttons[i]));
        nbtk_button_set_checked (NBTK_BUTTON (priv->buttons[i]), FALSE);
      }

  priv->in_hide_animation = FALSE;
  priv->dont_autohide = FALSE;

  moblin_netbook_unstash_window_focus (priv->plugin, CurrentTime);

  g_signal_emit (actor, toolbar_signals[HIDE_COMPLETED], 0);
  g_object_unref (actor);
}

static void
mnb_toolbar_first_show_cb (MnbToolbar *toolbar, gpointer data)
{
  MnbToolbarPrivate *priv = toolbar->priv;

  if (priv->hint)
    {
      clutter_actor_destroy (priv->hint);
      priv->hint = NULL;

      /* one-of */
      g_signal_handlers_disconnect_by_func (toolbar,
                                            mnb_toolbar_first_show_cb,
                                            data);
    }
}

static void
mnb_toolbar_hide (ClutterActor *actor)
{
  static gint count = 0;

  MnbToolbarPrivate *priv = MNB_TOOLBAR (actor)->priv;
  gfloat             height;
  gint               i;
  ClutterAnimation  *animation;

  if (priv->in_hide_animation)
    {
      g_signal_stop_emission_by_name (actor, "hide");
      return;
    }

  mnb_toolbar_hide_lowlight (MNB_TOOLBAR (actor));

  /*
   * Show toolbar hint the very first time we are hidden.
   */
  if ((++count == 1) && priv->hint)
    {
      clutter_actor_show (priv->hint);
      g_signal_connect (actor, "show",
                        G_CALLBACK (mnb_toolbar_first_show_cb), NULL);
    }

  for (i = 0; i < NUM_ZONES; ++i)
    if (priv->buttons[i])
      clutter_actor_set_reactive (CLUTTER_ACTOR (priv->buttons[i]), FALSE);

  g_signal_emit (actor, toolbar_signals[HIDE_BEGIN], 0);

  if (priv->input_region)
    {
      moblin_netbook_input_region_remove (priv->plugin, priv->input_region);
      priv->input_region = NULL;
    }

  if (priv->dropdown_region)
    {
      moblin_netbook_input_region_remove (priv->plugin, priv->dropdown_region);
      priv->dropdown_region = NULL;
    }

  priv->in_hide_animation = TRUE;

  g_object_ref (actor);

  height = clutter_actor_get_height (actor);

  /*
   * Start animation and wait for it to complete.
   */
  animation = clutter_actor_animate (actor, CLUTTER_LINEAR, 150,
                                     "y", -height, NULL);

  g_signal_connect (clutter_animation_get_timeline (animation),
                    "completed",
                    G_CALLBACK (mnb_toolbar_hide_completed_cb),
                    actor);
}

static void
mnb_toolbar_allocate (ClutterActor          *actor,
                      const ClutterActorBox *box,
                      ClutterAllocationFlags flags)
{
  MnbToolbarPrivate *priv = MNB_TOOLBAR (actor)->priv;
  ClutterActorClass *parent_class;

  /*
   * The show and hide animations trigger allocations with origin_changed
   * set to TRUE; if we call the parent class allocation in this case, it
   * will force relayout, which we do not want. Instead, we call directly the
   * ClutterActor implementation of allocate(); this ensures our actor box is
   * correct, which is all we call about during the animations.
   *
   * If the drop down is not visible, we just return; this insures that the
   * needs_allocation flag in ClutterActor remains set, and the actor will get
   * reallocated when we show it.
   */
  if (!CLUTTER_ACTOR_IS_VISIBLE (actor))
    return;

  if (priv->in_show_animation || priv->in_hide_animation)
    {
      ClutterActorClass  *actor_class;

      actor_class = g_type_class_peek (CLUTTER_TYPE_ACTOR);

      if (actor_class)
        actor_class->allocate (actor, box, flags);

      return;
    }

  parent_class = CLUTTER_ACTOR_CLASS (mnb_toolbar_parent_class);
  parent_class->allocate (actor, box, flags);
}

static gboolean
mnb_toolbar_dbus_launch_application (MnbToolbar  *self,
                                     gchar       *path,
                                     gint         workspace,
                                     gboolean     no_chooser,
                                     GError     **error)
{
  return moblin_netbook_launch_application (path, workspace, no_chooser);
}

static gboolean
mnb_toolbar_dbus_launch_application_by_desktop_file (MnbToolbar  *self,
                                                     gchar       *desktop_file,
                                                     gchar       *arguments,
                                                     gint         workspace,
                                                     gboolean     no_chooser,
                                                     GError     **error)
{
  GList    *files = NULL;
  gboolean  retval;

  if (arguments && *arguments)
    files = g_list_prepend (files, arguments);

  retval =  moblin_netbook_launch_application_from_desktop_file (desktop_file,
                                                                 files,
                                                                 workspace,
                                                                 no_chooser);

  g_list_free (files);

  return retval;
}

static gboolean
mnb_toolbar_dbus_launch_default_application_for_uri (MnbToolbar  *self,
                                                     gchar       *uri,
                                                     gint         workspace,
                                                     gboolean     no_chooser,
                                                     GError     **error)
{
  return moblin_netbook_launch_default_for_uri (uri, workspace, no_chooser);
}

#include "../src/mnb-toolbar-dbus-glue.h"

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

  clutter_class->show = mnb_toolbar_show;
  clutter_class->hide = mnb_toolbar_hide;
  clutter_class->allocate = mnb_toolbar_allocate;

  dbus_g_object_type_install_info (G_TYPE_FROM_CLASS (klass),
                                   &dbus_glib_mnb_toolbar_dbus_object_info);

  g_object_class_install_property (object_class,
                                   PROP_MUTTER_PLUGIN,
                                   g_param_spec_object ("mutter-plugin",
                                                      "Mutter Plugin",
                                                      "Mutter Plugin",
                                                      MUTTER_TYPE_PLUGIN,
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

static gboolean
mnb_toolbar_update_time_date (MnbToolbarPrivate *priv)
{
  time_t         t;
  struct tm     *tmp;
  char           time_str[64];

  t = time (NULL);
  tmp = localtime (&t);
  if (tmp)
    strftime (time_str, 64, "%l:%M %P", tmp);
  else
    snprintf (time_str, 64, "Time");
  nbtk_label_set_text (NBTK_LABEL (priv->time), time_str);

  if (tmp)
    strftime (time_str, 64, "%B %e, %Y", tmp);
  else
    snprintf (time_str, 64, "Date");
  nbtk_label_set_text (NBTK_LABEL (priv->date), time_str);

  return TRUE;
}

/*
 * Toolbar button click handler.
 *
 * If the new button stage is 'checked' we show the asociated panel and hide
 * all others; in the oposite case, we hide the associated panel.
 */
static void
mnb_toolbar_toggle_buttons (NbtkButton *button, gpointer data)
{
  MnbToolbarPrivate *priv = MNB_TOOLBAR (data)->priv;
  gint               i;
  gboolean           checked;

  checked = nbtk_button_get_checked (button);

  for (i = 0; i < G_N_ELEMENTS (priv->buttons); i++)
    if ((priv->buttons[i] != (NbtkWidget*)button))
      {
        if (priv->buttons[i])
          nbtk_button_set_checked (NBTK_BUTTON (priv->buttons[i]), FALSE);
      }
    else
      {
        if (priv->panels[i])
          {
            if (checked && !CLUTTER_ACTOR_IS_MAPPED (priv->panels[i]))
              {
                clutter_actor_show (CLUTTER_ACTOR (priv->panels[i]));
              }
            else if (!checked && CLUTTER_ACTOR_IS_MAPPED (priv->panels[i]))
              {
                clutter_actor_hide (CLUTTER_ACTOR (priv->panels[i]));
              }
          }
      }
}

#if 1
#ifdef USE_AHOGHILL
/*
 * TODO -- this should be moved inside Ahoghill and the signal handlers
 * connected on "parent-set".
 */
static void
_media_drop_down_hidden (MnbDropDown      *drop_down,
                         AhoghillGridView *view)
{
  ahoghill_grid_view_clear (view);
  ahoghill_grid_view_unfocus (view);
}

static void
_media_drop_down_shown (MnbDropDown      *drop_down,
                        AhoghillGridView *view)
{
  ahoghill_grid_view_focus (view);
}
#endif

#ifdef WITH_NETPANEL
/*
 * TODO -- we might need dbus API for launching things to retain control over
 * the application workspace; investigate further.
 */
static void
_netgrid_launch_cb (MoblinNetbookNetpanel *netpanel,
                    const gchar           *url,
                    MnbToolbar            *toolbar)
{
  MutterPlugin *plugin = toolbar->priv->plugin;
  gchar *exec, *esc_url;
  gint workspace;

  /* FIXME: Should not be hard-coded? */
  esc_url = g_strescape (url, NULL);
  exec = g_strdup_printf ("%s \"%s\"", "moblin-web-browser", esc_url);

  workspace =
    meta_screen_get_active_workspace_index (mutter_plugin_get_screen (plugin));
  moblin_netbook_launch_application (exec, TRUE, workspace);

  g_free (exec);
  g_free (esc_url);

  clutter_actor_hide (CLUTTER_ACTOR (toolbar));
}
#endif

#endif /* REMOVE ME block */

static gint
mnb_toolbar_panel_instance_to_index (MnbToolbar *toolbar, MnbPanel *panel)
{
  MnbToolbarPrivate *priv = toolbar->priv;
  gint i;

  for (i = 0; i < NUM_ZONES; ++i)
    if ((void*)priv->panels[i] == (void*)panel)
      return i;

  return -1;
}

/*
 * Translates panel name to the corresponding enum value.
 *
 * Returns -1 if there is no match.
 *
 * TODO -- stuff all the strings into a single big array used by both this
 * and the reverse lookup function to avoid duplication.
 */
static gint
mnb_toolbar_panel_name_to_index (const gchar *name)
{
  gint index;

  if (!strcmp (name, MPL_PANEL_MYZONE))
    index = MYZONE;
  else if (!strcmp (name, MPL_PANEL_STATUS))
    index = STATUS_ZONE;
  else if (!strcmp (name, MPL_PANEL_ZONES))
    index = SPACES_ZONE;
  else if (!strcmp (name, MPL_PANEL_INTERNET))
    index = INTERNET_ZONE;
  else if (!strcmp (name, MPL_PANEL_MEDIA))
    index = MEDIA_ZONE;
  else if (!strcmp (name, MPL_PANEL_APPLICATIONS))
    index = APPS_ZONE;
  else if (!strcmp (name, MPL_PANEL_PEOPLE))
    index = PEOPLE_ZONE;
  else if (!strcmp (name, MPL_PANEL_PASTEBOARD))
    index = PASTEBOARD_ZONE;
  else if (!strcmp (name, MPL_PANEL_NETWORK))
    index = WIFI_APPLET;
  else if (!strcmp (name, MPL_PANEL_BLUETOOTH))
    index = BT_APPLET;
  else if (!strcmp (name, MPL_PANEL_VOLUME))
    index = VOLUME_APPLET;
  else if (!strcmp (name, MPL_PANEL_POWER))
    index = BATTERY_APPLET;
  else if (!strcmp (name, MPL_PANEL_TEST))
    index = TEST_APPLET;
  else
    {
      g_warning ("Unknown panel [%s]", name);
      index = -1;
    }

  return index;
}

static const gchar *
mnb_toolbar_panel_index_to_name (gint index)
{
  switch (index)
    {
    case MYZONE: return MPL_PANEL_MYZONE;
    case STATUS_ZONE: return MPL_PANEL_STATUS;
    case SPACES_ZONE: return MPL_PANEL_ZONES;
    case INTERNET_ZONE: return MPL_PANEL_INTERNET;
    case MEDIA_ZONE: return MPL_PANEL_MEDIA;
    case APPS_ZONE: return MPL_PANEL_APPLICATIONS;
    case PEOPLE_ZONE: return MPL_PANEL_PEOPLE;
    case PASTEBOARD_ZONE: return MPL_PANEL_PASTEBOARD;
    case WIFI_APPLET: return MPL_PANEL_NETWORK;
    case BT_APPLET: return MPL_PANEL_BLUETOOTH;
    case VOLUME_APPLET: return MPL_PANEL_VOLUME;
    case BATTERY_APPLET: return MPL_PANEL_POWER;
    case TEST_APPLET: return MPL_PANEL_TEST;

    default: return NULL;
    }
}

static void
mnb_toolbar_dropdown_show_completed_full_cb (MnbDropDown *dropdown,
                                             MnbToolbar  *toolbar)
{
  MnbToolbarPrivate *priv = toolbar->priv;
  MutterPlugin      *plugin = priv->plugin;
  gfloat             w, h;

  clutter_actor_get_transformed_size (CLUTTER_ACTOR (dropdown), &w, &h);

  if (priv->dropdown_region)
    moblin_netbook_input_region_remove_without_update (plugin,
                                                       priv->dropdown_region);

  priv->dropdown_region =
    moblin_netbook_input_region_push (plugin, 0, TOOLBAR_HEIGHT,
                                      (guint)w, (guint)h);

  moblin_netbook_stash_window_focus (plugin, CurrentTime);
}

static void
mnb_toolbar_dropdown_show_completed_partial_cb (MnbDropDown *dropdown,
                                                MnbToolbar  *toolbar)
{
  /*
   * TODO -- only the bottom panel should be added to the input region once
   * we do multiproc
   */
  MnbToolbarPrivate *priv = toolbar->priv;
  MutterPlugin      *plugin = priv->plugin;
  gfloat             x, y,w, h;

  mnb_drop_down_get_footer_geometry (dropdown, &x, &y, &w, &h);

  if (priv->dropdown_region)
    moblin_netbook_input_region_remove_without_update (plugin,
                                                       priv->dropdown_region);

  priv->dropdown_region =
    moblin_netbook_input_region_push (plugin,
                                      (gint)x, TOOLBAR_HEIGHT + (gint)y,
                                      (guint)w, (guint)h);

  priv->panels_showing++;
}

static void
mnb_toolbar_dropdown_hide_completed_cb (MnbDropDown *dropdown, MnbToolbar  *toolbar)
{
  MnbToolbarPrivate *priv = toolbar->priv;
  MutterPlugin      *plugin = priv->plugin;

  if (priv->dropdown_region)
    {
      moblin_netbook_input_region_remove (plugin, priv->dropdown_region);
      priv->dropdown_region = NULL;
    }

  priv->panels_showing--;

  if (priv->panels_showing < 0)
    {
      g_warning ("Error in panel state accounting, fixing.");
      priv->panels_showing = 0;
    }
}

/*
 * Appends a panel of the given name, using the given tooltip.
 *
 * This is a legacy function filling in the gap until we are ready to
 * switch to the dbus API and mnb_toolbar_append_panel().
 *
 */
void
mnb_toolbar_append_panel_old (MnbToolbar  *toolbar,
                              const gchar *name,
                              const gchar *tooltip)
{
  MnbToolbarPrivate *priv = toolbar->priv;
  MutterPlugin      *plugin = priv->plugin;
  NbtkWidget        *button;
  NbtkWidget        *panel = NULL;
  gint               screen_width, screen_height;
  gint               index;
  gchar             *button_style;

  index = mnb_toolbar_panel_name_to_index (name);

  if (index < 0)
    return;

  button_style = g_strdup_printf ("%s-button", name);

  /*
   * If the respective slot is already occupied, remove the old objects.
   */
  if (priv->buttons[index])
    {
      if (index == SPACES_ZONE)
        {
          /*
           * TODO
           * The spaces zone exposes some singnal handlers require to track
           * the focus order, and replacing it would be bit messy. For now
           * we simply do not allow this.
           */
          g_warning ("The Spaces Zone cannot be replaced\n");
          return;
        }

      clutter_container_remove_actor (CLUTTER_CONTAINER (priv->hbox),
                                      CLUTTER_ACTOR (priv->buttons[index]));
    }

  if (priv->panels[index])
    {
      if (index == SPACES_ZONE)
        {
          /*
           * BTW -- this code should not be reached; we should have exited
           * already in the button test.
           */
          g_warning ("The Spaces Zone cannot be replaced\n");
          return;
        }

      clutter_container_remove_actor (CLUTTER_CONTAINER (priv->hbox),
                                      CLUTTER_ACTOR (priv->panels[index]));
    }

  mutter_plugin_query_screen_size (plugin, &screen_width, &screen_height);

  /*
   * Create the button for this zone.
   */
  button = mnb_toolbar_button_new ();
  nbtk_button_set_toggle_mode (NBTK_BUTTON (button), TRUE);
  nbtk_widget_set_tooltip_text (NBTK_WIDGET (button), tooltip);
  clutter_actor_set_name (CLUTTER_ACTOR (button), button_style);
  g_free (button_style);

  /*
   * The button size and positioning depends on whether this is a regular
   * zone button, but one of the applet buttons.
   */
  if (index < APPLETS_START)
    {
      /*
       * Zone button
       */
      clutter_actor_set_size (CLUTTER_ACTOR (button),
                              BUTTON_WIDTH, BUTTON_HEIGHT);

      clutter_actor_set_position (CLUTTER_ACTOR (button),
                                  213 + (BUTTON_WIDTH * index)
                                  + (BUTTON_SPACING * index),
                                  TOOLBAR_HEIGHT - BUTTON_HEIGHT);

      mnb_toolbar_button_set_reactive_area (MNB_TOOLBAR_BUTTON (button),
                                            0,
                                            -(TOOLBAR_HEIGHT - BUTTON_HEIGHT),
                                            BUTTON_WIDTH,
                                            TOOLBAR_HEIGHT);
    }
  else
    {
      /*
       * Applet button.
       */
      gint applets = index - APPLETS_START;
      gint x, y;

      y = TOOLBAR_HEIGHT - TRAY_BUTTON_HEIGHT;
      x = screen_width - (applets + 1) * (TRAY_BUTTON_WIDTH + TRAY_PADDING);

      clutter_actor_set_size (CLUTTER_ACTOR (button),
                              TRAY_BUTTON_WIDTH, TRAY_BUTTON_HEIGHT);
      clutter_actor_set_position (CLUTTER_ACTOR (button), x, y);

      mnb_toolbar_button_set_reactive_area (MNB_TOOLBAR_BUTTON (button),
                                         0,
                                         -(TOOLBAR_HEIGHT - TRAY_BUTTON_HEIGHT),
                                         TRAY_BUTTON_WIDTH,
                                         TOOLBAR_HEIGHT);
    }

  g_signal_connect (button, "clicked",
                    G_CALLBACK (mnb_toolbar_toggle_buttons),
                    toolbar);

  /*
   * Special case Space; we have an internal component for this.
   *
   * TODO -- should we allow it to be replaced by a thirdparty component just
   * like all the other dropdowns?
   */
  if (index == SPACES_ZONE)
    {
      panel = priv->panels[index] = mnb_switcher_new (plugin);

      g_signal_connect (panel, "show-completed",
                        G_CALLBACK(mnb_toolbar_dropdown_show_completed_full_cb),
                        toolbar);
    }
  else
    {
      /*
       * TODO -- create the contents from the xid and insert into the dropdown
       */

#if 1
      /*
       * For now we load the internal components, so we can get the toolbar
       * to a working state before we move onto the the actual multiproc
       * separation.
       */
      {
      switch (index)
        {
          /*
           * TODO: The Status, Apps and Pasteboard provide MnbDropDown
           */
        case STATUS_ZONE:
          panel = priv->panels[index] = NBTK_WIDGET (
            make_status (plugin, screen_width - TOOLBAR_X_PADDING * 2));
          break;
        case APPS_ZONE:
          g_warning ("Skipping built-in apps panel");
#if 0
          panel = priv->panels[index] = NBTK_WIDGET (
            make_launcher (plugin,
                           screen_width - TOOLBAR_X_PADDING * 2,
                           screen_height - 2 * TOOLBAR_HEIGHT));
#endif
          break;
        case PASTEBOARD_ZONE:
          panel = priv->panels[index] = NBTK_WIDGET (
            make_pasteboard (plugin,
                             screen_width - TOOLBAR_X_PADDING * 2));
          break;
        case PEOPLE_ZONE:
          break;
        case MYZONE:
          break;
        case MEDIA_ZONE:
#ifdef USE_AHOGHILL
          {
            ClutterActor *grid;

            panel = priv->panels[index] = mnb_drop_down_new (plugin);

            grid = g_object_new (AHOGHILL_TYPE_GRID_VIEW, NULL);

            clutter_actor_set_height (grid,
                                      screen_height - TOOLBAR_HEIGHT * 1.5);

            mnb_drop_down_set_child (MNB_DROP_DOWN (panel), grid);

            g_signal_connect (panel, "hide-completed",
                              G_CALLBACK (_media_drop_down_hidden), grid);
            g_signal_connect (panel, "show-completed",
                              G_CALLBACK (_media_drop_down_shown), grid);
            g_signal_connect_swapped (grid, "dismiss",
                                  G_CALLBACK (mnb_drop_down_hide_with_toolbar),
                                  panel);
          }
          break;
#endif
        case INTERNET_ZONE:
#ifdef WITH_NETPANEL
          {
            ClutterActor *grid;

            panel = priv->panels[index] = mnb_drop_down_new (plugin);

            grid = CLUTTER_ACTOR (moblin_netbook_netpanel_new ());

            mnb_drop_down_set_child (MNB_DROP_DOWN (panel), grid);

            /*
             * FIME -- why is this necessary ?
             */
            g_signal_connect_swapped (toolbar, "hide",
                                      G_CALLBACK (clutter_actor_hide), grid);
            g_signal_connect_swapped (toolbar, "show",
                                      G_CALLBACK (clutter_actor_show), grid);

            g_signal_connect_swapped (panel, "hide-completed",
                                    G_CALLBACK (moblin_netbook_netpanel_clear),
                                    grid);
            g_signal_connect_swapped (panel, "show-completed",
                                    G_CALLBACK (moblin_netbook_netpanel_focus),
                                    grid);

            g_signal_connect (grid, "launch",
                              G_CALLBACK (_netgrid_launch_cb), toolbar);
            g_signal_connect_swapped (grid, "launched",
                                 G_CALLBACK (mnb_drop_down_hide_with_toolbar),
                                 panel);
          }
          break;
#endif

        default:
          g_warning ("Zone %s is currently not implemented", name);
        }
      }
#endif

      if (panel)
        g_signal_connect (panel, "show-completed",
                    G_CALLBACK(mnb_toolbar_dropdown_show_completed_full_cb),
                    toolbar);
    }

  if (!panel)
    {
      g_debug ("Panel %s is not available", name);
      clutter_actor_destroy (CLUTTER_ACTOR (button));
      return;
    }
  else
    {
      priv->buttons[index] = button;

      clutter_container_add_actor (CLUTTER_CONTAINER (priv->hbox),
                                   CLUTTER_ACTOR (button));
    }

  g_signal_connect (panel, "hide-completed",
                    G_CALLBACK(mnb_toolbar_dropdown_hide_completed_cb),
                    toolbar);

  clutter_container_add_actor (CLUTTER_CONTAINER (priv->hbox),
                               CLUTTER_ACTOR (panel));
  clutter_actor_set_width (CLUTTER_ACTOR (panel), screen_width);

  mnb_drop_down_set_button (MNB_DROP_DOWN (panel), NBTK_BUTTON (button));
  clutter_actor_set_position (CLUTTER_ACTOR (panel), 0, TOOLBAR_HEIGHT);
  clutter_actor_raise (CLUTTER_ACTOR (panel), priv->lowlight);
}

static void
mnb_toolbar_panel_request_button_style_cb (MnbPanel    *panel,
                                           const gchar *style_id,
                                           MnbToolbar  *toolbar)
{
  MnbToolbarPrivate *priv = toolbar->priv;
  gint index;

  index = mnb_toolbar_panel_instance_to_index (toolbar, panel);

  if (index < 0)
    return;

  clutter_actor_set_name (CLUTTER_ACTOR (priv->buttons[index]), style_id);
}

static void
mnb_toolbar_panel_request_tooltip_cb (MnbPanel    *panel,
                                      const gchar *tooltip,
                                      MnbToolbar  *toolbar)
{
  MnbToolbarPrivate *priv = toolbar->priv;
  gint index;

  index = mnb_toolbar_panel_instance_to_index (toolbar, panel);

  if (index < 0)
    return;

  if (priv->buttons[index])
    nbtk_widget_set_tooltip_text (priv->buttons[index], tooltip);
}

/*
 * Removes the button/panel pair from the toolbar, avoiding any recursion
 * due to "destroy" signal handler.
 *
 * The panel_destroyed parameter should be set to TRUE if the panel is known
 * to be in the destroy sequence.
 */
static void
mnb_toolbar_dispose_of_panel (MnbToolbar *toolbar,
                              gint        index,
                              gboolean    panel_destroyed)
{
  MnbToolbarPrivate *priv = toolbar->priv;
  NbtkWidget        *button;
  NbtkWidget        *panel;

  if (index < 0)
    return;

  button = priv->buttons[index];
  panel  = priv->panels[index];

  /*
   * We first disconnect any signal handlers from *both* the button and the
   * panel, and only then remove the actors. This avoids any recursion on
   * any "destroy" signals we might have, etc.
   */
  if (button)
    g_signal_handlers_disconnect_matched (button,
                                          G_SIGNAL_MATCH_DATA,
                                          0, 0, NULL, NULL,
                                          toolbar);

  if (panel)
    g_signal_handlers_disconnect_matched (panel,
                                          G_SIGNAL_MATCH_DATA,
                                          0, 0, NULL, NULL,
                                          toolbar);

  if (button)
    {
      priv->buttons[index] = NULL;
      clutter_container_remove_actor (CLUTTER_CONTAINER (priv->hbox),
                                      CLUTTER_ACTOR (button));
    }

  if (panel)
    {
      priv->panels[index] = NULL;

      if (!panel_destroyed)
        clutter_container_remove_actor (CLUTTER_CONTAINER (priv->hbox),
                                        CLUTTER_ACTOR (panel));
    }
}

static void
mnb_toolbar_panel_set_size_cb (MnbPanel   *panel,
                               guint       width,
                               guint       height,
                               MnbToolbar *toolbar)
{
  MnbToolbarPrivate *priv;
  MutterPlugin      *plugin;
  gfloat             x, y,w, h;

  /*
   * If this panel is visible, we need to update the input region to match
   * the new geometry.
   */
  if (!CLUTTER_ACTOR_IS_MAPPED (panel))
    return;

  priv = toolbar->priv;
  plugin = priv->plugin;

  mnb_drop_down_get_footer_geometry (MNB_DROP_DOWN (panel), &x, &y, &w, &h);

  if (priv->dropdown_region)
    moblin_netbook_input_region_remove_without_update (plugin,
                                                       priv->dropdown_region);

  priv->dropdown_region =
    moblin_netbook_input_region_push (plugin,
                                      (gint)x, TOOLBAR_HEIGHT + (gint)y,
                                      (guint)w, (guint)h);
}

static void
mnb_toolbar_panel_died_cb (MnbPanel *panel, MnbToolbar *toolbar)
{
  gint   index;
  gchar *name = NULL;

  index = mnb_toolbar_panel_instance_to_index (toolbar, panel);

  if (index >= 0)
    {
      /*
       * Get the panel name before we dispose of it.
       */
      name = g_strdup (mnb_panel_get_name (panel));
      mnb_toolbar_dispose_of_panel (toolbar, index, FALSE);
    }

  /*
   * Try to restart the service
   */

  if (name)
    {
      gchar *dbus_name = g_strconcat (MPL_PANEL_DBUS_NAME_PREFIX, name, NULL);

      mnb_toolbar_handle_dbus_name (toolbar, dbus_name);

      g_free (dbus_name);
      g_free (name);
    }
}

static void
mnb_toolbar_panel_ready_cb (MnbPanel *panel, MnbToolbar *toolbar)
{
  if (MNB_IS_PANEL (panel))
    {
      MnbToolbarPrivate *priv = toolbar->priv;
      NbtkWidget        *button;
      const gchar       *name;
      const gchar       *tooltip;
      const gchar       *style_id;
      const gchar       *stylesheet;
      gint               index;

      name = mnb_panel_get_name (panel);

      index = mnb_toolbar_panel_instance_to_index (toolbar, panel);

      if (index < 0)
        return;

      button = priv->buttons[index];

      tooltip    = mnb_panel_get_tooltip (panel);
      stylesheet = mnb_panel_get_stylesheet (panel);
      style_id   = mnb_panel_get_button_style (panel);

if (0 == g_strcmp0 ("applications-button", style_id))
  printf ("apps button\n");

      if (button)
        {
          gchar *button_style = NULL;

          if (stylesheet && *stylesheet)
            {
              GError    *error = NULL;
              NbtkStyle *style = nbtk_style_new ();

              if (!nbtk_style_load_from_file (style, stylesheet, &error))
                {
                  if (error)
                    g_warning ("Unable to load stylesheet %s: %s",
                               stylesheet, error->message);

                  g_error_free (error);
                }
              else
                nbtk_stylable_set_style (NBTK_STYLABLE (button), style);
            }

          if (!style_id || !*style_id)
            button_style = g_strdup_printf ("%s-button", name);

          nbtk_widget_set_tooltip_text (NBTK_WIDGET (button), tooltip);
          clutter_actor_set_name (CLUTTER_ACTOR (button),
                                  button_style ? button_style : style_id);

          g_free (button_style);
        }

    }
}

static void
mnb_toolbar_panel_destroy_cb (MnbPanel *panel, MnbToolbar *toolbar)
{
  gint index;

  if (MNB_IS_SWITCHER (panel))
    {
      g_warning ("Cannot remove the Switcher !!!");
      return;
    }

  if (!MNB_IS_PANEL (panel))
    {
      g_warning ("Unhandled panel type: %s", G_OBJECT_TYPE_NAME (panel));
      return;
    }

  index = mnb_toolbar_panel_instance_to_index (toolbar, panel);

  if (index < 0)
    return;

  mnb_toolbar_dispose_of_panel (toolbar, index, TRUE);
}

/*
 * Appends a panel
 */
static void
mnb_toolbar_append_panel (MnbToolbar  *toolbar, MnbDropDown *panel)
{
  MnbToolbarPrivate *priv = toolbar->priv;
  MutterPlugin      *plugin = priv->plugin;
  NbtkWidget        *button;
  gint               screen_width, screen_height;
  gint               index;
  gchar             *button_style = NULL;
  const gchar       *name;
  const gchar       *tooltip;
  const gchar       *stylesheet = NULL;
  const gchar       *style_id = NULL;
  GSList            *l;

  if (MNB_IS_PANEL (panel))
    {
      name       = mnb_panel_get_name (MNB_PANEL (panel));
      tooltip    = mnb_panel_get_tooltip (MNB_PANEL (panel));
      stylesheet = mnb_panel_get_stylesheet (MNB_PANEL (panel));
      style_id   = mnb_panel_get_button_style (MNB_PANEL (panel));
    }
  else if (MNB_IS_SWITCHER (panel))
    {
      name    = "zones";
      tooltip = _("zones");
    }
  else
    {
      g_warning ("Unhandled panel type: %s", G_OBJECT_TYPE_NAME (panel));
      return;
    }

  /*
   * Remove this panel from the pending list.
   */
  l = priv->pending_panels;
  while (l)
    {
      gchar *n = l->data;

      if (!strcmp (n, name))
        {
          g_free (n);
          priv->pending_panels = g_slist_delete_link (priv->pending_panels, l);
          break;
        }

      l = l->next;
    }

  index = mnb_toolbar_panel_name_to_index (name);

  if (index < 0)
    return;

  if ((void*)panel == (void*)priv->panels[index])
    return;

  /*
   * Disconnect this function from the "ready" signal. Instead, we connect a
   * handler later on that updates things if this signal is issued again.
   */
  g_signal_handlers_disconnect_by_func (panel,
                                        mnb_toolbar_append_panel, toolbar);

  if (!style_id || !*style_id)
    button_style = g_strdup_printf ("%s-button", name);

  /*
   * If the respective slot is already occupied, remove the old objects.
   */
  mnb_toolbar_dispose_of_panel (toolbar, index, FALSE);

  mutter_plugin_query_screen_size (plugin, &screen_width, &screen_height);

  /*
   * Create the button for this zone.
   */
  button = priv->buttons[index] = mnb_toolbar_button_new ();

  if (stylesheet && *stylesheet)
    {
      GError    *error = NULL;
      NbtkStyle *style = nbtk_style_new ();

      if (!nbtk_style_load_from_file (style, stylesheet, &error))
        {
          if (error)
            g_warning ("Unable to load stylesheet %s: %s",
                       stylesheet, error->message);

          g_error_free (error);
        }
      else
        nbtk_stylable_set_style (NBTK_STYLABLE (button), style);
    }

  nbtk_button_set_toggle_mode (NBTK_BUTTON (button), TRUE);
  nbtk_widget_set_tooltip_text (NBTK_WIDGET (button), tooltip);
  clutter_actor_set_name (CLUTTER_ACTOR (button),
                          button_style ? button_style : style_id);

  g_free (button_style);

  /*
   * The button size and positioning depends on whether this is a regular
   * zone button, but one of the applet buttons.
   */
  if (index < APPLETS_START)
    {
      /*
       * Zone button
       */
      clutter_actor_set_size (CLUTTER_ACTOR (button),
                              BUTTON_WIDTH, BUTTON_HEIGHT);

      clutter_actor_set_position (CLUTTER_ACTOR (button),
                                  213 + (BUTTON_WIDTH * index)
                                  + (BUTTON_SPACING * index),
                                  TOOLBAR_HEIGHT - BUTTON_HEIGHT);

      mnb_toolbar_button_set_reactive_area (MNB_TOOLBAR_BUTTON (button),
                                            0,
                                            -(TOOLBAR_HEIGHT - BUTTON_HEIGHT),
                                            BUTTON_WIDTH,
                                            TOOLBAR_HEIGHT);

      clutter_container_add_actor (CLUTTER_CONTAINER (priv->hbox),
                                   CLUTTER_ACTOR (button));
    }
  else
    {
      /*
       * Applet button.
       */
      gint applets = index - APPLETS_START;
      gint x, y;

      y = TOOLBAR_HEIGHT - TRAY_BUTTON_HEIGHT;
      x = screen_width - (applets + 1) * (TRAY_BUTTON_WIDTH + TRAY_PADDING);

      clutter_actor_set_size (CLUTTER_ACTOR (button),
                              TRAY_BUTTON_WIDTH, TRAY_BUTTON_HEIGHT);
      clutter_actor_set_position (CLUTTER_ACTOR (button),
                                  (gfloat)x, (gfloat)y);

      mnb_toolbar_button_set_reactive_area (MNB_TOOLBAR_BUTTON (button),
                                         0,
                                         -(TOOLBAR_HEIGHT - TRAY_BUTTON_HEIGHT),
                                         TRAY_BUTTON_WIDTH,
                                         TOOLBAR_HEIGHT);

      clutter_container_add_actor (CLUTTER_CONTAINER (priv->hbox),
                                   CLUTTER_ACTOR (button));
    }

  g_signal_connect (button, "clicked",
                    G_CALLBACK (mnb_toolbar_toggle_buttons), toolbar);

  g_signal_connect (panel, "show-completed",
                    G_CALLBACK(mnb_toolbar_dropdown_show_completed_partial_cb),
                    toolbar);

  g_signal_connect (panel, "hide-begin",
                    G_CALLBACK (mnb_toolbar_dropdown_hide_completed_cb), toolbar);

  g_signal_connect (panel, "request-button-style",
                    G_CALLBACK (mnb_toolbar_panel_request_button_style_cb),
                    toolbar);

  g_signal_connect (panel, "request-tooltip",
                    G_CALLBACK (mnb_toolbar_panel_request_tooltip_cb),
                    toolbar);

  g_signal_connect (panel, "destroy",
                    G_CALLBACK (mnb_toolbar_panel_destroy_cb), toolbar);

  g_signal_connect (panel, "ready",
                    G_CALLBACK (mnb_toolbar_panel_ready_cb), toolbar);

  g_signal_connect (panel, "remote-process-died",
                    G_CALLBACK (mnb_toolbar_panel_died_cb), toolbar);

  g_signal_connect (panel, "set-size",
                    G_CALLBACK (mnb_toolbar_panel_set_size_cb), toolbar);

  clutter_container_add_actor (CLUTTER_CONTAINER (priv->hbox),
                               CLUTTER_ACTOR (panel));

  priv->panels[index] = NBTK_WIDGET (panel);

  mnb_drop_down_set_button (MNB_DROP_DOWN (panel), NBTK_BUTTON (button));
  clutter_actor_set_position (CLUTTER_ACTOR (panel), TOOLBAR_X_PADDING, TOOLBAR_HEIGHT);
  clutter_actor_lower_bottom (CLUTTER_ACTOR (panel));
  clutter_actor_raise (CLUTTER_ACTOR (panel), priv->lowlight);

  if (index == MYZONE)
    {
      if (priv->shown)
        {
          clutter_actor_show (CLUTTER_ACTOR (panel));
          priv->shown_myzone = TRUE;
        }
    }
}

static void
mnb_toolbar_init (MnbToolbar *self)
{
  MnbToolbarPrivate *priv;

  priv = self->priv = MNB_TOOLBAR_GET_PRIVATE (self);
}

#if 1
/*
 * TODO -- the tray manager will only be used for the legacy System Tray icons;
 *         this should be moved into a separate MnbSystemTray component (
 *         MnbDropDown sublass), together with the tray manager object.
 *
 *         The regular applets will be added via mnb_toolbar_append_panel().
 */
static void
shell_tray_manager_icon_added_cb (ShellTrayManager *mgr,
                                  ClutterActor     *icon,
                                  guint             xid,
                                  MnbToolbar       *toolbar)
{
  MnbToolbarPrivate *priv = toolbar->priv;
  const gchar       *name;
  const gchar       *tooltip = NULL;
  gint               col = -1;
  gint               screen_width, screen_height;
  gint               x, y;
  gint               index;

  mutter_plugin_query_screen_size (priv->plugin, &screen_width, &screen_height);

  name = clutter_actor_get_name (icon);

  if (!name || !*name)
    return;

  index = mnb_toolbar_panel_name_to_index (name);

  if (index < 0)
    return;

  col = index - APPLETS_START;

  y = TOOLBAR_HEIGHT - TRAY_BUTTON_HEIGHT;
  x = screen_width - (col + 1) * (TRAY_BUTTON_WIDTH + TRAY_PADDING);

  priv->buttons[index] = NBTK_WIDGET (icon);

  switch (index)
    {
    case BATTERY_APPLET:
      tooltip = _("power & brightness");
      break;
    case VOLUME_APPLET:
      tooltip = _("volume");
      break;
    case BT_APPLET:
      tooltip = _("bluetooth");
      break;
    case WIFI_APPLET:
      tooltip = _("networks");
      break;
    case TEST_APPLET:
      tooltip = _("test");
      break;
    default:;
    }

  if (tooltip)
    nbtk_widget_set_tooltip_text (NBTK_WIDGET (icon), tooltip);

  clutter_actor_set_position (icon, x, y);
  clutter_container_add_actor (CLUTTER_CONTAINER (priv->hbox), icon);


  g_signal_connect (icon, "clicked",
                    G_CALLBACK (mnb_toolbar_toggle_buttons), toolbar);
}

static void
shell_tray_manager_icon_removed_cb (ShellTrayManager *mgr,
                                    ClutterActor     *icon,
                                    guint             xid,
                                    MnbToolbar       *toolbar)
{
  MnbToolbarPrivate *priv = toolbar->priv;
  gint               i;

  /*
   * Find the stored button and xid and reset.
   */
  for (i = APPLETS_START; i < NUM_ZONES; ++i)
    {
      if (priv->buttons[i] == (NbtkWidget*)icon)
        {
          priv->buttons[i] = NULL;

          if (priv->tray_xids[i] != xid)
            g_warning ("Mismatch between xids (0x%x vs 0x%x)",
                       priv->tray_xids[i], xid);

          priv->tray_xids[i] = 0;
          break;
        }
    }

  clutter_actor_destroy (icon);

}
#endif

static void
mnb_toolbar_kbd_grab_notify_cb (MetaScreen *screen,
                                GParamSpec *pspec,
                                MnbToolbar *toolbar)
{
  gboolean grabbed;

  g_object_get (screen, "keyboard-grabbed", &grabbed, NULL);

  /*
   * If the property has changed to FALSE, i.e., Mutter just called
   * XUngrabKeyboard(), we have to re-establish our grabs.
   */
  if (!grabbed)
    mnb_toolbar_setup_kbd_grabs (toolbar);
}

static void
mnb_toolbar_make_hint (MnbToolbar *toolbar)
{
  MnbToolbarPrivate *priv = toolbar->priv;
  MutterPlugin      *plugin = priv->plugin;
  ClutterText       *txt;
  ClutterActor      *bin;
  ClutterActor      *overlay;
  NbtkWidget        *label;
  gint               screen_width, screen_height;

  bin = CLUTTER_ACTOR (nbtk_bin_new ());
  label = nbtk_label_new (_("To activate the toolbar, move "
                            "your cursor to the top of the screen"
                             ));

  txt = CLUTTER_TEXT(nbtk_label_get_clutter_text(NBTK_LABEL(label)));
  clutter_text_set_line_alignment (CLUTTER_TEXT (txt), PANGO_ALIGN_LEFT);
  clutter_text_set_ellipsize (CLUTTER_TEXT (txt), PANGO_ELLIPSIZE_NONE);
  clutter_text_set_line_wrap (CLUTTER_TEXT (txt), TRUE);

  nbtk_widget_set_style_class_name (label, "toolbar-instruction-label");

  nbtk_bin_set_child (NBTK_BIN (bin), CLUTTER_ACTOR (label));
  nbtk_bin_set_alignment (NBTK_BIN (bin),
                          NBTK_ALIGN_CENTER, NBTK_ALIGN_LEFT);

  clutter_actor_set_name (CLUTTER_ACTOR (bin),
                          "toolbar-instruction-box");

  nbtk_widget_set_style_class_name (NBTK_WIDGET (bin),
                                    "toolbar-instruction-box");

  mutter_plugin_query_screen_size (plugin, &screen_width, &screen_height);

  clutter_actor_set_width (bin, 300);
  clutter_actor_set_position (bin,
                              screen_width - clutter_actor_get_width (bin) - 10,
                              10);

  overlay = mutter_plugin_get_overlay_group (plugin);

  clutter_container_add_actor (CLUTTER_CONTAINER (overlay), bin);
  clutter_actor_hide (bin);

  priv->hint = bin;
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

static void
mnb_toolbar_handle_dbus_name (MnbToolbar *toolbar, const gchar *name)
{
  MnbToolbarPrivate *priv = toolbar->priv;
  const gchar       *short_name = name + strlen (MPL_PANEL_DBUS_NAME_PREFIX);

  if (!strcmp (short_name, MPL_PANEL_MYZONE) ||
      !strcmp (short_name, MPL_PANEL_STATUS) ||
      !strcmp (short_name, MPL_PANEL_PEOPLE) ||
      !strcmp (short_name, MPL_PANEL_MEDIA) ||
      !strcmp (short_name, MPL_PANEL_INTERNET) ||
      !strcmp (short_name, MPL_PANEL_APPLICATIONS) ||
      !strcmp (short_name, MPL_PANEL_POWER) ||
      !strcmp (short_name, MPL_PANEL_NETWORK) ||
      !strcmp (short_name, MPL_PANEL_BLUETOOTH) ||
      !strcmp (short_name, MPL_PANEL_VOLUME) ||
      !strcmp (short_name, MPL_PANEL_TEST))
    {
      MnbPanel *panel;
      gint      screen_width, screen_height;

      mutter_plugin_query_screen_size (priv->plugin,
                                       &screen_width, &screen_height);

      panel  = mnb_panel_new (priv->plugin, name,
                              screen_width - TOOLBAR_X_PADDING * 2,
                              screen_height - 1.5 * TOOLBAR_HEIGHT);

      if (panel)
        {
          if (mnb_panel_is_ready (panel))
            {
              mnb_toolbar_append_panel (toolbar, (MnbDropDown*)panel);
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
 * Create panels for any of our services that are already up.
 */
static void
mnb_toolbar_dbus_setup_panels (MnbToolbar *toolbar)
{
  MnbToolbarPrivate  *priv = toolbar->priv;
  gchar             **names = NULL;
  GError             *error = NULL;
  gboolean            found_panels[NUM_ZONES];
  gint                i;

  if (!priv->dbus_conn || !priv->dbus_proxy)
    {
      g_warning ("DBus connection not available, cannot start panels !!!");
      return;
    }

  memset (&found_panels, 0, sizeof(found_panels));

  /*
   * Insert panels for any services already running.
   *
   * FIXME -- should probably do this asynchronously.
   */
  if (org_freedesktop_DBus_list_names (priv->dbus_proxy,
                                       &names, &error))
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
                  const gchar *short_name;
                  gint         index;

                  short_name = *p + strlen (MPL_PANEL_DBUS_NAME_PREFIX);

                  index = mnb_toolbar_panel_name_to_index (short_name);

                  if (index >= 0)
                    found_panels[index] = TRUE;

                  g_debug ("Found dbus name %s", *p);

                  mnb_toolbar_handle_dbus_name (toolbar, *p);
                }
            }

          p++;
        }
    }

  dbus_free_string_array (names);

  for (i = 0; i < NUM_ZONES; ++i)
    {
      switch (i)
        {
          /* Add here any apps that have been converted to multiproc */
        case APPS_ZONE:
        case PEOPLE_ZONE:
        case MYZONE:
          if (!found_panels[i])
            {
              DBusConnection *conn;
              const gchar    *name;
              gchar          *dbus_name;

              name =  mnb_toolbar_panel_index_to_name (i);

              g_debug ("Panel service [%s] is not running, attempting to start.",
                       name);

              conn = dbus_g_connection_get_connection (priv->dbus_conn);

              dbus_name = g_strconcat (MPL_PANEL_DBUS_NAME_PREFIX, name, NULL);

              dbus_bus_start_service_by_name (conn, dbus_name, 0, NULL, NULL);

              g_free (dbus_name);
            }
          break;
        default:;
        }
    }

  dbus_g_proxy_connect_signal (priv->dbus_proxy, "NameOwnerChanged",
                               G_CALLBACK (mnb_toolbar_noc_cb),
                               toolbar, NULL);
}

static void
mnb_toolbar_constructed (GObject *self)
{
  MnbToolbarPrivate *priv = MNB_TOOLBAR (self)->priv;
  MutterPlugin      *plugin = priv->plugin;
  ClutterActor      *actor = CLUTTER_ACTOR (self);
  ClutterActor      *hbox;
  ClutterActor      *background, *bg_texture;
  ClutterActor      *lowlight;
  gint               screen_width, screen_height;
  ClutterColor       clr = {0x0, 0x0, 0x0, 0xce};
  ClutterColor       low_clr = { 0, 0, 0, 0x7f };
  DBusGConnection   *conn;

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

  hbox = priv->hbox = clutter_group_new ();

  g_object_set (self,
                "show-on-set-parent", FALSE,
                NULL);

  mutter_plugin_query_screen_size (plugin, &screen_width, &screen_height);

  clutter_actor_set_size (actor, screen_width, TOOLBAR_SHADOW_HEIGHT);

  lowlight = clutter_rectangle_new_with_color (&low_clr);

  /*
   * The lowlight has to be tall enough to cover the screen when the toolbar
   * is fully withdrawn.
   */
  clutter_actor_set_size (lowlight,
                          screen_width, screen_height + TOOLBAR_SHADOW_HEIGHT);
  clutter_container_add_actor (CLUTTER_CONTAINER (hbox), lowlight);
  clutter_actor_hide (lowlight);
  priv->lowlight = lowlight;

  bg_texture =
    clutter_texture_new_from_file (PLUGIN_PKGDATADIR
                                   "/theme/panel/panel-background.png",
                                   NULL);
  if (bg_texture)
    {
      background = nbtk_texture_frame_new (CLUTTER_TEXTURE (bg_texture),
                                           0,   /* top */
                                           200, /* right */
                                           0,   /* bottom */
                                           200  /* left */);
      clutter_actor_set_size (background, screen_width - 8, TOOLBAR_HEIGHT);
      clutter_actor_set_x (background, 4);
      clutter_container_add_actor (CLUTTER_CONTAINER (hbox), background);
    }

  /* create time and date labels */
  priv->time = nbtk_label_new ("");
  clutter_actor_set_name (CLUTTER_ACTOR (priv->time), "time-label");

  priv->date = nbtk_label_new ("");
  clutter_actor_set_name (CLUTTER_ACTOR (priv->date), "date-label");

  clutter_container_add (CLUTTER_CONTAINER (hbox),
                         CLUTTER_ACTOR (priv->time),
                         CLUTTER_ACTOR (priv->date),
                         NULL);

  mnb_toolbar_update_time_date (priv);

  clutter_actor_set_position (CLUTTER_ACTOR (priv->time),
       (192 / 2) - clutter_actor_get_width (CLUTTER_ACTOR (priv->time)) /
                              2, 8);

  clutter_actor_set_position (CLUTTER_ACTOR (priv->date),
       (192 / 2) - clutter_actor_get_width (CLUTTER_ACTOR (priv->date)) /
                              2, 40);

  nbtk_bin_set_alignment (NBTK_BIN (self), NBTK_ALIGN_LEFT, NBTK_ALIGN_TOP);
  nbtk_bin_set_child (NBTK_BIN (self), hbox);

  g_timeout_add_seconds (60, (GSourceFunc) mnb_toolbar_update_time_date, priv);

  /*
   * Tray area
   */
  priv->tray_manager = g_object_new (SHELL_TYPE_TRAY_MANAGER,
                                     "bg-color", &clr,
                                     "mutter-plugin", plugin,
                                     NULL);

  g_signal_connect (priv->tray_manager, "tray-icon-added",
                    G_CALLBACK (shell_tray_manager_icon_added_cb), self);

  g_signal_connect (priv->tray_manager, "tray-icon-removed",
                    G_CALLBACK (shell_tray_manager_icon_removed_cb), self);

  shell_tray_manager_manage_stage (priv->tray_manager,
                                   CLUTTER_STAGE (
                                           mutter_plugin_get_stage (plugin)));

  /*
   * Set up the dedicated kbd grabs and notification callback.
   */
  mnb_toolbar_setup_kbd_grabs (MNB_TOOLBAR (self));

  g_signal_connect (mutter_plugin_get_screen (plugin),
                    "notify::keyboard-grabbed",
                    G_CALLBACK (mnb_toolbar_kbd_grab_notify_cb),
                    self);

  /*
   * Disable autohiding of the panel to start with (the panel needs to show
   * on startup regardless where the pointer initially is).
   */
  priv->dont_autohide = TRUE;

  g_signal_connect (mutter_plugin_get_stage (MUTTER_PLUGIN (plugin)),
                    "captured-event",
                    G_CALLBACK (mnb_toolbar_stage_captured_cb),
                    self);
  g_signal_connect (mutter_plugin_get_stage (plugin),
                    "button-press-event",
                    G_CALLBACK (mnb_toolbar_stage_input_cb),
                    self);

  /*
   * Construct the toolbar hint
   *
   * TODO -- only on first 5 boots ...
   */
  mnb_toolbar_make_hint (MNB_TOOLBAR (self));

  /*
   * Hook into "show" signal on stage, to set up input regions.
   * (We cannot set up the stage here, because the overlay window, etc.,
   * is not in place until the stage is shown.)
   */
  g_signal_connect (mutter_plugin_get_stage (MUTTER_PLUGIN (plugin)),
                    "show", G_CALLBACK (mnb_toolbar_stage_show_cb),
                    self);

  if (conn)
    mnb_toolbar_dbus_setup_panels (MNB_TOOLBAR (self));
}

NbtkWidget*
mnb_toolbar_new (MutterPlugin *plugin)
{
  return g_object_new (MNB_TYPE_TOOLBAR,
                       "mutter-plugin", plugin, NULL);
}

void
mnb_toolbar_activate_panel (MnbToolbar *toolbar, const gchar *panel_name)
{
  MnbToolbarPrivate *priv  = toolbar->priv;
  gint               index = mnb_toolbar_panel_name_to_index (panel_name);
  gint               i;

  if (index < 0 || !priv->panels[index] ||
      CLUTTER_ACTOR_IS_MAPPED (priv->panels[index]))
    {
      return;
    }

  for (i = 0; i < G_N_ELEMENTS (priv->buttons); i++)
    if (i != index)
      {
        if (priv->panels[i] && CLUTTER_ACTOR_IS_MAPPED (priv->panels[i]))
          clutter_actor_hide (CLUTTER_ACTOR (priv->panels[i]));
      }
    else
      {
        clutter_actor_show (CLUTTER_ACTOR (priv->panels[i]));
      }
}

void
mnb_toolbar_deactivate_panel (MnbToolbar *toolbar, const gchar *panel_name)
{
  MnbToolbarPrivate *priv  = toolbar->priv;
  gint               index = mnb_toolbar_panel_name_to_index (panel_name);

  if (index < 0 || !priv->panels[index] ||
      !CLUTTER_ACTOR_IS_MAPPED (priv->panels[index]))
    {
      return;
    }

  clutter_actor_hide (CLUTTER_ACTOR (priv->panels[index]));
}

/* returns NULL if no panel active */
const gchar *
mnb_toolbar_get_active_panel_name (MnbToolbar *toolbar)
{
  MnbToolbarPrivate *priv  = toolbar->priv;
  gint               index = -1;
  gint               i;

  for (i = 0; i < G_N_ELEMENTS (priv->buttons); i++)
    if (priv->panels[i] && CLUTTER_ACTOR_IS_MAPPED (priv->panels[i]))
      {
        index = i;
        break;
      }

  if (index < 0)
    return NULL;

  return mnb_toolbar_panel_index_to_name (index);
}

/* Are we animating in or out */
gboolean
mnb_toolbar_in_transition (MnbToolbar *toolbar)
{
  MnbToolbarPrivate *priv  = toolbar->priv;

  return (priv->in_show_animation || priv->in_hide_animation);
}

static void
tray_actor_destroy_cb (ClutterActor *actor, gpointer data)
{
  ClutterActor *background = data;
  ClutterActor *parent = clutter_actor_get_parent (background);

  if (CLUTTER_IS_CONTAINER (parent))
    clutter_container_remove_actor (CLUTTER_CONTAINER (parent), background);
  else
    clutter_actor_unparent (background);
}

struct config_map_data
{
  MnbToolbar   *toolbar;
  MutterWindow *mcw;
};

static void
tray_actor_show_completed_cb (ClutterActor *actor, gpointer data)
{
  struct config_map_data *map_data = data;
  MnbToolbarPrivate      *priv     = map_data->toolbar->priv;
  MutterPlugin           *plugin   = priv->plugin;
  MutterWindow           *mcw      = map_data->mcw;
  gfloat                  x, y, w, h;

  g_free (map_data);

  priv->systray_window_showing = TRUE;

  mnb_drop_down_get_footer_geometry (MNB_DROP_DOWN (actor), &x, &y, &w, &h);

  if (priv->dropdown_region)
    moblin_netbook_input_region_remove_without_update (plugin,
                                                       priv->dropdown_region);

  priv->dropdown_region =
    moblin_netbook_input_region_push (plugin, x, TOOLBAR_HEIGHT + y, w, h);

  /* Notify the manager that we are done with this effect */
  mutter_plugin_effect_completed (plugin, mcw, MUTTER_PLUGIN_MAP);
}

struct config_hide_data
{
  MnbToolbar *toolbar;
  Window      config_xwin;
};

/*
 * The tray machinery -- this will go once the new dbus API is in place.
 */
static void
tray_actor_hide_completed_cb (ClutterActor *actor, gpointer data)
{
  struct config_hide_data *hide_data = data;
  MnbToolbarPrivate       *priv = hide_data->toolbar->priv;
  ShellTrayManager        *tmgr = priv->tray_manager;
  gint                     i;

  priv->systray_window_showing = FALSE;

  for (i = APPLETS_START; i < NUM_ZONES; ++i)
    {
      if (priv->panels[i] == (NbtkWidget*) actor)
        {
          priv->panels[i] = NULL;
          priv->tray_xids[i] = 0;
          break;
        }
    }

  shell_tray_manager_close_config_window (tmgr, hide_data->config_xwin);
}

static void
tray_actor_hide_begin_cb (ClutterActor *actor, gpointer data)
{
  struct config_hide_data *hide_data = data;
  MnbToolbarPrivate       *priv = hide_data->toolbar->priv;
  MutterPlugin            *plugin = priv->plugin;
  ShellTrayManager        *tmgr = priv->tray_manager;

  if (priv->dropdown_region)
    {
      moblin_netbook_input_region_remove (plugin, priv->dropdown_region);
      priv->dropdown_region = NULL;
    }

  shell_tray_manager_hide_config_window (tmgr,
                                         hide_data->config_xwin);
}

void
mnb_toolbar_append_tray_window (MnbToolbar *toolbar, MutterWindow *mcw)
{
  /*
   * Insert the actor into a custom frame, and then animate it to
   * position.
   *
   * Because of the way Mutter stacking we are unable to insert an actor
   * into the MutterWindow stack at an arbitrary position; instead
   * any actor we insert will end up on the very top. Additionally,
   * we do not want to reparent the MutterWindow, as that would
   * wreak havoc with the stacking.
   *
   * Consequently we have two options:
   *
   * (a) The center of our frame is transparent, and we overlay it over
   * the MutterWindow.
   *
   * (b) We reparent the actual glx texture inside Mutter window to
   * our frame, and destroy it manually when we close the window.
   *
   * We do (b).
   */
  MnbToolbarPrivate *priv = toolbar->priv;

  struct config_map_data  *map_data;
  struct config_hide_data *hide_data;

  ClutterActor *actor = CLUTTER_ACTOR (mcw);
  ClutterActor *background;
  ClutterActor *parent;
  ClutterActor *button;
  ClutterActor *texture = mutter_window_get_texture (mcw);

  Window xwin = mutter_window_get_x_window (mcw);

  gint  x = clutter_actor_get_x (actor);
  gint  y = clutter_actor_get_y (actor);

  background = CLUTTER_ACTOR (mnb_drop_down_new (priv->plugin));

  g_object_ref (texture);
  clutter_actor_unparent (texture);
  mnb_drop_down_set_child (MNB_DROP_DOWN (background), texture);
  g_object_unref (texture);

  g_signal_connect (actor, "destroy",
                    G_CALLBACK (tray_actor_destroy_cb), background);

  map_data          = g_new (struct config_map_data, 1);
  map_data->toolbar = toolbar;
  map_data->mcw     = mcw;

  g_signal_connect (background, "show-completed",
                    G_CALLBACK (tray_actor_show_completed_cb),
                    map_data);

  hide_data              = g_new (struct config_hide_data, 1);
  hide_data->toolbar     = toolbar;
  hide_data->config_xwin = xwin;

  g_signal_connect (background, "hide-begin",
                    G_CALLBACK (tray_actor_hide_begin_cb),
                    hide_data);

  g_signal_connect_data (background, "hide-completed",
                         G_CALLBACK (tray_actor_hide_completed_cb),
                         hide_data,
                         (GClosureNotify)g_free, 0);

  clutter_actor_set_position (background, x, y);

  g_object_set (actor, "no-shadow", TRUE, NULL);

  clutter_actor_hide (actor);

  parent = mutter_plugin_get_overlay_group (priv->plugin);

  clutter_container_add_actor (CLUTTER_CONTAINER (priv->hbox), background);
  clutter_actor_raise (background, priv->lowlight);

  button = shell_tray_manager_find_button_for_xid (priv->tray_manager, xwin);

  if (button)
    {
      mnb_drop_down_set_button (MNB_DROP_DOWN (background),
                                NBTK_BUTTON (button));
    }
  else
    g_warning ("No button found for xid 0x%x", (guint)xwin);

  /*
   * Hide all config windows.
   */
  shell_tray_manager_close_all_other_config_windows (priv->tray_manager, xwin);
  clutter_actor_show_all (background);
}

gboolean
mnb_toolbar_is_tray_config_window (MnbToolbar *toolbar, Window xwin)
{
  MnbToolbarPrivate *priv = toolbar->priv;

  return shell_tray_manager_is_config_window (priv->tray_manager, xwin);
}

/*
 * Returns the switcher zone if it exists.
 *
 * (This is needed because we have to hookup the switcher focus related
 * callbacks plugin so it can maintain accurate switching order list.)
 */
NbtkWidget *
mnb_toolbar_get_switcher (MnbToolbar *toolbar)
{
  MnbToolbarPrivate *priv = toolbar->priv;

  return priv->panels[SPACES_ZONE];
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
 * Sets up passive key grabs on any dedicated shortcut keys that we cannot
 * hook into throught metacity key bindings.
 */
static void
mnb_toolbar_setup_kbd_grabs (MnbToolbar *toolbar)
{
  MutterPlugin *plugin = toolbar->priv->plugin;
  MetaScreen   *screen;
  Display      *xdpy;
  Window        root_xwin;

  screen    = mutter_plugin_get_screen (MUTTER_PLUGIN (plugin));
  xdpy      = mutter_plugin_get_xdisplay (MUTTER_PLUGIN (plugin));
  root_xwin = RootWindow (xdpy, meta_screen_get_screen_number (screen));

  /*
   * Grab the panel shortcut key.
   */
  XGrabKey (xdpy, XKeysymToKeycode (xdpy, MOBLIN_PANEL_SHORTCUT_KEY),
            AnyModifier,
            root_xwin, True, GrabModeAsync, GrabModeAsync);
}

/*
 * Machinery for showing and hiding the panel in response to pointer position.
 */

/*
 * The timeout callback that shows the panel if the pointer stayed long enough
 * in the trigger region.
 */
static gboolean
mnb_toolbar_trigger_timeout_cb (gpointer data)
{
  MnbToolbar *toolbar = MNB_TOOLBAR (data);

  clutter_actor_show (CLUTTER_ACTOR (toolbar));
  toolbar->priv->trigger_timeout_id = 0;

  return FALSE;
}

/*
 * Changes the size of the trigger region (we increase the size of the trigger
 * region while wating for the trigger timeout to reduce effects of jitter).
 */
static void
mnb_toolbar_trigger_region_set_height (MnbToolbar *toolbar, gint height)
{
  MnbToolbarPrivate *priv = toolbar->priv;
  MutterPlugin      *plugin = priv->plugin;
  gint               screen_width, screen_height;

  mutter_plugin_query_screen_size (plugin, &screen_width, &screen_height);

  if (priv->trigger_region != NULL)
    moblin_netbook_input_region_remove (plugin, priv->trigger_region);

  priv->trigger_region
    = moblin_netbook_input_region_push (plugin,
                                        0,
                                        0,
                                        screen_width,
                                        TOOLBAR_TRIGGER_THRESHOLD + height);
}

/*
 * Returns TRUE if one of the out of process panels is showing; used to
 * block Toolbar closing on stage leave event.
 *
 * (NB: returns FALSE for panels that are not of MnbPanel type!)
 */
static gboolean
mnb_toolbar_panels_showing (MnbToolbar *toolbar)
{
  MnbToolbarPrivate *priv = toolbar->priv;
  gint i;

  for (i = 0; i < NUM_ZONES; ++i)
    {
      MnbPanel *panel = (MnbPanel*)priv->panels[i];

      if (!panel || !MNB_IS_PANEL (panel))
        continue;

      if (CLUTTER_ACTOR_IS_MAPPED (panel))
        return TRUE;
    }

  return FALSE;
}

/*
 * Callback for ClutterStage::captured-event singal.
 *
 * Processes CLUTTER_ENTER and CLUTTER_LEAVE events and shows/hides the
 * panel as required.
 */
static gboolean
mnb_toolbar_stage_captured_cb (ClutterActor *stage,
                               ClutterEvent *event,
                               gpointer      data)
{
  MnbToolbar        *toolbar = MNB_TOOLBAR (data);
  MnbToolbarPrivate *priv    = toolbar->priv;
  gboolean           show_toolbar;

  /*
   * Shortcircuit what we can:
   *
   * a) toolbar is disabled (e.g., in lowlight),
   * b) the event is something other than enter/leave
   * c) we got an enter event on something other than stage,
   * d) we got a leave event bug are showing panels
   * e) we are already animating.
   *
   * Split into multiple statments for readability.
   */

  if (priv->disabled)
    {
      /* g_debug (G_STRLOC " leaving early"); */
      return FALSE;
    }

  if (!(event->type == CLUTTER_ENTER || event->type == CLUTTER_LEAVE))
      return FALSE;

  if ((event->type == CLUTTER_ENTER) && (event->crossing.source != stage))
    {
      /* g_debug (G_STRLOC " leaving early"); */
      return FALSE;
    }

  if ((event->type == CLUTTER_LEAVE) &&
      (priv->systray_window_showing || mnb_toolbar_panels_showing (toolbar)))
    {
      /* g_debug (G_STRLOC " leaving early"); */
      return FALSE;
    }

  if (mnb_toolbar_in_transition (toolbar))
    {
      /* g_debug (G_STRLOC " leaving early"); */
      return FALSE;
    }

  /*
   * This is when we want to show the toolbar:
   *
   *  a) we got an enter event on stage,
   *
   *    OR
   *
   *  b) we got a leave event on stage at the very top of the screen (when the
   *     pointer is at the position that coresponds to the top of the window,
   *     it is considered to have left the window; when the user slides pointer
   *     to the top, we get an enter event immediately followed by a leave
   *     event).
   *
   *  In all cases, only if the toolbar is not already visible.
   */
  show_toolbar  = (event->type == CLUTTER_ENTER);
  show_toolbar |= ((event->type == CLUTTER_LEAVE) && (event->crossing.y == 0));
  show_toolbar &= !CLUTTER_ACTOR_IS_MAPPED (toolbar);

  if (show_toolbar)
    {
      /*
       * If any fullscreen apps are present, then bail out.
       */
      if (moblin_netbook_fullscreen_apps_present (priv->plugin))
            return FALSE;

      /*
       * Increase sensitivity -- increasing size of the trigger zone while the
       * timeout reduces the effect of a shaking hand.
       */
      mnb_toolbar_trigger_region_set_height (toolbar, 5);

      priv->trigger_timeout_id =
        g_timeout_add (TOOLBAR_TRIGGER_THRESHOLD_TIMEOUT,
                       mnb_toolbar_trigger_timeout_cb, toolbar);
    }
  else if (event->type == CLUTTER_LEAVE)
    {
      /*
       * The most reliable way of detecting that the pointer is leaving the
       * stage is from the related actor -- no related == pointer gone
       * elsewhere.
       */
      if (event->crossing.related != NULL)
        return FALSE;

      if (priv->trigger_timeout_id)
        {
          /*
           * Pointer left us before the required timeout triggered; clean up.
           */
          mnb_toolbar_trigger_region_set_height (toolbar, 0);
          g_source_remove (priv->trigger_timeout_id);
          priv->trigger_timeout_id = 0;
        }
      else if (CLUTTER_ACTOR_IS_MAPPED (toolbar))
        {
          mnb_toolbar_trigger_region_set_height (toolbar, 0);
          clutter_actor_hide (CLUTTER_ACTOR (toolbar));
        }
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

      if (CLUTTER_ACTOR_IS_MAPPED (toolbar))
        clutter_actor_hide (CLUTTER_ACTOR (toolbar));
    }

  return FALSE;
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
  MutterPlugin      *plugin = priv->plugin;
  XWindowAttributes  attr;
  long               event_mask;
  Window             xwin;
  MetaScreen        *screen;
  Display           *xdpy;
  ClutterStage      *stg;

  xdpy   = mutter_plugin_get_xdisplay (plugin);
  stg    = CLUTTER_STAGE (mutter_plugin_get_stage (plugin));
  screen = mutter_plugin_get_screen (plugin);

  /*
   * Set up the stage input region
   */
  mnb_toolbar_trigger_region_set_height (toolbar, 0);

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

  xwin = mutter_get_overlay_window (screen);
  event_mask = EnterWindowMask | LeaveWindowMask;

  if (XGetWindowAttributes (xdpy, xwin, &attr))
    {
      event_mask |= attr.your_event_mask;
    }

  XSelectInput (xdpy, xwin, event_mask);

  priv->shown = TRUE;

  if (!priv->shown_myzone && priv->panels[MYZONE])
    {
      priv->shown_myzone = TRUE;
      clutter_actor_show (CLUTTER_ACTOR (priv->panels[MYZONE]));
    }
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
  gint i;

  for (i = 0; i < NUM_ZONES; ++i)
    {
      MnbPanel *panel = (MnbPanel*) priv->panels[i];

      if (!panel || !MNB_IS_PANEL (panel))
        continue;

      if (xid == mnb_panel_get_xid (panel))
        {
          return panel;
        }
    }

  return NULL;
}

