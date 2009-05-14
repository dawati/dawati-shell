/* mnb-toolbar.c */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <glib/gi18n.h>

#include "moblin-netbook.h"

#include "mnb-toolbar.h"
#include "mnb-toolbar-button.h"
#include "mnb-drop-down.h"
#include "mnb-switcher.h"

/* TODO -- remove these after multiprocing */
#include "penge/penge-grid-view.h"
#include "moblin-netbook-launcher.h"
#include "moblin-netbook-status.h"
#include "moblin-netbook-netpanel.h"
#include "moblin-netbook-pasteboard.h"
#include "moblin-netbook-people.h"

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

#if 0
/*
 * TODO
 * This is currently define in moblin-netbook.h, as it is needed by the
 * tray manager and MnbDropDown -- this should not be hardcoded, and we need
 * a way for the drop down to query it from the panel.
 */
#define TOOLBAR_HEIGHT 64
#endif

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

enum {
    M_ZONE = 0,
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
    BATTERY_APPLET = APPLETS_START,
    VOLUME_APPLET,
    BT_APPLET,
    WIFI_APPLET,
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

  NbtkWidget   *time; /* The time and date fields, needed for the updates */
  NbtkWidget   *date;

  NbtkWidget   *buttons[NUM_ZONES]; /* Buttons, one per zone & applet */
  NbtkWidget   *panels[NUM_ZONES];  /* Panels (the dropdowns) */

  ShellTrayManager *tray_manager;

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

#if 1
  /* TODO remove */
  gboolean systray_window_showing;

  guint tray_xids [NUM_ZONES];
#endif
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
mnb_toolbar_show (ClutterActor *actor)
{
  MnbToolbarPrivate *priv = MNB_TOOLBAR (actor)->priv;
  guint              width;
  guint              screen_width, screen_height;
  gint               i;
  ClutterAnimation  *animation;

  if (priv->in_show_animation)
    {
      g_signal_stop_emission_by_name (actor, "show");
      return;
    }

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

  priv->in_show_animation = TRUE;

  /*
   * Start animation and wait for it to complete.
   */
  animation = clutter_actor_animate (actor, CLUTTER_LINEAR, 150, "y", 0, NULL);

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

  g_signal_emit (actor, toolbar_signals[HIDE_COMPLETED], 0);
  g_object_unref (actor);
}

static void
mnb_toolbar_hide (ClutterActor *actor)
{
  MnbToolbarPrivate *priv = MNB_TOOLBAR (actor)->priv;
  gint               height;
  gint               i;
  ClutterAnimation  *animation;

  if (priv->in_hide_animation)
    {
      g_signal_stop_emission_by_name (actor, "hide");
      return;
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
            if (checked && !CLUTTER_ACTOR_IS_VISIBLE (priv->panels[i]))
              {
                clutter_actor_show (CLUTTER_ACTOR (priv->panels[i]));
              }
            else if (!checked && CLUTTER_ACTOR_IS_VISIBLE (priv->panels[i]))
              {
                clutter_actor_hide (CLUTTER_ACTOR (priv->panels[i]));
              }
          }
      }
}

#if 1
/*
 * TODO Remove.
 *
 * Helper functions for the m_zone, internet zone and media zone -- there will
 * need to go and be handled internally in the zones/via new dbus API.
 */
static void
_mzone_activated_cb (PengeGridView *view, gpointer data)
{
  MnbToolbarPrivate *priv = MNB_TOOLBAR (data)->priv;

  if (priv->panels[M_ZONE])
    clutter_actor_hide (CLUTTER_ACTOR (priv->panels[M_ZONE]));

  clutter_actor_hide (CLUTTER_ACTOR (data));
}

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
  moblin_netbook_spawn (plugin, exec, 0L, TRUE, workspace);

  g_free (exec);
  g_free (esc_url);

  clutter_actor_hide (CLUTTER_ACTOR (toolbar));
}
#endif

#endif /* REMOVE ME block */

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

  if (!strcmp (name, "m-zone"))
    index = M_ZONE;
  else if (!strcmp (name, "status-zone"))
    index = STATUS_ZONE;
  else if (!strcmp (name, "spaces-zone"))
    index = SPACES_ZONE;
  else if (!strcmp (name, "internet-zone"))
    index = INTERNET_ZONE;
  else if (!strcmp (name, "media-zone"))
    index = MEDIA_ZONE;
  else if (!strcmp (name, "applications-zone"))
    index = APPS_ZONE;
  else if (!strcmp (name, "people-zone"))
    index = PEOPLE_ZONE;
  else if (!strcmp (name, "pasteboard-zone"))
    index = PASTEBOARD_ZONE;
  else if (!strcmp (name, "tray-button-wifi"))
    index = WIFI_APPLET;
  else if (!strcmp (name, "tray-button-bluetooth"))
    index = BT_APPLET;
  else if (!strcmp (name, "tray-button-sound"))
    index = VOLUME_APPLET;
  else if (!strcmp (name, "tray-button-battery"))
    index = BATTERY_APPLET;
  else if (!strcmp (name, "tray-button-test"))
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
    case M_ZONE: return "m-zone";
    case STATUS_ZONE: return "status-zone";
    case SPACES_ZONE: return "spaces-zone";
    case INTERNET_ZONE: return "internet-zone";
    case MEDIA_ZONE: return "media-zone";
    case APPS_ZONE: return "applications-zone";
    case PEOPLE_ZONE: return "people-zone";
    case PASTEBOARD_ZONE: return "pasteboard-zone";
    case WIFI_APPLET: return "tray-button-wifi";
    case BT_APPLET: return "tray-button-bluetooth";
    case VOLUME_APPLET: return "tray-button-volume";
    case BATTERY_APPLET: return "tray-button-battery";
    case TEST_APPLET: return "tray-button-test";

    default: return NULL;
    }
}

static void
mnb_toolbar_dropdown_show_completed_full_cb (MnbDropDown *dropdown,
                                             MnbToolbar  *toolbar)
{
  MnbToolbarPrivate *priv = toolbar->priv;
  MutterPlugin      *plugin = priv->plugin;
  guint w, h;

  clutter_actor_get_transformed_size (CLUTTER_ACTOR (dropdown), &w, &h);

  if (priv->dropdown_region)
    moblin_netbook_input_region_remove_without_update (plugin,
                                                       priv->dropdown_region);

  priv->dropdown_region =
    moblin_netbook_input_region_push (plugin, 0, TOOLBAR_HEIGHT, w, h);
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
  gint  x, y;
  guint w, h;

  mnb_drop_down_get_footer_geometry (dropdown, &x, &y, &w, &h);

  if (priv->dropdown_region)
    moblin_netbook_input_region_remove_without_update (plugin,
                                                       priv->dropdown_region);

  priv->dropdown_region =
    moblin_netbook_input_region_push (plugin, x, TOOLBAR_HEIGHT + y, w, h);
}

static void
mnb_toolbar_dropdown_hide_begin_cb (MnbDropDown *dropdown, MnbToolbar  *toolbar)
{
  MnbToolbarPrivate *priv = toolbar->priv;
  MutterPlugin      *plugin = priv->plugin;

  if (priv->dropdown_region)
    {
      moblin_netbook_input_region_remove (plugin, priv->dropdown_region);
      priv->dropdown_region = NULL;
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
  guint              screen_width, screen_height;
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
      gint zones   = APPLETS_START;
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
        ClutterActor *child = NULL;

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
          panel = priv->panels[index] = NBTK_WIDGET (
            make_launcher (plugin,
                           screen_width - TOOLBAR_X_PADDING * 2,
                           screen_height - 2 * TOOLBAR_HEIGHT));
          break;
        case PASTEBOARD_ZONE:
          panel = priv->panels[index] = NBTK_WIDGET (
            make_pasteboard (plugin,
                             screen_width - TOOLBAR_X_PADDING * 2));
          break;
#if WITH_PEOPLE
        case PEOPLE_ZONE:
          panel = priv->panels[index] = NBTK_WIDGET (
            make_people_panel (plugin, screen_width - TOOLBAR_X_PADDING * 2));
          break;
#endif
        case M_ZONE:
          {
            ClutterActor *grid;

            panel = priv->panels[index] = mnb_drop_down_new (plugin);

            grid = g_object_new (PENGE_TYPE_GRID_VIEW, NULL);

            g_signal_connect (grid, "activated",
                              G_CALLBACK (_mzone_activated_cb), toolbar);

            clutter_actor_set_height (grid,
                                      screen_height - TOOLBAR_HEIGHT * 1.5);

            mnb_drop_down_set_child (MNB_DROP_DOWN (panel), grid);
          }
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

            g_signal_connect_swapped (panel, "hide-completed",
                                    G_CALLBACK (moblin_netbook_netpanel_clear),
                                    grid);
            g_signal_connect_swapped (panel, "show-completed",
                                    G_CALLBACK (moblin_netbook_netpanel_focus),
                                    grid);

            g_signal_connect (grid, "launch",
                              G_CALLBACK (_netgrid_launch_cb), toolbar);
            g_signal_connect_swapped (grid, "launched",
                                      G_CALLBACK (clutter_actor_hide), toolbar);
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

  g_signal_connect (panel, "hide-begin",
                    G_CALLBACK(mnb_toolbar_dropdown_hide_begin_cb),
                    toolbar);

  clutter_container_add_actor (CLUTTER_CONTAINER (priv->hbox),
                               CLUTTER_ACTOR (panel));
  clutter_actor_set_width (CLUTTER_ACTOR (panel), screen_width);

  mnb_drop_down_set_button (MNB_DROP_DOWN (panel), NBTK_BUTTON (button));
  clutter_actor_set_position (CLUTTER_ACTOR (panel), 0, TOOLBAR_HEIGHT);
  clutter_actor_lower_bottom (CLUTTER_ACTOR (panel));
}

#if 0
static void
mnb_toolbar_panel_request_icon_cb (MnbPanel    *panel,
                                   const gchar *icon,
                                   MnbToolbar  *toolbar)
{
  MnbToolbarPrivate *priv = toolbar->priv;
  gint index;

  index = mnb_toolbar_panel_name_to_index (mnb_panel_get_name (panel));

  if (index < 0)
    return;

  nbtk_button_set_icon_from_file (NBTK_BUTTON (priv->buttons[index]),
                                  (gchar*)icon);
}

/*
 * Appends a panel
 */
void
mnb_toolbar_append_panel (MnbToolbar  *toolbar, MnbDropDown *panel)
{
  MnbToolbarPrivate *priv = toolbar->priv;
  MutterPlugin      *plugin = priv->plugin;
  NbtkWidget        *button;
  guint              screen_width, screen_height;
  gint               index;
  gchar             *button_style;
  const gchar       *name;
  const gchar       *tooltip;

  if (MNB_IS_PANEL (panel))
    {
      name    = mnb_panel_get_name (MNB_PANEL (panel));
      tooltip = mnb_panel_get_tooltip (MNB_PANEL (panel));
    }
  else if (MNB_IS_SWITCHER (panel))
    {
      name    = "spaces-zone";
      tooltip = _("zones");
    }
  else
    {
      g_warning ("Unhandled panel type: %s", G_OBJECT_TYPE_NAME (panel));
      return;
    }

  index = mnb_toolbar_panel_name_to_index (name);

  if (index < 0)
    return;

  button_style = g_strdup_printf ("%s-button", name);

  /*
   * If the respective slot is already occupied, remove the old objects.
   */
  if (priv->buttons[index])
    {
      clutter_container_remove_actor (CLUTTER_CONTAINER (priv->hbox),
                                      CLUTTER_ACTOR (priv->buttons[index]));
    }

  if (priv->panels[index])
    {
      clutter_container_remove_actor (CLUTTER_CONTAINER (priv->hbox),
                                      CLUTTER_ACTOR (priv->panels[index]));
    }

  mutter_plugin_query_screen_size (plugin, &screen_width, &screen_height);

  /*
   * Create the button for this zone.
   */
  button = priv->buttons[index] = mnb_toolbar_button_new ();
  nbtk_button_set_toggle_mode (NBTK_BUTTON (button), TRUE);
  nbtk_widget_set_tooltip_text (NBTK_WIDGET (button), tooltip);
  clutter_actor_set_name (CLUTTER_ACTOR (button), button_style);

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
      gint zones   = APPLETS_START;
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

      clutter_container_add_actor (CLUTTER_CONTAINER (priv->hbox),
                               CLUTTER_ACTOR (button));
    }

  g_signal_connect (button, "clicked",
                    G_CALLBACK (mnb_toolbar_toggle_buttons),
                    toolbar);

  g_signal_connect (panel, "show-completed",
                    G_CALLBACK(mnb_toolbar_dropdown_show_completed_partial_cb),
                    toolbar);

  g_signal_connect (panel, "hide-begin",
                    G_CALLBACK (mnb_toolbar_dropdown_hide_begin_cb),
                    toolbar);

  g_signal_connect (panel, "request-icon",
                    G_CALLBACK (mnb_toolbar_panel_request_icon_cb),
                    toolbar);

  clutter_container_add_actor (CLUTTER_CONTAINER (priv->hbox),
                               CLUTTER_ACTOR (panel));

  mnb_drop_down_set_button (MNB_DROP_DOWN (panel), NBTK_BUTTON (button));
  clutter_actor_set_position (CLUTTER_ACTOR (panel), 0, TOOLBAR_HEIGHT);
  clutter_actor_lower_bottom (CLUTTER_ACTOR (panel));
}
#endif

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
mnb_toolbar_constructed (GObject *self)
{
  MnbToolbarPrivate *priv = MNB_TOOLBAR (self)->priv;
  MutterPlugin      *plugin = priv->plugin;
  ClutterActor      *actor = CLUTTER_ACTOR (self);
  ClutterActor      *hbox;
  guint              screen_width, screen_height;
  ClutterColor       clr = {0x0, 0x0, 0x0, 0xce};

  hbox = priv->hbox = clutter_group_new ();

  g_object_set (self,
                "show-on-set-parent", FALSE,
                NULL);

  mutter_plugin_query_screen_size (plugin, &screen_width, &screen_height);

  clutter_actor_set_size (actor, screen_width, TOOLBAR_HEIGHT);

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
   * Hook into "show" signal on stage, to set up input regions.
   * (We cannot set up the stage here, because the overlay window, etc.,
   * is not in place until the stage is shown.)
   */
  g_signal_connect (mutter_plugin_get_stage (MUTTER_PLUGIN (plugin)),
                    "show", G_CALLBACK (mnb_toolbar_stage_show_cb),
                    self);
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
      CLUTTER_ACTOR_IS_VISIBLE (priv->panels[index]))
    {
      return;
    }

  for (i = 0; i < G_N_ELEMENTS (priv->buttons); i++)
    if (i != index)
      {
        if (priv->panels[i] && CLUTTER_ACTOR_IS_VISIBLE (priv->panels[i]))
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
      !CLUTTER_ACTOR_IS_VISIBLE (priv->panels[index]))
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
    if (priv->panels[i] && CLUTTER_ACTOR_IS_VISIBLE (priv->panels[i]))
      {
        index = i;
        break;
      }

  if (index < 0)
    return;

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
  gint  x, y;
  guint w, h;

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

  for (i = APPLETS_START; i < 0; ++i)
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
  gint  i;

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

  button = shell_tray_manager_find_button_for_xid (priv->tray_manager, xwin);

  if (button)
    mnb_drop_down_set_button (MNB_DROP_DOWN (background), NBTK_BUTTON (button));
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
   * d) we are already animating.
   */
  if (!(((event->type == CLUTTER_ENTER) && (event->crossing.source == stage)) ||
        (event->type == CLUTTER_LEAVE && !priv->systray_window_showing)) ||
      priv->disabled ||
      mnb_toolbar_in_transition (toolbar))
    return FALSE;

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
  show_toolbar &= !CLUTTER_ACTOR_IS_VISIBLE (toolbar);

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
      else if (CLUTTER_ACTOR_IS_VISIBLE (toolbar))
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

      if (CLUTTER_ACTOR_IS_VISIBLE (toolbar))
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
  MutterPlugin      *plugin = toolbar->priv->plugin;
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

