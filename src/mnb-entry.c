#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <glib.h>
#include <glib/gi18n.h>
#include <stdlib.h>

#include <penge/penge-utils.h>
#include "mnb-entry.h"

#define MNB_ENTRY_GET_PRIVATE(obj) \
        (G_TYPE_INSTANCE_GET_PRIVATE ((obj), MNB_TYPE_ENTRY, MnbEntryPrivate))

struct _MnbEntryPrivate
{
  ClutterActor *entry;
  ClutterActor *button;
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

  LAST_SIGNAL
};

static guint _signals[LAST_SIGNAL] = { 0, };

G_DEFINE_TYPE (MnbEntry, mnb_entry, NBTK_TYPE_WIDGET);

static void
button_clicked_cb (NbtkButton *button,
                   MnbEntry *entry)
{
  g_signal_emit (entry, _signals[BUTTON_CLICKED], 0);
}

static void
text_changed_cb (ClutterText  *text,
                 MnbEntry     *entry)
{
  g_signal_emit (entry, _signals[TEXT_CHANGED], 0);
}

static void
mnb_entry_get_preferred_width (ClutterActor *actor,
                               ClutterUnit   for_height,
                               ClutterUnit  *min_width_p,
                               ClutterUnit  *natural_width_p)
{
  MnbEntryPrivate *priv = MNB_ENTRY (actor)->priv;
  NbtkPadding padding = { 0, 0, 0, 0 };
  ClutterUnit min_width_entry, min_width_button;
  ClutterUnit natural_width_entry, natural_width_button;

  nbtk_widget_get_padding (NBTK_WIDGET (actor), &padding);

  clutter_actor_get_preferred_width (priv->entry, for_height,
                                     &min_width_entry,
                                     &natural_width_entry);

  clutter_actor_get_preferred_width (priv->button, for_height,
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
mnb_entry_get_preferred_height (ClutterActor *actor,
                                ClutterUnit   for_width,
                                ClutterUnit  *min_height_p,
                                ClutterUnit  *natural_height_p)
{
  MnbEntryPrivate *priv = MNB_ENTRY (actor)->priv;
  NbtkPadding padding = { 0, 0, 0, 0 };
  ClutterUnit min_height_entry, min_height_button;
  ClutterUnit natural_height_entry, natural_height_button;

  nbtk_widget_get_padding (NBTK_WIDGET (actor), &padding);

  clutter_actor_get_preferred_height (priv->entry, for_width,
                                      &min_height_entry,
                                      &natural_height_entry);

  clutter_actor_get_preferred_height (priv->button, for_width,
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
mnb_entry_allocate (ClutterActor          *actor,
                    const ClutterActorBox *box,
                    gboolean               origin_changed)
{
  MnbEntryPrivate *priv = MNB_ENTRY (actor)->priv;
  NbtkPadding padding = { 0, 0, 0, 0 };
  ClutterActorBox entry_box, button_box;
  ClutterUnit entry_width, entry_height, button_width, button_height;

  CLUTTER_ACTOR_CLASS (mnb_entry_parent_class)->
    allocate (actor, box, origin_changed);

  nbtk_widget_get_padding (NBTK_WIDGET (actor), &padding);

  /* Button is right-aligned. */
  clutter_actor_get_preferred_size (priv->button,
                                    NULL, NULL,
                                    &button_width, &button_height);

  button_box.x2 = box->x2 - box->x1 - padding.right;
  button_box.x1 = button_box.x2 - button_width;
  button_box.y1 = ((box->y2 - box->y1) - button_height) / 2;
  button_box.y2 = button_box.y1 + button_height;

  /* Sanity check. */  
  button_box.x1 = MAX (padding.left, button_box.x1);
  button_box.x2 = MAX (padding.left, button_box.x2);
  
  /* Entry is left-aligned. */
  clutter_actor_get_preferred_size (priv->entry,
                                    NULL, NULL,
                                    &entry_width, &entry_height);

  entry_box.x1 = padding.left;
  entry_box.x2 = button_box.x1;
  entry_box.y1 = ((box->y2 - box->y1) - entry_height) / 2;
  entry_box.y2 = entry_box.y1 + entry_height;

  clutter_actor_allocate (priv->entry, &entry_box, origin_changed);
  clutter_actor_allocate (priv->button, &button_box, origin_changed);
}

static void
mnb_entry_paint (ClutterActor *actor)
{
  MnbEntryPrivate *priv = MNB_ENTRY (actor)->priv;

  CLUTTER_ACTOR_CLASS (mnb_entry_parent_class)->paint (actor);

  clutter_actor_paint (priv->entry);

  clutter_actor_paint (priv->button);
}

static void
mnb_entry_pick (ClutterActor       *actor,
                const ClutterColor *pick_color)
{
  MnbEntryPrivate *priv = MNB_ENTRY (actor)->priv;

  CLUTTER_ACTOR_CLASS (mnb_entry_parent_class)->pick (actor, pick_color);

  clutter_actor_paint (priv->entry);

  clutter_actor_paint (priv->button);
}

static void
mnb_entry_finalize (GObject *gobject)
{
  MnbEntryPrivate *priv = MNB_ENTRY (gobject)->priv;

  clutter_actor_destroy (priv->entry), priv->entry = NULL;
  clutter_actor_destroy (priv->button), priv->button = NULL;

  G_OBJECT_CLASS (mnb_entry_parent_class)->finalize (gobject);
}

static void
mnb_entry_set_property (GObject      *gobject,
                        guint         prop_id,
                        const GValue *value,
                        GParamSpec   *pspec)
{
  MnbEntryPrivate *priv = MNB_ENTRY (gobject)->priv;

  switch (prop_id)
    {
    case PROP_LABEL:
      mnb_entry_set_label (MNB_ENTRY (gobject), g_value_get_string (value));
      break;

    case PROP_TEXT:
      mnb_entry_set_text (MNB_ENTRY (gobject), g_value_get_string (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
      break;
    }
}

static void
mnb_entry_get_property (GObject    *gobject,
                        guint       prop_id,
                        GValue     *value,
                        GParamSpec *pspec)
{
  MnbEntryPrivate *priv = MNB_ENTRY (gobject)->priv;

  switch (prop_id)
    {
    case PROP_LABEL:
      g_value_set_string (value, mnb_entry_get_label (MNB_ENTRY (gobject)));
      break;

    case PROP_TEXT:
      g_value_set_string (value, mnb_entry_get_text (MNB_ENTRY (gobject)));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
      break;
    }
}

static void
mnb_entry_constructed (GObject *gobject)
{
  MnbEntryPrivate *priv = MNB_ENTRY (gobject)->priv;

  if (G_OBJECT_CLASS (mnb_entry_parent_class)->constructed)
    G_OBJECT_CLASS (mnb_entry_parent_class)->constructed (gobject);
}

static void
mnb_entry_focus_in (ClutterActor *actor)
{
  MnbEntryPrivate *priv = MNB_ENTRY (actor)->priv;

  clutter_actor_grab_key_focus (priv->entry);
}

static void
mnb_entry_class_init (MnbEntryClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  ClutterActorClass *actor_class = CLUTTER_ACTOR_CLASS (klass);
  NbtkWidgetClass *widget_class = NBTK_WIDGET_CLASS (klass);
  GParamSpec *pspec;

  g_type_class_add_private (klass, sizeof (MnbEntryPrivate));

  gobject_class->constructed = mnb_entry_constructed;
  gobject_class->set_property = mnb_entry_set_property;
  gobject_class->get_property = mnb_entry_get_property;
  gobject_class->finalize = mnb_entry_finalize;

  actor_class->get_preferred_width = mnb_entry_get_preferred_width;
  actor_class->get_preferred_height = mnb_entry_get_preferred_height;
  actor_class->allocate = mnb_entry_allocate;
  actor_class->focus_in = mnb_entry_focus_in;
  actor_class->paint = mnb_entry_paint;
  actor_class->pick = mnb_entry_pick;

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
                  G_STRUCT_OFFSET (MnbEntryClass, button_clicked),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  _signals[TEXT_CHANGED] =
    g_signal_new ("text-changed",
                  G_TYPE_FROM_CLASS (gobject_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (MnbEntryClass, text_changed),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);
}

static void
mnb_entry_init (MnbEntry *self)
{
  MnbEntryPrivate *priv;
  ClutterActor    *text;

  self->priv = priv = MNB_ENTRY_GET_PRIVATE (self);

  priv->entry = CLUTTER_ACTOR (nbtk_entry_new (""));
  clutter_actor_set_parent (priv->entry, CLUTTER_ACTOR (self));
  nbtk_widget_set_style_class_name (NBTK_WIDGET (priv->entry),
                                    "MnbEntryEntry");
  text = nbtk_entry_get_clutter_text (NBTK_ENTRY (priv->entry));
  g_signal_connect (text, "text-changed",
                    G_CALLBACK (text_changed_cb),
                    self);

  priv->button = CLUTTER_ACTOR (nbtk_button_new ());
  clutter_actor_set_parent (priv->button, CLUTTER_ACTOR (self));
  nbtk_widget_set_style_class_name (NBTK_WIDGET (priv->button),
                                    "MnbEntryButton");
  g_signal_connect (priv->button, "clicked",
                    G_CALLBACK (button_clicked_cb),
                    self);
}

NbtkWidget *
mnb_entry_new (const char *label)
{
  return g_object_new (MNB_TYPE_ENTRY,
                       "label", label,
                       NULL);
}

const gchar *
mnb_entry_get_label (MnbEntry *self)
{
  g_return_val_if_fail (MNB_IS_ENTRY (self), NULL);  

  return nbtk_button_get_label (NBTK_BUTTON (self->priv->button));
}

void
mnb_entry_set_label (MnbEntry     *self,
                     const gchar  *text)
{
  g_return_if_fail (self);

  nbtk_button_set_label (NBTK_BUTTON (self->priv->button), text);
}

const gchar *
mnb_entry_get_text (MnbEntry *self)
{
  g_return_val_if_fail (MNB_IS_ENTRY (self), NULL);  

  return nbtk_entry_get_text (NBTK_ENTRY (self->priv->entry));
}

void
mnb_entry_set_text (MnbEntry     *self,
                    const gchar  *text)
{
  g_return_if_fail (self);
  
  if (text)
    nbtk_entry_set_text (NBTK_ENTRY (self->priv->entry), text);
}

