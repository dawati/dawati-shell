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
  ClutterActor  *table;
  ClutterActor  *icon;
  NbtkLabel     *title;
  NbtkLabel     *description;
  NbtkLabel     *comment;

  char          *category;

  guint is_pressed : 1;
};

static guint _signals[LAST_SIGNAL] = { 0, };

G_DEFINE_TYPE (MnbLauncherButton, mnb_launcher_button, NBTK_TYPE_BIN);

static void
dispose (GObject *object)
{
  MnbLauncherButton *self = MNB_LAUNCHER_BUTTON (object);

  if (self->priv->table)
    {
      clutter_actor_unparent (self->priv->table);
      self->priv->table = NULL;
    }

  if (self->priv->category)
    {
      g_free (self->priv->category);
      self->priv->category = NULL;
    }

  G_OBJECT_CLASS (mnb_launcher_button_parent_class)->dispose (object);
}

static void
get_preferred_width (ClutterActor *actor,
                     ClutterUnit   for_height,
                     ClutterUnit  *min_width_p,
                     ClutterUnit  *natural_width_p)
{
  MnbLauncherButton *self = MNB_LAUNCHER_BUTTON (actor);
  NbtkPadding        padding;
  ClutterUnit        min_width, natural_width;

  nbtk_bin_get_padding (NBTK_BIN (self), &padding);

  clutter_actor_get_preferred_width (self->priv->table, for_height,
                                     &min_width, &natural_width);

  if (min_width_p)
    *min_width_p = padding.left + min_width + padding.right;

  if (natural_width_p)
    *natural_width_p = padding.left + natural_width + padding.right;
}

static void
get_preferred_height (ClutterActor *actor,
                      ClutterUnit   for_width,
                      ClutterUnit  *min_height_p,
                      ClutterUnit  *natural_height_p)
{
  MnbLauncherButton *self = MNB_LAUNCHER_BUTTON (actor);
  NbtkPadding        padding;
  ClutterUnit        min_height, natural_height;

  nbtk_bin_get_padding (NBTK_BIN (self), &padding);

  clutter_actor_get_preferred_height (self->priv->table, for_width,
                                      &min_height, &natural_height);

  if (min_height_p)
    *min_height_p = padding.left + min_height + padding.right;

  if (natural_height_p)
    *natural_height_p = padding.left + natural_height + padding.right;
}

static void
allocate (ClutterActor          *actor,
          const ClutterActorBox *box,
          gboolean               origin_changed)
{
  MnbLauncherButton *self = MNB_LAUNCHER_BUTTON (actor);
  ClutterActorBox    child_box;
  NbtkPadding        padding;

  CLUTTER_ACTOR_CLASS (mnb_launcher_button_parent_class)
    ->allocate (actor, box, origin_changed);

  nbtk_bin_get_padding (NBTK_BIN (self), &padding);

  child_box.x1 = padding.left;
  child_box.y1 = padding.top;
  child_box.x2 = box->x2 - box->x1 - padding.right;
  child_box.y2 = box->y2 - box->y1 - padding.bottom;

  clutter_actor_allocate (self->priv->table, &child_box, origin_changed);
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

  clutter_actor_paint (self->priv->table);
}

static void
paint (ClutterActor *actor)
{
  MnbLauncherButton *self = MNB_LAUNCHER_BUTTON (actor);

  CLUTTER_ACTOR_CLASS (mnb_launcher_button_parent_class)->paint (actor);

  clutter_actor_paint (self->priv->table);
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

  /* table */
  self->priv->table = CLUTTER_ACTOR (nbtk_table_new ());
  clutter_actor_set_reactive (CLUTTER_ACTOR (self->priv->table), FALSE);
  nbtk_table_set_col_spacing (NBTK_TABLE (self->priv->table), COL_SPACING);

  /* icon */
  self->priv->icon = NULL;

  /* "app" label */
  self->priv->title = (NbtkLabel *) nbtk_label_new (NULL);
  clutter_actor_set_reactive (CLUTTER_ACTOR (self->priv->title), FALSE);
  clutter_actor_set_name (CLUTTER_ACTOR (self->priv->title), "mnb-launcher-button-title");
  nbtk_table_add_widget_full (NBTK_TABLE (self->priv->table),
                              NBTK_WIDGET (self->priv->title),
                              0, 1, 1, 1,
                              NBTK_X_EXPAND | NBTK_X_FILL,
                              0., 0.);

  label = nbtk_label_get_clutter_text (self->priv->title);
  clutter_text_set_ellipsize (CLUTTER_TEXT (label), PANGO_ELLIPSIZE_END);
  clutter_text_set_line_alignment (CLUTTER_TEXT (label), PANGO_ALIGN_LEFT);

  /* "category" label */
  self->priv->description = (NbtkLabel *) nbtk_label_new (NULL);
  clutter_actor_set_reactive (CLUTTER_ACTOR (self->priv->description), FALSE);
  clutter_actor_set_name (CLUTTER_ACTOR (self->priv->description), "mnb-launcher-button-description");
  nbtk_table_add_widget_full (NBTK_TABLE (self->priv->table),
                              NBTK_WIDGET (self->priv->description),
                              1, 1, 1, 1,
                              NBTK_X_EXPAND | NBTK_X_FILL,
                              0., 0.);

  label = nbtk_label_get_clutter_text (self->priv->description);
  clutter_text_set_ellipsize (CLUTTER_TEXT (label), PANGO_ELLIPSIZE_END);
  clutter_text_set_line_alignment (CLUTTER_TEXT (label), PANGO_ALIGN_LEFT);

  /* "comment" label */
  self->priv->comment = (NbtkLabel *) nbtk_label_new (NULL);
  clutter_actor_set_reactive (CLUTTER_ACTOR (self->priv->comment), FALSE);
  clutter_actor_set_name (CLUTTER_ACTOR (self->priv->comment), "mnb-launcher-button-comment");
  nbtk_table_add_widget_full (NBTK_TABLE (self->priv->table),
                              NBTK_WIDGET (self->priv->comment),
                              2, 1, 1, 1,
                              NBTK_X_EXPAND | NBTK_X_FILL,
                              0., 0.);

  label = nbtk_label_get_clutter_text (self->priv->comment);
  clutter_text_set_ellipsize (CLUTTER_TEXT (label), PANGO_ELLIPSIZE_END);
  clutter_text_set_line_alignment (CLUTTER_TEXT (label), PANGO_ALIGN_LEFT);
}

NbtkWidget *
mnb_launcher_button_new (const gchar *icon_file,
                         gint         icon_size,
                         const gchar *title,
                         const gchar *category,
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
    nbtk_table_add_actor_full (NBTK_TABLE (self->priv->table),
                               self->priv->icon,
                               0, 0, 3, 1,
                               0,
                               0., 0.5);
  }

  if (title)
    nbtk_label_set_text (self->priv->title, title);

  if (category)
    self->priv->category = g_strdup (category);

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
mnb_launcher_button_get_category (MnbLauncherButton *self)
{
  g_return_val_if_fail (self, NULL);

  return self->priv->category;
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

