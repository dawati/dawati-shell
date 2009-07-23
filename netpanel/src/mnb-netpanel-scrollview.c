/* mnb-netpanel-scrollview.c */
/*
 * Copyright (c) 2009 Intel Corp.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 */

#include "mnb-netpanel-scrollview.h"
#include "mwb-utils.h"

/* FIXME: replace with styles or properties */
#define MAX_DISPLAY 4
#define ROW_SPACING 6
#define COL_SPACING 6
#define SCROLLBAR_HEIGHT 24

G_DEFINE_TYPE (MnbNetpanelScrollview, mnb_netpanel_scrollview, NBTK_TYPE_WIDGET)

#define SCROLLVIEW_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MNB_TYPE_NETPANEL_SCROLLVIEW, MnbNetpanelScrollviewPrivate))

typedef struct
{
  ClutterActor *item;
  ClutterActor *title;
  guint order;
  gfloat position;
} ItemProps;

struct _MnbNetpanelScrollviewPrivate
{
  GList          *items;

  NbtkWidget     *scroll_bar;
  NbtkAdjustment *scroll_adjustment;
  gint            scroll_offset;
  gint            scroll_page;
  gint            scroll_item;
  gint            scroll_total;
};

static void
mnb_netpanel_scrollview_dispose (GObject *object)
{
  MnbNetpanelScrollview *self = MNB_NETPANEL_SCROLLVIEW (object);
  MnbNetpanelScrollviewPrivate *priv = self->priv;

  while (priv->items)
    {
      ItemProps *props = priv->items->data;
      clutter_actor_unparent (props->item);
      clutter_actor_unparent (props->title);
      g_slice_free (ItemProps, props);
      priv->items = g_list_delete_link (priv->items, priv->items);
    }

  if (priv->scroll_bar)
    {
      clutter_actor_unparent (CLUTTER_ACTOR (priv->scroll_bar));
      priv->scroll_bar = NULL;
    }

  G_OBJECT_CLASS (mnb_netpanel_scrollview_parent_class)->dispose (object);
}

static void
mnb_netpanel_scrollview_finalize (GObject *object)
{
  G_OBJECT_CLASS (mnb_netpanel_scrollview_parent_class)->finalize (object);
}

static void
mnb_netpanel_scrollview_allocate (ClutterActor           *actor,
                                  const ClutterActorBox  *box,
                                  ClutterAllocationFlags  flags)
{
  ClutterActorBox child_box;
  NbtkPadding padding;
  gfloat width, height;
  gfloat item_width = 0.0, item_height = 0.0, title_height = 0.0;
  guint n_items;
  ItemProps *props;
  GList *item;
  MnbNetpanelScrollviewPrivate *priv = MNB_NETPANEL_SCROLLVIEW (actor)->priv;

  CLUTTER_ACTOR_CLASS (mnb_netpanel_scrollview_parent_class)->
    allocate (actor, box, flags);

  if (!priv->items)
    return;

  nbtk_widget_get_padding (NBTK_WIDGET (actor), &padding);
  padding.left   = MWB_PIXBOUND (padding.left);
  padding.top    = MWB_PIXBOUND (padding.top);
  padding.right  = MWB_PIXBOUND (padding.right);
  padding.bottom = MWB_PIXBOUND (padding.bottom);

  width = box->x2 - box->x1 - padding.left - padding.right;
  height = box->y2 - box->y1 - padding.top - padding.bottom;

  props = priv->items->data;
  clutter_actor_get_preferred_size (props->item, NULL, NULL,
                                    &item_width, &item_height);
  clutter_actor_get_preferred_height (props->title, -1, NULL, &title_height);

  /* Allocate for the items and titles */
  child_box.x1 = padding.left;

  for (item = priv->items; item != NULL; item = item->next)
    {
      props = item->data;
      props->position = child_box.x1;

      child_box.x2 = child_box.x1 + item_width;
      child_box.y1 = padding.top;
      child_box.y2 = child_box.y1 + item_height;
      clutter_actor_allocate (props->item, &child_box, flags);

      child_box.y1 = child_box.y2 + ROW_SPACING;
      child_box.y2 = child_box.y1 + title_height;
      clutter_actor_allocate (props->title, &child_box, flags);

      child_box.x1 = child_box.x2 + COL_SPACING;
    }

  n_items = g_list_length (priv->items);
  if (n_items > MAX_DISPLAY)
    {
      priv->scroll_total = n_items * item_width;
      priv->scroll_total += (n_items - 1) * COL_SPACING;

      priv->scroll_page = width;
      priv->scroll_item = item_width + COL_SPACING;

      /* Sync up the scroll-bar properties */
      g_object_set (G_OBJECT (priv->scroll_adjustment),
                    "page-increment", (gdouble)width,
                    "page-size", (gdouble)width,
                    "upper", (gdouble)priv->scroll_total,
                    NULL);

      nbtk_adjustment_set_value (priv->scroll_adjustment,
                                 (gdouble)priv->scroll_offset);

      /* Allocate for the scrollbar */
      child_box.x1 = padding.left;
      child_box.x2 = child_box.x1 + width;
      child_box.y1 = padding.top + item_height + title_height + 2 * ROW_SPACING;
      child_box.y2 = child_box.y1 + SCROLLBAR_HEIGHT;
      clutter_actor_allocate (CLUTTER_ACTOR (priv->scroll_bar), &child_box,
                              flags);
    }
}

static void
mnb_netpanel_scrollview_get_preferred_width (ClutterActor *self,
                                             gfloat        for_height,
                                             gfloat       *min_width_p,
                                             gfloat       *natural_width_p)
{
  gfloat item_width = 0.0, width;
  guint n_items;
  ItemProps *props;
  MnbNetpanelScrollviewPrivate *priv = MNB_NETPANEL_SCROLLVIEW (self)->priv;

  CLUTTER_ACTOR_CLASS (mnb_netpanel_scrollview_parent_class)->
    get_preferred_height (self, for_height, min_width_p, natural_width_p);

  if (!priv->items)
    return;

  props = priv->items->data;
  clutter_actor_get_preferred_width (props->item, -1, NULL, &item_width);

  n_items = g_list_length (priv->items);
  if (n_items > MAX_DISPLAY)
    n_items = MAX_DISPLAY;

  width = n_items * item_width;
  width += (n_items - 1) * COL_SPACING;

  if (min_width_p)
    *min_width_p += width;
  if (natural_width_p)
    *natural_width_p += width;
}

static void
add_child_height (ClutterActor *actor,
                  gfloat        for_width,
                  gfloat       *min_height_p,
                  gfloat       *natural_height_p,
                  gfloat        spacing)
{
  gfloat min_height = 0.0, natural_height = 0.0;

  if (!actor)
    return;

  clutter_actor_get_preferred_height (actor, for_width, &min_height,
                                      &natural_height);

  if (min_height_p)
    *min_height_p += min_height + spacing;
  if (natural_height_p)
    *natural_height_p += natural_height + spacing;
}

static void
mnb_netpanel_scrollview_get_preferred_height (ClutterActor *self,
                                              gfloat        for_width,
                                              gfloat       *min_height_p,
                                              gfloat       *natural_height_p)
{
  guint n_items;
  ItemProps *props;
  MnbNetpanelScrollviewPrivate *priv = MNB_NETPANEL_SCROLLVIEW (self)->priv;

  CLUTTER_ACTOR_CLASS (mnb_netpanel_scrollview_parent_class)->
    get_preferred_height (self, for_width, min_height_p, natural_height_p);

  if (!priv->items)
    return;

  props = priv->items->data;
  add_child_height (props->item, -1,
                    min_height_p, natural_height_p, 0.0);
  add_child_height (props->title, -1,
                    min_height_p, natural_height_p, ROW_SPACING);

  n_items = g_list_length (priv->items);
  if (n_items > MAX_DISPLAY)  /* Include scrollbar */
    {
      if (min_height_p)
        *min_height_p += SCROLLBAR_HEIGHT + ROW_SPACING;
      if (natural_height_p)
        *natural_height_p += SCROLLBAR_HEIGHT + ROW_SPACING;
    }
}

static void
mnb_netpanel_scrollview_paint (ClutterActor *actor)
{
  GList *item;
  gfloat width, height, offset;
  MnbNetpanelScrollviewPrivate *priv = MNB_NETPANEL_SCROLLVIEW (actor)->priv;

  /* Chain up to get the background */
  CLUTTER_ACTOR_CLASS (mnb_netpanel_scrollview_parent_class)->paint (actor);

  clutter_actor_get_size (actor, &width, &height);

  cogl_clip_push (0, 0, width, height);

  offset = priv->scroll_offset;
  cogl_translate (-priv->scroll_offset, 0, 0);

  /* Draw items */
  for (item = priv->items; item != NULL; item = item->next)
    {
      ItemProps *props = item->data;

      if (props->position > width + offset)
        break;
      if (props->position + priv->scroll_item < offset)
        continue;

      if (CLUTTER_ACTOR_IS_MAPPED (props->item))
        {
          clutter_actor_paint (props->item);
          clutter_actor_paint (props->title);
        }
    }

  cogl_translate (priv->scroll_offset, 0, 0);

  /* Draw scrollbar */
  if (CLUTTER_ACTOR_IS_MAPPED (priv->scroll_bar))
    clutter_actor_paint (CLUTTER_ACTOR (priv->scroll_bar));

  cogl_clip_pop ();
}

static void
mnb_netpanel_scrollview_pick (ClutterActor       *actor,
                              const ClutterColor *color)
{
  cogl_set_source_color4ub (color->red,
                            color->green,
                            color->blue,
                            color->alpha);
  cogl_rectangle (0, 0,
                  clutter_actor_get_width (actor),
                  clutter_actor_get_height (actor));

  mnb_netpanel_scrollview_paint (actor);
}

static void
mnb_netpanel_scrollview_map (ClutterActor *actor)
{
  GList *item;
  MnbNetpanelScrollviewPrivate *priv = MNB_NETPANEL_SCROLLVIEW (actor)->priv;

  CLUTTER_ACTOR_CLASS (mnb_netpanel_scrollview_parent_class)->map (actor);

  for (item = priv->items; item != NULL; item = item->next)
    {
      ItemProps *props = item->data;
      clutter_actor_map (props->item);
      clutter_actor_map (props->title);
    }

  if (priv->scroll_bar)
    clutter_actor_map (CLUTTER_ACTOR (priv->scroll_bar));
}

static void
mnb_netpanel_scrollview_unmap (ClutterActor *actor)
{
  GList *item;
  MnbNetpanelScrollviewPrivate *priv = MNB_NETPANEL_SCROLLVIEW (actor)->priv;

  CLUTTER_ACTOR_CLASS (mnb_netpanel_scrollview_parent_class)->unmap (actor);

  for (item = priv->items; item != NULL; item = item->next)
    {
      ItemProps *props = item->data;
      clutter_actor_unmap (props->item);
      clutter_actor_unmap (props->title);
    }

  if (priv->scroll_bar)
    clutter_actor_unmap (CLUTTER_ACTOR (priv->scroll_bar));
}

static void
mnb_netpanel_scrollview_class_init (MnbNetpanelScrollviewClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  ClutterActorClass *actor_class = CLUTTER_ACTOR_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MnbNetpanelScrollviewPrivate));

  object_class->dispose = mnb_netpanel_scrollview_dispose;
  object_class->finalize = mnb_netpanel_scrollview_finalize;

  actor_class->allocate = mnb_netpanel_scrollview_allocate;
  actor_class->get_preferred_width =
    mnb_netpanel_scrollview_get_preferred_width;
  actor_class->get_preferred_height =
    mnb_netpanel_scrollview_get_preferred_height;
  actor_class->paint = mnb_netpanel_scrollview_paint;
  actor_class->pick = mnb_netpanel_scrollview_pick;
  actor_class->map = mnb_netpanel_scrollview_map;
  actor_class->unmap = mnb_netpanel_scrollview_unmap;
}

static gboolean
mnb_netpanel_scrollview_scroll_event_cb (ClutterActor       *actor,
                                         ClutterScrollEvent *event,
                                         gpointer            ignored)
{
  GList *item = NULL, *last;
  gint offset = 0;
  MnbNetpanelScrollview *self = MNB_NETPANEL_SCROLLVIEW (actor);
  MnbNetpanelScrollviewPrivate *priv = self->priv;

  if (!priv->items)
    return TRUE;

  switch (event->direction)
    {
    case CLUTTER_SCROLL_UP:
    case CLUTTER_SCROLL_LEFT:
      offset = priv->scroll_offset - priv->scroll_item;
      break;

    case CLUTTER_SCROLL_DOWN:
    case CLUTTER_SCROLL_RIGHT:
      offset = priv->scroll_offset + priv->scroll_item;
      break;
    }

  last = priv->items;
  for (item = priv->items; item != NULL; item = item->next)
    {
      ItemProps *props = item->data;
      if (offset < props->position)
        break;
      last = item;
    }

  offset = ((ItemProps *)last->data)->position;

  if (offset > priv->scroll_total - priv->scroll_page)
    offset = priv->scroll_total - priv->scroll_page;
  if (offset < 0)
    offset = 0;

  if (offset != priv->scroll_offset)
    {
      priv->scroll_offset = offset;
      nbtk_adjustment_set_value (priv->scroll_adjustment, (gdouble)offset);
    }

  return TRUE;
}

static void
mnb_netpanel_scrollview_scroll_value_cb (NbtkAdjustment        *adjustment,
                                         GParamSpec            *pspec,
                                         MnbNetpanelScrollview *self)
{
  MnbNetpanelScrollviewPrivate *priv = self->priv;
  gint value = (gint)nbtk_adjustment_get_value (adjustment);

  if (value != priv->scroll_offset)
    {
      priv->scroll_offset = value;
      clutter_actor_queue_relayout (CLUTTER_ACTOR (self));
    }
}

static void
mnb_netpanel_scrollview_init (MnbNetpanelScrollview *self)
{
  MnbNetpanelScrollviewPrivate *priv = self->priv = SCROLLVIEW_PRIVATE (self);

  clutter_actor_set_reactive (CLUTTER_ACTOR (self), TRUE);

  priv->scroll_adjustment = nbtk_adjustment_new (0, 0, 0, 100, 200, 200);
  priv->scroll_bar = nbtk_scroll_bar_new (priv->scroll_adjustment);
  g_object_unref (priv->scroll_adjustment);
  clutter_actor_set_parent (CLUTTER_ACTOR (priv->scroll_bar),
                            CLUTTER_ACTOR (self));
  clutter_actor_hide (CLUTTER_ACTOR (priv->scroll_bar));

  g_signal_connect (self, "scroll-event",
                    G_CALLBACK (mnb_netpanel_scrollview_scroll_event_cb), NULL);
  g_signal_connect (priv->scroll_adjustment, "notify::value",
                    G_CALLBACK (mnb_netpanel_scrollview_scroll_value_cb), self);
}

NbtkWidget*
mnb_netpanel_scrollview_new (void)
{
  return g_object_new (MNB_TYPE_NETPANEL_SCROLLVIEW, NULL);
}

void
mnb_netpanel_scrollview_add_item (MnbNetpanelScrollview *self,
                                  guint                  order,
                                  ClutterActor          *item,
                                  ClutterActor          *title)
{
  GList *i;
  ItemProps *props;
  MnbNetpanelScrollviewPrivate *priv = self->priv;

  clutter_actor_set_parent (item, CLUTTER_ACTOR (self));
  clutter_actor_set_parent (title, CLUTTER_ACTOR (self));

  props = g_slice_new0 (ItemProps);
  props->item = item;
  props->title = title;
  props->order = order;

  for (i = priv->items; i != NULL; i = i->next)
    {
      ItemProps *cur = i->data;
      if (cur->order > order)
        break;
    }

  priv->items = g_list_insert_before (priv->items, i, props);

  if (g_list_length (priv->items) > MAX_DISPLAY)
    clutter_actor_show (CLUTTER_ACTOR (priv->scroll_bar));
}
