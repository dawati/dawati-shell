/* mnb-toolbar.c */

#include "moblin-netbook.h"

#include "mnb-toolbar.h"
#include "mnb-toolbar-button.h"
#include "mnb-drop-down.h"
#include "mnb-switcher.h"

#define BUTTON_WIDTH 66
#define BUTTON_HEIGHT 55
#define BUTTON_SPACING 10

#define TRAY_PADDING   3
#define TRAY_BUTTON_HEIGHT 55
#define TRAY_BUTTON_WIDTH 44

#define TOOLBAR_HEIGHT 64
#define TOOLBAR_X_PADDING 4

G_DEFINE_TYPE (MnbToolbar, mnb_toolbar, NBTK_TYPE_BIN)

#define MNB_TOOLBAR_GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MNB_TYPE_TOOLBAR, MnbToolbarPrivate))

enum {
    M_ZONE = 0,
    STATUS_ZONE,
    SPACES_ZONE,
    INTERNET_ZONE,
    MEDIA_ZONE,
    APPS_ZONE,
    PEOPLE_ZONE,
    PASTEBOARD_ZONE,

    /* LAST */
    NUM_ZONES
};

enum {
  PROP_0,

  PROP_MUTTER_PLUGIN,
};

struct _MnbToolbarPrivate {
  MutterPlugin *plugin;

  ClutterActor *hbox;

  NbtkWidget *time;
  NbtkWidget *date;

  NbtkWidget *buttons[NUM_ZONES];
  NbtkWidget *panels[NUM_ZONES];

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

static void
mnb_toolbar_show (ClutterActor *actor)
{
  MnbToolbarPrivate *priv = MNB_TOOLBAR (actor)->priv;
  gint width;
  guint screen_width, screen_height;

  mutter_plugin_query_screen_size (priv->plugin, &screen_width, &screen_height);

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

  clutter_actor_animate (actor, CLUTTER_LINEAR, 150, "y", 0, NULL);
}

static void
mnb_toolbar_hide_completed_cb (ClutterTimeline *timeline,
                             ClutterActor    *actor)
{
  CLUTTER_ACTOR_CLASS (mnb_toolbar_parent_class)->hide (actor);
}

static void
mnb_toolbar_hide (ClutterActor *actor)
{
  MnbToolbarPrivate *priv = MNB_TOOLBAR (actor)->priv;
  gint height;
  ClutterAnimation *animation;

  if (priv->input_region)
    {
      moblin_netbook_input_region_remove (priv->plugin,
                                          priv->input_region);

      priv->input_region = NULL;
    }

  height = clutter_actor_get_height (actor);

  animation = clutter_actor_animate (actor, CLUTTER_LINEAR, 150,
                                     "y", -height, NULL);

  g_signal_connect (clutter_animation_get_timeline (animation),
                    "completed",
                    G_CALLBACK (mnb_toolbar_hide_completed_cb),
                    actor);
}

static void mnb_toolbar_constructed (GObject *self);

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

static void
mnb_toolbar_append_panel (MnbToolbar  *toolbar,
                          const gchar *name,
                          const gchar *tooltip,
                          Window       xid)
{
  MnbToolbarPrivate *priv = MNB_TOOLBAR (toolbar)->priv;
  NbtkWidget *button;
  NbtkWidget *panel;
  guint       screen_width, screen_height;
  guint       index;
  gboolean    internal = FALSE;

  if (!strcmp (name, "m_zone"))
    index = M_ZONE;
  else if (!strcmp (name, "status_zone"))
    index = STATUS_ZONE;
  else if (!strcmp (name, "spaces_zone"))
    index = SPACES_ZONE;
  else if (!strcmp (name, "spaces_zone_internal"))
    {
      index = SPACES_ZONE;
      internal = TRUE;
    }
  else if (!strcmp (name, "internet_zone"))
    index = INTERNET_ZONE;
  else if (!strcmp (name, "media_zone"))
    index = MEDIA_ZONE;
  else if (!strcmp (name, "applications_zone"))
    index = APPS_ZONE;
  else if (!strcmp (name, "people_zone"))
    index = PEOPLE_ZONE;
  else if (!strcmp (name, "pasteboard_zone"))
    index = PASTEBOARD_ZONE;
  else
    return;

  mutter_plugin_query_screen_size (priv->plugin,
                                   &screen_width, &screen_height);

  button = priv->buttons[index] = mnb_toolbar_button_new ();
  nbtk_button_set_toggle_mode (NBTK_BUTTON (button), TRUE);
  nbtk_button_set_tooltip (NBTK_BUTTON (button), tooltip);
  clutter_actor_set_name (CLUTTER_ACTOR (button), name);
  clutter_actor_set_size (CLUTTER_ACTOR (button),
			  BUTTON_WIDTH, BUTTON_HEIGHT);
  clutter_actor_set_position (CLUTTER_ACTOR (button),
                              213 + (BUTTON_WIDTH * index)
                              + (BUTTON_SPACING * index),
                              TOOLBAR_HEIGHT - BUTTON_HEIGHT);

  mnb_panel_button_set_reactive_area (MNB_PANEL_BUTTON (button),
                                      0,
                                      -(TOOLBAR_HEIGHT - BUTTON_HEIGHT),
                                      BUTTON_WIDTH,
                                      TOOLBAR_HEIGHT);

  clutter_container_add_actor (CLUTTER_CONTAINER (priv->hbox),
                               CLUTTER_ACTOR (button));

  g_signal_connect (button, "clicked",
                    G_CALLBACK (mnb_toolbar_toggle_buttons),
                    toolbar);

  /*
   * Special case Space; we have an internal component for this, but will
   * allow it to be replaced by a thirdparty component just like all the
   * other dropdowns.
   */
  if (index == SPACES_ZONE && internal)
    {
      panel = priv->panels[index] = mnb_switcher_new (priv->plugin);
    }
  else
    {
      panel = priv->panels[index] = mnb_drop_down_new ();

      /*
       * TODO -- create the contents from the xid and insert into the dropdown
       */
    }

  clutter_container_add_actor (CLUTTER_CONTAINER (priv->hbox),
                               CLUTTER_ACTOR (panel));
  clutter_actor_set_width (CLUTTER_ACTOR (panel), screen_width);

  mnb_drop_down_set_button (MNB_DROP_DOWN (panel), NBTK_BUTTON (button));
  clutter_actor_set_position (CLUTTER_ACTOR (panel), 0, PANEL_HEIGHT);
  clutter_actor_lower_bottom (CLUTTER_ACTOR (panel));
}

static void
mnb_toolbar_init (MnbToolbar *self)
{
  MnbToolbarPrivate *priv;

  priv = self->priv = MNB_TOOLBAR_GET_PRIVATE (self);
}

static void
mnb_toolbar_constructed (GObject *self)
{
  MnbToolbarPrivate *priv = MNB_TOOLBAR (self)->priv;
  ClutterActor *actor = CLUTTER_ACTOR (self);
  ClutterActor *hbox;
  guint         screen_width, screen_height;

  hbox = priv->hbox = clutter_group_new ();

  g_object_set (self,
                "show-on-set-parent", FALSE,
                NULL);

  mutter_plugin_query_screen_size (priv->plugin,
                                   &screen_width, &screen_height);

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
