#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <glib.h>
#include <stdlib.h>

#include "mnb-status-entry.h"

#define MNB_STATUS_ENTRY_GET_PRIVATE(obj)       (G_TYPE_INSTANCE_GET_PRIVATE ((obj), MNB_TYPE_STATUS_ENTRY, MnbStatusEntryPrivate))

#define ICON_SIZE       (CLUTTER_UNITS_FROM_FLOAT (48.0))
#define H_PADDING       (CLUTTER_UNITS_FROM_FLOAT (9.0))

struct _MnbStatusEntryPrivate
{
  ClutterActor *icon;
  ClutterActor *text;
  ClutterActor *button;

  gchar *service_name;

  gchar *status_text;
  gchar *status_time;

  NbtkPadding padding;

  guint in_hover  : 1;
  guint is_active : 1;

  ClutterUnit icon_separator_x;
};

enum
{
  PROP_0,

  PROP_SERVICE_NAME
};

G_DEFINE_TYPE (MnbStatusEntry, mnb_status_entry, NBTK_TYPE_WIDGET);

static void
on_button_clicked (NbtkButton *button,
                   MnbStatusEntry *entry)
{
  MnbStatusEntryPrivate *priv = entry->priv;
  ClutterActor *text;

  text = nbtk_entry_get_clutter_text (NBTK_ENTRY (priv->text));

  if (!priv->is_active)
    {
      nbtk_button_set_label (NBTK_BUTTON (priv->button), "Post");

      clutter_actor_set_reactive (text, TRUE);

      clutter_text_set_editable (CLUTTER_TEXT (text), TRUE);
      clutter_text_set_activatable (CLUTTER_TEXT (text), TRUE);

      clutter_actor_grab_key_focus (priv->text);

      nbtk_widget_set_style_pseudo_class (NBTK_WIDGET (priv->text), "active");

      priv->is_active = TRUE;

      g_debug (G_STRLOC ": edit status");
    }
  else
    {
      nbtk_button_set_label (NBTK_BUTTON (priv->button), "Edit");

      clutter_actor_set_reactive (text, FALSE);

      clutter_text_set_editable (CLUTTER_TEXT (text), FALSE);
      clutter_text_set_activatable (CLUTTER_TEXT (text), FALSE);

      nbtk_widget_set_style_pseudo_class (NBTK_WIDGET (priv->text), "active");

      priv->is_active = FALSE;

      g_debug (G_STRLOC ": send status");
    }
}

static void
mnb_status_entry_get_preferred_width (ClutterActor *actor,
                                      ClutterUnit   for_height,
                                      ClutterUnit  *min_width_p,
                                      ClutterUnit  *natural_width_p)
{
  MnbStatusEntryPrivate *priv = MNB_STATUS_ENTRY (actor)->priv;
  ClutterUnit min_width, natural_width;

  clutter_actor_get_preferred_width (priv->text, for_height,
                                     &min_width,
                                     &natural_width);

  if (min_width_p)
    *min_width_p = priv->padding.left + ICON_SIZE + priv->padding.right;

  if (natural_width_p)
    *natural_width_p = priv->padding.left
                     + ICON_SIZE + natural_width + H_PADDING * 2
                     + priv->padding.right;
}

static void
mnb_status_entry_get_preferred_height (ClutterActor *actor,
                                       ClutterUnit   for_width,
                                       ClutterUnit  *min_height_p,
                                       ClutterUnit  *natural_height_p)
{
  MnbStatusEntryPrivate *priv = MNB_STATUS_ENTRY (actor)->priv;

  if (min_height_p)
    *min_height_p = priv->padding.top + ICON_SIZE + priv->padding.bottom;

  if (natural_height_p)
    *natural_height_p = priv->padding.top + ICON_SIZE + priv->padding.bottom;
}

static void
mnb_status_entry_allocate (ClutterActor          *actor,
                           const ClutterActorBox *box,
                           gboolean               origin_changed)
{
  MnbStatusEntryPrivate *priv = MNB_STATUS_ENTRY (actor)->priv;
  ClutterActorClass *parent_class;
  ClutterUnit available_width, available_height;
  ClutterUnit min_width, min_height;
  ClutterUnit natural_width, natural_height;
  ClutterUnit button_width, button_height;
  ClutterUnit text_width, text_height;
  NbtkPadding border = { 0, };
  ClutterActorBox child_box = { 0, };

  parent_class = CLUTTER_ACTOR_CLASS (mnb_status_entry_parent_class);
  parent_class->allocate (actor, box, origin_changed);

//  nbtk_widget_get_border (NBTK_WIDGET (actor), &border);

  available_width  = box->x2 - box->x1
                   - priv->padding.left - priv->padding.right
                   - border.left - border.right;
  available_height = box->y2 - box->y1
                   - priv->padding.top - priv->padding.bottom
                   - border.top - border.right;

  clutter_actor_get_preferred_size (priv->button,
                                    &min_width, &min_height,
                                    &natural_width, &natural_height);

  if (natural_width >= available_width)
    {
      if (min_width >= available_width)
        button_width = available_width;
      else
        button_width = min_width;
    }
  else
    button_width = natural_width;

  if (natural_height >= available_height)
    {
      if (min_height >= available_width)
        button_height = available_height;
      else
        button_height = min_height;
    }
  else
    button_height = natural_height;

  /* layout
   *
   * +--------------------------------------------------------+
   * | +---+ | +-----------------------------------+--------+ |
   * | | X | | |xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx... | xxxxxx | |
   * | +---+ | +-----------------------------------+--------+ |
   * +--------------------------------------------------------+
   *
   *   icon  |  text                               | button
   */

  /* icon */
  child_box.x1 = border.left + priv->padding.left;
  child_box.y1 = border.top  + priv->padding.top;
  child_box.x2 = child_box.x1 + ICON_SIZE;
  child_box.y2 = child_box.y1 + ICON_SIZE;
  clutter_actor_allocate (priv->icon, &child_box, origin_changed);

  /* separator */
  priv->icon_separator_x = child_box.x2 + H_PADDING;

  /* text */
  text_width = available_width - button_width;
  clutter_actor_get_preferred_height (priv->text, text_width,
                                      NULL,
                                      &text_height);

  child_box.x1 = border.left + priv->padding.left
               + ICON_SIZE
               + H_PADDING;
  child_box.y1 = (int) border.top + priv->padding.top
               + ((ICON_SIZE - text_height) / 2);
  child_box.x2 = child_box.x1 + (text_width - ICON_SIZE - H_PADDING);
  child_box.y2 = child_box.y1 + text_height;
  clutter_actor_allocate (priv->text, &child_box, origin_changed);

  /* button */
  child_box.x1 = available_width
               - (border.right + priv->padding.right)
               - button_width;
  child_box.y1 = border.top + priv->padding.top
               + ((ICON_SIZE - button_height) / 2);
  child_box.x2 = child_box.x1 + button_width;
  child_box.y2 = child_box.y1 + button_height;
  clutter_actor_allocate (priv->button, &child_box, origin_changed);
}

static void
mnb_status_entry_paint (ClutterActor *actor)
{
  MnbStatusEntryPrivate *priv = MNB_STATUS_ENTRY (actor)->priv;

  CLUTTER_ACTOR_CLASS (mnb_status_entry_parent_class)->paint (actor);

  if (priv->icon && CLUTTER_ACTOR_IS_VISIBLE (priv->icon))
    clutter_actor_paint (priv->icon);

  if (priv->text && CLUTTER_ACTOR_IS_VISIBLE (priv->text))
    clutter_actor_paint (priv->text);

  if (priv->button && CLUTTER_ACTOR_IS_VISIBLE (priv->button))
    clutter_actor_paint (priv->button);
}

static void
mnb_status_entry_pick (ClutterActor       *actor,
                       const ClutterColor *pick_color)
{
  MnbStatusEntryPrivate *priv = MNB_STATUS_ENTRY (actor)->priv;

  CLUTTER_ACTOR_CLASS (mnb_status_entry_parent_class)->pick (actor,
                                                             pick_color);

  if (priv->icon && clutter_actor_should_pick_paint (priv->icon))
    clutter_actor_paint (priv->icon);

  if (priv->text && clutter_actor_should_pick_paint (priv->text))
    clutter_actor_paint (priv->text);

  if (priv->button && clutter_actor_should_pick_paint (priv->button))
    clutter_actor_paint (priv->button);
}

static gboolean
mnb_status_entry_enter (ClutterActor *actor,
                        ClutterCrossingEvent *event)
{
  MnbStatusEntryPrivate *priv = MNB_STATUS_ENTRY (actor)->priv;

  if (!priv->is_active)
    {
      nbtk_widget_set_style_pseudo_class (NBTK_WIDGET (priv->text), "hover");
      clutter_actor_show (priv->button);
    }

  priv->in_hover = TRUE;
}

static gboolean
mnb_status_entry_leave (ClutterActor *actor,
                        ClutterCrossingEvent *event)
{
  MnbStatusEntryPrivate *priv = MNB_STATUS_ENTRY (actor)->priv;

  if (!priv->is_active)
    {
      nbtk_widget_set_style_pseudo_class (NBTK_WIDGET (priv->text), NULL);
      clutter_actor_hide (priv->button);
    }
  else
    nbtk_widget_set_style_pseudo_class (NBTK_WIDGET (priv->text), "active");

  priv->in_hover = FALSE;
}

static void
mnb_status_entry_style_changed (NbtkWidget *widget)
{
  MnbStatusEntryPrivate *priv = MNB_STATUS_ENTRY (widget)->priv;
  NbtkPadding *padding = NULL;

  nbtk_stylable_get (NBTK_STYLABLE (widget),
                     "padding", &padding,
                     NULL);

  if (padding)
    {
      priv->padding = *padding;

      g_boxed_free (NBTK_TYPE_PADDING, padding);
    }

  /* chain up */
  NBTK_WIDGET_CLASS (mnb_status_entry_parent_class)->style_changed (widget);
}

static void
mnb_status_entry_finalize (GObject *gobject)
{
  MnbStatusEntryPrivate *priv = MNB_STATUS_ENTRY (gobject)->priv;

  g_free (priv->service_name);

  g_free (priv->status_text);
  g_free (priv->status_time);

  clutter_actor_destroy (priv->icon);
  clutter_actor_destroy (priv->text);
  clutter_actor_destroy (priv->button);

  G_OBJECT_CLASS (mnb_status_entry_parent_class)->finalize (gobject);
}

static void
mnb_status_entry_set_property (GObject      *gobject,
                               guint         prop_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
  MnbStatusEntryPrivate *priv = MNB_STATUS_ENTRY (gobject)->priv;

  switch (prop_id)
    {
    case PROP_SERVICE_NAME:
      g_free (priv->service_name);
      priv->service_name = g_value_dup_string (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
      break;
    }
}

static void
mnb_status_entry_get_property (GObject    *gobject,
                               guint       prop_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
  MnbStatusEntryPrivate *priv = MNB_STATUS_ENTRY (gobject)->priv;

  switch (prop_id)
    {
    case PROP_SERVICE_NAME:
      g_value_set_string (value, priv->service_name);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
      break;
    }
}

static void
mnb_status_entry_constructed (GObject *gobject)
{
  MnbStatusEntry *entry = MNB_STATUS_ENTRY (gobject);
  MnbStatusEntryPrivate *priv = entry->priv;

  g_assert (priv->service_name != NULL);

  if (G_OBJECT_CLASS (mnb_status_entry_parent_class)->constructed)
    G_OBJECT_CLASS (mnb_status_entry_parent_class)->constructed (gobject);
}

static void
mnb_status_entry_class_init (MnbStatusEntryClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  ClutterActorClass *actor_class = CLUTTER_ACTOR_CLASS (klass);
  NbtkWidgetClass *widget_class = NBTK_WIDGET_CLASS (klass);
  GParamSpec *pspec;

  g_type_class_add_private (klass, sizeof (MnbStatusEntryPrivate));

  gobject_class->constructed = mnb_status_entry_constructed;
  gobject_class->set_property = mnb_status_entry_set_property;
  gobject_class->get_property = mnb_status_entry_get_property;
  gobject_class->finalize = mnb_status_entry_finalize;

  actor_class->get_preferred_width = mnb_status_entry_get_preferred_width;
  actor_class->get_preferred_height = mnb_status_entry_get_preferred_height;
  actor_class->allocate = mnb_status_entry_allocate;
  actor_class->paint = mnb_status_entry_paint;
  actor_class->pick = mnb_status_entry_pick;
  actor_class->enter_event = mnb_status_entry_enter;
  actor_class->leave_event = mnb_status_entry_leave;

  widget_class->style_changed = mnb_status_entry_style_changed;

  pspec = g_param_spec_string ("service-name",
                               "Service Name",
                               "The name of the Mojito service",
                               NULL,
                               G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
  g_object_class_install_property (gobject_class, PROP_SERVICE_NAME, pspec);
}

static void
mnb_status_entry_init (MnbStatusEntry *self)
{
  MnbStatusEntryPrivate *priv;
  ClutterActor *text;
  gchar *no_icon_file;

  self->priv = priv = MNB_STATUS_ENTRY_GET_PRIVATE (self);

  no_icon_file = g_build_filename (PLUGIN_PKGDATADIR,
                                   "theme",
                                   "status",
                                   "no_image_icon.png",
                                   NULL);
  priv->icon = clutter_texture_new_from_file (no_icon_file, NULL);
  if (G_UNLIKELY (priv->icon == NULL))
    {
      const ClutterColor color = { 204, 204, 0, 255 };

      priv->icon = clutter_rectangle_new_with_color (&color);
    }

  clutter_actor_set_size (priv->icon, ICON_SIZE, ICON_SIZE);
  clutter_actor_set_parent (priv->icon, CLUTTER_ACTOR (self));

  g_free (no_icon_file);

  priv->text = CLUTTER_ACTOR (nbtk_entry_new ("Enter your status here..."));
  nbtk_widget_set_style_class_name (NBTK_WIDGET (priv->text),
                                    "MnbStatusEntryText");
  clutter_actor_set_parent (priv->text, CLUTTER_ACTOR (self));
  text = nbtk_entry_get_clutter_text (NBTK_ENTRY (priv->text));
  clutter_text_set_editable (CLUTTER_TEXT (text), FALSE);
  clutter_text_set_single_line_mode (CLUTTER_TEXT (text), TRUE);

  priv->button = CLUTTER_ACTOR (nbtk_button_new_with_label ("Edit"));
  nbtk_widget_set_style_class_name (NBTK_WIDGET (priv->button),
                                    "MnbStatusEntryButton");
  clutter_actor_hide (priv->button);
  clutter_actor_set_reactive (priv->button, TRUE);
  clutter_actor_set_parent (priv->button, CLUTTER_ACTOR (self));
  g_signal_connect (priv->button, "clicked",
                    G_CALLBACK (on_button_clicked),
                    self);
}

NbtkWidget *
mnb_status_entry_new (const gchar *service_name)
{
  g_return_val_if_fail (service_name != NULL, NULL);

  return g_object_new (MNB_TYPE_STATUS_ENTRY,
                       "service-name", service_name,
                       NULL);
}
