/* mnb-toolbar.c */

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

#define BUTTON_WIDTH 66
#define BUTTON_HEIGHT 55
#define BUTTON_SPACING 10

#define TRAY_PADDING   3
#define TRAY_BUTTON_HEIGHT 55
#define TRAY_BUTTON_WIDTH 44

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

enum {
    M_ZONE = 0,
    STATUS_ZONE,
    SPACES_ZONE,
    INTERNET_ZONE,
    MEDIA_ZONE,
    APPS_ZONE,
    PEOPLE_ZONE,
    PASTEBOARD_ZONE,

    APPLETS_START,
    WIFI_APPLET = APPLETS_START,
    BT_APPLET,
    VOLUME_APPLET,
    BATTERY_APPLET,
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

struct _MnbToolbarPrivate {
  MutterPlugin *plugin;

  ClutterActor *hbox;

  NbtkWidget *time;
  NbtkWidget *date;

  NbtkWidget *buttons[NUM_ZONES];
  NbtkWidget *panels[NUM_ZONES];

  ShellTrayManager *tray_manager;

  gboolean in_show_animation : 1;
  gboolean in_hide_animation : 1;
  gboolean dont_autohide     : 1;

  MnbInputRegion input_region;
};



static void
mnb_toolbar_get_property (GObject *object, guint property_id,
                              GValue *value, GParamSpec *pspec)
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
      moblin_netbook_input_region_remove (priv->plugin,
                                          priv->input_region);

      priv->input_region = NULL;
    }

  priv->in_hide_animation = TRUE;

  g_object_ref (actor);

  height = clutter_actor_get_height (actor);

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
                                                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));


  toolbar_signals[SHOW_COMPLETED] =
    g_signal_new ("show-completed",
                  G_TYPE_FROM_CLASS (object_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (MnbToolbarClass, show_completed),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  toolbar_signals[HIDE_BEGIN] =
    g_signal_new ("hide-begin",
                  G_TYPE_FROM_CLASS (object_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (MnbToolbarClass, hide_begin),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

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
 */
static void
_mzone_activated_cb (PengeGridView *view, gpointer data)
{
  MnbToolbarPrivate *priv = MNB_TOOLBAR (data)->priv;

  if (priv->panels[M_ZONE])
    clutter_actor_hide (CLUTTER_ACTOR (priv->panels[M_ZONE]));

  clutter_actor_hide (CLUTTER_ACTOR (data));
}
#endif

void
mnb_toolbar_append_panel (MnbToolbar  *toolbar,
                          const gchar *name,
                          const gchar *tooltip,
                          const gchar *icon,
                          Window       xid)
{
  MnbToolbarPrivate *priv = MNB_TOOLBAR (toolbar)->priv;
  MutterPlugin      *plugin = priv->plugin;
  NbtkWidget        *button;
  NbtkWidget        *panel;
  guint              screen_width, screen_height;
  guint              index;
  gchar             *button_style;

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
      g_warning ("Unknown panel [%s]");
      return;
    }

  button_style = g_strdup_printf ("%s-button", name);

  /*
   * If the respective slot is already occupied, remove the old objects.
   */
  if (priv->buttons[index])
    {
      if (index == SPACES_ZONE)
        {
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
          g_warning ("The Spaces Zone cannot be replaced\n");
          return;
        }

      clutter_container_remove_actor (CLUTTER_CONTAINER (priv->hbox),
                                      CLUTTER_ACTOR (priv->panels[index]));
    }

  mutter_plugin_query_screen_size (plugin, &screen_width, &screen_height);

  button = priv->buttons[index] = mnb_toolbar_button_new ();
  nbtk_button_set_toggle_mode (NBTK_BUTTON (button), TRUE);
  nbtk_button_set_tooltip (NBTK_BUTTON (button), tooltip);
  clutter_actor_set_name (CLUTTER_ACTOR (button), button_style);

  if (icon)
    {
      ClutterActor *icon_actor;

      icon_actor = clutter_texture_new_from_file (icon, NULL);

      if (icon_actor)
        {
          clutter_container_add_actor (CLUTTER_CONTAINER (button), icon_actor);
          clutter_actor_set_reactive (icon_actor, TRUE);
        }
    }

  if (index < APPLETS_START)
    {
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

  /*
   * Special case Space; we have an internal component for this, but will
   * allow it to be replaced by a thirdparty component just like all the
   * other dropdowns.
   */
  if (index == SPACES_ZONE)
    {
      panel = priv->panels[index] = mnb_switcher_new (plugin);

      meta_keybindings_set_custom_handler ("switch_windows",
                                           mnb_switcher_alt_tab_key_handler,
                                           panel, NULL);
      meta_keybindings_set_custom_handler ("switch_windows_backward",
                                           mnb_switcher_alt_tab_key_handler,
                                           panel, NULL);
      meta_keybindings_set_custom_handler ("switch_panels",
                                           mnb_switcher_alt_tab_key_handler,
                                           panel, NULL);
      meta_keybindings_set_custom_handler ("switch_panels_backward",
                                           mnb_switcher_alt_tab_key_handler,
                                           panel, NULL);
      meta_keybindings_set_custom_handler ("cycle_group",
                                           mnb_switcher_alt_tab_key_handler,
                                           panel, NULL);
      meta_keybindings_set_custom_handler ("cycle_group_backward",
                                           mnb_switcher_alt_tab_key_handler,
                                           panel, NULL);
      meta_keybindings_set_custom_handler ("cycle_windows",
                                           mnb_switcher_alt_tab_key_handler,
                                           panel, NULL);
      meta_keybindings_set_custom_handler ("cycle_windows_backward",
                                           mnb_switcher_alt_tab_key_handler,
                                           panel, NULL);
      meta_keybindings_set_custom_handler ("cycle_panels",
                                           mnb_switcher_alt_tab_key_handler,
                                           panel, NULL);
      meta_keybindings_set_custom_handler ("cycle_panels_backward",
                                           mnb_switcher_alt_tab_key_handler,
                                           panel, NULL);
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
        case M_ZONE:
          {
            ClutterActor *grid;

            panel = priv->panels[index] =
              mnb_drop_down_new ();

            grid = g_object_new (PENGE_TYPE_GRID_VIEW, NULL);

            g_signal_connect (grid, "activated",
                              G_CALLBACK (_mzone_activated_cb), toolbar);

            clutter_actor_set_height (grid,
                                      screen_height - TOOLBAR_HEIGHT * 1.5);

            mnb_drop_down_set_child (MNB_DROP_DOWN (panel), grid);
          }
          break;
        default:
          g_warning ("Zone %s is currently not implemented", name);
        }
      }
#endif
    }

  if (!panel)
    return;

  clutter_container_add_actor (CLUTTER_CONTAINER (priv->hbox),
                               CLUTTER_ACTOR (panel));
  clutter_actor_set_width (CLUTTER_ACTOR (panel), screen_width);

  mnb_drop_down_set_button (MNB_DROP_DOWN (panel), NBTK_BUTTON (button));
  clutter_actor_set_position (CLUTTER_ACTOR (panel), 0, TOOLBAR_HEIGHT);
  clutter_actor_lower_bottom (CLUTTER_ACTOR (panel));
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
                                  MutterPlugin     *plugin)
{
  MoblinNetbookPluginPrivate *priv = MOBLIN_NETBOOK_PLUGIN (plugin)->priv;
  const gchar                *name;
  gint                        col = -1;
  gint                        screen_width, screen_height;
  gint                        x, y;

  mutter_plugin_query_screen_size (plugin, &screen_width, &screen_height);

  name = clutter_actor_get_name (icon);

  if (!name || !*name)
    return;

  if (!strcmp (name, "tray-button-bluetooth"))
    col = 3;
  else if (!strcmp (name, "tray-button-wifi"))
    col = 2;
  else if (!strcmp (name, "tray-button-sound"))
    col = 1;
  else if (!strcmp (name, "tray-button-battery"))
    col = 0;
  else if (!strcmp (name, "tray-button-test"))
    col = 4;

  if (col < 0)
    return;

  y = TOOLBAR_HEIGHT - TRAY_BUTTON_HEIGHT;
  x = screen_width - (col + 1) * (TRAY_BUTTON_WIDTH + TRAY_PADDING);

  clutter_actor_set_position (icon, x, y);
  clutter_container_add_actor (CLUTTER_CONTAINER (priv->toolbar), icon);
}

static void
shell_tray_manager_icon_removed_cb (ShellTrayManager *mgr,
                                    ClutterActor     *icon,
                                    MutterPlugin     *plugin)
{
  clutter_actor_destroy (icon);
}
#endif

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
                    G_CALLBACK (shell_tray_manager_icon_added_cb), plugin);

  g_signal_connect (priv->tray_manager, "tray-icon-removed",
                    G_CALLBACK (shell_tray_manager_icon_removed_cb), plugin);

  shell_tray_manager_manage_stage (priv->tray_manager,
                                   CLUTTER_STAGE (
                                           mutter_plugin_get_stage (plugin)));

  mnb_toolbar_setup_kbd_grabs (MNB_TOOLBAR (self));

  /*
   * Disable autohiding of the panel to start with.
   */
  priv->dont_autohide = TRUE;
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

}

/* return NULL if no panel active */
const gchar *
mnb_toolbar_get_active_panel_name (MnbToolbar *toolbar)
{

}

/* Are we animating in or out */
gboolean
mnb_toolbar_in_transition (MnbToolbar *toolbar)
{

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
  MutterPlugin           *plugin   = map_data->toolbar->priv->plugin;
  MutterWindow           *mcw      = map_data->mcw;

  g_free (map_data);

  /* Notify the manager that we are done with this effect */
  mutter_plugin_effect_completed (plugin, mcw, MUTTER_PLUGIN_MAP);
}

struct config_hide_data
{
  MnbToolbar *toolbar;
  Window      config_xwin;
};

/*
 * TODO -- extract the entire dropdown creation for the tray panels out of here
 * into MnbToolbar.
 */
static void
tray_actor_hide_completed_cb (ClutterActor *actor, gpointer data)
{
  struct config_hide_data *hide_data = data;
  MnbToolbar              *toolbar = hide_data->toolbar;
  ShellTrayManager        *tmgr = toolbar->priv->tray_manager;

  shell_tray_manager_close_config_window (tmgr, hide_data->config_xwin);
}

static void
tray_actor_hide_begin_cb (ClutterActor *actor, gpointer data)
{
  struct config_hide_data *hide_data = data;
  MnbToolbar              *toolbar = hide_data->toolbar;
  ShellTrayManager        *tmgr = toolbar->priv->tray_manager;

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
  ClutterActor *texture = mutter_window_get_texture (mcw);

  Window xwin = mutter_window_get_x_window (mcw);

  gint  x = clutter_actor_get_x (actor);
  gint  y = clutter_actor_get_y (actor);

  background = CLUTTER_ACTOR (mnb_drop_down_new ());

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

  clutter_container_add_actor (CLUTTER_CONTAINER (toolbar), background);

#if 0
  /* TODO */
  /*
   * Hide all other dropdowns.
   */
  show_panel_and_control (plugin, MNBK_CONTROL_UNKNOWN);
#endif

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

NbtkWidget *
mnb_toolbar_get_switcher (MnbToolbar *toolbar)
{
  MnbToolbarPrivate *priv = toolbar->priv;

  return priv->panels[SPACES_ZONE];
}

void
mnb_toolbar_set_dont_autohide (MnbToolbar *toolbar, gboolean dont)
{
  MnbToolbarPrivate *priv = toolbar->priv;

  priv->dont_autohide = dont;
}

/*
 * Sets up passive key grabs on any dedicated shortcut keys that we cannot
 * hook into throught metacity key bindings.
 *
 * Needs to be public, because the kbd grabs need to be re-establised by the
 * alt_tab handler.
 */
void
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
