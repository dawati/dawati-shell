/*
 * Copyright (c) 2009, 2010 Intel Corp.
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

#include "mnb-zones-preview.h"
#include "mnb-fancy-bin.h"

#include <stdlib.h>

static void mx_stylable_iface_init (MxStylableIface *iface);

G_DEFINE_TYPE_WITH_CODE (MnbZonesPreview, mnb_zones_preview, MX_TYPE_WIDGET,
                         G_IMPLEMENT_INTERFACE (MX_TYPE_STYLABLE,
                                                mx_stylable_iface_init))

#define ZONES_PREVIEW_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MNB_TYPE_ZONES_PREVIEW, MnbZonesPreviewPrivate))

enum
{
  PROP_0,

  PROP_ZOOM,
  PROP_WORKSPACE,
  PROP_WORKSPACE_WIDTH,
  PROP_WORKSPACE_HEIGHT,
  PROP_WORKSPACE_BG
};

enum
{
  SWITCH_COMPLETED,
  LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0, };

typedef enum
{
  MNB_ZP_STATIC,
  MNB_ZP_ZOOM_OUT,
  MNB_ZP_PAN,
  MNB_ZP_ZOOM_IN
} MnbZonesPreviewPhase;

struct _MnbZonesPreviewPrivate
{
  GList                *workspace_bins;
  ClutterActor         *workspace_bg;
  guint                 spacing;
  gdouble               zoom;
  gdouble               workspace;
  gint                  dest_workspace;
  guint                 width;
  guint                 height;
  MnbZonesPreviewPhase  anim_phase;
};

static void
mnb_zones_preview_get_property (GObject    *object,
                                guint       property_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
  MnbZonesPreviewPrivate *priv = MNB_ZONES_PREVIEW (object)->priv;

  switch (property_id)
    {
    case PROP_ZOOM:
      g_value_set_double (value, priv->zoom);
      break;

    case PROP_WORKSPACE:
      g_value_set_double (value, priv->workspace);
      break;

    case PROP_WORKSPACE_WIDTH:
      g_value_set_uint (value, priv->width);
      break;

    case PROP_WORKSPACE_HEIGHT:
      g_value_set_uint (value, priv->height);
      break;

    case PROP_WORKSPACE_BG:
      g_value_set_object (value, priv->workspace_bg);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
mnb_zones_preview_set_property (GObject      *object,
                                guint         property_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
  MnbZonesPreviewPrivate *priv = MNB_ZONES_PREVIEW (object)->priv;

  switch (property_id)
    {
    case PROP_ZOOM:
      priv->zoom = g_value_get_double (value);
      break;

    case PROP_WORKSPACE:
      priv->workspace = g_value_get_double (value);
      break;

    case PROP_WORKSPACE_WIDTH:
      priv->width = g_value_get_uint (value);
      break;

    case PROP_WORKSPACE_HEIGHT:
      priv->height = g_value_get_uint (value);
      break;

    case PROP_WORKSPACE_BG:
      if (priv->workspace_bg)
        g_object_unref (priv->workspace_bg);
      priv->workspace_bg = g_value_dup_object (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      return;
    }

  clutter_actor_queue_relayout (CLUTTER_ACTOR (object));
}

static void
mnb_zones_preview_dispose (GObject *object)
{
  MnbZonesPreview *self = MNB_ZONES_PREVIEW (object);
  MnbZonesPreviewPrivate *priv = self->priv;

  mnb_zones_preview_clear (self);

  if (priv->workspace_bg)
    {
      g_object_unref (priv->workspace_bg);
      priv->workspace_bg = NULL;
    }

  G_OBJECT_CLASS (mnb_zones_preview_parent_class)->dispose (object);
}

static void
mnb_zones_preview_finalize (GObject *object)
{
  G_OBJECT_CLASS (mnb_zones_preview_parent_class)->finalize (object);
}

static void
mnb_zones_preview_paint (ClutterActor *actor)
{
  GList *w;
  MnbZonesPreviewPrivate *priv = MNB_ZONES_PREVIEW (actor)->priv;

  /* Chain up for background */
  CLUTTER_ACTOR_CLASS (mnb_zones_preview_parent_class)->paint (actor);

  /* Paint bins */
  for (w = priv->workspace_bins; w; w = w->next)
    clutter_actor_paint (CLUTTER_ACTOR (w->data));
}

static void
mnb_zones_preview_map (ClutterActor *actor)
{
  GList *w;
  MnbZonesPreviewPrivate *priv = MNB_ZONES_PREVIEW (actor)->priv;

  CLUTTER_ACTOR_CLASS (mnb_zones_preview_parent_class)->map (actor);

  for (w = priv->workspace_bins; w; w = w->next)
    clutter_actor_map (CLUTTER_ACTOR (w->data));
}

static void
mnb_zones_preview_unmap (ClutterActor *actor)
{
  GList *w;
  MnbZonesPreviewPrivate *priv = MNB_ZONES_PREVIEW (actor)->priv;

  CLUTTER_ACTOR_CLASS (mnb_zones_preview_parent_class)->unmap (actor);

  for (w = priv->workspace_bins; w; w = w->next)
    clutter_actor_unmap (CLUTTER_ACTOR (w->data));
}

static void
mnb_zones_preview_get_preferred_width (ClutterActor *actor,
                                       gfloat        for_height,
                                       gfloat       *min_width_p,
                                       gfloat       *nat_width_p)
{
  MnbZonesPreviewPrivate *priv = MNB_ZONES_PREVIEW (actor)->priv;

  if (min_width_p)
    *min_width_p = priv->width;
  if (nat_width_p)
    *nat_width_p = priv->width;
}

static void
mnb_zones_preview_get_preferred_height (ClutterActor *actor,
                                        gfloat        for_width,
                                        gfloat       *min_height_p,
                                        gfloat       *nat_height_p)
{
  MnbZonesPreviewPrivate *priv = MNB_ZONES_PREVIEW (actor)->priv;

  if (min_height_p)
    *min_height_p = priv->height;
  if (nat_height_p)
    *nat_height_p = priv->height;
}

static void
mnb_zones_preview_allocate (ClutterActor           *actor,
                            const ClutterActorBox  *box,
                            ClutterAllocationFlags  flags)
{
  gint n;
  GList *w;
  gfloat origin, bin_width;

  MnbZonesPreviewPrivate *priv = MNB_ZONES_PREVIEW (actor)->priv;

  CLUTTER_ACTOR_CLASS (mnb_zones_preview_parent_class)->
    allocate (actor, box, flags);

  /* Figure out the origin */
  bin_width = priv->width + priv->spacing;
  origin = - (priv->workspace * bin_width * priv->zoom);

  /* Make sure we zoom out from the centre */
  origin += (bin_width - (bin_width * priv->zoom)) / 2.0;

  for (n = 0, w = priv->workspace_bins; w; w = w->next, n++)
    {
      ClutterActorBox child_box;
      gfloat width, height;
      MxPadding padding;

      ClutterActor *bin = CLUTTER_ACTOR (w->data);

      clutter_actor_get_preferred_size (bin, NULL, NULL, &width, &height);
      width *= priv->zoom;
      height *= priv->zoom;

      mx_widget_get_padding (MX_WIDGET (bin), &padding);

      /* We allocate the preferred size, but we make sure the padding is
       * 'outside' the target area.
       */
      child_box.x1 = origin - padding.left;
      child_box.x2 = child_box.x1 + width;
      child_box.y1 = (((box->y2 - box->y1) -
                       (height - padding.top - padding.bottom)) / 2.f) -
                     padding.top;
      child_box.y2 = child_box.y1 + height;

      clutter_actor_allocate (bin, &child_box, flags);

      origin = (child_box.x2 - padding.right) + (priv->spacing * priv->zoom);
    }
}

static void
mnb_zones_preview_class_init (MnbZonesPreviewClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  ClutterActorClass *actor_class = CLUTTER_ACTOR_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MnbZonesPreviewPrivate));

  object_class->get_property = mnb_zones_preview_get_property;
  object_class->set_property = mnb_zones_preview_set_property;
  object_class->dispose = mnb_zones_preview_dispose;
  object_class->finalize = mnb_zones_preview_finalize;

  actor_class->paint = mnb_zones_preview_paint;
  actor_class->map = mnb_zones_preview_map;
  actor_class->unmap = mnb_zones_preview_unmap;
  actor_class->get_preferred_width = mnb_zones_preview_get_preferred_width;
  actor_class->get_preferred_height = mnb_zones_preview_get_preferred_height;
  actor_class->allocate = mnb_zones_preview_allocate;

  g_object_class_install_property (object_class,
                                   PROP_ZOOM,
                                   g_param_spec_double ("zoom",
                                                        "Zoom",
                                                        "Simulated zoom level.",
                                                        0.0, G_MAXDOUBLE, 1.0,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_STATIC_NAME |
                                                        G_PARAM_STATIC_NICK |
                                                        G_PARAM_STATIC_BLURB));

  g_object_class_install_property (object_class,
                                   PROP_WORKSPACE,
                                   g_param_spec_double ("workspace",
                                                        "Workspace",
                                                        "Current workspace.",
                                                        0.0, G_MAXDOUBLE, 0.0,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_STATIC_NAME |
                                                        G_PARAM_STATIC_NICK |
                                                        G_PARAM_STATIC_BLURB));

  g_object_class_install_property (object_class,
                                   PROP_WORKSPACE_WIDTH,
                                   g_param_spec_uint ("workspace-width",
                                                      "Workspace width",
                                                      "Width of a workspace.",
                                                      0, G_MAXUINT, 0,
                                                      G_PARAM_READWRITE |
                                                      G_PARAM_STATIC_NAME |
                                                      G_PARAM_STATIC_NICK |
                                                      G_PARAM_STATIC_BLURB));

  g_object_class_install_property (object_class,
                                   PROP_WORKSPACE_HEIGHT,
                                   g_param_spec_uint ("workspace-height",
                                                      "Workspace height",
                                                      "Height of a workspace.",
                                                      0, G_MAXUINT, 0,
                                                      G_PARAM_READWRITE |
                                                      G_PARAM_STATIC_NAME |
                                                      G_PARAM_STATIC_NICK |
                                                      G_PARAM_STATIC_BLURB));

  g_object_class_install_property (object_class,
                                   PROP_WORKSPACE_BG,
                                   g_param_spec_object ("workspace-bg",
                                                        "Workspace background",
                                                        "Workspace background.",
                                                        CLUTTER_TYPE_ACTOR,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_STATIC_NAME |
                                                        G_PARAM_STATIC_NICK |
                                                        G_PARAM_STATIC_BLURB));

  signals[SWITCH_COMPLETED] =
    g_signal_new ("switch-completed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (MnbZonesPreviewClass, switch_completed),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);
}

static void
mx_stylable_iface_init (MxStylableIface *iface)
{
  static gboolean is_initialised = FALSE;

  if (!is_initialised)
    {
      GParamSpec *pspec;

      is_initialised = TRUE;

      pspec = g_param_spec_uint ("spacing",
                                 "Spacing",
                                 "Spacing between workspaces, in px.",
                                 0, G_MAXUINT, 24,
                                 G_PARAM_READWRITE);
      mx_stylable_iface_install_property (iface, MNB_TYPE_ZONES_PREVIEW, pspec);
    }
}

static void
mnb_zones_preview_style_changed_cb (MxStylable          *stylable,
                                    MxStyleChangedFlags  flags,
                                    MnbZonesPreview     *self)
{
  MnbZonesPreviewPrivate *priv = self->priv;

  mx_stylable_get (stylable,
                   "spacing", &priv->spacing,
                     NULL);
}

static void
mnb_zones_preview_init (MnbZonesPreview *self)
{
  MnbZonesPreviewPrivate *priv = self->priv = ZONES_PREVIEW_PRIVATE (self);

  priv->zoom = 1.0;
  priv->spacing = 24;
  priv->dest_workspace = -1;

  g_signal_connect (self, "style-changed",
                    G_CALLBACK (mnb_zones_preview_style_changed_cb), self);
}

ClutterActor *
mnb_zones_preview_new ()
{
  return g_object_new (MNB_TYPE_ZONES_PREVIEW, NULL);
}

static void
mnb_zones_preview_enable_fanciness (MnbZonesPreview *preview,
                                    gboolean         enable)
{
  GList *b;
  MnbZonesPreviewPrivate *priv = preview->priv;

  for (b = priv->workspace_bins; b; b = b->next)
    mnb_fancy_bin_set_fancy (MNB_FANCY_BIN (b->data), enable);
}

static void
mnb_zones_preview_completed_cb (ClutterAnimation *animation,
                                MnbZonesPreview  *preview)
{
  MnbZonesPreviewPrivate *priv = preview->priv;

  switch (priv->anim_phase)
    {
    case MNB_ZP_STATIC:
      /* Start zooming out */
      priv->anim_phase = MNB_ZP_ZOOM_OUT;
      mnb_zones_preview_enable_fanciness (preview, TRUE);
      clutter_actor_animate (CLUTTER_ACTOR (preview),
                             CLUTTER_EASE_IN_SINE,
                             220,
                             "zoom", 0.3,
                             NULL);
      break;

    case MNB_ZP_ZOOM_OUT:
      /* Start panning */
      {
        guint duration = 175 * abs (priv->dest_workspace - priv->workspace);

        if (duration)
          {
            priv->anim_phase = MNB_ZP_PAN;
            clutter_actor_animate (CLUTTER_ACTOR (preview),
                                   CLUTTER_LINEAR,
                                   duration,
                                   "workspace", (gdouble)priv->dest_workspace,
                                   NULL);
            break;
          }
        /* If duration == 0, fall through here to the next phase*/
      }
    case MNB_ZP_PAN:
      /* Start zooming in */
      mnb_zones_preview_enable_fanciness (preview, FALSE);
      priv->anim_phase = MNB_ZP_ZOOM_IN;
      clutter_actor_animate (CLUTTER_ACTOR (preview),
                             CLUTTER_EASE_OUT_CUBIC,
                             250,
                             "zoom", 1.0,
                             NULL);
      break;

    case MNB_ZP_ZOOM_IN:
      /* Complete the animation */
      priv->anim_phase = MNB_ZP_STATIC;
      g_signal_emit (preview, signals[SWITCH_COMPLETED], 0);
      return;

    default:
      g_warning (G_STRLOC ": This shouldn't happen");
      return;
    }

  animation = clutter_actor_get_animation (CLUTTER_ACTOR (preview));
  g_signal_connect_after (animation, "completed",
                          G_CALLBACK (mnb_zones_preview_completed_cb), preview);
}

void
mnb_zones_preview_change_workspace (MnbZonesPreview *preview,
                                    gint             workspace)
{
  gboolean reset_anim;
  MnbZonesPreviewPrivate *priv = preview->priv;

  /* If we're already going towards this workspace, ignore */
  if ((priv->dest_workspace == workspace) && priv->anim_phase)
    return;

  /* Figure out what we need to be doing with this animation */
  switch (priv->anim_phase)
    {
    default:
    case MNB_ZP_STATIC:
      /* We weren't animating, start a new one */
      reset_anim = TRUE;
      break;

    case MNB_ZP_ZOOM_OUT:
      /* If we're on the right workspace, zoom in and finish, otherwise
       * continue the animation like normal.
       */
      if (priv->dest_workspace == workspace)
        {
          priv->anim_phase = MNB_ZP_PAN;
          reset_anim = TRUE;
        }
      else
        reset_anim = FALSE;
      break;

    case MNB_ZP_PAN:
      /* If we're heading towards the right workspace, continue the
       * animation, otherwise change direction.
       */
      if (priv->dest_workspace != workspace)
        {
          priv->anim_phase = MNB_ZP_ZOOM_OUT;
          reset_anim = TRUE;
        }
      else
        reset_anim = FALSE;
      break;

    case MNB_ZP_ZOOM_IN:
      /* Restart the animation if we're not heading towards the right
       * workspace.
       */
      if (priv->dest_workspace != workspace)
        {
          priv->anim_phase = MNB_ZP_STATIC;
          reset_anim = TRUE;
        }
      else
        reset_anim = FALSE;
      break;
    }

  priv->dest_workspace = workspace;
  if (reset_anim)
    {
      ClutterAnimation *animation =
        clutter_actor_get_animation (CLUTTER_ACTOR (preview));

      if (animation)
        g_signal_handlers_disconnect_by_func (animation,
                                              mnb_zones_preview_completed_cb,
                                              preview);

      mnb_zones_preview_completed_cb (animation, preview);
    }
}

/* Gets the desired workspace and creates any workspaces
 * necessary before it.
 */
static ClutterActor *
mnb_zones_preview_get_workspace_group (MnbZonesPreview *preview,
                                       gint             workspace)
{
  gint i;
  GList *w;

  ClutterActor *bin = NULL;
  MnbZonesPreviewPrivate *priv = preview->priv;

  for (w = priv->workspace_bins, i = 0;
       (i < workspace) && w; w = w->next, i++);

  if ((i == workspace) && w)
    return mnb_fancy_bin_get_child (MNB_FANCY_BIN (w->data));

  for (; i <= workspace; i++)
    {
      ClutterActor *group;

      /* Create a workspace clone */
      bin = mnb_fancy_bin_new ();
      group = clutter_group_new ();

      /* Add background if it's set */
      if (priv->workspace_bg)
        {
          ClutterActor *bg = clutter_clone_new (priv->workspace_bg);
          clutter_actor_set_size (bg, priv->width, priv->height);
          clutter_container_add_actor (CLUTTER_CONTAINER (group), bg);
        }

      clutter_actor_set_clip (group, 0, 0, priv->width, priv->height);
      mnb_fancy_bin_set_child (MNB_FANCY_BIN (bin), group);

      clutter_actor_set_parent (bin, CLUTTER_ACTOR (preview));

      /* This is a bit of a hack, depending on the fact that GList
       * doesn't change the beginning of the list when appending
       * (unless the list is NULL).
       */
      priv->workspace_bins = g_list_append (priv->workspace_bins, bin);
    }

  return mnb_fancy_bin_get_child (MNB_FANCY_BIN (bin));
}

void
mnb_zones_preview_set_n_workspaces (MnbZonesPreview *preview,
                                    gint             workspace)
{
  gint current_length;

  MnbZonesPreviewPrivate *priv = preview->priv;

  current_length = g_list_length (priv->workspace_bins);
  if (current_length < workspace)
    mnb_zones_preview_get_workspace_group (preview, workspace - 1);
  else if (current_length > workspace)
    {
      gint i;
      for (i = 0; i < current_length - workspace; i++)
        {
          GList *link = g_list_last (priv->workspace_bins);
          clutter_actor_destroy (CLUTTER_ACTOR (link->data));
          priv->workspace_bins =
            g_list_delete_link (priv->workspace_bins, link);
        }
    }
}

static void mnb_zones_preview_clone_destroy_cb (ClutterActor *, ClutterActor *);

/*
 * If the window is destroyed, we have to destroy the clone.
 */
static void
mnb_zones_preview_mcw_destroy_cb (ClutterActor *mcw, ClutterActor *clone)
{
  /*
   * First disconnect the clone_destroy handler to avoid circularity.
   */
  g_signal_handlers_disconnect_by_func (clone,
                                        mnb_zones_preview_clone_destroy_cb,
                                        mcw);

  clutter_actor_destroy (clone);
}

/*
 * When the clone goes away, disconnect the mcw destroy handler.
 */
static void
mnb_zones_preview_clone_destroy_cb (ClutterActor *clone, ClutterActor *mcw)
{
  g_signal_handlers_disconnect_by_func (mcw,
                                        mnb_zones_preview_mcw_destroy_cb,
                                        clone);
}

void
mnb_zones_preview_add_window (MnbZonesPreview *preview,
                              MutterWindow    *window)
{
  ClutterActor *clone;
  ClutterActor *group;
  MetaRectangle rect;
  gint workspace;

  /* TODO: Determine if we need to add a weak reference on the window
   *       in case it gets destroyed during the animation.
   *       I'd have thought that the clone's reference on the texture
   *       would be enough that this wouldn't be necessary.
   *
   * We do; while the clone's reference is enough to keep the texture about,
   * it is not enough to make it possible to map the clone once the texture
   * has been unparented.
   */
  workspace = mutter_window_get_workspace (window);
  group = mnb_zones_preview_get_workspace_group (preview, workspace);

  clone = clutter_clone_new (mutter_window_get_texture (window));

  g_signal_connect (window, "destroy",
                    G_CALLBACK (mnb_zones_preview_mcw_destroy_cb),
                    clone);
  g_signal_connect (clone, "destroy",
                    G_CALLBACK (mnb_zones_preview_clone_destroy_cb),
                    window);

  meta_window_get_outer_rect (mutter_window_get_meta_window (window), &rect);
  clutter_actor_set_position (clone, rect.x, rect.y);

  clutter_container_add_actor (CLUTTER_CONTAINER (group), clone);
}

void
mnb_zones_preview_clear (MnbZonesPreview *preview)
{

  MnbZonesPreviewPrivate *priv = preview->priv;

  while (priv->workspace_bins)
    {
      clutter_actor_destroy (CLUTTER_ACTOR (priv->workspace_bins->data));
      priv->workspace_bins = g_list_delete_link (priv->workspace_bins,
                                                 priv->workspace_bins);
    }

  clutter_actor_queue_relayout (CLUTTER_ACTOR (preview));
}

