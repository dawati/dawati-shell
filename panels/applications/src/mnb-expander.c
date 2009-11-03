
/*
 * Copyright (c) 2009 Intel Corp.
 *
 * Author: Robert Staudinger <robertx.staudinger@intel.com>
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
 */

#include "mnb-expander.h"

static void _stylable_iface_init (NbtkStylableIface *iface);

G_DEFINE_TYPE_WITH_CODE (MnbExpander, mnb_expander, NBTK_TYPE_EXPANDER,
                         G_IMPLEMENT_INTERFACE (NBTK_TYPE_STYLABLE, _stylable_iface_init))

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MNB_TYPE_EXPANDER, MnbExpanderPrivate))

enum
{
  FRAME_ALLOCATED,

  LAST_SIGNAL
};

typedef struct  {
  gboolean is_animating;
} MnbExpanderPrivate;

static guint _signals[LAST_SIGNAL] = { 0, };

/*
 * NbtkStylable, needed to get parent type styling applied.
 */

static const gchar *
_stylable_get_style_type (NbtkStylable *stylable)
{
  return "NbtkExpander";
}

static void
_stylable_iface_init (NbtkStylableIface *iface)
{
  static gboolean _is_initialized = FALSE;

  if (!_is_initialized)
  {
    iface->get_style_type = _stylable_get_style_type;
    _is_initialized = TRUE;
  }
}

/*
 * MnbExpander
 */

static void
_expanded_notify_cb (MnbExpander  *self,
                     GParamSpec   *pspec,
                     gpointer      user_data)
{
  MnbExpanderPrivate *priv = GET_PRIVATE (self);

  priv->is_animating = nbtk_expander_get_expanded (NBTK_EXPANDER (self));
}

static void
_expand_complete_cb (MnbExpander *self,
                     gpointer     user_data)
{
  MnbExpanderPrivate *priv = GET_PRIVATE (self);

  priv->is_animating = FALSE;
}

static void
_allocate (ClutterActor           *actor,
           const ClutterActorBox  *box,
           ClutterAllocationFlags  flags)
{
  MnbExpanderPrivate *priv = GET_PRIVATE (actor);

  CLUTTER_ACTOR_CLASS (mnb_expander_parent_class)->allocate (actor, box, flags);

  if (priv->is_animating)
    g_signal_emit (actor, _signals[FRAME_ALLOCATED], 0, box);
}

static void
mnb_expander_class_init (MnbExpanderClass *klass)
{
  /* GObjectClass *object_class = G_OBJECT_CLASS (klass); */
  ClutterActorClass *actor_class = CLUTTER_ACTOR_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MnbExpanderPrivate));

  actor_class->allocate = _allocate;

  _signals[FRAME_ALLOCATED] = g_signal_new ("frame-allocated",
                                           G_TYPE_FROM_CLASS (klass),
                                           G_SIGNAL_RUN_LAST,
                                           0, NULL, NULL,
                                           g_cclosure_marshal_VOID__POINTER,
                                           G_TYPE_NONE, 1, G_TYPE_POINTER);
}

static void
mnb_expander_init (MnbExpander *self)
{
  g_signal_connect (self, "notify::expanded",
                    G_CALLBACK (_expanded_notify_cb), NULL);

  g_signal_connect (self, "expand-complete",
                    G_CALLBACK (_expand_complete_cb), NULL);
}

MnbExpander *
mnb_expander_new (void)
{
  return g_object_new (MNB_TYPE_EXPANDER, NULL);
}

