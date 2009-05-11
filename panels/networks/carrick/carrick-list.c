/* carrick-list.c */

#include "carrick-list.h"

#include <config.h>
#include <gtk/gtk.h>

#define CARRICK_DRAG_TARGET "CARRICK_DRAG_TARGET"

static const GtkTargetEntry carrick_targets[] = {
  { CARRICK_DRAG_TARGET, GTK_TARGET_SAME_APP, 0 },
};

G_DEFINE_TYPE (CarrickList, carrick_list, GTK_TYPE_VBOX)

#define LIST_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), CARRICK_TYPE_LIST, CarrickListPrivate))

typedef struct _CarrickListPrivate CarrickListPrivate;

struct _CarrickListPrivate
{
  GtkWidget *drag_window;
  guint list_count;
  guint dropped_new_order;
  guint counter;
};

static void
carrick_list_get_property (GObject *object, guint property_id,
                              GValue *value, GParamSpec *pspec)
{
  switch (property_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
carrick_list_set_property (GObject *object, guint property_id,
                              const GValue *value, GParamSpec *pspec)
{
  switch (property_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
carrick_list_dispose (GObject *object)
{
  G_OBJECT_CLASS (carrick_list_parent_class)->dispose (object);
}

static void
carrick_list_finalize (GObject *object)
{
  G_OBJECT_CLASS (carrick_list_parent_class)->finalize (object);
}

static void
carrick_list_drag_begin (GtkWidget         *widget,
                         GdkDragContext    *context,
                         gpointer           user_data)
{
  CarrickListPrivate *priv = LIST_PRIVATE (user_data);

  g_object_ref (widget);
  gtk_container_remove (GTK_CONTAINER (user_data),
                        widget);
  priv->drag_window = gtk_window_new (GTK_WINDOW_POPUP);
  gtk_container_add (GTK_CONTAINER (priv->drag_window),
                     widget);
  gtk_widget_set_state (widget,
                        GTK_STATE_SELECTED);

  g_object_unref (widget);
  gtk_drag_set_icon_widget (context,
                            priv->drag_window,
                            5,
                            5);
}

static void
carrick_list_drag_data_get (GtkWidget        *widget,
                            GdkDragContext   *context,
                            GtkSelectionData *data,
                            guint             info,
                            guint             time)
{
  if (data->target == gdk_atom_intern_static_string (CARRICK_DRAG_TARGET))
  {
    gtk_selection_data_set (data,
                            data->target,
                            8,
                            (gpointer)&widget,
                            sizeof (gpointer));
  }
}

static gboolean
carrick_list_drag_failed (GtkWidget      *widget,
                          GdkDragContext *context,
                          GtkDragResult   result,
                          gpointer        user_data)
{
  guint order;

  if (result != GTK_DRAG_RESULT_SUCCESS)
  {
    CarrickListPrivate *priv = LIST_PRIVATE (user_data);

    g_object_ref (widget);
    gtk_container_remove (GTK_CONTAINER (priv->drag_window),
                          widget);
    gtk_widget_set_state (widget,
                          GTK_STATE_NORMAL);
    gtk_box_pack_start (GTK_BOX (user_data),
                        widget,
                        TRUE,
                        TRUE,
                        6);
    order = GPOINTER_TO_UINT (g_object_get_data ((GObject *)widget,
                                                 "order"));
    gtk_box_reorder_child (GTK_BOX (user_data),
                           widget,
                           GPOINTER_TO_UINT (order));

    g_object_unref (widget);

    return TRUE;
  }

  return FALSE;
}

static void
carrick_list_drag_end (GtkWidget      *widget,
                       GdkDragContext *context,
                       gpointer        user_data)
{
  CarrickListPrivate *priv = LIST_PRIVATE (user_data);

  GTK_BIN (priv->drag_window)->child = NULL;
  gtk_widget_destroy (priv->drag_window);
}

void
carrick_list_add_item (CarrickList *list,
                       GtkWidget *widget)
{
  g_return_if_fail (CARRICK_IS_LIST (list));
  g_return_if_fail (GTK_IS_WIDGET (widget));

  CarrickListPrivate *priv = LIST_PRIVATE (list);

  /*gtk_drag_source_set (widget,
                       GDK_BUTTON1_MASK,
                       carrick_targets,
                       G_N_ELEMENTS (carrick_targets),
                       GDK_ACTION_MOVE);*/

  g_signal_connect (widget,
                    "drag-begin",
                    G_CALLBACK (carrick_list_drag_begin),
                    list);

  g_signal_connect (widget,
                    "drag-data-get",
                    G_CALLBACK (carrick_list_drag_data_get),
                    list);

  g_signal_connect (widget,
                    "drag-failed",
                    G_CALLBACK (carrick_list_drag_failed),
                    list);

  g_signal_connect (widget,
                    "drag-end",
                    G_CALLBACK (carrick_list_drag_end),
                    list);

  priv->list_count++;

  g_object_set_data (G_OBJECT (widget),
                     "order",
                     GUINT_TO_POINTER (priv->list_count));

  gtk_box_pack_start (GTK_BOX (list),
                      widget,
                      TRUE,
                      TRUE,
                      6);
}


static gboolean
carrick_list_drag_drop (GtkWidget       *widget,
                        GdkDragContext  *context,
                        gint             x,
                        gint             y,
                        guint            time)
{
  GdkAtom target, carrick_target_type;

  target = gtk_drag_dest_find_target (widget,
                                      context,
                                      NULL);
  carrick_target_type = gdk_atom_intern_static_string (CARRICK_DRAG_TARGET);

  if (target == carrick_target_type)
  {
    gtk_drag_get_data (widget,
                       context,
                       target,
                       time);
    return TRUE;
  }

  return FALSE;
}

/*
 * FIXME: This is all very nasty, someone please tell me a better way to
 * do it....
 */

void
_is_drop_target (GtkWidget *child,
                 gpointer   data)
{
  CarrickListPrivate *priv = LIST_PRIVATE (gtk_widget_get_parent (child));
  gint y = GPOINTER_TO_UINT (data);
  gpointer pos;

  if (child->allocation.y <= y &&
      child->allocation.y + child->allocation.height > y)
  {
    pos = g_object_get_data(G_OBJECT (child),
                            "order");
    priv->dropped_new_order = GPOINTER_TO_UINT (pos);
  }
}

void
_verify_orders (GtkWidget *child,
                gpointer data)
{
  CarrickListPrivate *priv = LIST_PRIVATE (gtk_widget_get_parent (child));
  gpointer pos = g_object_get_data (G_OBJECT (child),
                                    "order");
  guint order = GPOINTER_TO_UINT (pos);

  if (order!= priv->counter)
  {
    order = priv->counter;
    g_object_set_data (G_OBJECT (child),
                       "order",
                       GUINT_TO_POINTER (order));
  }

  priv->counter++;
}

static void
carrick_list_drag_data_received (GtkWidget        *widget,
                                 GdkDragContext   *context,
                                 gint              x,
                                 gint              y,
                                 GtkSelectionData *data,
                                 guint             info,
                                 guint             time)
{
  CarrickListPrivate *priv = LIST_PRIVATE (widget);
  GtkWidget *source;

  source = gtk_drag_get_source_widget (context);

  if (source &&
      data->target == gdk_atom_intern_static_string (CARRICK_DRAG_TARGET))
  {
    gtk_container_foreach (GTK_CONTAINER (widget),
                           _is_drop_target,
                           GUINT_TO_POINTER (y));

    g_object_ref (source);
    gtk_container_remove (GTK_CONTAINER (priv->drag_window),
                          source);
    gtk_widget_set_state (GTK_WIDGET (source),
                          GTK_STATE_NORMAL);
    gtk_box_pack_start (GTK_BOX (widget),
                        source,
                        TRUE,
                        TRUE,
                        6);
    gtk_box_reorder_child (GTK_BOX (widget),
                           source,
                           priv->dropped_new_order);
    g_object_unref (source);

    priv->counter = 0;
    gtk_container_foreach (GTK_CONTAINER (widget),
                           _verify_orders,
                           NULL);

    gtk_drag_finish (context,
                     TRUE,
                     FALSE,
                     time);
  }
  else
  {
    gtk_drag_finish (context,
                     FALSE,
                     FALSE,
                     time);
  }
}

static void
carrick_list_class_init (CarrickListClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  g_type_class_add_private (klass, sizeof (CarrickListPrivate));

  object_class->get_property = carrick_list_get_property;
  object_class->set_property = carrick_list_set_property;
  object_class->dispose = carrick_list_dispose;
  object_class->finalize = carrick_list_finalize;
  widget_class->drag_drop = carrick_list_drag_drop;
  widget_class->drag_data_received = carrick_list_drag_data_received;
}

static void
carrick_list_init (CarrickList *self)
{
  CarrickListPrivate *priv = LIST_PRIVATE (self);

  priv->list_count = 0;
  /*gtk_drag_dest_set (GTK_WIDGET (self),
                     GTK_DEST_DEFAULT_ALL,
                     carrick_targets,
                     G_N_ELEMENTS (carrick_targets),
                     GDK_ACTION_MOVE);*/

  gtk_box_set_homogeneous (GTK_BOX (self),
                           TRUE);
  gtk_box_set_spacing (GTK_BOX (self),
                       12);
  gtk_container_set_border_width (GTK_CONTAINER (self),
                                  12);
}

GtkWidget*
carrick_list_new (void)
{
  return g_object_new (CARRICK_TYPE_LIST, NULL);
}
