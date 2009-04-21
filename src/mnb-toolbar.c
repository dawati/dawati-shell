/* mnb-toolbar.c */

#include "mnb-toolbar.h"

#include "mnb-panel-button.h"

#define BUTTON_WIDTH 66
#define BUTTON_HEIGHT 55
#define BUTTON_SPACING 10

#define TRAY_PADDING   3
#define TRAY_BUTTON_HEIGHT 55
#define TRAY_BUTTON_WIDTH 44

#define TOOLBAR_HEIGHT 64
#define TOOLBAR_X_PADDING 4

G_DEFINE_TYPE (MnbToolbar, mnb_toolbar, NBTK_TYPE_TABLE)

#define MNB_TOOLBAR_GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MNB_TYPE_TOOLBAR, MnbToolbarPrivate))

enum {
    M_ZONE,
    STATUS_ZONE,
    SPACES_ZONE,
    INTERNET_ZONE,
    MEDIA_ZONE,
    APPS_ZONE,
    PEOPLE_ZONE,
    PASTEBOARD_ZONE,
    NUM_ZONES
};

struct _MnbToolbarPrivate {
    NbtkWidget *time;
    NbtkWidget *date;

    NbtkWidget *buttons[NUM_ZONES];
    NbtkWidget *drop_downs[NUM_ZONES];
};



static void
mnb_toolbar_get_property (GObject *object, guint property_id,
                              GValue *value, GParamSpec *pspec)
{
  switch (property_id) {
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
mnb_toolbar_set_property (GObject *object, guint property_id,
                              const GValue *value, GParamSpec *pspec)
{
  switch (property_id) {
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
  gint width;

  /* set initial width and height */
  clutter_actor_set_position (actor, 0, -(clutter_actor_get_height (actor)));
  width = clutter_actor_get_width (clutter_actor_get_parent (actor));
  clutter_actor_set_width (actor, width);

  CLUTTER_ACTOR_CLASS (mnb_toolbar_parent_class)->show (actor);

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
  gint height;
  ClutterAnimation *animation;

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

static NbtkWidget*
mnb_toolbar_append_toolbar_button (MnbToolbar   *toolbar,
				   gchar      *name,
				   gchar      *tooltip)
{
  static int n_buttons = 0;
  NbtkWidget *button;

  /* FIXME: rename to toolbar button */
  button = mnb_panel_button_new ();
  nbtk_button_set_toggle_mode (NBTK_BUTTON (button), TRUE);
  nbtk_button_set_tooltip (NBTK_BUTTON (button), tooltip);
  clutter_actor_set_name (CLUTTER_ACTOR (button), name);
  clutter_actor_set_size (CLUTTER_ACTOR (button), 
			  BUTTON_WIDTH, BUTTON_HEIGHT);
  clutter_actor_set_position (CLUTTER_ACTOR (button),
                              213 + (BUTTON_WIDTH * n_buttons)
                              + (BUTTON_SPACING * n_buttons),
                              TOOLBAR_HEIGHT - BUTTON_HEIGHT);

  mnb_panel_button_set_reactive_area (MNB_PANEL_BUTTON (button),
                                      0,
                                      -(TOOLBAR_HEIGHT - BUTTON_HEIGHT),
                                      BUTTON_WIDTH,
                                      TOOLBAR_HEIGHT);

  clutter_container_add_actor (CLUTTER_CONTAINER (toolbar),
                               CLUTTER_ACTOR (button));

  /* FIXME - errors
  g_object_set (G_OBJECT (button),
                "transition-type", NBTK_TRANSITION_BOUNCE,
                "transition-duration", 500, NULL);
  */

  if (!strcmp(tooltip, "people"))
    {
      clutter_actor_set_opacity (CLUTTER_ACTOR (button), 0x60);
      nbtk_button_set_tooltip (NBTK_BUTTON (button),
                               "☠ NOT IMPLEMENTED ☠");
    }
  /*
  else
    g_signal_connect_data (button, "clicked", G_CALLBACK (toggle_buttons_cb),
                           button_data, (GClosureNotify)g_free, 0);
  */

  n_buttons++;

  return NBTK_WIDGET (button);
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

  g_object_set (self,
                "show-on-set-parent", FALSE,
                NULL);

  /* create time and date labels */
  priv->time = nbtk_label_new ("");
  clutter_actor_set_name (CLUTTER_ACTOR (priv->time), "time-label");
  clutter_container_add_actor (CLUTTER_CONTAINER (self),
                               CLUTTER_ACTOR (priv->time));
  clutter_actor_set_position (CLUTTER_ACTOR (priv->time),
       (192 / 2) - clutter_actor_get_width (CLUTTER_ACTOR (priv->time)) /
                              2, 8);

  priv->date = nbtk_label_new ("");
  clutter_actor_set_name (CLUTTER_ACTOR (priv->date), "date-label");

  clutter_container_add_actor (CLUTTER_CONTAINER (self),
                               CLUTTER_ACTOR (priv->date));
  clutter_actor_set_position (CLUTTER_ACTOR (priv->date),
       (192 / 2) - clutter_actor_get_width (CLUTTER_ACTOR (priv->date)) /
                              2, 40);

  mnb_toolbar_update_time_date (priv);
  g_timeout_add_seconds (60, (GSourceFunc) mnb_toolbar_update_time_date, priv);
}

NbtkWidget*
mnb_toolbar_new (void)
{
  return g_object_new (MNB_TYPE_TOOLBAR, NULL);
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
