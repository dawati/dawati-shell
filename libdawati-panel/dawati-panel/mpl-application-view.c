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

G_DEFINE_TYPE (MplApplicationView, mpl_application_view, MX_TYPE_TABLE)

#define APPLICATION_VIEW_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MPL_TYPE_APPLICATION_VIEW, MplApplicationViewPrivate))

#define TILE_WIDTH  (266)
#define TILE_HEIGHT (212)

struct _MplApplicationViewPrivate
{
  ClutterActor *icon;
  ClutterActor *title;
  ClutterActor *subtitle;
  ClutterActor *close_button;
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
  ClutterActor *frame;

  priv = self->priv = APPLICATION_VIEW_PRIVATE (self);

  /* tile */
  clutter_actor_set_reactive (actor, TRUE);
  mx_stylable_set_style_class (MX_STYLABLE (actor), "switcherTile");
  clutter_actor_set_size (actor, TILE_WIDTH, TILE_HEIGHT);
  g_signal_connect (self, "button-release-event",
                    G_CALLBACK (activate_clicked), NULL);

  /* title */
  priv->title = mx_label_new ();
  mx_stylable_set_style_class (MX_STYLABLE (priv->title), "appTitle");
  mx_table_add_actor (MX_TABLE (actor), priv->title, 0, 1);
  mx_table_child_set_y_expand (MX_TABLE (actor), priv->title, FALSE);

  /* subtitle */
  priv->subtitle = mx_label_new ();
  mx_stylable_set_style_class (MX_STYLABLE (priv->subtitle), "appSubTitle");
  mx_table_add_actor (MX_TABLE (actor), priv->subtitle, 1, 1);
  mx_table_child_set_y_expand (MX_TABLE (actor), priv->subtitle, FALSE);

  /* close button */
  priv->close_button = mx_button_new ();
  mx_stylable_set_style_class (MX_STYLABLE (priv->close_button), "appCloseButton");
  mx_table_add_actor (MX_TABLE (actor), priv->close_button, 0, 2);
  mx_table_child_set_y_fill (MX_TABLE (actor), priv->close_button, FALSE);
  mx_table_child_set_x_fill (MX_TABLE (actor), priv->close_button, FALSE);
  mx_table_child_set_y_expand (MX_TABLE (actor), priv->close_button, FALSE);
  mx_table_child_set_x_expand (MX_TABLE (actor), priv->close_button, FALSE);
  g_signal_connect (priv->close_button, "clicked",
                    G_CALLBACK (close_btn_clicked), self);

  /* frame */
  frame = mx_frame_new ();
  mx_stylable_set_style_class (MX_STYLABLE (frame), "appBackground");
  mx_table_add_actor (MX_TABLE (actor), frame, 2, 0);
  mx_table_child_set_column_span (MX_TABLE (actor), frame, 3);
  mx_table_child_set_x_expand (MX_TABLE (actor), frame, FALSE);

  /* shadow */
  priv->shadow = mx_frame_new ();
  mx_stylable_set_style_class (MX_STYLABLE (priv->shadow), "appShadow");
  mx_bin_set_child (MX_BIN (frame), priv->shadow);
  mx_bin_set_fill (MX_BIN (frame), FALSE, FALSE);
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
  MxTable *table;

  g_return_if_fail (MPL_IS_APPLICATION_VIEW (view));
  g_return_if_fail (CLUTTER_IS_ACTOR (icon));

  table = MX_TABLE (view);
  priv = view->priv;

  if (priv->icon)
    {
      clutter_actor_destroy (priv->icon);
      priv->icon = NULL;
    }

  priv->icon = icon;
  mx_table_add_actor (MX_TABLE (table), icon, 0, 0);
  mx_table_child_set_y_expand (MX_TABLE (table), icon, FALSE);
  mx_table_child_set_x_expand (MX_TABLE (table), icon, FALSE);
  mx_table_child_set_y_fill (MX_TABLE (table), icon, FALSE);
  mx_table_child_set_row_span (MX_TABLE (table), icon, 2);
  clutter_actor_set_size (icon, 32, 32);
}

void
mpl_application_view_set_title (MplApplicationView *view,
                                const gchar        *text)
{
  MplApplicationViewPrivate *priv;

  g_return_if_fail (MPL_IS_APPLICATION_VIEW (view));

  priv = view->priv;

  mx_label_set_text (MX_LABEL (priv->title), text);
}

void
mpl_application_view_set_subtitle (MplApplicationView *view,
                                   const gchar        *text)
{
  MplApplicationViewPrivate *priv;

  g_return_if_fail (MPL_IS_APPLICATION_VIEW (view));

  priv = view->priv;

  mx_label_set_text (MX_LABEL (priv->subtitle), text);
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
