/*
 * Copyright (C) 2008 Intel Corporation
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * Written by: Robert Staudinger <robsta@o-hand.com>.
 * Based on nbtk-label.c by Thomas Wood <thomas@linux.intel.com>.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <clutter/clutter.h>
#include <nbtk/nbtk.h>
#include "mnb-launcher-button.h"

#define MNB_LAUNCHER_BUTTON_GET_PRIVATE(obj)    \
        (G_TYPE_INSTANCE_GET_PRIVATE ((obj), MNB_TYPE_LAUNCHER_BUTTON, MnbLauncherButtonPrivate))

/* Distance between icon and text. */
#define COL_SPACING 10

enum
{
  ACTIVATED,

  LAST_SIGNAL
};

struct _MnbLauncherButtonPrivate
{
  ClutterActor  *icon;
  NbtkLabel     *title;
  NbtkLabel     *description;
  NbtkLabel     *comment;

  guint is_pressed : 1;
};

static guint _signals[LAST_SIGNAL] = { 0, };

G_DEFINE_TYPE (MnbLauncherButton, mnb_launcher_button, NBTK_TYPE_WIDGET)

static void
dispose (GObject *object)
{
  MnbLauncherButton *self = MNB_LAUNCHER_BUTTON (object);

  if (self->priv->icon)
    {
      clutter_actor_unparent (CLUTTER_ACTOR (self->priv->icon));
      self->priv->icon = NULL;
    }

  if (self->priv->title)
    {
      clutter_actor_unparent (CLUTTER_ACTOR (self->priv->title));
      self->priv->title = NULL;
    }

  if (self->priv->description)
    {
      clutter_actor_unparent (CLUTTER_ACTOR (self->priv->description));
      self->priv->description = NULL;
    }

  if (self->priv->comment)
    {
      clutter_actor_unparent (CLUTTER_ACTOR (self->priv->comment));
      self->priv->comment = NULL;
    }

  G_OBJECT_CLASS (mnb_launcher_button_parent_class)->dispose (object);
}

static void
get_preferred_width (ClutterActor *actor,
                     ClutterUnit   for_height,
                     ClutterUnit  *min_width,
                     ClutterUnit  *natural_width)
{
  MnbLauncherButton *self = MNB_LAUNCHER_BUTTON (actor);
  ClutterUnit natural_text_width;

  *min_width = 0;
  *natural_width = 0;

  if (self->priv->icon)
    {
      *min_width += clutter_actor_get_widthu (self->priv->icon);
      *natural_width += clutter_actor_get_widthu (self->priv->icon);
    }

  natural_text_width = 0;
  natural_text_width = MAX (natural_text_width,
                            clutter_actor_get_widthu (CLUTTER_ACTOR (self->priv->title)));
  natural_text_width = MAX (natural_text_width,
                            clutter_actor_get_widthu (CLUTTER_ACTOR (self->priv->description)));
  natural_text_width = MAX (natural_text_width,
                            clutter_actor_get_widthu (CLUTTER_ACTOR (self->priv->description)));

  *natural_width += COL_SPACING + natural_text_width;
}

static void
get_preferred_height (ClutterActor *actor,
                      ClutterUnit   for_width,
                      ClutterUnit  *min_height,
                      ClutterUnit  *natural_height)
{
  MnbLauncherButton *self = MNB_LAUNCHER_BUTTON (actor);
  ClutterUnit natural_text_height;

  *min_height = 0;
  *natural_height = 0;

  if (self->priv->icon)
    {
      *min_height += clutter_actor_get_heightu (self->priv->icon);
      *natural_height += clutter_actor_get_heightu (self->priv->icon);
    }

  natural_text_height = 0;
  natural_text_height += clutter_actor_get_heightu (CLUTTER_ACTOR (self->priv->title));
  natural_text_height += clutter_actor_get_heightu (CLUTTER_ACTOR (self->priv->description));
  natural_text_height += clutter_actor_get_heightu (CLUTTER_ACTOR (self->priv->comment));

  *natural_height += natural_text_height;
}

static void
allocate (ClutterActor          *actor,
          const ClutterActorBox *box,
          gboolean               origin_changed)
{
  MnbLauncherButton *self = MNB_LAUNCHER_BUTTON (actor);
  ClutterActorBox child_box;
  NbtkPadding *padding;
  gint border_top, border_right, border_bottom, border_left;
  ClutterUnit client_width, client_height, text_x, app_bottom, comment_top;

  CLUTTER_ACTOR_CLASS (mnb_launcher_button_parent_class)->allocate (actor, box, origin_changed);

  nbtk_stylable_get (NBTK_STYLABLE (self),
                    "border-top-width", &border_top,
                    "border-bottom-width", &border_bottom,
                    "border-right-width", &border_right,
                    "border-left-width", &border_left,
                    "padding", &padding,
                    NULL);

  child_box.x1 = CLUTTER_UNITS_FROM_INT (border_left) + padding->left;
  child_box.y1 = CLUTTER_UNITS_FROM_INT (border_top) + padding->top;
  child_box.x2 = box->x2 - box->x1 - child_box.x1 - CLUTTER_UNITS_FROM_INT (border_right) - padding->right;
  child_box.y2 = box->y2 - box->y1 - child_box.y1 - CLUTTER_UNITS_FROM_INT (border_bottom) - padding->bottom;

#define CLAMP_INSIDE(box_, bounding_box_)                 \
          box_.x1 = MIN ((box_).x1, (bounding_box_).x2);  \
          box_.y1 = MIN ((box_).y1, (bounding_box_).y2);  \
          box_.x2 = MAX ((box_).x1, (box_).x2);           \
          box_.y2 = MAX ((box_).y1, (box_).y2);

  /* Clamp for sanity, so values stay inside of the allocation box. */
  CLAMP_INSIDE (child_box, *box);

  client_width = child_box.x2 - child_box.x1;
  client_height = child_box.y2 - child_box.y1;
  /* Left-align text. */
  text_x = child_box.x1;
  app_bottom = client_height;
  comment_top = 0;

  /* Icon goes vertically centered in the left column. */
  if (self->priv->icon)
    {
      ClutterActorBox icon_box;

      icon_box.x1 = child_box.x1;
      icon_box.y1 = child_box.y1 +
                    (client_height - clutter_actor_get_heightu (self->priv->icon)) / 2;
      icon_box.x2 = icon_box.x1 + clutter_actor_get_widthu (self->priv->icon);
      icon_box.y2 = icon_box.y1 + clutter_actor_get_heightu (self->priv->icon);

      CLAMP_INSIDE (icon_box, child_box);
      clutter_actor_allocate (self->priv->icon, &icon_box, origin_changed);

      /* Have icon, text to the right of it. */
      text_x = icon_box.x2 + COL_SPACING;
    }

  /* Category label goes vertically centered in the right column. */
  if (self->priv->description)
    {
      ClutterActorBox category_box;
      ClutterUnit category_width, category_height;

      clutter_actor_get_preferred_size (CLUTTER_ACTOR (self->priv->description),
                                        NULL, NULL,
                                        &category_width, &category_height);
      category_box.x1 = text_x;
      category_box.y1 = child_box.y1 +
                        (client_height - clutter_actor_get_heightu (CLUTTER_ACTOR (self->priv->description))) / 2;
      category_box.x2 = MIN (category_box.x1 + category_width, child_box.x2);
      category_box.y2 = category_box.y1 +
                        clutter_actor_get_heightu (CLUTTER_ACTOR (self->priv->description));

      CLAMP_INSIDE (category_box, child_box);
      clutter_actor_allocate (CLUTTER_ACTOR (self->priv->description),
                              &category_box, origin_changed);

      /* "app name" and "commment" go to top and bottom of this. */
      app_bottom = category_box.y1;
      comment_top = category_box.y2;
    }

  /* App label goes on top of the right column. */
  if (self->priv->title)
    {
      ClutterActorBox app_box;
      ClutterUnit app_width, app_height;

      clutter_actor_get_preferred_size (CLUTTER_ACTOR (self->priv->title),
                                        NULL, NULL,
                                        &app_width, &app_height);
      app_box.x1 = text_x;
      app_box.y1 = app_bottom - app_height;
      app_box.x2 = MIN (app_box.x1 + app_width, child_box.x2);
      app_box.y2 = app_box.y1 + app_height;

      CLAMP_INSIDE (app_box, child_box);
      clutter_actor_allocate (CLUTTER_ACTOR (self->priv->title),
                              &app_box, origin_changed);
    }


  /* Comment label to bottom of the right column. */
  if (self->priv->comment)
    {
      ClutterActorBox comment_box;
      ClutterUnit comment_width, comment_height;

      clutter_actor_get_preferred_size (CLUTTER_ACTOR (self->priv->comment),
                                        NULL, NULL,
                                        &comment_width, &comment_height);
      comment_box.x1 = text_x;
      comment_box.y1 = comment_top;
      comment_box.x2 = MIN (comment_box.x1 + comment_width, child_box.x2);
      comment_box.y2 = comment_top + comment_height;

      CLAMP_INSIDE (comment_box, child_box);
      clutter_actor_allocate (CLUTTER_ACTOR (self->priv->comment),
                              &comment_box, origin_changed);
    }

#undef CLAMP_INSIDE
}

static gboolean
button_press_event (ClutterActor       *actor,
                    ClutterButtonEvent *event)
{
  if (event->button == 1)
    {
      MnbLauncherButton *self = MNB_LAUNCHER_BUTTON (actor);

      self->priv->is_pressed = TRUE;
      clutter_grab_pointer (actor);
      return TRUE;
    }

  return FALSE;
}

static gboolean
button_release_event (ClutterActor       *actor,
                      ClutterButtonEvent *event)
{
  if (event->button == 1)
    {
      MnbLauncherButton *self = MNB_LAUNCHER_BUTTON (actor);

      if (!self->priv->is_pressed)
        return FALSE;

      clutter_ungrab_pointer ();
      self->priv->is_pressed = FALSE;
      g_signal_emit (self, _signals[ACTIVATED], 0, event);

      return TRUE;
    }

  return FALSE;
}

static gboolean
enter_event (ClutterActor         *actor,
             ClutterCrossingEvent *event)
{
  MnbLauncherButton *self = MNB_LAUNCHER_BUTTON (actor);

  nbtk_widget_set_style_pseudo_class (NBTK_WIDGET (self), "hover");

  return FALSE;
}

static gboolean
leave_event (ClutterActor         *actor,
             ClutterCrossingEvent *event)
{
  MnbLauncherButton *self = MNB_LAUNCHER_BUTTON (actor);

  nbtk_widget_set_style_pseudo_class (NBTK_WIDGET (self), NULL);

  if (self->priv->is_pressed)
    {
      clutter_ungrab_pointer ();
      self->priv->is_pressed = FALSE;
    }

  return FALSE;
}

static void
pick (ClutterActor       *actor,
      const ClutterColor *pick_color)
{
  MnbLauncherButton *self = MNB_LAUNCHER_BUTTON (actor);

  CLUTTER_ACTOR_CLASS (mnb_launcher_button_parent_class)->pick (actor, pick_color);

  if (self->priv->icon && CLUTTER_ACTOR_IS_VISIBLE (self->priv->icon))
    clutter_actor_paint (self->priv->icon);

  if (self->priv->title && CLUTTER_ACTOR_IS_VISIBLE (self->priv->title))
    clutter_actor_paint (CLUTTER_ACTOR (self->priv->title));

  if (self->priv->description && CLUTTER_ACTOR_IS_VISIBLE (self->priv->description))
    clutter_actor_paint (CLUTTER_ACTOR (self->priv->description));

  if (self->priv->comment && CLUTTER_ACTOR_IS_VISIBLE (self->priv->comment))
    clutter_actor_paint (CLUTTER_ACTOR (self->priv->comment));
}

static void
paint (ClutterActor *actor)
{
  MnbLauncherButton *self = MNB_LAUNCHER_BUTTON (actor);

  CLUTTER_ACTOR_CLASS (mnb_launcher_button_parent_class)->paint (actor);

  if (self->priv->icon && CLUTTER_ACTOR_IS_VISIBLE (self->priv->icon))
    clutter_actor_paint (self->priv->icon);

  if (self->priv->title && CLUTTER_ACTOR_IS_VISIBLE (self->priv->title))
    clutter_actor_paint (CLUTTER_ACTOR (self->priv->title));

  if (self->priv->description && CLUTTER_ACTOR_IS_VISIBLE (self->priv->description))
    clutter_actor_paint (CLUTTER_ACTOR (self->priv->description));

  if (self->priv->comment && CLUTTER_ACTOR_IS_VISIBLE (self->priv->comment))
    clutter_actor_paint (CLUTTER_ACTOR (self->priv->comment));
}

static void
mnb_launcher_button_class_init (MnbLauncherButtonClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  ClutterActorClass *actor_class = CLUTTER_ACTOR_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MnbLauncherButtonPrivate));

  object_class->dispose = dispose;

  actor_class->get_preferred_height = get_preferred_height;
  actor_class->get_preferred_width = get_preferred_width;
  actor_class->allocate = allocate;
  actor_class->button_press_event = button_press_event;
  actor_class->button_release_event = button_release_event;
  actor_class->enter_event = enter_event;
  actor_class->leave_event = leave_event;
  actor_class->pick = pick;
  actor_class->paint = paint;

  _signals[ACTIVATED] = g_signal_new ("activated",
                                    G_TYPE_FROM_CLASS (klass),
                                    G_SIGNAL_RUN_LAST,
                                    G_STRUCT_OFFSET (MnbLauncherButtonClass, activated),
                                    NULL, NULL,
                                    g_cclosure_marshal_VOID__POINTER,
                                    G_TYPE_NONE, 1, G_TYPE_POINTER);
}

static void
mnb_launcher_button_init (MnbLauncherButton *self)
{
  ClutterActor *label;

  self->priv = MNB_LAUNCHER_BUTTON_GET_PRIVATE (self);

  /* icon */
  self->priv->icon = NULL;

  /* "app" label */
  self->priv->title = (NbtkLabel *) nbtk_label_new (NULL);
  clutter_actor_set_name (CLUTTER_ACTOR (self->priv->title), "launcher-text");
  clutter_actor_set_parent (CLUTTER_ACTOR (self->priv->title), CLUTTER_ACTOR (self));

  label = nbtk_label_get_clutter_text (self->priv->title);
  clutter_text_set_ellipsize (CLUTTER_TEXT (label), PANGO_ELLIPSIZE_END);
  clutter_text_set_line_alignment (CLUTTER_TEXT (label), PANGO_ALIGN_LEFT);

  /* "category" label */
  self->priv->description = (NbtkLabel *) nbtk_label_new (NULL);
  clutter_actor_set_name (CLUTTER_ACTOR (self->priv->description), "launcher-category");
  clutter_actor_set_parent (CLUTTER_ACTOR (self->priv->description), CLUTTER_ACTOR (self));

  label = nbtk_label_get_clutter_text (self->priv->description);
  clutter_text_set_ellipsize (CLUTTER_TEXT (label), PANGO_ELLIPSIZE_END);
  clutter_text_set_line_alignment (CLUTTER_TEXT (label), PANGO_ALIGN_LEFT);

  /* "comment" label */
  self->priv->comment = (NbtkLabel *) nbtk_label_new (NULL);
  clutter_actor_set_name (CLUTTER_ACTOR (self->priv->comment), "launcher-comment");
  clutter_actor_set_parent (CLUTTER_ACTOR (self->priv->comment), CLUTTER_ACTOR (self));

  label = nbtk_label_get_clutter_text (self->priv->comment);
  clutter_text_set_ellipsize (CLUTTER_TEXT (label), PANGO_ELLIPSIZE_END);
  clutter_text_set_line_alignment (CLUTTER_TEXT (label), PANGO_ALIGN_LEFT);
}

NbtkWidget *
mnb_launcher_button_new (const gchar *icon_file,
                         gint         icon_size,
                         const gchar *title,
                         const gchar *description,
                         const gchar *comment)
{
  MnbLauncherButton *self;
  GError            *error;

  self = g_object_new (MNB_TYPE_LAUNCHER_BUTTON, NULL);

  error = NULL;
  self->priv->icon = clutter_texture_new_from_file (icon_file, &error);
  if (error) {
    g_warning ("%s", error->message);
    g_error_free (error);
  }

  if (self->priv->icon) {

    if (icon_size > -1) {
      clutter_actor_set_size (self->priv->icon, icon_size, icon_size);
      g_object_set (G_OBJECT (self->priv->icon), "sync-size", TRUE, NULL);
    }
    clutter_actor_set_parent (CLUTTER_ACTOR (self->priv->icon), CLUTTER_ACTOR (self));
  }

  if (title)
    nbtk_label_set_text (self->priv->title, title);

  if (description)
    nbtk_label_set_text (self->priv->description, description);

  if (comment)
    nbtk_label_set_text (self->priv->comment, comment);

  return NBTK_WIDGET (self);
}

const char *
mnb_launcher_button_get_title (MnbLauncherButton *self)
{
  g_return_val_if_fail (self, NULL);

  return nbtk_label_get_text (self->priv->title);
}

const char *
mnb_launcher_button_get_description (MnbLauncherButton *self)
{
  g_return_val_if_fail (self, NULL);

  return nbtk_label_get_text (self->priv->description);
}

const char *
mnb_launcher_button_get_comment (MnbLauncherButton *self)
{
  g_return_val_if_fail (self, NULL);

  return nbtk_label_get_text (self->priv->comment);
}

