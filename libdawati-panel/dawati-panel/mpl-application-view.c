/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */

/*
 * Copyright (C) 2012 Intel Corporation.
 *
 * Author: Lionel Landwerlin <lionel.g.landwerlin@linux.intel.com>
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
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "mpl-application-view.h"

#include <string.h>

G_DEFINE_TYPE (MplApplicationView, mpl_application_view, MX_TYPE_WIDGET)

#define APPLICATION_VIEW_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MPL_TYPE_APPLICATION_VIEW, MplApplicationViewPrivate))

#define TILE_WIDTH  (266)
#define TILE_HEIGHT (212)

struct _MplApplicationViewPrivate
{
  ClutterActor *icon;

  ClutterActor *title_box;
  ClutterActor *title;
  ClutterActor *subtitle;

  ClutterActor *close_button;

  ClutterActor *app_frame;
  ClutterActor *shadow;
  ClutterActor *thumbnail;
};

enum
{
  PROP_0,
  PROP_ICON,
  PROP_TITLE,
  PROP_SUBTITLE,
  PROP_CAN_CLOSE,
  PROP_THUMBNAIL
};

enum
{
  ACTIVATED,
  CLOSED,

  LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0, };

static void
mpl_application_view_get_property (GObject    *object,
                                   guint       property_id,
                                   GValue     *value,
                                   GParamSpec *pspec)
{
  MplApplicationViewPrivate *priv = APPLICATION_VIEW_PRIVATE (object);

  switch (property_id)
    {
    case PROP_ICON:
      g_value_set_object (value, priv->icon);
      break;

    case PROP_TITLE:
      g_value_set_string (value, mx_label_get_text (MX_LABEL (priv->title)));
      break;

    case PROP_SUBTITLE:
      g_value_set_string (value, mx_label_get_text (MX_LABEL (priv->subtitle)));
      break;

    case PROP_CAN_CLOSE:
      g_value_set_boolean (value, CLUTTER_ACTOR_IS_VISIBLE (priv->close_button));
      break;

    case PROP_THUMBNAIL:
      g_value_set_object (value, priv->thumbnail);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
mpl_application_view_set_property (GObject      *object,
                                   guint         property_id,
                                   const GValue *value,
                                   GParamSpec   *pspec)
{
  MplApplicationView *self = (MplApplicationView *) object;

  switch (property_id)
    {
    case PROP_ICON:
      mpl_application_view_set_icon (self,
                                     (ClutterActor *) g_value_get_object (value));
      break;

    case PROP_TITLE:
      mpl_application_view_set_title (self,
                                      g_value_get_string (value));
      break;

    case PROP_SUBTITLE:
      mpl_application_view_set_subtitle (self,
                                         g_value_get_string (value));
      break;

    case PROP_CAN_CLOSE:
      mpl_application_view_set_can_close (self,
                                          g_value_get_boolean (value));
      break;

    case PROP_THUMBNAIL:
      mpl_application_view_set_thumbnail (self,
                                          (ClutterActor *) g_value_get_object (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
mpl_application_view_dispose (GObject *object)
{
  G_OBJECT_CLASS (mpl_application_view_parent_class)->dispose (object);
}

static void
mpl_application_view_finalize (GObject *object)
{
  G_OBJECT_CLASS (mpl_application_view_parent_class)->finalize (object);
}

static void
mpl_application_view_get_preferred_width (ClutterActor *actor,
                                          gfloat for_height,
                                          gfloat *min_width,
                                          gfloat *nat_width)
{
  if (min_width)
    *min_width = TILE_WIDTH;
  if (nat_width)
    *nat_width = TILE_WIDTH;
}

static void
mpl_application_view_get_preferred_height (ClutterActor *actor,
                                           gfloat for_width,
                                           gfloat *min_height,
                                           gfloat *nat_height)
{
  if (min_height)
    *min_height = TILE_HEIGHT;
  if (nat_height)
    *nat_height = TILE_HEIGHT;
}

static void
mpl_application_view_allocate (ClutterActor          *actor,
                               const ClutterActorBox *box,
                               ClutterAllocationFlags flags)
{
  MplApplicationViewPrivate *priv = ((MplApplicationView *) actor)->priv;
  MxPadding padding;
  ClutterActorBox child_box;
  gfloat icon_width = 0, icon_height = 0, button_width, button_height;

  CLUTTER_ACTOR_CLASS (mpl_application_view_parent_class)->allocate (actor,
                                                                     box,
                                                                     flags);

  mx_widget_get_padding (MX_WIDGET (actor), &padding);

  /* Icon */
  child_box.x1 = padding.left;
  child_box.y1 = padding.top;

  if (priv->icon)
    {
      clutter_actor_get_preferred_width (priv->icon, -1, NULL, &icon_width);
      clutter_actor_get_preferred_height (priv->icon, -1, NULL, &icon_height);

      child_box.x2 = child_box.x1 + icon_width;
      child_box.y2 = child_box.y1 + icon_height;

      clutter_actor_allocate (priv->icon, &child_box, flags);
    }

  /* Close button */
  clutter_actor_get_preferred_width (priv->close_button, -1, NULL, &button_width);
  clutter_actor_get_preferred_height (priv->close_button, -1, NULL, &button_height);

  child_box.x2 = box->x2 - box->x1 - padding.right;
  child_box.y2 = child_box.y1 + button_height;
  child_box.x1 = child_box.x2 - button_width;

  clutter_actor_allocate (priv->close_button, &child_box, flags);

  /* Titles */
  child_box.x1 = padding.left + icon_width;
  child_box.x2 = box->x2 - box->x1 - padding.right - button_width;
  child_box.y2 = padding.top + icon_height;

  clutter_actor_allocate (priv->title_box, &child_box, flags);

  /* App frame */
  child_box.x1 = padding.left;
  child_box.x2 = box->x2 - box->x1 - padding.right;
  child_box.y1 = padding.top + icon_width;
  child_box.y2 = box->y2 - box->y1 - padding.bottom;

  clutter_actor_allocate (priv->app_frame, &child_box, flags);
}

static void
mpl_application_view_paint (ClutterActor *actor)
{
  MplApplicationViewPrivate *priv = ((MplApplicationView *) actor)->priv;

  CLUTTER_ACTOR_CLASS (mpl_application_view_parent_class)->paint (actor);

  clutter_actor_paint (priv->icon);
  clutter_actor_paint (priv->title_box);
  clutter_actor_paint (priv->close_button);
  clutter_actor_paint (priv->app_frame);
}

static void
mpl_application_view_pick (ClutterActor *actor, const ClutterColor *color)
{
  MplApplicationViewPrivate *priv = ((MplApplicationView *) actor)->priv;

  CLUTTER_ACTOR_CLASS (mpl_application_view_parent_class)->pick (actor, color);

  clutter_actor_paint (priv->icon);
  clutter_actor_paint (priv->title_box);
  clutter_actor_paint (priv->close_button);
  clutter_actor_paint (priv->app_frame);
}

static void
mpl_application_view_class_init (MplApplicationViewClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  ClutterActorClass *actor_class = CLUTTER_ACTOR_CLASS (klass);
  GParamSpec *pspec;

  g_type_class_add_private (klass, sizeof (MplApplicationViewPrivate));

  object_class->get_property = mpl_application_view_get_property;
  object_class->set_property = mpl_application_view_set_property;
  object_class->dispose = mpl_application_view_dispose;
  object_class->finalize = mpl_application_view_finalize;

  actor_class->allocate = mpl_application_view_allocate;
  actor_class->paint = mpl_application_view_paint;
  actor_class->pick = mpl_application_view_pick;
  actor_class->get_preferred_width = mpl_application_view_get_preferred_width;
  actor_class->get_preferred_height = mpl_application_view_get_preferred_height;

  pspec = g_param_spec_object ("icon", "Icon",
                               "Application icon",
                               CLUTTER_TYPE_ACTOR,
                               G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_ICON, pspec);

  pspec = g_param_spec_string ("title", "Tile",
                               "Application title",
                               NULL,
                               G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_TITLE, pspec);

  pspec = g_param_spec_string ("subtitle", "Subtile",
                               "Application secondary title",
                               NULL,
                               G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_SUBTITLE, pspec);

  pspec = g_param_spec_boolean ("can-close",
                                "Can close",
                                "Whether the view can close the application",
                                TRUE,
                                G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_CAN_CLOSE, pspec);

  pspec = g_param_spec_object ("thumbnail", "Thumbnail",
                               "Application thumbnail",
                               CLUTTER_TYPE_ACTOR,
                               G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_THUMBNAIL, pspec);

  /**
   * MplApplicationView::activated:
   * @view: the object that received the signal
   *
   * Emitted when the user activates the application view with a mouse
   * release.
   */
  signals[ACTIVATED] =
    g_signal_new ("activated",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  0,
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  /**
   * MplApplicationView::closed:
   * @view: the object that received the signal
   *
   * Emitted when the user clicks the close button from the
   * application view.
   */
  signals[CLOSED] =
    g_signal_new ("closed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  0,
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);
}

static gboolean
activate_clicked (MplApplicationView *self,
                  ClutterEvent       *event,
                  gpointer            data)
{
  g_signal_emit (self, signals[ACTIVATED], 0);

  return FALSE;
}

static void
close_btn_clicked (ClutterActor       *button,
                   MplApplicationView *self)
{
  g_signal_emit (self, signals[CLOSED], 0);
}

static void
mpl_application_view_init (MplApplicationView *self)
{
  MplApplicationViewPrivate *priv;
  ClutterActor *actor = CLUTTER_ACTOR (self);

  priv = self->priv = APPLICATION_VIEW_PRIVATE (self);

  /* tile */
  clutter_actor_set_reactive (actor, TRUE);
  mx_stylable_set_style_class (MX_STYLABLE (actor), "switcherTile");
  clutter_actor_set_size (actor, TILE_WIDTH, TILE_HEIGHT);
  g_signal_connect (self, "button-release-event",
                    G_CALLBACK (activate_clicked), NULL);

  priv->title_box = mx_box_layout_new_with_orientation (MX_ORIENTATION_VERTICAL);
  clutter_actor_set_parent (priv->title_box, actor);

  /* title */
  priv->title = mx_label_new ();
  mx_label_set_y_align (MX_LABEL (priv->title), MX_ALIGN_MIDDLE);
  mx_stylable_set_style_class (MX_STYLABLE (priv->title), "appTitle");
  mx_box_layout_add_actor (MX_BOX_LAYOUT (priv->title_box), priv->title, 0);
  mx_box_layout_child_set_expand (MX_BOX_LAYOUT (priv->title_box),
                                  priv->title, TRUE);

  /* subtitle */
  priv->subtitle = mx_label_new ();
  mx_label_set_y_align (MX_LABEL (priv->subtitle), MX_ALIGN_MIDDLE);
  mx_stylable_set_style_class (MX_STYLABLE (priv->subtitle), "appSubTitle");
  mx_box_layout_add_actor (MX_BOX_LAYOUT (priv->title_box), priv->subtitle, 1);
  mx_box_layout_child_set_expand (MX_BOX_LAYOUT (priv->title_box),
                                  priv->subtitle, FALSE);

  /* close button */
  priv->close_button = mx_button_new ();
  mx_stylable_set_style_class (MX_STYLABLE (priv->close_button), "appCloseButton");
  clutter_actor_set_parent (priv->close_button, actor);
  g_signal_connect (priv->close_button, "clicked",
                    G_CALLBACK (close_btn_clicked), self);

  /* frame */
  priv->app_frame = mx_frame_new ();
  clutter_actor_set_size (priv->app_frame, 250, 100);
  mx_stylable_set_style_class (MX_STYLABLE (priv->app_frame), "appBackground");
  clutter_actor_set_parent (priv->app_frame, actor);

  /* shadow */
  priv->shadow = mx_frame_new ();
  mx_stylable_set_style_class (MX_STYLABLE (priv->shadow), "appShadow");
  mx_bin_set_child (MX_BIN (priv->app_frame), priv->shadow);
  mx_bin_set_fill (MX_BIN (priv->app_frame), FALSE, FALSE);

  clutter_actor_show_all (actor);
}

ClutterActor *
mpl_application_view_new (void)
{
  return g_object_new (MPL_TYPE_APPLICATION_VIEW, NULL);
}

void
mpl_application_view_set_icon (MplApplicationView *view,
                               ClutterActor       *icon)
{
  MplApplicationViewPrivate *priv;

  g_return_if_fail (MPL_IS_APPLICATION_VIEW (view));
  g_return_if_fail (CLUTTER_IS_ACTOR (icon));

  priv = view->priv;

  if (priv->icon)
    {
      clutter_actor_destroy (priv->icon);
      priv->icon = NULL;
    }

  priv->icon = icon;
  clutter_actor_set_parent (priv->icon, CLUTTER_ACTOR (view));
}

void
mpl_application_view_set_title (MplApplicationView *view,
                                const gchar        *text)
{
  MplApplicationViewPrivate *priv;

  g_return_if_fail (MPL_IS_APPLICATION_VIEW (view));

  priv = view->priv;

  mx_label_set_text (MX_LABEL (priv->title), text);

  if (!strcmp (mx_label_get_text (MX_LABEL (priv->title)),
               mx_label_get_text (MX_LABEL (priv->subtitle))))
    clutter_actor_hide (priv->subtitle);
  else
    clutter_actor_show (priv->subtitle);
}

void
mpl_application_view_set_subtitle (MplApplicationView *view,
                                   const gchar        *text)
{
  MplApplicationViewPrivate *priv;

  g_return_if_fail (MPL_IS_APPLICATION_VIEW (view));

  priv = view->priv;

  mx_label_set_text (MX_LABEL (priv->subtitle), text);

  if (!strcmp (mx_label_get_text (MX_LABEL (priv->title)),
               mx_label_get_text (MX_LABEL (priv->subtitle))))
    clutter_actor_hide (priv->subtitle);
  else
    clutter_actor_show (priv->subtitle);
}

void
mpl_application_view_set_can_close (MplApplicationView *view,
                                    gboolean            can_close)
{
  MplApplicationViewPrivate *priv;

  g_return_if_fail (MPL_IS_APPLICATION_VIEW (view));

  priv = view->priv;

  if (can_close)
    clutter_actor_show (priv->close_button);
  else
    clutter_actor_hide (priv->close_button);
}

void
mpl_application_view_set_thumbnail (MplApplicationView *view,
                                    ClutterActor       *thumbnail)
{
  MplApplicationViewPrivate *priv;

  g_return_if_fail (MPL_IS_APPLICATION_VIEW (view));

  priv = view->priv;

  if (priv->thumbnail)
    {
      clutter_actor_destroy (priv->thumbnail);
      priv->thumbnail = NULL;
      clutter_actor_hide (priv->shadow);
    }

  if (thumbnail)
    {
      priv->thumbnail = thumbnail;
      mx_bin_set_child (MX_BIN (priv->shadow), thumbnail);
      clutter_actor_show_all (priv->shadow);
    }
}

gfloat
mpl_application_get_tile_width (void)
{
  return TILE_WIDTH;
}
