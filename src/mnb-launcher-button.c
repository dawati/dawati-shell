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

#include <string.h>
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
  FAV_TOGGLED,

  LAST_SIGNAL
};

struct _MnbLauncherButtonPrivate
{
  ClutterActor  *icon;
  NbtkLabel     *title;
  NbtkLabel     *description;
  NbtkLabel     *comment;
  ClutterActor  *fav_toggle;

  char          *category;
  char          *executable;
  char          *desktop_file_path;
  char          *icon_file;
  gint           icon_size;

  guint is_pressed  : 1;

  /* Cached for matching. */
  char          *title_key;
  char          *description_key;
  char          *comment_key;
};

static guint _signals[LAST_SIGNAL] = { 0, };

G_DEFINE_TYPE (MnbLauncherButton, mnb_launcher_button, NBTK_TYPE_TABLE);

static void
fav_button_clicked_cb (NbtkButton         *button,
                       MnbLauncherButton  *self)
{
  g_signal_emit (self, _signals[FAV_TOGGLED], 0);
}

static void
finalize (GObject *object)
{
  MnbLauncherButton *self = MNB_LAUNCHER_BUTTON (object);

  /* Child actors are managed by clutter. */

  g_free (self->priv->category);
  g_free (self->priv->executable);
  g_free (self->priv->desktop_file_path);
  g_free (self->priv->icon_file);

  g_free (self->priv->title_key);
  g_free (self->priv->description_key);
  g_free (self->priv->comment_key);

  G_OBJECT_CLASS (mnb_launcher_button_parent_class)->finalize (object);
}

static gboolean
mnb_launcher_button_button_press_event (ClutterActor       *actor,
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
mnb_launcher_button_button_release_event (ClutterActor       *actor,
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
mnb_launcher_button_enter_event (ClutterActor         *actor,
                                 ClutterCrossingEvent *event)
{
  MnbLauncherButton *self = MNB_LAUNCHER_BUTTON (actor);

  nbtk_widget_set_style_pseudo_class (NBTK_WIDGET (self), "hover");
  clutter_actor_show (self->priv->fav_toggle);

  return FALSE;
}

static gboolean
mnb_launcher_button_leave_event (ClutterActor         *actor,
                                 ClutterCrossingEvent *event)
{
  MnbLauncherButton *self = MNB_LAUNCHER_BUTTON (actor);

  clutter_actor_hide (self->priv->fav_toggle);
  nbtk_widget_set_style_pseudo_class (NBTK_WIDGET (self), NULL);

  if (self->priv->is_pressed)
    {
      clutter_ungrab_pointer ();
      self->priv->is_pressed = FALSE;
    }

  return FALSE;
}

static void
mnb_launcher_button_allocate (ClutterActor          *actor,
                              const ClutterActorBox *box,
                              gboolean               origin_changed)
{
  MnbLauncherButton *self = MNB_LAUNCHER_BUTTON (actor);
  NbtkPadding        padding;
  ClutterActorBox    child_box;

  CLUTTER_ACTOR_CLASS (mnb_launcher_button_parent_class)
    ->allocate (actor, box, origin_changed);

  nbtk_widget_get_padding (NBTK_WIDGET (self), &padding);

  /* Hack allocation of the labels, so the fav toggle overlaps. */

  clutter_actor_get_allocation_box (CLUTTER_ACTOR (self->priv->title), &child_box);
  child_box.x2 = box->x2 - box->x1 - padding.right;
  clutter_actor_allocate (CLUTTER_ACTOR (self->priv->title), &child_box, origin_changed);

  clutter_actor_get_allocation_box (CLUTTER_ACTOR (self->priv->description), &child_box);
  child_box.x2 = box->x2 - box->x1 - padding.right;
  clutter_actor_allocate (CLUTTER_ACTOR (self->priv->description), &child_box, origin_changed);

  clutter_actor_get_allocation_box (CLUTTER_ACTOR (self->priv->comment), &child_box);
  child_box.x2 = box->x2 - box->x1 - padding.right;
  clutter_actor_allocate (CLUTTER_ACTOR (self->priv->comment), &child_box, origin_changed);
}

static void
mnb_launcher_button_class_init (MnbLauncherButtonClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  ClutterActorClass *actor_class = CLUTTER_ACTOR_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MnbLauncherButtonPrivate));

  object_class->finalize = finalize;

  actor_class->button_press_event = mnb_launcher_button_button_press_event;
  actor_class->button_release_event = mnb_launcher_button_button_release_event;
  actor_class->enter_event = mnb_launcher_button_enter_event;
  actor_class->leave_event = mnb_launcher_button_leave_event;
  actor_class->allocate = mnb_launcher_button_allocate;

  _signals[ACTIVATED] = g_signal_new ("activated",
                                    G_TYPE_FROM_CLASS (klass),
                                    G_SIGNAL_RUN_LAST,
                                    G_STRUCT_OFFSET (MnbLauncherButtonClass, activated),
                                    NULL, NULL,
                                    g_cclosure_marshal_VOID__POINTER,
                                    G_TYPE_NONE, 1, G_TYPE_POINTER);

  _signals[FAV_TOGGLED] = g_signal_new ("fav-toggled",
                                    G_TYPE_FROM_CLASS (klass),
                                    G_SIGNAL_RUN_LAST,
                                    G_STRUCT_OFFSET (MnbLauncherButtonClass, fav_toggled),
                                    NULL, NULL,
                                    g_cclosure_marshal_VOID__VOID,
                                    G_TYPE_NONE, 0);
}

static void
mnb_launcher_button_init (MnbLauncherButton *self)
{
  ClutterActor *label;

  self->priv = MNB_LAUNCHER_BUTTON_GET_PRIVATE (self);

  nbtk_table_set_col_spacing (NBTK_TABLE (self), COL_SPACING);

  /* icon */
  self->priv->icon = NULL;

  /* "app" label */
  self->priv->title = (NbtkLabel *) nbtk_label_new (NULL);
  clutter_actor_set_reactive (CLUTTER_ACTOR (self->priv->title), FALSE);
  clutter_actor_set_name (CLUTTER_ACTOR (self->priv->title),
                          "mnb-launcher-button-title");
  nbtk_table_add_widget_full (NBTK_TABLE (self),
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
  clutter_actor_set_name (CLUTTER_ACTOR (self->priv->description),
                          "mnb-launcher-button-description");
  nbtk_table_add_widget_full (NBTK_TABLE (self),
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
  clutter_actor_set_name (CLUTTER_ACTOR (self->priv->comment),
                          "mnb-launcher-button-comment");
  nbtk_table_add_widget_full (NBTK_TABLE (self),
                              NBTK_WIDGET (self->priv->comment),
                              2, 1, 1, 1,
                              NBTK_X_EXPAND | NBTK_X_FILL,
                              0., 0.);

  label = nbtk_label_get_clutter_text (self->priv->comment);
  clutter_text_set_ellipsize (CLUTTER_TEXT (label), PANGO_ELLIPSIZE_END);
  clutter_text_set_line_alignment (CLUTTER_TEXT (label), PANGO_ALIGN_LEFT);

  /* "fav app" toggle */
  self->priv->fav_toggle = g_object_ref_sink (CLUTTER_ACTOR (nbtk_button_new ()));
  nbtk_button_set_toggle_mode (NBTK_BUTTON (self->priv->fav_toggle), TRUE);
  clutter_actor_hide (self->priv->fav_toggle);
  clutter_actor_set_name (CLUTTER_ACTOR (self->priv->fav_toggle),
                          "mnb-launcher-button-fav-toggle");
  clutter_actor_set_size (self->priv->fav_toggle, 24, 24);
  nbtk_table_add_widget_full (NBTK_TABLE (self),
                              NBTK_WIDGET (self->priv->fav_toggle),
                              0, 2, 3, 1,
                              0,
                              0., 0.);
  g_signal_connect (self->priv->fav_toggle, "clicked",
                    G_CALLBACK (fav_button_clicked_cb), self);
}

NbtkWidget *
mnb_launcher_button_new (const gchar *icon_file,
                         gint         icon_size,
                         const gchar *title,
                         const gchar *category,
                         const gchar *description,
                         const gchar *comment,
                         const gchar *executable,
                         const gchar *desktop_file_path)
{
  MnbLauncherButton *self;
  GError            *error;

  self = g_object_new (MNB_TYPE_LAUNCHER_BUTTON, NULL);

  self->priv->icon_file = g_strdup (icon_file);

  self->priv->icon_size = icon_size;

  error = NULL;
  self->priv->icon = clutter_texture_new_from_file (self->priv->icon_file,
                                                    &error);
  if (error) {
    g_warning ("%s", error->message);
    g_error_free (error);
  }

  if (self->priv->icon) {

    if (self->priv->icon_size > -1) {
      clutter_actor_set_size (self->priv->icon,
                              self->priv->icon_size,
                              self->priv->icon_size);
      g_object_set (G_OBJECT (self->priv->icon), "sync-size", TRUE, NULL);
    }
    nbtk_table_add_actor_full (NBTK_TABLE (self),
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

  if (executable)
    self->priv->executable = g_strdup (executable);

  if (desktop_file_path)
    self->priv->desktop_file_path = g_strdup (desktop_file_path);

  return NBTK_WIDGET (self);
}

NbtkWidget *
mnb_launcher_button_create_favorite (MnbLauncherButton *self)
{
  NbtkWidget *clone;

  g_return_val_if_fail (self, NULL);
  clone = mnb_launcher_button_new (self->priv->icon_file,
                                   self->priv->icon_size,
                                   nbtk_label_get_text (self->priv->title),
                                   self->priv->category,
                                   nbtk_label_get_text (self->priv->description),
                                   nbtk_label_get_text (self->priv->comment),
                                   self->priv->executable,
                                   self->priv->desktop_file_path);

  mnb_launcher_button_set_favorite (MNB_LAUNCHER_BUTTON (clone), TRUE);

  return clone;
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

void
mnb_launcher_button_set_comment (MnbLauncherButton *self,
                                 const gchar       *comment)
{
  g_return_if_fail (self);

  nbtk_label_set_text (self->priv->comment, comment);
}

const char *
mnb_launcher_button_get_executable (MnbLauncherButton *self)
{
  g_return_val_if_fail (self, NULL);

  return self->priv->executable;
}

const char *
mnb_launcher_button_get_desktop_file_path (MnbLauncherButton *self)
{
  g_return_val_if_fail (self, NULL);

  return self->priv->desktop_file_path;
}

gboolean
mnb_launcher_button_get_favorite (MnbLauncherButton *self)
{
  gboolean is_checked;

  g_return_val_if_fail (self, FALSE);

  g_object_get (self->priv->fav_toggle,
                "checked", &is_checked,
                NULL);

  return is_checked;
}

void
mnb_launcher_button_set_favorite (MnbLauncherButton *self,
                                  gboolean           is_favorite)
{
  g_return_if_fail (self);

  g_object_set (self->priv->fav_toggle,
                "checked", is_favorite,
                NULL);
}

gint
mnb_launcher_button_compare (MnbLauncherButton *self,
                             MnbLauncherButton *other)
{
  g_return_val_if_fail (self, 0);
  g_return_val_if_fail (other, 0);

  return g_utf8_collate (nbtk_label_get_text (NBTK_LABEL (self->priv->title)),
                         nbtk_label_get_text (NBTK_LABEL (other->priv->title)));
}

gboolean
mnb_launcher_button_match (MnbLauncherButton *self,
                           const gchar       *lcase_needle)
{
  g_return_val_if_fail (self, 0);

  /* Empty key matches. */
  if (g_utf8_strlen (lcase_needle, -1) == 0)
    return TRUE;

  /* Title. */
  if (!self->priv->title_key)
    self->priv->title_key =
      g_utf8_strdown (nbtk_label_get_text (NBTK_LABEL (self->priv->title)),
                      -1);

  if (self->priv->title_key &&
      NULL != strstr (self->priv->title_key, lcase_needle))
    {
      return TRUE;
    }

  /* Description. */
  if (!self->priv->description_key)
    self->priv->description_key =
      g_utf8_strdown (nbtk_label_get_text (NBTK_LABEL (self->priv->description)),
                      -1);

  if (self->priv->description_key &&
      NULL != strstr (self->priv->description_key, lcase_needle))
    {
      return TRUE;
    }

  /* Comment. */
  if (!self->priv->comment_key)
    self->priv->comment_key =
      g_utf8_strdown (nbtk_label_get_text (NBTK_LABEL (self->priv->comment)),
                      -1);

  if (self->priv->comment_key &&
      NULL != strstr (self->priv->comment_key, lcase_needle))
    {
      return TRUE;
    }

  return FALSE;
}

