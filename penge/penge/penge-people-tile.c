#include "penge-people-tile.h"

G_DEFINE_TYPE (PengePeopleTile, penge_people_tile, NBTK_TYPE_TABLE)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), PENGE_TYPE_PEOPLE_TILE, PengePeopleTilePrivate))

typedef struct _PengePeopleTilePrivate PengePeopleTilePrivate;

struct _PengePeopleTilePrivate {
    ClutterActor *body;
    ClutterActor *icon;
    NbtkWidget *primary_text;
    NbtkWidget *secondary_text;
};

enum
{
  PROP_0,
  PROP_BODY,
  PROP_ICON_PATH,
  PROP_PRIMARY_TEXT,
  PROP_SECONDARY_TEXT
};

static void
penge_people_tile_get_property (GObject *object, guint property_id,
                              GValue *value, GParamSpec *pspec)
{
  switch (property_id) {
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
penge_people_tile_set_property (GObject *object, guint property_id,
                              const GValue *value, GParamSpec *pspec)
{
  PengePeopleTilePrivate *priv = GET_PRIVATE (object);
  GError *error = NULL;

  switch (property_id) {
    case PROP_BODY:
      if (priv->body)
      {
        clutter_container_remove_actor (CLUTTER_CONTAINER (object),
                                        priv->body);
        g_object_unref (priv->body);
      }

      priv->body = g_value_dup_object (value);
      nbtk_table_add_actor (NBTK_TABLE (object),
                            priv->body,
                            0,
                            0);
      clutter_container_child_set (CLUTTER_CONTAINER (object),
                                   priv->body,
                                   "col-span",
                                   2,
                                   "y-expand",
                                   TRUE,
                                   NULL);
      break;
    case PROP_ICON_PATH:
      if (!clutter_texture_set_from_file (CLUTTER_TEXTURE (priv->icon), 
                                          g_value_get_string (value),
                                          &error))
      {
        g_critical (G_STRLOC ": error setting icon texture from file: %s",
                    error->message);
        g_clear_error (&error);
      }
      break;
    case PROP_PRIMARY_TEXT:
      nbtk_label_set_text (NBTK_LABEL (priv->primary_text),
                           g_value_get_string (value));
      break;
    case PROP_SECONDARY_TEXT:
      nbtk_label_set_text (NBTK_LABEL (priv->secondary_text),
                           g_value_get_string (value));
      break;

  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
penge_people_tile_dispose (GObject *object)
{
  /* TODO: Ensure we don't leave anything un'freed */
  G_OBJECT_CLASS (penge_people_tile_parent_class)->dispose (object);
}

static void
penge_people_tile_finalize (GObject *object)
{
  G_OBJECT_CLASS (penge_people_tile_parent_class)->finalize (object);
}

static void
penge_people_tile_class_init (PengePeopleTileClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GParamSpec *pspec;

  g_type_class_add_private (klass, sizeof (PengePeopleTilePrivate));

  object_class->get_property = penge_people_tile_get_property;
  object_class->set_property = penge_people_tile_set_property;
  object_class->dispose = penge_people_tile_dispose;
  object_class->finalize = penge_people_tile_finalize;

  pspec = g_param_spec_object ("body",
                               "Body",
                               "Actor for the body of the tile",
                               CLUTTER_TYPE_ACTOR,
                               G_PARAM_WRITABLE);
  g_object_class_install_property (object_class, PROP_BODY, pspec);

  pspec = g_param_spec_string ("icon-path",
                               "Icon path",
                               "Path to icon to use in the tile",
                               NULL,
                               G_PARAM_WRITABLE);
  g_object_class_install_property (object_class, PROP_ICON_PATH, pspec);

  pspec = g_param_spec_string ("primary-text",
                               "Primary text",
                               "Primary text for the tile",
                               NULL,
                               G_PARAM_WRITABLE);
  g_object_class_install_property (object_class, PROP_PRIMARY_TEXT, pspec);

  pspec = g_param_spec_string ("secondary-text",
                               "Secondary text",
                               "Secondary text for the tile",
                               NULL,
                               G_PARAM_WRITABLE);
  g_object_class_install_property (object_class, PROP_SECONDARY_TEXT, pspec);
}

static gboolean
_enter_event_cb (ClutterActor *actor,
                 ClutterEvent *event,
                 gpointer      userdata)
{
  nbtk_widget_set_style_pseudo_class (NBTK_WIDGET (actor),
                                      "hover");
  return FALSE;
}

static gboolean
_leave_event_cb (ClutterActor *actor,
                 ClutterEvent *event,
                 gpointer      userdata)
{
  nbtk_widget_set_style_pseudo_class (NBTK_WIDGET (actor),
                                      NULL);
  return FALSE;
}

/*
 *        0                   1 
 *  *************************************
 *  *           |                       *
 *  *           |                       *
 *  *           | Body                  *   0
 *  *           |                       *
 *  *           |                       *
 *  *           |                       *
 *  *-----------------------------------*
 *  *  ******** |                       *   1
 *  *  *      * |  Primary text         *
 *  * -* Icon *-|-----------------------*
 *  *  *      * |  Secondary text       * 
 *  *  ******** |                       *   2
 *  *           |                       *
 *  *************************************
 *
 *  Body is in 0,0 with column span of 2
 *  Icon is in 1,0 with row span of 2
 *  Primary text is in 1,1;
 *  Secondary text is in 2,1
 */
static void
penge_people_tile_init (PengePeopleTile *self)
{
  PengePeopleTilePrivate *priv = GET_PRIVATE (self);
  ClutterActor *tmp_label;
  NbtkPadding padding = { CLUTTER_UNITS_FROM_DEVICE (8),
                          CLUTTER_UNITS_FROM_DEVICE (8),
                          CLUTTER_UNITS_FROM_DEVICE (8),
                          CLUTTER_UNITS_FROM_DEVICE (8) };

  priv->primary_text = nbtk_label_new ("Primary text");
  nbtk_widget_set_style_class_name (priv->primary_text, 
                                    "PengePeopleTilePrimaryLabel");
  nbtk_widget_set_alignment (priv->primary_text, 0, 0.5);
  tmp_label = nbtk_label_get_clutter_text (NBTK_LABEL (priv->primary_text));
  clutter_text_set_alignment (CLUTTER_TEXT (tmp_label),
                               PANGO_ALIGN_LEFT);
  clutter_text_set_ellipsize (CLUTTER_TEXT (tmp_label), 
                               PANGO_ELLIPSIZE_END);

  priv->secondary_text = nbtk_label_new ("Secondary text");
  nbtk_widget_set_style_class_name (priv->secondary_text, 
                                    "PengePeopleTileSecondaryLabel");
  nbtk_widget_set_alignment (priv->secondary_text, 0, 0.5);
  tmp_label = nbtk_label_get_clutter_text (NBTK_LABEL (priv->secondary_text));
  clutter_text_set_alignment (CLUTTER_TEXT (tmp_label),
                               PANGO_ALIGN_LEFT);
  clutter_text_set_ellipsize (CLUTTER_TEXT (tmp_label), 
                               PANGO_ELLIPSIZE_END);

  priv->icon = clutter_texture_new ();
  clutter_actor_set_size (priv->icon, 28, 28);

  nbtk_table_add_actor (NBTK_TABLE (self),
                        (ClutterActor *)priv->primary_text,
                        1,
                        1);

  clutter_container_child_set (CLUTTER_CONTAINER (self),
                               (ClutterActor *)priv->primary_text,
                               "x-expand",
                               TRUE,
                               "y-expand",
                               FALSE,
                               NULL);


  nbtk_table_add_actor (NBTK_TABLE (self),
                        (ClutterActor *)priv->secondary_text,
                        2,
                        1);
  clutter_container_child_set (CLUTTER_CONTAINER (self),
                               (ClutterActor *)priv->secondary_text,
                               "x-expand",
                               TRUE,
                               "y-expand",
                               FALSE,
                               NULL);

  /*
   * Explicitly set the width to 100 to avoid overflowing text. Slightly
   * hackyish but works around a strange bug in NbtkTable where if the text
   * is too long it will cause negative positioning of the icon.
   */
  clutter_actor_set_width ((ClutterActor *)priv->primary_text, 100);
  clutter_actor_set_width ((ClutterActor *)priv->secondary_text, 100);

  nbtk_table_add_actor (NBTK_TABLE (self),
                        priv->icon,
                        1,
                        0);
  clutter_container_child_set (CLUTTER_CONTAINER (self),
                               priv->icon,
                               "row-span",
                               2,
                               "keep-aspect-ratio",
                               TRUE,
                               "y-expand",
                               FALSE,
                               NULL);

  nbtk_table_set_row_spacing (NBTK_TABLE (self), 4);
  nbtk_table_set_col_spacing (NBTK_TABLE (self), 4);

  nbtk_widget_set_padding (NBTK_WIDGET (self), &padding);

  g_signal_connect (self,
                    "enter-event",
                    (GCallback)_enter_event_cb,
                    NULL);
  g_signal_connect (self,
                    "leave-event",
                    (GCallback)_leave_event_cb,
                    NULL);
}


