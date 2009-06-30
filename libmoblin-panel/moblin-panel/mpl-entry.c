#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <glib.h>
#include <glib/gi18n.h>
#include <stdlib.h>

#include "mpl-clutter-theme.h"
#include "mpl-entry.h"

#define MPL_ENTRY_GET_PRIVATE(obj) \
        (G_TYPE_INSTANCE_GET_PRIVATE ((obj), MPL_TYPE_ENTRY, MplEntryPrivate))

struct _MplEntryPrivate
{
  ClutterActor *entry;
  ClutterActor *table;
  ClutterActor *clear_button;
  ClutterActor *search_button;
};

enum
{
  PROP_0,

  PROP_LABEL,
  PROP_TEXT
};

enum
{
  BUTTON_CLICKED,
  TEXT_CHANGED,
  KEYNAV_EVENT,

  LAST_SIGNAL
};

static void clutter_container_iface_init (ClutterContainerIface *iface);

static guint _signals[LAST_SIGNAL] = { 0, };

G_DEFINE_TYPE_WITH_CODE (MplEntry, mpl_entry, NBTK_TYPE_BIN,
                         G_IMPLEMENT_INTERFACE (CLUTTER_TYPE_CONTAINER,
                                                clutter_container_iface_init));

static void
mpl_entry_foreach (ClutterContainer *container,
                   ClutterCallback   callback,
                   gpointer          user_data)
{
  MplEntryPrivate *priv = MPL_ENTRY (container)->priv;

  callback (priv->entry, user_data);
  callback (priv->table, user_data);
}

static void
clutter_container_iface_init (ClutterContainerIface *iface)
{
  iface->foreach = mpl_entry_foreach;
}

static void
search_button_clicked_cb (NbtkButton  *button,
                          MplEntry    *entry)
{
  g_signal_emit (entry, _signals[BUTTON_CLICKED], 0);
}

static void
clear_button_clicked_cb (NbtkButton *button,
                         MplEntry   *entry)
{
  nbtk_entry_set_text (NBTK_ENTRY (entry->priv->entry), NULL);
}

static void
text_changed_cb (ClutterText  *actor,
                 MplEntry     *entry)
{
  gchar const *text;

  text = clutter_text_get_text (actor);
  if (text && strlen (text) > 0)
    clutter_actor_show (entry->priv->clear_button);
  else
    clutter_actor_hide (entry->priv->clear_button);

  g_signal_emit (entry, _signals[TEXT_CHANGED], 0);
}

static gboolean
text_key_press_cb (ClutterActor     *actor,
                   ClutterKeyEvent  *event,
                   MplEntry         *entry)
{
  MplEntryPrivate *priv = entry->priv;
  ClutterText     *text;
  gint             pos;

  text = CLUTTER_TEXT (nbtk_entry_get_clutter_text (NBTK_ENTRY (priv->entry)));
  pos = clutter_text_get_cursor_position (text);

  switch (event->keyval)
    {
      case CLUTTER_Return:
      case CLUTTER_Left:
      case CLUTTER_Up:
      case CLUTTER_Right:
      case CLUTTER_Down:
        /* Only emit if caret at end of text. */
        if (pos == -1)
          {
            g_signal_emit (entry, _signals[KEYNAV_EVENT], 0, event->keyval);
            return TRUE;
          }
    }

  return FALSE;
}

static void
mpl_entry_get_preferred_width (ClutterActor *actor,
                               gfloat        for_height,
                               gfloat       *min_width_p,
                               gfloat       *natural_width_p)
{
  MplEntryPrivate *priv = MPL_ENTRY (actor)->priv;
  NbtkPadding padding = { 0, 0, 0, 0 };
  gfloat min_width_entry, min_width_button;
  gfloat natural_width_entry, natural_width_button;

  nbtk_widget_get_padding (NBTK_WIDGET (actor), &padding);

  clutter_actor_get_preferred_width (priv->entry, for_height,
                                     &min_width_entry,
                                     &natural_width_entry);

  clutter_actor_get_preferred_width (priv->table, for_height,
                                     &min_width_button,
                                     &natural_width_button);

  if (min_width_p)
    *min_width_p = padding.left +
                   min_width_entry +
                   min_width_button +
                   padding.right;

  if (natural_width_p)
    *natural_width_p = padding.left +
                       natural_width_entry +
                       natural_width_button +
                       padding.right;
}

static void
mpl_entry_get_preferred_height (ClutterActor *actor,
                                gfloat        for_width,
                                gfloat       *min_height_p,
                                gfloat       *natural_height_p)
{
  MplEntryPrivate *priv = MPL_ENTRY (actor)->priv;
  NbtkPadding padding = { 0, 0, 0, 0 };
  gfloat min_height_entry, min_height_button;
  gfloat natural_height_entry, natural_height_button;

  nbtk_widget_get_padding (NBTK_WIDGET (actor), &padding);

  clutter_actor_get_preferred_height (priv->entry, for_width,
                                      &min_height_entry,
                                      &natural_height_entry);

  clutter_actor_get_preferred_height (priv->table, for_width,
                                     &min_height_button,
                                     &natural_height_button);

  if (min_height_p)
    *min_height_p = padding.top +
                    MAX (min_height_entry, min_height_button) +
                    padding.bottom;

  if (natural_height_p)
    *natural_height_p = padding.top +
                        MAX (natural_height_entry, natural_height_button) +
                        padding.bottom;
}

static void
mpl_entry_allocate (ClutterActor          *actor,
                    const ClutterActorBox *box,
                    ClutterAllocationFlags flags)
{
  MplEntryPrivate *priv = MPL_ENTRY (actor)->priv;
  NbtkPadding padding = { 0, 0, 0, 0 };
  ClutterActorBox entry_box, button_box;
  gfloat entry_width, entry_height, button_width, button_height;

  CLUTTER_ACTOR_CLASS (mpl_entry_parent_class)->
    allocate (actor, box, flags);

  nbtk_widget_get_padding (NBTK_WIDGET (actor), &padding);

  /* Button is right-aligned. */
  clutter_actor_get_preferred_size (priv->table,
                                    NULL, NULL,
                                    &button_width, &button_height);

  button_box.x2 = (int) (box->x2 - box->x1 - padding.right);
  button_box.x1 = (int) (button_box.x2 - button_width);
  button_box.y1 = (int) (((box->y2 - box->y1) - button_height) / 2);
  button_box.y2 = (int) (button_box.y1 + button_height);

  /* Sanity check. */
  button_box.x1 = (int) (MAX (padding.left, button_box.x1));
  button_box.x2 = (int) (MAX (padding.left, button_box.x2));

  /* Entry is left-aligned. */
  clutter_actor_get_preferred_size (priv->entry,
                                    NULL, NULL,
                                    &entry_width, &entry_height);

  entry_box.x1 = (int) (padding.left);
  entry_box.x2 = (int) (button_box.x1);
  entry_box.y1 = (int) (((box->y2 - box->y1) - entry_height) / 2);
  entry_box.y2 = (int) (entry_box.y1 + entry_height);

  clutter_actor_allocate (priv->entry, &entry_box, flags);
  clutter_actor_allocate (priv->table, &button_box, flags);
}

static void
mpl_entry_paint (ClutterActor *actor)
{
  MplEntryPrivate *priv = MPL_ENTRY (actor)->priv;

  CLUTTER_ACTOR_CLASS (mpl_entry_parent_class)->paint (actor);

  clutter_actor_paint (priv->entry);

  clutter_actor_paint (priv->table);
}

static void
mpl_entry_pick (ClutterActor       *actor,
                const ClutterColor *pick_color)
{
  MplEntryPrivate *priv = MPL_ENTRY (actor)->priv;

  CLUTTER_ACTOR_CLASS (mpl_entry_parent_class)->pick (actor, pick_color);

  clutter_actor_paint (priv->entry);

  clutter_actor_paint (priv->table);
}

static void
mpl_entry_style_changed (NbtkWidget *widget)
{
  MplEntryPrivate *priv = MPL_ENTRY (widget)->priv;

  /* this is needed to propagate the ::style-changed signal to
   * the internal children on MplEntry, otherwise the style changes
   * will not reach them
   */
  g_signal_emit_by_name (priv->entry, "style-changed", 0);
  g_signal_emit_by_name (priv->table, "style-changed", 0);
}

static void
mpl_entry_finalize (GObject *gobject)
{
  MplEntryPrivate *priv = MPL_ENTRY (gobject)->priv;

  clutter_actor_destroy (priv->entry), priv->entry = NULL;
  clutter_actor_destroy (priv->table), priv->table = NULL;

  G_OBJECT_CLASS (mpl_entry_parent_class)->finalize (gobject);
}

static void
mpl_entry_set_property (GObject      *gobject,
                        guint         prop_id,
                        const GValue *value,
                        GParamSpec   *pspec)
{
  switch (prop_id)
    {
    case PROP_LABEL:
      mpl_entry_set_label (MPL_ENTRY (gobject), g_value_get_string (value));
      break;

    case PROP_TEXT:
      mpl_entry_set_text (MPL_ENTRY (gobject), g_value_get_string (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
      break;
    }
}

static void
mpl_entry_get_property (GObject    *gobject,
                        guint       prop_id,
                        GValue     *value,
                        GParamSpec *pspec)
{
  switch (prop_id)
    {
    case PROP_LABEL:
      g_value_set_string (value, mpl_entry_get_label (MPL_ENTRY (gobject)));
      break;

    case PROP_TEXT:
      g_value_set_string (value, mpl_entry_get_text (MPL_ENTRY (gobject)));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
      break;
    }
}

static void
mpl_entry_constructed (GObject *gobject)
{
  if (G_OBJECT_CLASS (mpl_entry_parent_class)->constructed)
    G_OBJECT_CLASS (mpl_entry_parent_class)->constructed (gobject);
}

static void
mpl_entry_focus_in (ClutterActor *actor)
{
  MplEntryPrivate *priv = MPL_ENTRY (actor)->priv;

  clutter_actor_grab_key_focus (priv->entry);
}

static void
mpl_entry_class_init (MplEntryClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  ClutterActorClass *actor_class = CLUTTER_ACTOR_CLASS (klass);
  GParamSpec *pspec;

  g_type_class_add_private (klass, sizeof (MplEntryPrivate));

  gobject_class->constructed = mpl_entry_constructed;
  gobject_class->set_property = mpl_entry_set_property;
  gobject_class->get_property = mpl_entry_get_property;
  gobject_class->finalize = mpl_entry_finalize;

  actor_class->get_preferred_width = mpl_entry_get_preferred_width;
  actor_class->get_preferred_height = mpl_entry_get_preferred_height;
  actor_class->allocate = mpl_entry_allocate;
  actor_class->key_focus_in = mpl_entry_focus_in;
  actor_class->paint = mpl_entry_paint;
  actor_class->pick = mpl_entry_pick;

  pspec = g_param_spec_string ("label",
                               "Label",
                               "The label inside the button.",
                               NULL,
                               G_PARAM_READWRITE | G_PARAM_CONSTRUCT);
  g_object_class_install_property (gobject_class, PROP_LABEL, pspec);

  pspec = g_param_spec_string ("text",
                               "Text",
                               "The text inside the entry.",
                               NULL,
                               G_PARAM_READWRITE | G_PARAM_CONSTRUCT);
  g_object_class_install_property (gobject_class, PROP_TEXT, pspec);

  _signals[BUTTON_CLICKED] =
    g_signal_new ("button-clicked",
                  G_TYPE_FROM_CLASS (gobject_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (MplEntryClass, button_clicked),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  _signals[TEXT_CHANGED] =
    g_signal_new ("text-changed",
                  G_TYPE_FROM_CLASS (gobject_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (MplEntryClass, text_changed),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  _signals[KEYNAV_EVENT] =
    g_signal_new ("keynav-event",
                  G_TYPE_FROM_CLASS (gobject_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (MplEntryClass, keynav_event),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__UINT,
                  G_TYPE_NONE, 1, G_TYPE_UINT);

  mpl_clutter_theme_ensure_loaded ();
}

static void
set_clear_button_size (ClutterActor *clear_button)
{
  GValue background_image = { 0, };

  g_value_init (&background_image, G_TYPE_STRING);
  nbtk_stylable_get_property (NBTK_STYLABLE (clear_button),
                              "background-image", &background_image);
  if (g_value_get_string (&background_image))
    {
      GError        *error = NULL;
      ClutterActor  *background_texture = clutter_texture_new_from_file (
                                            g_value_get_string (&background_image),
                                            &error);
      if (error)
        {
          g_warning ("%s", error->message);
          g_error_free (error);
        }
      else
        {
          gint width, height;
          clutter_texture_get_base_size (CLUTTER_TEXTURE (background_texture),
                                         &width, &height);
          clutter_actor_set_size (clear_button, width, height);
          g_object_unref (background_texture);
        }
      g_value_unset (&background_image);
    }
}

static void
mpl_entry_init (MplEntry *self)
{
  MplEntryPrivate *priv;
  ClutterActor    *text;

  self->priv = priv = MPL_ENTRY_GET_PRIVATE (self);

  g_signal_connect (self, "style-changed",
                    G_CALLBACK (mpl_entry_style_changed), NULL);

  priv->entry = CLUTTER_ACTOR (nbtk_entry_new (""));
  clutter_actor_set_parent (priv->entry, CLUTTER_ACTOR (self));
  nbtk_widget_set_style_class_name (NBTK_WIDGET (priv->entry),
                                    "MplEntryEntry");
  text = nbtk_entry_get_clutter_text (NBTK_ENTRY (priv->entry));
  clutter_text_set_single_line_mode (CLUTTER_TEXT (text), TRUE);
  g_signal_connect (text, "text-changed",
                    G_CALLBACK (text_changed_cb),
                    self);
  g_signal_connect (text, "key-press-event",
                    G_CALLBACK (text_key_press_cb),
                    self);

  priv->table = CLUTTER_ACTOR (nbtk_table_new ());
  clutter_actor_set_parent (priv->table, CLUTTER_ACTOR (self));

  priv->clear_button = CLUTTER_ACTOR (nbtk_button_new ());
  clutter_actor_hide (priv->clear_button);
  nbtk_table_add_actor (NBTK_TABLE (priv->table), priv->clear_button, 0, 0);
  nbtk_widget_set_style_class_name (NBTK_WIDGET (priv->clear_button),
                                    "MplEntryClearButton");
  set_clear_button_size (priv->clear_button);
  g_signal_connect (priv->clear_button, "clicked",
                    G_CALLBACK (clear_button_clicked_cb),
                    self);

  priv->search_button = CLUTTER_ACTOR (nbtk_button_new ());
  nbtk_table_add_actor (NBTK_TABLE (priv->table), priv->search_button, 0, 1);
  nbtk_widget_set_style_class_name (NBTK_WIDGET (priv->search_button),
                                    "MplEntryButton");
  g_signal_connect (priv->search_button, "clicked",
                    G_CALLBACK (search_button_clicked_cb),
                    self);
}

NbtkWidget *
mpl_entry_new (const char *label)
{
  return g_object_new (MPL_TYPE_ENTRY,
                       "label", label,
                       NULL);
}

const gchar *
mpl_entry_get_label (MplEntry *self)
{
  g_return_val_if_fail (MPL_IS_ENTRY (self), NULL);

  return nbtk_button_get_label (NBTK_BUTTON (self->priv->search_button));
}

void
mpl_entry_set_label (MplEntry     *self,
                     const gchar  *text)
{
  g_return_if_fail (self);

  nbtk_button_set_label (NBTK_BUTTON (self->priv->search_button), text);
}

const gchar *
mpl_entry_get_text (MplEntry *self)
{
  g_return_val_if_fail (MPL_IS_ENTRY (self), NULL);

  return nbtk_entry_get_text (NBTK_ENTRY (self->priv->entry));
}

void
mpl_entry_set_text (MplEntry     *self,
                    const gchar  *text)
{
  g_return_if_fail (self);

  if (text)
    nbtk_entry_set_text (NBTK_ENTRY (self->priv->entry), text);
}

NbtkWidget *
mpl_entry_get_nbtk_entry (MplEntry *self)
{
  g_return_val_if_fail (MPL_IS_ENTRY (self), NULL);

  return NBTK_WIDGET (self->priv->entry);
}

