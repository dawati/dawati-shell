/*
 * nbtk-gtk-expander.c: GTK+ Expander widget with extra styling properties
 *
 * Copyright 2009 Intel Corporation.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU Lesser General Public License,
 * version 2.1, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
 * more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
 * Boston, MA 02111-1307, USA.
 *
 * Written by: Thomas Wood <thomas.wood@intel.com>
 *
 */

/**
 * SECTION:nbtk-gtk-expander
 * @short_description: a gtk expander widget with extra styling properties
 *
 * A GTK+ expander widget with extra styling properties. May well be removed
 * in future versions.
 */


#include "nbtk-gtk-expander.h"

struct _NbtkGtkExpanderPrivate
{
  gboolean has_indicator : 1;

  gint indicator_size;
  gint indicator_padding; /* padding between indicator and label */
  gint child_padding; /* padding between label and child */

  GtkWidget *label;
  //GdkWindow *event_window;
};

enum
{
  PROP_0,

  PROP_EXPANDED,
  PROP_LABEL_WIDGET,
  PROP_HAS_INDICATOR
};

G_DEFINE_TYPE (NbtkGtkExpander, nbtk_gtk_expander, GTK_TYPE_BIN)

#define DEFAULT_INDICATOR_SIZE 12

static gboolean
nbtk_gtk_expander_draw (GtkWidget      *widget,
                        cairo_t *cr)
{
  if (gtk_widget_is_drawable (widget))
    {
      NbtkGtkExpanderPrivate *priv = NBTK_GTK_EXPANDER (widget)->priv;
      GtkContainer *container = GTK_CONTAINER (widget);
      gint label_h;
      GtkBorder border;

      gtk_style_context_get_border (gtk_widget_get_style_context (widget), 0, &border);

      if (priv->label)
        label_h = gtk_widget_get_allocated_height (priv->label);
      else
        label_h = 0;

      gtk_render_frame (gtk_widget_get_style_context (widget),
                        cr,
                        0, 0,
                        gtk_widget_get_allocated_width (widget),
                        gtk_widget_get_allocated_width (widget));

      if (gtk_widget_get_state_flags (widget) & GTK_STATE_FLAG_ACTIVE)
        {
          gint hline_y = border.top + MAX (label_h, priv->indicator_size);

          gtk_render_line (gtk_widget_get_style_context (widget),
                           cr,
                           0, hline_y,
                           gtk_widget_get_allocated_width (widget), hline_y);
        }

      if (priv->has_indicator)
        {
          gint indicator_size, top;

          gtk_widget_style_get (widget, "expander-size", &indicator_size, NULL);
          top = border.top + MAX (0,  (label_h - indicator_size)/2);

          gtk_render_expander (gtk_widget_get_style_context (widget),
                               cr,
                               border.left, top,
                               border.left + indicator_size,
                               top + indicator_size);
        }

      if (priv->label)
        gtk_container_propagate_draw (container, priv->label, cr);

      /*
      if (GTK_WIDGET_HAS_FOCUS (expander))
        gtk_expander_paint_focus (expander, &event->area);
      */

    }

  /* chain up to get child painted */
  GTK_WIDGET_CLASS (nbtk_gtk_expander_parent_class)->draw (widget, cr);

  return FALSE;
}

static void
nbtk_gtk_expander_size_allocate (GtkWidget     *widget,
                                 GtkAllocation *allocation)
{
  NbtkGtkExpanderPrivate *priv = ((NbtkGtkExpander*) widget)->priv;
  GtkWidget *child, *label;
  GtkAllocation label_alloc, child_alloc;
  gint child_y;
  GtkBorder border;

  GTK_WIDGET_CLASS (nbtk_gtk_expander_parent_class)->size_allocate (widget,
                                                                    allocation);

  child = gtk_bin_get_child ((GtkBin*) widget);
  label = ((NbtkGtkExpander *) widget)->priv->label;
  gtk_style_context_get_border (gtk_widget_get_style_context (widget), 0, &border);
  child_y = border.top + priv->child_padding;

  if (label && gtk_widget_get_visible (label))
    {
      gtk_widget_get_preferred_height (label, NULL, &label_alloc.height);

      label_alloc.x = allocation->x + border.left;
      label_alloc.y = allocation->y + border.top;
      label_alloc.width = allocation->width - (border.left + border.right);

      if (priv->has_indicator)
        {
          label_alloc.y += MAX (0, (priv->indicator_size - label_alloc.height) / 2);
          label_alloc.x += priv->indicator_size + priv->indicator_padding;
          label_alloc.width -= priv->indicator_size + priv->indicator_padding;
        }

      gtk_widget_size_allocate (label, &label_alloc);

      child_y += MAX (priv->indicator_size, label_alloc.height);
    }

  if (gtk_widget_get_state_flags (widget) & GTK_STATE_FLAG_ACTIVE
      && child && gtk_widget_get_visible (child))
    {
      gtk_widget_get_preferred_height (child, NULL, &child_alloc.height);

      child_alloc.x = allocation->x;
      child_alloc.y = child_y;
      child_alloc.width = allocation->width;

      gtk_widget_size_allocate (child, &child_alloc);
    }


  /*if (priv->event_window)
    {
      gdk_window_move_resize (priv->event_window, allocation->x, allocation->y,
                              allocation->width, allocation->height);
                              }*/

}

static void
nbtk_gtk_expander_get_preferred_height (GtkWidget *widget,
                                        gint      *minimum,
                                        gint      *natural)
{
  NbtkGtkExpanderPrivate *priv = ((NbtkGtkExpander*) widget)->priv;
  GtkWidget *child, *label;
  gint label_min, label_nat, child_min, child_nat, indicator;
  GtkBorder border;

  child = gtk_bin_get_child ((GtkBin*) widget);
  label = priv->label;
  gtk_style_context_get_border (gtk_widget_get_style_context (widget), 0, &border);

  label_min = label_nat = indicator = child_min = child_nat = 0;

  if (label && gtk_widget_get_visible (label))
    gtk_widget_get_preferred_height (label, &label_min, &label_nat);

  if (child && gtk_widget_get_visible (child))
    {
      gtk_widget_get_preferred_height (child, &child_min, &child_nat);
      child_min += priv->child_padding;
      child_nat += priv->child_padding;
    }

  if (priv->has_indicator)
    indicator = priv->indicator_size;

  *natural = border.top + border.bottom +
             MAX (indicator, label_nat) +
             child_nat;
  *minimum = border.top + border.bottom +
             MAX (indicator, label_min) +
             child_min;
}

static void
nbtk_gtk_expander_get_preferred_width (GtkWidget *widget,
                                       gint      *minimum,
                                       gint      *natural)
{
  NbtkGtkExpanderPrivate *priv = ((NbtkGtkExpander*) widget)->priv;
  GtkWidget *child, *label;
  gint label_min, label_nat, child_min, child_nat, indicator;
  GtkBorder border;

  child = gtk_bin_get_child ((GtkBin*) widget);
  label = priv->label;
  gtk_style_context_get_border (gtk_widget_get_style_context (widget), 0, &border);

  label_min = label_nat = indicator = child_min = child_nat = 0;

  if (label && gtk_widget_get_visible (label))
    gtk_widget_get_preferred_width (label, &label_min, &label_nat);

  if (child && gtk_widget_get_visible (child))
      gtk_widget_get_preferred_width (child, &child_min, &child_nat);

  if (priv->has_indicator)
    indicator = priv->indicator_size + priv->indicator_padding;

  *natural = MAX (border.left + border.right + indicator + label_nat,
                  child_nat);
  *minimum = MAX (border.left + border.right + indicator + label_min,
                  child_min);
}

static void
nbtk_gtk_expander_forall (GtkContainer *container,
                          gboolean      include_internals,
                          GtkCallback   callback,
                          gpointer      callback_data)
{
  NbtkGtkExpanderPrivate *priv = ((NbtkGtkExpander*) container)->priv;

  GTK_CONTAINER_CLASS (nbtk_gtk_expander_parent_class)->forall (container,
                                                                include_internals,
                                                                callback,
                                                                callback_data);

  if (priv->label)
    (* callback) (priv->label, callback_data);
}

static void
nbtk_gtk_expander_map (GtkWidget *widget)
{
  NbtkGtkExpanderPrivate *priv = ((NbtkGtkExpander*) widget)->priv;

  if (priv->label)
      gtk_widget_map (priv->label);

  /*if (priv->event_window)
    gdk_window_show (priv->event_window);*/

  GTK_WIDGET_CLASS (nbtk_gtk_expander_parent_class)->map (widget);
}

static void
nbtk_gtk_expander_unmap (GtkWidget *widget)
{
  NbtkGtkExpanderPrivate *priv = ((NbtkGtkExpander*) widget)->priv;

  if (priv->label)
    gtk_widget_unmap (priv->label);

  /*if (priv->event_window)
    gdk_window_hide (priv->event_window);*/

  GTK_WIDGET_CLASS (nbtk_gtk_expander_parent_class)->unmap (widget);
}

static void
nbtk_gtk_expander_add (GtkContainer *container,
                       GtkWidget    *widget)
{
  GTK_CONTAINER_CLASS (nbtk_gtk_expander_parent_class)->add (container, widget);

  gtk_widget_set_child_visible (widget,
                                gtk_widget_get_state_flags (GTK_WIDGET (container)) & GTK_STATE_FLAG_ACTIVE);
  gtk_widget_queue_resize (GTK_WIDGET (container));
}

static void
nbtk_gtk_expander_remove (GtkContainer *container,
                          GtkWidget    *widget)
{
  NbtkGtkExpander *expander = NBTK_GTK_EXPANDER (container);

  if (expander->priv->label == widget)
    nbtk_gtk_expander_set_label_widget (expander, NULL);
  else
    GTK_CONTAINER_CLASS (nbtk_gtk_expander_parent_class)->remove (container,
                                                                  widget);
}

static gboolean
nbtk_gtk_expander_button_release (GtkWidget      *widget,
                                  GdkEventButton *event)
{
  nbtk_gtk_expander_set_expanded ((NbtkGtkExpander *) widget,
                                  !(gtk_widget_get_state_flags (widget) & GTK_STATE_FLAG_ACTIVE));

  return FALSE;
}

static void
nbtk_gtk_expander_realize (GtkWidget *widget)
{
  /*
  NbtkGtkExpanderPrivate *priv = ((NbtkGtkExpander*) widget)->priv;
  GdkWindowAttr attributes;
  */

  GTK_WIDGET_CLASS (nbtk_gtk_expander_parent_class)->realize (widget);

  /*
  attributes.window_type = GDK_WINDOW_CHILD;
  attributes.wclass = GDK_INPUT_ONLY;
  attributes.event_mask = gtk_widget_get_events (widget) |
                            GDK_BUTTON_PRESS_MASK        |
                            GDK_BUTTON_RELEASE_MASK      |
                            GDK_ENTER_NOTIFY_MASK        |
                            GDK_LEAVE_NOTIFY_MASK;

  priv->event_window = gdk_window_new (gtk_widget_get_parent_window (widget),
                                       &attributes, GDK_WA_X | GDK_WA_Y);
  gdk_window_set_user_data (priv->event_window, widget);

  gdk_window_move_resize (priv->event_window,
                          widget->allocation.x,
                          widget->allocation.y,
                          widget->allocation.width,
                          widget->allocation.height);
  */
}

static void
nbtk_gtk_expander_unrealize (GtkWidget *widget)
{
  /*
  NbtkGtkExpanderPrivate *priv = ((NbtkGtkExpander*) widget)->priv;

  if (priv->event_window)
    {
      gdk_window_destroy (priv->event_window);
      priv->event_window = NULL;
      }
   */

  GTK_WIDGET_CLASS (nbtk_gtk_expander_parent_class)->unrealize (widget);
}

static void
nbtk_gtk_expander_style_set (GtkWidget *widget,
                             GtkStyle  *previous)
{
  NbtkGtkExpanderPrivate *priv = ((NbtkGtkExpander*) widget)->priv;

  gtk_widget_style_get (widget, "expander-size", &priv->indicator_size, NULL);
}

static void
nbtk_gtk_expander_set_property (GObject      *object,
                                guint         property_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
  NbtkGtkExpander *expander = (NbtkGtkExpander *) object;

  switch (property_id)
    {
    case PROP_EXPANDED:
      nbtk_gtk_expander_set_expanded (expander, g_value_get_boolean (value));
      break;

    case PROP_LABEL_WIDGET:
      nbtk_gtk_expander_set_label_widget (expander, g_value_get_object (value));
      break;

    case PROP_HAS_INDICATOR:
      nbtk_gtk_expander_set_has_indicator (expander,
                                           g_value_get_boolean (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
nbtk_gtk_expander_get_property (GObject    *object,
                                guint       property_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
  NbtkGtkExpanderPrivate *priv = ((NbtkGtkExpander*) object)->priv;

  switch (property_id)
    {
    case PROP_EXPANDED:
      g_value_set_boolean (value, gtk_widget_get_state_flags (GTK_WIDGET (object)) & GTK_STATE_FLAG_ACTIVE);

    case PROP_LABEL_WIDGET:
      g_value_set_object (value, priv->label);
      break;

    case PROP_HAS_INDICATOR:
      g_value_set_boolean (value, priv->has_indicator);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
nbtk_gtk_expander_class_init (NbtkGtkExpanderClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  GtkContainerClass *container_class = GTK_CONTAINER_CLASS (klass);
  GParamSpec *pspec;

  g_type_class_add_private (klass, sizeof (NbtkGtkExpanderPrivate));

  object_class->set_property = nbtk_gtk_expander_set_property;
  object_class->get_property = nbtk_gtk_expander_get_property;

  widget_class->draw = nbtk_gtk_expander_draw;
  widget_class->style_set = nbtk_gtk_expander_style_set;

  widget_class->get_preferred_width = nbtk_gtk_expander_get_preferred_width;
  widget_class->get_preferred_height = nbtk_gtk_expander_get_preferred_height;
  widget_class->size_allocate = nbtk_gtk_expander_size_allocate;

  widget_class->realize = nbtk_gtk_expander_realize;
  widget_class->unrealize = nbtk_gtk_expander_unrealize;
  widget_class->map = nbtk_gtk_expander_map;
  widget_class->unmap = nbtk_gtk_expander_unmap;

  container_class->add = nbtk_gtk_expander_add;
  container_class->remove = nbtk_gtk_expander_remove;
  container_class->forall = nbtk_gtk_expander_forall;

  widget_class->button_release_event = nbtk_gtk_expander_button_release;

  pspec = g_param_spec_boolean ("expanded",
                                "Expanded",
                                "Whether the expander is open or closed",
                                FALSE,
                                G_PARAM_READWRITE);
  g_object_class_install_property (object_class, PROP_EXPANDED, pspec);

  pspec = g_param_spec_object ("label-widget",
                               "Label Widget",
                               "Widget to use as the title of the expander",
                               GTK_TYPE_WIDGET,
                               G_PARAM_READWRITE);
  g_object_class_install_property (object_class, PROP_LABEL_WIDGET, pspec);

  pspec = g_param_spec_boolean ("has-indicator",
                                "Has Indicator",
                                "Determines whether to show an indicator",
                                TRUE,
                                G_PARAM_READWRITE);
  g_object_class_install_property (object_class, PROP_HAS_INDICATOR, pspec);

  pspec = g_param_spec_int ("expander-size",
                            "Expander Size",
                            "Size of the expander indicator",
                            0, G_MAXINT, DEFAULT_INDICATOR_SIZE,
                            G_PARAM_READABLE);

  gtk_widget_class_install_style_property (widget_class, pspec);


}

static void
nbtk_gtk_expander_init (NbtkGtkExpander *self)
{
  NbtkGtkExpanderPrivate *priv;

  self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self,
                                            NBTK_TYPE_GTK_EXPANDER,
                                            NbtkGtkExpanderPrivate);

  gtk_widget_set_can_focus (GTK_WIDGET (self), TRUE);

  priv = self->priv;

  priv->child_padding = 2;
  priv->indicator_padding = 2;
  priv->indicator_size = DEFAULT_INDICATOR_SIZE;
  priv->has_indicator = TRUE;
}

/**
 * nbtk_gtk_expander_new:
 *
 * Create a new #NbtkGtkExpander.
 *
 * Returns: a newly allocated #NbtkGtkExpander
 */
GtkWidget*
nbtk_gtk_expander_new (void)
{
  return g_object_new (NBTK_TYPE_GTK_EXPANDER, NULL);
}

/**
 * nbtk_gtk_expander_set_expanded:
 * @expander: A #NbtkGtkExpander
 * @expanded: #TRUE to open the expander
 *
 * Set the value of the "expanded" property
 *
 */
void
nbtk_gtk_expander_set_expanded (NbtkGtkExpander *expander,
                                gboolean         expanded)
{
  GtkWidget *widget;
  g_return_if_fail (NBTK_IS_GTK_EXPANDER (expander));

  widget = GTK_WIDGET (expander);

  if ((gtk_widget_get_state_flags (widget) & GTK_STATE_FLAG_ACTIVE) != expanded)
    {
      GtkWidget *child;

      gtk_widget_set_state_flags (widget, GTK_STATE_FLAG_ACTIVE, FALSE);
      child = gtk_bin_get_child (GTK_BIN (expander));
      gtk_widget_set_child_visible (child, gtk_widget_get_state_flags (widget) & GTK_STATE_FLAG_ACTIVE);

      gtk_widget_queue_resize (widget);

      g_object_notify ((GObject*) expander, "expanded");
    }
}

/**
 * nbtk_gtk_expander_get_expanded:
 * @expander: A #NbtkGtkExpander
 *
 * Get the value of the "expanded" property
 *
 * Returns: #TRUE if the expander is "open"
 */
gboolean
nbtk_gtk_expander_get_expanded (NbtkGtkExpander *expander)
{
  g_return_val_if_fail (NBTK_IS_GTK_EXPANDER (expander), FALSE);

  return (gtk_widget_get_state_flags (GTK_WIDGET (expander)) & GTK_STATE_FLAG_ACTIVE);
}

/**
 * nbtk_gtk_expander_set_label_widget:
 * @expander: A #NbtkGtkExpander
 * @label: A #GtkWidget
 *
 * Set the widget to use as the label of the expander.
 *
 */
void
nbtk_gtk_expander_set_label_widget (NbtkGtkExpander *expander,
                                    GtkWidget       *label)
{
  g_return_if_fail (NBTK_IS_GTK_EXPANDER (expander));
  g_return_if_fail (label == NULL || GTK_IS_WIDGET (label));

  if (expander->priv->label)
    gtk_widget_unparent (expander->priv->label);

  if (label)
    {
      expander->priv->label = label;
      gtk_widget_set_parent (label, (GtkWidget*) expander);
    }
  else
    expander->priv->label = NULL;
}

/**
 * nbtk_gtk_expander_get_label_widget:
 * @expander: A #NbtkGtkExpander
 *
 * Get the widget used as the label of the expander.
 *
 * Returns: a #GtkWidget
 */
GtkWidget*
nbtk_gtk_expander_get_label_widget (NbtkGtkExpander *expander)
{
  g_return_val_if_fail (NBTK_IS_GTK_EXPANDER (expander), NULL);

  return expander->priv->label;
}

/**
 * nbtk_gtk_expander_set_has_indicator:
 * @expander: A #NbtkGtkExpander
 * @has_indicator: value to set
 *
 * Set the value of the has-indicator property
 *
 */
void
nbtk_gtk_expander_set_has_indicator (NbtkGtkExpander *expander,
                                     gboolean         has_indicator)
{
  g_return_if_fail (NBTK_IS_GTK_EXPANDER (expander));

  expander->priv->has_indicator = has_indicator;

  gtk_widget_queue_resize ((GtkWidget*) expander);
}

/**
 * nbtk_gtk_expander_get_has_indicator:
 * @expander: A #NbtkGtkExpander
 *
 * Get the value of the has-indicator property
 *
 * Returns: the value the has-indicator property
 */
gboolean
nbtk_gtk_expander_get_has_indicator (NbtkGtkExpander *expander)
{
  g_return_val_if_fail (NBTK_IS_GTK_EXPANDER (expander), 0);

  return expander->priv->has_indicator;
}
