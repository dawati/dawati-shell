
#include "mwb-radical-bar.h"
#include "mwb-utils.h"

G_DEFINE_TYPE (MwbRadicalBar, mwb_radical_bar, NBTK_TYPE_WIDGET)

#define RADICAL_BAR_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MWB_TYPE_RADICAL_BAR, \
   MwbRadicalBarPrivate))

enum
{
  PROP_0,

  PROP_ICON,
  PROP_TEXT,
  PROP_LOADING,
  PROP_PROGRESS,
};

enum
{
  GO,
  RELOAD,
  STOP,
  
  LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0, };

struct _MwbRadicalBarPrivate
{
  NbtkWidget   *table;
  ClutterActor *progress_bar;
  ClutterActor *icon;
  NbtkWidget   *entry;
  NbtkWidget   *button;
  
  gboolean      loading;
  gdouble       progress;
  gboolean      text_changed;
};

static void
mwb_radical_bar_get_property (GObject *object, guint property_id,
                              GValue *value, GParamSpec *pspec)
{
  MwbRadicalBar *radical_bar = MWB_RADICAL_BAR (object);
  
  switch (property_id)
    {
    case PROP_ICON:
      g_value_set_object (value, mwb_radical_bar_get_icon (radical_bar));
      break;
    
    case PROP_TEXT:
      g_value_set_string (value, mwb_radical_bar_get_text (radical_bar));
      break;
    
    case PROP_LOADING:
      g_value_set_boolean (value, mwb_radical_bar_get_loading (radical_bar));
      break;
    
    case PROP_PROGRESS:
      g_value_set_double (value, mwb_radical_bar_get_progress (radical_bar));
      break;
    
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
mwb_radical_bar_set_property (GObject *object, guint property_id,
                              const GValue *value, GParamSpec *pspec)
{
  MwbRadicalBar *radical_bar = MWB_RADICAL_BAR (object);
  
  switch (property_id)
    {
    case PROP_ICON:
      mwb_radical_bar_set_icon (radical_bar, g_value_get_object (value));
      break;
    
    case PROP_TEXT:
      mwb_radical_bar_set_text (radical_bar, g_value_get_string (value));
      break;
    
    case PROP_LOADING:
      mwb_radical_bar_set_loading (radical_bar, g_value_get_boolean (value));
      break;
    
    case PROP_PROGRESS:
      mwb_radical_bar_set_progress (radical_bar, g_value_get_double (value));
      break;
    
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
mwb_radical_bar_dispose (GObject *object)
{
  MwbRadicalBarPrivate *priv = MWB_RADICAL_BAR (object)->priv;
  
  if (priv->progress_bar)
    {
      clutter_actor_unparent (priv->progress_bar);
      priv->progress_bar = NULL;
    }
  
  G_OBJECT_CLASS (mwb_radical_bar_parent_class)->dispose (object);
}

static void
mwb_radical_bar_finalize (GObject *object)
{
  G_OBJECT_CLASS (mwb_radical_bar_parent_class)->finalize (object);
}

static void
mwb_radical_bar_resize_progress (MwbRadicalBar         *self,
                                 const ClutterActorBox *box,
                                 gboolean               aoc)
{
  ClutterUnit width, padding;
  ClutterActorBox child_box;
  
  MwbRadicalBarPrivate *priv = self->priv;
  
  padding = CLUTTER_UNITS_FROM_INT (6);
  
  width = CLUTTER_UNITS_FROM_FIXED (
    CLUTTER_FIXED_MUL (
      CLUTTER_UNITS_TO_FIXED (box->x2 - box->x1 - (padding * 2)),
      CLUTTER_FLOAT_TO_FIXED (priv->progress)));

  child_box.x1 = child_box.y1 = padding;
  child_box.x2 = child_box.x1 + width;
  child_box.y2 = child_box.y1 + (box->y2 - box->y1) - (padding * 2);
  clutter_actor_allocate (priv->progress_bar, &child_box, aoc);
}

static void
mwb_radical_bar_allocate (ClutterActor          *actor,
                          const ClutterActorBox *box,
                          gboolean               absolute_origin_changed)
{
  NbtkPadding padding;
  ClutterActorBox child_box;
  
  MwbRadicalBar *self = MWB_RADICAL_BAR (actor);
  MwbRadicalBarPrivate *priv = self->priv;
  
  CLUTTER_ACTOR_CLASS (mwb_radical_bar_parent_class)->
    allocate (actor, box, absolute_origin_changed);
  
  nbtk_widget_get_padding (NBTK_WIDGET (actor), &padding);

  child_box.x1 = MWB_PIXBOUND (padding.left);
  child_box.y1 = MWB_PIXBOUND (padding.top);
  child_box.x2 = box->x2 - box->x1 - padding.right;
  child_box.y2 = box->y2 - box->y1 - padding.bottom;

  clutter_actor_allocate (CLUTTER_ACTOR (priv->table),
                          &child_box, absolute_origin_changed);
  
  mwb_radical_bar_resize_progress (self, box, absolute_origin_changed);
}

static void
mwb_radical_bar_get_preferred_width (ClutterActor *actor,
                                     ClutterUnit   for_height,
                                     ClutterUnit  *min_width_p,
                                     ClutterUnit  *natural_width_p)
{
  ClutterUnit min_width, natural_width;
  
  MwbRadicalBarPrivate *priv = MWB_RADICAL_BAR (actor)->priv;
  
  CLUTTER_ACTOR_CLASS (mwb_radical_bar_parent_class)->
    get_preferred_width (actor, for_height, min_width_p, natural_width_p);
  
  clutter_actor_get_preferred_width (CLUTTER_ACTOR (priv->table),
                                     for_height,
                                     &min_width,
                                     &natural_width);
  
  if (min_width_p)
    *min_width_p += min_width;
  
  if (natural_width_p)
    *natural_width_p += natural_width;
}

static void
mwb_radical_bar_get_preferred_height (ClutterActor *actor,
                                      ClutterUnit   for_width,
                                      ClutterUnit  *min_height_p,
                                      ClutterUnit  *natural_height_p)
{
  ClutterUnit min_height, natural_height;
  
  MwbRadicalBarPrivate *priv = MWB_RADICAL_BAR (actor)->priv;
  
  CLUTTER_ACTOR_CLASS (mwb_radical_bar_parent_class)->
    get_preferred_height (actor, for_width, min_height_p, natural_height_p);
  
  clutter_actor_get_preferred_height (CLUTTER_ACTOR (priv->table),
                                      for_width,
                                      &min_height,
                                      &natural_height);
  
  if (min_height_p)
    *min_height_p += min_height;
  
  if (natural_height_p)
    *natural_height_p += natural_height;
}

static void
mwb_radical_bar_paint (ClutterActor *actor)
{
  MwbRadicalBarPrivate *priv = MWB_RADICAL_BAR (actor)->priv;
  
  /* Chain up to get the background */
  CLUTTER_ACTOR_CLASS (mwb_radical_bar_parent_class)->paint (actor);
  
  if (CLUTTER_ACTOR_IS_VISIBLE (priv->progress_bar))
    clutter_actor_paint (priv->progress_bar);
  
  clutter_actor_paint (CLUTTER_ACTOR (priv->table));
}

static void
mwb_radical_bar_pick (ClutterActor *actor, const ClutterColor *color)
{
  mwb_radical_bar_paint (actor);
}

static void
mwb_radical_bar_class_init (MwbRadicalBarClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  ClutterActorClass *actor_class = CLUTTER_ACTOR_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MwbRadicalBarPrivate));

  object_class->get_property = mwb_radical_bar_get_property;
  object_class->set_property = mwb_radical_bar_set_property;
  object_class->dispose = mwb_radical_bar_dispose;
  object_class->finalize = mwb_radical_bar_finalize;
  
  actor_class->allocate = mwb_radical_bar_allocate;
  actor_class->get_preferred_width = mwb_radical_bar_get_preferred_width;
  actor_class->get_preferred_height = mwb_radical_bar_get_preferred_height;
  actor_class->paint = mwb_radical_bar_paint;
  actor_class->pick = mwb_radical_bar_pick;

  g_object_class_install_property (object_class,
                                   PROP_ICON,
                                   g_param_spec_object ("icon",
                                                        "Icon",
                                                        "Icon actor.",
                                                        CLUTTER_TYPE_ACTOR,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_STATIC_NAME |
                                                        G_PARAM_STATIC_NICK |
                                                        G_PARAM_STATIC_BLURB));

  g_object_class_install_property (object_class,
                                   PROP_TEXT,
                                   g_param_spec_string ("text",
                                                        "Text",
                                                        "RadicalBar text.",
                                                        "",
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_STATIC_NAME |
                                                        G_PARAM_STATIC_NICK |
                                                        G_PARAM_STATIC_BLURB));

  g_object_class_install_property (object_class,
                                   PROP_LOADING,
                                   g_param_spec_boolean ("loading",
                                                         "Loading",
                                                         "Loading.",
                                                         FALSE,
                                                         G_PARAM_READWRITE |
                                                         G_PARAM_STATIC_NAME |
                                                         G_PARAM_STATIC_NICK |
                                                         G_PARAM_STATIC_BLURB));

  g_object_class_install_property (object_class,
                                   PROP_PROGRESS,
                                   g_param_spec_double ("progress",
                                                        "Progress",
                                                        "Load progress.",
                                                        0.0, 1.0, 0.0,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_STATIC_NAME |
                                                        G_PARAM_STATIC_NICK |
                                                        G_PARAM_STATIC_BLURB));

  signals[GO] =
    g_signal_new ("go",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (MwbRadicalBarClass, go),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__STRING,
                  G_TYPE_NONE, 1, G_TYPE_STRING);

  signals[RELOAD] =
    g_signal_new ("reload",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (MwbRadicalBarClass, reload),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  signals[STOP] =
    g_signal_new ("stop",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (MwbRadicalBarClass, stop),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);
}

static void
mwb_radical_bar_button_clicked_cb (NbtkButton *button, MwbRadicalBar *self)
{
  MwbRadicalBarPrivate *priv = self->priv;
  
  if (priv->loading)
    g_signal_emit (self, signals[STOP], 0);
  else
    {
      if (priv->text_changed)
        g_signal_emit (self, signals[GO], 0,
                       nbtk_entry_get_text (NBTK_ENTRY (priv->entry)));
      else
        g_signal_emit (self, signals[RELOAD], 0);
    }
}

static void
mwb_radical_bar_update_button (MwbRadicalBar *self)
{
  MwbRadicalBarPrivate *priv = self->priv;
  
  nbtk_button_set_icon_from_file (NBTK_BUTTON (priv->button),
                                  priv->loading ?
                                    PKGDATADIR "/stop.png" :
                                    (priv->text_changed ?
                                       PKGDATADIR "/play.png" :
                                       PKGDATADIR "/reload.png"));
}

static void
mwb_radical_bar_text_changed_cb (ClutterText *text, MwbRadicalBar *self)
{
  MwbRadicalBarPrivate *priv = self->priv;
  
  if (!priv->text_changed)
    {
      priv->text_changed = TRUE;
      mwb_radical_bar_update_button (self);
    }
}

static void
mwb_radical_bar_activate_cb (ClutterText *text, MwbRadicalBar *self)
{
  MwbRadicalBarPrivate *priv = self->priv;
  
  if (priv->text_changed)
    g_signal_emit (self, signals[GO], 0, clutter_text_get_text (text));
  else
    g_signal_emit (self, signals[RELOAD], 0);
}

static void
mwb_radical_bar_init (MwbRadicalBar *self)
{
  ClutterActor *text, *loading_texture, *separator;
  
  MwbRadicalBarPrivate *priv = self->priv = RADICAL_BAR_PRIVATE (self);

  priv->text_changed = TRUE;

  priv->table = nbtk_table_new ();
  priv->entry = nbtk_entry_new ("");
  priv->button = nbtk_button_new ();
  
  loading_texture =
    clutter_texture_new_from_file (PKGDATADIR "/progress.png", NULL);
  priv->progress_bar =
    nbtk_texture_frame_new (CLUTTER_TEXTURE (loading_texture),
                            3, 3, 3, 3);
  clutter_actor_set_parent (priv->progress_bar, CLUTTER_ACTOR (self));
  clutter_actor_hide (priv->progress_bar);
  
  text = nbtk_entry_get_clutter_text (NBTK_ENTRY (priv->entry));
  
  mwb_radical_bar_update_button (self);
  
  g_signal_connect (priv->button, "clicked",
                    G_CALLBACK (mwb_radical_bar_button_clicked_cb), self);
  g_signal_connect (text, "text-changed",
                    G_CALLBACK (mwb_radical_bar_text_changed_cb), self);
  g_signal_connect (text, "activate",
                    G_CALLBACK (mwb_radical_bar_activate_cb), self);
  
  clutter_actor_set_reactive (text, TRUE);
  g_signal_connect (text, "button-press-event",
                    G_CALLBACK (mwb_utils_focus_on_click_cb),
                    GINT_TO_POINTER (FALSE));
  
  nbtk_table_set_col_spacing (NBTK_TABLE (priv->table), 6);

  nbtk_table_add_widget_full (NBTK_TABLE (priv->table), priv->entry, 0, 1, 1, 1,
                              NBTK_X_EXPAND | NBTK_X_FILL | NBTK_Y_EXPAND,
                              0.0, 0.5);
  separator = clutter_texture_new_from_file (PKGDATADIR "/entry-separator.png",
                                             NULL);
  nbtk_table_add_actor_full (NBTK_TABLE (priv->table), separator,
                             0, 2, 1, 1, 0, 0.5, 0.5);
  nbtk_table_add_widget_full (NBTK_TABLE (priv->table), priv->button,
                              0, 3, 1, 1, NBTK_KEEP_ASPECT_RATIO, 1.0, 0.5);
  
  clutter_actor_set_parent (CLUTTER_ACTOR (priv->table), CLUTTER_ACTOR (self));
}

NbtkWidget*
mwb_radical_bar_new (void)
{
  return NBTK_WIDGET (g_object_new (MWB_TYPE_RADICAL_BAR, NULL));
}

void
mwb_radical_bar_focus (MwbRadicalBar *radical_bar)
{
  ClutterActor *text;
  MwbRadicalBarPrivate *priv = radical_bar->priv;
  
  text = nbtk_entry_get_clutter_text (NBTK_ENTRY (priv->entry));
  mwb_utils_focus_on_click_cb (text, NULL, NULL);
}

void
mwb_radical_bar_set_text (MwbRadicalBar *radical_bar, const gchar *text)
{
  MwbRadicalBarPrivate *priv = radical_bar->priv;
  
  if (!text)
    text = "";
  
  nbtk_entry_set_text (NBTK_ENTRY (priv->entry), text);

  priv->text_changed = FALSE;
  mwb_radical_bar_update_button (radical_bar);

  g_object_notify (G_OBJECT (radical_bar), "text");
}

void
mwb_radical_bar_set_icon (MwbRadicalBar *radical_bar, ClutterActor *icon)
{
  MwbRadicalBarPrivate *priv = radical_bar->priv;
  
  if (priv->icon)
    clutter_container_remove_actor (CLUTTER_CONTAINER (priv->table),
                                    priv->icon);
  
  priv->icon = icon;

  if (icon)
    nbtk_table_add_actor_full (NBTK_TABLE (priv->table), priv->icon,
                               0, 0, 1, 1, NBTK_KEEP_ASPECT_RATIO, 0, 0.5);
  
  g_object_notify (G_OBJECT (radical_bar), "icon");
}

void
mwb_radical_bar_set_loading (MwbRadicalBar *radical_bar, gboolean loading)
{
  MwbRadicalBarPrivate *priv = radical_bar->priv;
  
  if (priv->loading == loading)
    return;
  
  if (!priv->loading)
    {
      priv->text_changed = FALSE;
      clutter_actor_show (priv->progress_bar);
    }
  else
    clutter_actor_hide (priv->progress_bar);
  
  priv->loading = loading;
  
  mwb_radical_bar_update_button (radical_bar);
  
  g_object_notify (G_OBJECT (radical_bar), "loading");
}

void
mwb_radical_bar_set_progress (MwbRadicalBar *radical_bar, gdouble progress)
{
  ClutterActorBox box;
  MwbRadicalBarPrivate *priv = radical_bar->priv;
  
  priv->progress = CLAMP (progress, 0.0, 1.0);
  
  clutter_actor_get_allocation_box (CLUTTER_ACTOR (radical_bar), &box);
  mwb_radical_bar_resize_progress (radical_bar, &box, FALSE);

  g_object_notify (G_OBJECT (radical_bar), "progress");
  
  clutter_actor_queue_redraw (CLUTTER_ACTOR (radical_bar));
}

const gchar *
mwb_radical_bar_get_text (MwbRadicalBar *radical_bar)
{
  return nbtk_entry_get_text (NBTK_ENTRY (radical_bar->priv->entry));
}

ClutterActor *
mwb_radical_bar_get_icon (MwbRadicalBar *radical_bar)
{
  return radical_bar->priv->icon;
}

gboolean
mwb_radical_bar_get_loading (MwbRadicalBar *radical_bar)
{
  return radical_bar->priv->loading;
}

gdouble
mwb_radical_bar_get_progress (MwbRadicalBar *radical_bar)
{
  return radical_bar->priv->progress;
}

