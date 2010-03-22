/*
 * Copyright (c) 2009 Intel Corp.
 *
 * Author: Chris Lord <chris@linux.intel.com>
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

#include "mnb-fancy-bin.h"

static void mx_stylable_iface_init (MxStylableIface *iface);

G_DEFINE_TYPE_WITH_CODE (MnbFancyBin, mnb_fancy_bin, MX_TYPE_WIDGET,
                         G_IMPLEMENT_INTERFACE (MX_TYPE_STYLABLE,
                                                mx_stylable_iface_init))

#define FANCY_BIN_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MNB_TYPE_FANCY_BIN, MnbFancyBinPrivate))

enum
{
  PROP_0,

  PROP_FANCY,
  PROP_FANCINESS,
};

struct _MnbFancyBinPrivate
{
  gboolean      fancy;
  gdouble       fanciness;
  ClutterActor *real_child;
  ClutterActor *child;
  ClutterActor *clone;
  guint         curve_radius;
};


static void
mnb_fancy_bin_get_property (GObject    *object,
                            guint       property_id,
                            GValue     *value,
                            GParamSpec *pspec)
{
  MnbFancyBinPrivate *priv = MNB_FANCY_BIN (object)->priv;

  switch (property_id)
    {
    case PROP_FANCY :
      g_value_set_boolean (value, priv->fancy);
      break;

    case PROP_FANCINESS :
      g_value_set_double (value, priv->fanciness);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
mnb_fancy_bin_set_property (GObject      *object,
                            guint         property_id,
                            const GValue *value,
                            GParamSpec   *pspec)
{
  MnbFancyBin *self = MNB_FANCY_BIN (object);
  MnbFancyBinPrivate *priv = self->priv;

  switch (property_id)
    {
    case PROP_FANCY :
      mnb_fancy_bin_set_fancy (self, g_value_get_boolean (value));
      break;

    case PROP_FANCINESS :
      priv->fanciness = g_value_get_double (value);

      if (priv->clone)
        clutter_actor_set_opacity (priv->clone,
                                   (guint8)((1.0 - priv->fanciness) * 255));

      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
mnb_fancy_bin_dispose (GObject *object)
{
  MnbFancyBinPrivate *priv = MNB_FANCY_BIN (object)->priv;

  if (priv->clone)
    {
      clutter_actor_destroy (priv->clone);
      priv->clone = NULL;
    }

  if (priv->child)
    {
      clutter_actor_destroy (priv->child);
      priv->child = NULL;
    }

  if (priv->real_child)
    {
      clutter_actor_unparent (priv->real_child);
      priv->real_child = NULL;
    }

  G_OBJECT_CLASS (mnb_fancy_bin_parent_class)->dispose (object);
}

static void
mnb_fancy_bin_finalize (GObject *object)
{
  G_OBJECT_CLASS (mnb_fancy_bin_parent_class)->finalize (object);
}

static void
mnb_fancy_bin_paint (ClutterActor *actor)
{
  MnbFancyBin *self = MNB_FANCY_BIN (actor);
  MnbFancyBinPrivate *priv = self->priv;

  /* Draw the clipped child if necessary */
  if (priv->fanciness > 0.0)
    {
      MxPadding padding;
      gfloat width, height;

      clutter_actor_get_size (actor, &width, &height);
      mx_widget_get_padding (MX_WIDGET (actor), &padding);

      /* Create a clip path so that the clone won't poke out
       * from underneath the background.
       */
      cogl_path_new ();
      cogl_path_move_to (padding.left + priv->curve_radius,
                         padding.top);
      cogl_path_arc (width - padding.right - priv->curve_radius,
                     priv->curve_radius + padding.top,
                     priv->curve_radius,
                     priv->curve_radius,
                     270,
                     360);
      cogl_path_arc (width - padding.right - priv->curve_radius,
                     height - padding.bottom - priv->curve_radius,
                     priv->curve_radius,
                     priv->curve_radius,
                     0,
                     90);
      cogl_path_arc (padding.left + priv->curve_radius,
                     height - padding.bottom - priv->curve_radius,
                     priv->curve_radius,
                     priv->curve_radius,
                     90,
                     180);
      cogl_path_arc (padding.left + priv->curve_radius,
                     padding.top + priv->curve_radius,
                     priv->curve_radius,
                     priv->curve_radius,
                     180,
                     270);
      cogl_path_close ();
      cogl_clip_push_from_path ();

      /* Paint child */
      if (priv->child)
        clutter_actor_paint (priv->child);

      cogl_clip_pop ();

      /* Chain up for background */
      CLUTTER_ACTOR_CLASS (mnb_fancy_bin_parent_class)->paint (actor);
    }

  /* Draw the un-fancy child */
  if ((priv->fanciness < 1.0) && priv->clone)
    clutter_actor_paint (priv->clone);
}

static void
mnb_fancy_bin_map (ClutterActor *actor)
{
  MnbFancyBinPrivate *priv = MNB_FANCY_BIN (actor)->priv;

  CLUTTER_ACTOR_CLASS (mnb_fancy_bin_parent_class)->map (actor);

  if (priv->child)
    clutter_actor_map (priv->child);
  if (priv->clone)
    clutter_actor_map (priv->clone);
  if (priv->real_child)
    clutter_actor_map (priv->real_child);
}

static void
mnb_fancy_bin_unmap (ClutterActor *actor)
{
  MnbFancyBinPrivate *priv = MNB_FANCY_BIN (actor)->priv;

  CLUTTER_ACTOR_CLASS (mnb_fancy_bin_parent_class)->unmap (actor);

  if (priv->child)
    clutter_actor_unmap (priv->child);
  if (priv->clone)
    clutter_actor_unmap (priv->clone);
  if (priv->real_child)
    clutter_actor_unmap (priv->real_child);
}

static void
mnb_fancy_bin_get_preferred_width (ClutterActor *actor,
                                   gfloat        for_height,
                                   gfloat       *min_width_p,
                                   gfloat       *nat_width_p)
{
  MxPadding padding;
  MnbFancyBinPrivate *priv = MNB_FANCY_BIN (actor)->priv;

  if (min_width_p)
    *min_width_p = 0;
  if (nat_width_p)
    *nat_width_p = 0;

  if (priv->real_child)
    clutter_actor_get_preferred_width (priv->real_child,
                                       for_height,
                                       min_width_p,
                                       nat_width_p);

  mx_widget_get_padding (MX_WIDGET (actor), &padding);

  if (min_width_p)
    *min_width_p += padding.left + padding.right;
  if (nat_width_p)
    *nat_width_p += padding.left + padding.right;
}

static void
mnb_fancy_bin_get_preferred_height (ClutterActor *actor,
                                    gfloat        for_width,
                                    gfloat       *min_height_p,
                                    gfloat       *nat_height_p)
{
  MxPadding padding;
  MnbFancyBinPrivate *priv = MNB_FANCY_BIN (actor)->priv;

  if (min_height_p)
    *min_height_p = 0;
  if (nat_height_p)
    *nat_height_p = 0;

  if (priv->real_child)
    clutter_actor_get_preferred_height (priv->real_child,
                                        for_width,
                                        min_height_p,
                                        nat_height_p);

  mx_widget_get_padding (MX_WIDGET (actor), &padding);

  if (min_height_p)
    *min_height_p += padding.top + padding.bottom;
  if (nat_height_p)
    *nat_height_p += padding.top + padding.bottom;
}

static void
mnb_fancy_bin_allocate (ClutterActor          *actor,
                        const ClutterActorBox *box,
                        ClutterAllocationFlags flags)
{
  MxPadding padding;
  ClutterActorBox child_box;

  MnbFancyBinPrivate *priv = MNB_FANCY_BIN (actor)->priv;

  mx_widget_get_padding (MX_WIDGET (actor), &padding);

  if (priv->real_child)
    clutter_actor_allocate_preferred_size (priv->real_child, flags);

  child_box.x1 = padding.left;
  child_box.x2 = box->x2 - box->x1 - padding.right;
  child_box.y1 = padding.top;
  child_box.y2 = box->y2 - box->y1 - padding.bottom;

  if (priv->child)
    clutter_actor_allocate (priv->child, &child_box, flags);

  if (priv->clone)
    clutter_actor_allocate (priv->clone, &child_box, flags);

  CLUTTER_ACTOR_CLASS (mnb_fancy_bin_parent_class)->
    allocate (actor, box, flags);
}

static void
mnb_fancy_bin_class_init (MnbFancyBinClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  ClutterActorClass *actor_class = CLUTTER_ACTOR_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MnbFancyBinPrivate));

  object_class->get_property = mnb_fancy_bin_get_property;
  object_class->set_property = mnb_fancy_bin_set_property;
  object_class->dispose = mnb_fancy_bin_dispose;
  object_class->finalize = mnb_fancy_bin_finalize;

  actor_class->paint = mnb_fancy_bin_paint;
  actor_class->map = mnb_fancy_bin_map;
  actor_class->unmap = mnb_fancy_bin_unmap;
  actor_class->get_preferred_width = mnb_fancy_bin_get_preferred_width;
  actor_class->get_preferred_height = mnb_fancy_bin_get_preferred_height;
  actor_class->allocate = mnb_fancy_bin_allocate;

  g_object_class_install_property (object_class,
                                   PROP_FANCY,
                                   g_param_spec_boolean ("fancy",
                                                         "Fancy",
                                                         "Display a fancy "
                                                         "frame.",
                                                         FALSE,
                                                         G_PARAM_READWRITE |
                                                         G_PARAM_STATIC_NAME |
                                                         G_PARAM_STATIC_NICK |
                                                         G_PARAM_STATIC_BLURB));

  g_object_class_install_property (object_class,
                                   PROP_FANCINESS,
                                   g_param_spec_double ("fanciness",
                                                        "Fanciness",
                                                        "How opaque the fancy "
                                                        "frame is.",
                                                        0.0, 1.0, 0.0,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_STATIC_NAME |
                                                        G_PARAM_STATIC_NICK |
                                                        G_PARAM_STATIC_BLURB));
}

static void
mx_stylable_iface_init (MxStylableIface *iface)
{
  static gboolean is_initialised = FALSE;

  if (!is_initialised)
    {
      GParamSpec *pspec;

      is_initialised = TRUE;

      pspec = g_param_spec_uint ("curve-radius",
                                 "Curve radius",
                                 "The curve radius used when cutting off "
                                 "the corners of the cloned actor, in px.",
                                 0, G_MAXUINT, 2,
                                 G_PARAM_READWRITE);
      mx_stylable_iface_install_property (iface, MNB_TYPE_FANCY_BIN, pspec);
    }
}

static void
mnb_fancy_bin_style_changed_cb (MxStylable          *stylable,
                                MxStyleChangedFlags  flags,
                                MnbFancyBin         *self)
{
  MnbFancyBinPrivate *priv = self->priv;

  mx_stylable_get (stylable,
                   "curve-radius", &priv->curve_radius,
                   NULL);
}

static void
mnb_fancy_bin_init (MnbFancyBin *self)
{
  MnbFancyBinPrivate *priv = self->priv = FANCY_BIN_PRIVATE (self);

  priv->curve_radius = 2;

  priv->child = clutter_clone_new (NULL);
  priv->clone = clutter_clone_new (NULL);
  clutter_actor_set_parent (priv->child, CLUTTER_ACTOR (self));
  clutter_actor_set_parent (priv->clone, CLUTTER_ACTOR (self));

  g_signal_connect (self, "style-changed",
                    G_CALLBACK (mnb_fancy_bin_style_changed_cb), self);
}

ClutterActor *
mnb_fancy_bin_new ()
{
  return g_object_new (MNB_TYPE_FANCY_BIN, NULL);
}

void
mnb_fancy_bin_set_child (MnbFancyBin *bin, ClutterActor *child)
{
  MnbFancyBinPrivate *priv = bin->priv;

  if (priv->real_child)
    {
      clutter_clone_set_source (CLUTTER_CLONE (priv->child), NULL);
      clutter_clone_set_source (CLUTTER_CLONE (priv->clone), NULL);
      clutter_actor_unparent (priv->real_child);
    }

  priv->real_child = child;

  if (priv->real_child)
    {
      clutter_actor_set_parent (priv->real_child, CLUTTER_ACTOR (bin));
      clutter_clone_set_source (CLUTTER_CLONE (priv->child), priv->real_child);
      clutter_clone_set_source (CLUTTER_CLONE (priv->clone), priv->real_child);
    }

  clutter_actor_queue_relayout (CLUTTER_ACTOR (bin));
}

ClutterActor *
mnb_fancy_bin_get_child (MnbFancyBin *bin)
{
  return bin->priv->real_child;
}

void
mnb_fancy_bin_set_fancy (MnbFancyBin *bin, gboolean fancy)
{
  MnbFancyBinPrivate *priv = bin->priv;

  if (priv->fancy != fancy)
    {
      priv->fancy = fancy;

      clutter_actor_animate (CLUTTER_ACTOR (bin),
                             CLUTTER_EASE_IN_QUAD,
                             200,
                             "fanciness", fancy ? 1.0 : 0.0,
                             NULL);
    }
}

gboolean
mnb_fancy_bin_get_fancy (MnbFancyBin *bin)
{
  return bin->priv->fancy;
}
