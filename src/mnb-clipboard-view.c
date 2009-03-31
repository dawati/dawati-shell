#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <clutter/clutter.h>

#include "mnb-clipboard-view.h"
#include "mnb-clipboard-store.h"
#include "mnb-clipboard-item.h"

#define MNB_CLIPBOARD_VIEW_GET_PRIVATE(obj)     (G_TYPE_INSTANCE_GET_PRIVATE ((obj), MNB_TYPE_CLIPBOARD_VIEW, MnbClipboardViewPrivate))

#define ROW_SPACING     (2.0)

struct _MnbClipboardViewPrivate
{
  MnbClipboardStore *store;

  GSList *rows;
};

enum
{
  PROP_0,

  PROP_STORE
};

G_DEFINE_TYPE (MnbClipboardView, mnb_clipboard_view, NBTK_TYPE_WIDGET);

static void
on_action_clicked (MnbClipboardItem *item,
                   MnbClipboardView *view)
{
  g_debug ("%s: Action clicked", G_STRLOC);
}

static void
on_remove_clicked (MnbClipboardItem *item,
                   MnbClipboardView *view)
{
  g_debug ("%s: Remove clicked", G_STRLOC);
}

static void
on_store_item_added (MnbClipboardStore    *store,
                     MnbClipboardItemType  item_type,
                     MnbClipboardView     *view)
{
  ClutterActor *row = NULL;
  ClutterUnit view_width = 0;

  switch (item_type)
    {
    case MNB_CLIPBOARD_ITEM_TEXT:
      {
        gint64 mtime = 0;
        gchar *text = NULL;

        text = mnb_clipboard_store_get_last_text (store, &mtime);
        if (text != NULL)
          {
            row = g_object_new (MNB_TYPE_CLIPBOARD_ITEM,
                                "contents", text,
                                NULL);

            g_signal_connect (row, "remove-clicked",
                              G_CALLBACK (on_remove_clicked),
                              view);
            g_signal_connect (row, "action-clicked",
                              G_CALLBACK (on_action_clicked),
                              view);

            g_free (text);
          }
      }
      break;

    case MNB_CLIPBOARD_ITEM_URIS:
      break;

    case MNB_CLIPBOARD_ITEM_IMAGE:
      break;

    case MNB_CLIPBOARD_ITEM_INVALID:
      g_assert_not_reached ();
      break;
    }

  if (row == NULL)
    return;

  g_debug ("%s: Adding new row to Pasteboard", G_STRLOC);

  /* FIXME - hack hack hack; we need NbtkBox */
  view_width = clutter_actor_get_widthu (CLUTTER_ACTOR (view));
  clutter_actor_set_widthu (row, view_width - 1);

  view->priv->rows = g_slist_prepend (view->priv->rows, row);
  clutter_actor_set_parent (row, CLUTTER_ACTOR (view));
  clutter_actor_queue_relayout (CLUTTER_ACTOR (row));
}

static void
mnb_clipboard_view_get_preferred_height (ClutterActor *actor,
                                         ClutterUnit   for_width,
                                         ClutterUnit  *min_height_p,
                                         ClutterUnit  *natural_height_p)
{
  MnbClipboardViewPrivate *priv = MNB_CLIPBOARD_VIEW (actor)->priv;
  ClutterUnit min_height, natural_height;
  NbtkPadding padding = { 0, };
  GSList *l;

  nbtk_widget_get_padding (NBTK_WIDGET (actor), &padding);

  min_height = natural_height = padding.top + padding.bottom;

  for (l = priv->rows; l != NULL; l = l->next)
    {
      ClutterActor *child = l->data;
      ClutterUnit child_min, child_natural;

      clutter_actor_get_preferred_height (child, for_width,
                                          &child_min,
                                          &child_natural);

      min_height += (child_min + ROW_SPACING);
      natural_height += (child_natural + ROW_SPACING);
    }

  if (min_height_p)
    *min_height_p = min_height;

  if (natural_height_p)
    *natural_height_p = natural_height;
}

static void
mnb_clipboard_view_allocate (ClutterActor *actor,
                             const ClutterActorBox *box,
                             gboolean origin_changed)
{
  MnbClipboardViewPrivate *priv = MNB_CLIPBOARD_VIEW (actor)->priv;
  ClutterUnit available_width, available_height;
  NbtkPadding padding = { 0, };
  ClutterUnit y;
  GSList *l;

  CLUTTER_ACTOR_CLASS (mnb_clipboard_view_parent_class)->allocate (actor, box, origin_changed);

  nbtk_widget_get_padding (NBTK_WIDGET (actor), &padding);

  available_width  = box->x2 - box->x1
                   - padding.left
                   - padding.right;
  available_height = box->y2 - box->y1
                   - padding.top
                   - padding.bottom;

  y = box->y1 + padding.top;

  for (l = priv->rows; l != NULL; l = l->next)
    {
      ClutterActor *child = l->data;
      ClutterUnit child_height;
      ClutterActorBox child_box = { 0, };

      clutter_actor_get_preferred_height (child, available_width,
                                          NULL,
                                          &child_height);

      child_box.x1 = box->x1 + padding.left;
      child_box.y1 = y;
      child_box.x2 = child_box.x1 + available_width;
      child_box.y2 = child_box.y1 + child_height;

      clutter_actor_allocate (child, &child_box, origin_changed);

      y += (child_height + ROW_SPACING);
    }
}

static void
mnb_clipboard_view_paint (ClutterActor *actor)
{
  MnbClipboardViewPrivate *priv = MNB_CLIPBOARD_VIEW (actor)->priv;
  GSList *l;
  gint i;

  CLUTTER_ACTOR_CLASS (mnb_clipboard_view_parent_class)->paint (actor);

  for (l = priv->rows, i = 0; l != NULL; l = l->next, i++)
    {
      ClutterActor *child = l->data;

      if (!CLUTTER_ACTOR_IS_VISIBLE (child))
        {
          i--;
          continue;
        }

      if (i == 0)
        {
          ClutterActorBox box = { 0, };

          clutter_actor_get_allocation_box (child, &box);

          cogl_set_source_color4ub (0xef, 0xef, 0xef, 255);
          cogl_rectangle (box.x1 + 1, box.y1 + 1,
                          box.x2 - 1, box.y2 - 1);
        }

      clutter_actor_paint (child);

      if (i != 0)
        {
          ClutterActorBox box = { 0, };

          clutter_actor_get_allocation_box (child, &box);

          cogl_set_source_color4ub (0x1a, 0x1a, 0x1a, 255);
          cogl_path_move_to (box.x1, box.y1);
          cogl_path_line_to (box.x2, box.y1);
        }
    }
}

static void
mnb_clipboard_view_pick (ClutterActor *actor,
                         const ClutterColor *pick_color)
{
  MnbClipboardViewPrivate *priv = MNB_CLIPBOARD_VIEW (actor)->priv;
  GSList *l;

  CLUTTER_ACTOR_CLASS (mnb_clipboard_view_parent_class)->pick (actor, pick_color);

  for (l = priv->rows; l != NULL; l = l->next)
    clutter_actor_paint (l->data);
}

static void
mnb_clipboard_view_finalize (GObject *gobject)
{
  MnbClipboardViewPrivate *priv = MNB_CLIPBOARD_VIEW (gobject)->priv;
  GSList *l;

  g_object_unref (priv->store);

  g_slist_foreach (priv->rows, (GFunc) clutter_actor_destroy, NULL);
  g_slist_free (priv->rows);

  G_OBJECT_CLASS (mnb_clipboard_view_parent_class)->finalize (gobject);
}

static void
mnb_clipboard_view_set_property (GObject      *gobject,
                                 guint         prop_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
  MnbClipboardViewPrivate *priv = MNB_CLIPBOARD_VIEW (gobject)->priv;

  switch (prop_id)
    {
    case PROP_STORE:
      if (priv->store != NULL)
        g_object_unref (priv->store);

      if (g_value_get_object (value) != NULL)
        priv->store = g_value_dup_object (value);
      else
        priv->store = mnb_clipboard_store_new ();

      g_signal_connect (priv->store, "item-added",
                        G_CALLBACK (on_store_item_added),
                        gobject);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
      break;
    }
}

static void
mnb_clipboard_view_get_property (GObject    *gobject,
                                 guint       prop_id,
                                 GValue     *value,
                                 GParamSpec *pspec)
{
  MnbClipboardViewPrivate *priv = MNB_CLIPBOARD_VIEW (gobject)->priv;

  switch (prop_id)
    {
    case PROP_STORE:
      g_value_set_object (value, priv->store);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
      break;
    }
}

static void
mnb_clipboard_view_class_init (MnbClipboardViewClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  ClutterActorClass *actor_class = CLUTTER_ACTOR_CLASS (klass);
  GParamSpec *pspec;

  g_type_class_add_private (klass, sizeof (MnbClipboardViewPrivate));

  gobject_class->set_property = mnb_clipboard_view_set_property;
  gobject_class->get_property = mnb_clipboard_view_get_property;
  gobject_class->finalize = mnb_clipboard_view_finalize;

  actor_class->get_preferred_height = mnb_clipboard_view_get_preferred_height;
  actor_class->allocate = mnb_clipboard_view_allocate;
  actor_class->paint = mnb_clipboard_view_paint;
  actor_class->pick = mnb_clipboard_view_pick;

  pspec = g_param_spec_object ("store",
                               "Store",
                               "The MnbClipboardStore",
                               MNB_TYPE_CLIPBOARD_STORE,
                               G_PARAM_READWRITE |
                               G_PARAM_CONSTRUCT);
  g_object_class_install_property (gobject_class, PROP_STORE, pspec);
}

static void
mnb_clipboard_view_init (MnbClipboardView *view)
{
  view->priv = MNB_CLIPBOARD_VIEW_GET_PRIVATE (view);
}

NbtkWidget *
mnb_clipboard_view_new (MnbClipboardStore *store)
{
  g_return_val_if_fail (store == NULL || MNB_IS_CLIPBOARD_STORE (store), NULL);

  return g_object_new (MNB_TYPE_CLIPBOARD_VIEW,
                       "store", store,
                       NULL);
}
