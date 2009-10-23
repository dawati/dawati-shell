/*
 * Moblin-Web-Browser: The web browser for Moblin
 * Copyright (c) 2009, Intel Corporation.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU Lesser General Public License,
 * version 2.1, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <gdk/gdkx.h>

#include "mwb-radical-bar.h"
#include "mwb-utils.h"
#include "mwb-ac-list.h"
#include "mwb-spindle.h"

G_DEFINE_TYPE (MwbRadicalBar, mwb_radical_bar, NBTK_TYPE_WIDGET)

#define RADICAL_BAR_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MWB_TYPE_RADICAL_BAR, \
   MwbRadicalBarPrivate))

enum
{
  PROP_0,

  PROP_ICON,
  PROP_URI,
  PROP_TITLE,
  PROP_LOADING,
  PROP_PROGRESS,
  PROP_FAKE_PROGRESS,
  PROP_PINNED,
  PROP_SHOW_PIN,
};

enum
{
  GO,
  RELOAD,
  STOP,
  PIN_BUTTON_CLICKED,

  LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0, };

/* We want to show the title when either the entry has focus or the
   mouse is over the ClutterText, the NbtkEntry or the MwbSpindle. */
typedef enum
{
  MWB_RADICAL_BAR_ENTRY_HAS_FOCUS    = 1,
  MWB_RADICAL_BAR_MOUSE_OVER_TEXT    = 2,
  MWB_RADICAL_BAR_MOUSE_OVER_ENTRY   = 4,
  MWB_RADICAL_BAR_MOUSE_OVER_SPINDLE = 8
} MwbRadicalBarShowTitleState;

struct _MwbRadicalBarPrivate
{
  NbtkWidget      *table;
  ClutterActor    *broken_highlight;
  ClutterActor    *encrypted_highlight;
  ClutterActor    *progress_bar;
  ClutterActor    *icon;
  ClutterActor    *default_icon;
  ClutterActor    *spinner;
  ClutterTimeline *spinner_timeline;
  NbtkWidget      *entry;
  NbtkWidget      *title;
  ClutterActor    *spindle;
  NbtkWidget      *instructions;
  NbtkWidget      *button;
  NbtkWidget      *pin_button;
  NbtkWidget      *lock_icon;

  gboolean         loading;
  gdouble          progress;
  /* text_changed is TRUE if the user has modified the URL. Otherwise
     the radical bar will just be showing the URI */
  gboolean         text_changed;
  gboolean         show_ac_list;
  guint            security;

  /* The URI that is currently shown in the browser. The entry will be
     reset to this when escape is pressed */
  gchar           *uri;

  MwbRadicalBarShowTitleState show_title_state;

  /* Whether to show the pin for pinning pages */
  gboolean         show_pin;
  /* Whether the pin should be pushed in or not */
  gboolean         pinned;

  NbtkWidget      *ac_list;
  guint            ac_list_activate_handler;
  ClutterTimeline *ac_list_timeline;
  gdouble          ac_list_anim_progress;

  ClutterAnimation *fake_progress_anim;
  gdouble           fake_progress;
};

static void
mwb_radical_bar_resize_progress (MwbRadicalBar          *self,
                                 const ClutterActorBox  *box,
                                 ClutterAllocationFlags  flags);


static void
mwb_radical_bar_get_property (GObject *object, guint property_id,
                              GValue *value, GParamSpec *pspec)
{
  MwbRadicalBar *radical_bar = MWB_RADICAL_BAR (object);
  MwbRadicalBarPrivate *priv = radical_bar->priv;

  switch (property_id)
    {
    case PROP_ICON:
      g_value_set_object (value, mwb_radical_bar_get_icon (radical_bar));
      break;

    case PROP_URI:
      g_value_set_string (value, mwb_radical_bar_get_uri (radical_bar));
      break;

    case PROP_TITLE:
      g_value_set_string (value, mwb_radical_bar_get_title (radical_bar));
      break;

    case PROP_LOADING:
      g_value_set_boolean (value, mwb_radical_bar_get_loading (radical_bar));
      break;

    case PROP_PROGRESS:
      g_value_set_double (value, mwb_radical_bar_get_progress (radical_bar));
      break;

    case PROP_FAKE_PROGRESS:
      g_value_set_double (value, priv->fake_progress);
      break;

    case PROP_PINNED:
      g_value_set_boolean (value, mwb_radical_bar_get_pinned (radical_bar));
      break;

    case PROP_SHOW_PIN:
      g_value_set_boolean (value, mwb_radical_bar_get_show_pin (radical_bar));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
mwb_radical_bar_set_property (GObject *object, guint property_id,
                              const GValue *value, GParamSpec *pspec)
{
  MwbRadicalBar *radical_bar = MWB_RADICAL_BAR (object);
  MwbRadicalBarPrivate *priv = radical_bar->priv;

  switch (property_id)
    {
    case PROP_ICON:
      mwb_radical_bar_set_icon (radical_bar, g_value_get_object (value));
      break;

    case PROP_URI:
      mwb_radical_bar_set_uri (radical_bar, g_value_get_string (value));
      break;

    case PROP_TITLE:
      mwb_radical_bar_set_title (radical_bar, g_value_get_string (value));
      break;

    case PROP_LOADING:
      mwb_radical_bar_set_loading (radical_bar, g_value_get_boolean (value));
      break;

    case PROP_PROGRESS:
      mwb_radical_bar_set_progress (radical_bar, g_value_get_double (value));
      break;

    case PROP_FAKE_PROGRESS:
      priv->fake_progress = g_value_get_double (value);
      mwb_radical_bar_resize_progress (radical_bar, NULL, 0);
      clutter_actor_queue_redraw (CLUTTER_ACTOR (radical_bar));
      break;

    case PROP_PINNED:
      mwb_radical_bar_set_pinned (radical_bar, g_value_get_boolean (value));
      break;

    case PROP_SHOW_PIN:
      mwb_radical_bar_set_show_pin (radical_bar, g_value_get_boolean (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
mwb_radical_bar_stop_fake_progress (MwbRadicalBar *radical_bar)
{
  MwbRadicalBarPrivate *priv = radical_bar->priv;

  if (priv->fake_progress_anim)
    {
      g_object_unref (G_OBJECT (priv->fake_progress_anim));
      priv->fake_progress_anim = NULL;
    }
}

static void
mwb_radical_bar_dispose (GObject *object)
{
  MwbRadicalBar *radical_bar = MWB_RADICAL_BAR (object);
  MwbRadicalBarPrivate *priv = radical_bar->priv;

  if (priv->ac_list_timeline)
    {
      clutter_timeline_stop (priv->ac_list_timeline);
      g_object_unref (priv->ac_list_timeline);
      priv->ac_list_timeline = NULL;
    }

  if (priv->broken_highlight)
    {
      clutter_actor_unparent (priv->broken_highlight);
      priv->broken_highlight = NULL;
    }

  if (priv->encrypted_highlight)
    {
      clutter_actor_unparent (priv->encrypted_highlight);
      priv->encrypted_highlight = NULL;
    }

  if (priv->progress_bar)
    {
      clutter_actor_unparent (priv->progress_bar);
      priv->progress_bar = NULL;
    }

  if (priv->ac_list)
    {
      g_signal_handler_disconnect (priv->ac_list,
                                   priv->ac_list_activate_handler);
      clutter_actor_unparent (CLUTTER_ACTOR (priv->ac_list));
      priv->ac_list = NULL;
    }

  if (priv->default_icon)
    {
      g_object_unref (priv->default_icon);
      priv->default_icon = NULL;
    }

  if (priv->table)
    {
      clutter_actor_unparent (CLUTTER_ACTOR (priv->table));
      priv->table = NULL;
    }

  mwb_radical_bar_stop_fake_progress (radical_bar);

  G_OBJECT_CLASS (mwb_radical_bar_parent_class)->dispose (object);
}

static void
mwb_radical_bar_finalize (GObject *object)
{
  MwbRadicalBar *radical_bar = MWB_RADICAL_BAR (object);
  MwbRadicalBarPrivate *priv = radical_bar->priv;

  g_free (priv->uri);

  G_OBJECT_CLASS (mwb_radical_bar_parent_class)->finalize (object);
}

static void
mwb_radical_bar_resize_progress (MwbRadicalBar          *self,
                                 const ClutterActorBox  *box,
                                 ClutterAllocationFlags  flags)
{
  gfloat width, height;
  ClutterActorBox self_box, child_box;

  MwbRadicalBarPrivate *priv = self->priv;

  if (!box)
    {
      clutter_actor_get_allocation_box (CLUTTER_ACTOR (self), &self_box);
      box = &self_box;
    }

  /* progress ignores padding */
  width = (box->x2 - box->x1) * (0.1 + (priv->fake_progress * 0.9));
  height = box->y2 - box->y1;

  child_box.x1 = 0;
  child_box.y1 = 0;
  child_box.x2 = width;
  child_box.y2 = height;

  clutter_actor_allocate (priv->progress_bar, &child_box, flags);

  if (!priv->loading)
    child_box.x2 = (box->x2 - box->x1);

  clutter_actor_allocate (priv->broken_highlight, &child_box, flags);
  clutter_actor_allocate (priv->encrypted_highlight, &child_box, flags);
}

static void
mwb_radical_bar_allocate (ClutterActor           *actor,
                          const ClutterActorBox  *box,
                          ClutterAllocationFlags  flags)
{
  NbtkPadding padding;
  ClutterActorBox child_box;
  gfloat ac_preferred_height;

  MwbRadicalBar *self = MWB_RADICAL_BAR (actor);
  MwbRadicalBarPrivate *priv = self->priv;

  CLUTTER_ACTOR_CLASS (mwb_radical_bar_parent_class)->
    allocate (actor, box, flags);

  nbtk_widget_get_padding (NBTK_WIDGET (actor), &padding);

  /* padding only applies to the table */
  child_box.x1 = MWB_PIXBOUND (padding.left);
  child_box.y1 = MWB_PIXBOUND (padding.top);
  child_box.x2 = box->x2 - box->x1 - MWB_PIXBOUND (padding.right);
  child_box.y2 = box->y2 - box->y1 - MWB_PIXBOUND (padding.bottom);

  clutter_actor_allocate (CLUTTER_ACTOR (priv->table), &child_box, flags);

  child_box.x1 = 0;
  child_box.y1 = 0;
  child_box.x2 = box->x2 - box->x1;
  child_box.y2 = box->y2 - box->y1;

  mwb_radical_bar_resize_progress (self, box, flags);

  if (priv->ac_list)
    {
      /* Work out the maximum height that the ac list can have before
         it would go off the end of the stage so that it can allocate
         a whole number of rows */
      gfloat y_pos = box->y2 - box->y1;
      ClutterActor *actor_pos = actor, *parent;
      ClutterActorBox allocation;

      /* Add in each actor's y position until we get to one with no
         parent (the stage) */
      while (TRUE)
        {
          clutter_actor_get_allocation_box (actor_pos, &allocation);
          if ((parent = clutter_actor_get_parent (actor_pos)) == NULL)
            break;
          y_pos += allocation.y1;
          actor_pos = parent;
        }

      /* remove padding */
      child_box.x1 = 0;
      child_box.x2 = box->x2 - box->x1;
      mwb_ac_list_get_preferred_height_with_max (MWB_AC_LIST (priv->ac_list),
                                                 child_box.x2 - child_box.x1,
                                                 allocation.y2 - allocation.y1 -
                                                 y_pos,
                                                 NULL,
                                                 &ac_preferred_height);
      child_box.y1 = box->y2 - box->y1;
      child_box.y2 = child_box.y1 + ac_preferred_height;

      clutter_actor_allocate (CLUTTER_ACTOR (priv->ac_list), &child_box, flags);
    }
}

static void
mwb_radical_bar_get_preferred_width (ClutterActor *actor,
                                     gfloat        for_height,
                                     gfloat       *min_width_p,
                                     gfloat       *natural_width_p)
{
  gfloat min_width, natural_width;

  MwbRadicalBarPrivate *priv = MWB_RADICAL_BAR (actor)->priv;

  /* Chain up to get padding */
  CLUTTER_ACTOR_CLASS (mwb_radical_bar_parent_class)->
    get_preferred_width (actor,
                         for_height,
                         min_width_p,
                         natural_width_p);

  clutter_actor_get_preferred_width (CLUTTER_ACTOR (priv->table),
                                     for_height,
                                     &min_width,
                                     &natural_width);

  if (min_width_p)
    *min_width_p += min_width;

  if (natural_width_p)
    *natural_width_p += natural_width;
}

static void
mwb_radical_bar_get_preferred_height (ClutterActor *actor,
                                      gfloat        for_width,
                                      gfloat       *min_height_p,
                                      gfloat       *natural_height_p)
{
  gfloat min_height, natural_height;

  MwbRadicalBarPrivate *priv = MWB_RADICAL_BAR (actor)->priv;

  /* Chain up to get padding */
  CLUTTER_ACTOR_CLASS (mwb_radical_bar_parent_class)->
    get_preferred_width (actor,
                         for_width,
                         min_height_p,
                         natural_height_p);

  clutter_actor_get_preferred_height (CLUTTER_ACTOR (priv->table),
                                      for_width,
                                      &min_height,
                                      &natural_height);

  if (min_height_p)
    *min_height_p += min_height;

  if (natural_height_p)
    *natural_height_p += natural_height;
}

static void
mwb_radical_bar_paint (ClutterActor *actor)
{
  MwbRadicalBarPrivate *priv = MWB_RADICAL_BAR (actor)->priv;

  /* Chain up to get the background */
  CLUTTER_ACTOR_CLASS (mwb_radical_bar_parent_class)->paint (actor);

  if (CLUTTER_ACTOR_IS_MAPPED (priv->broken_highlight))
    clutter_actor_paint (priv->broken_highlight);
  else if (CLUTTER_ACTOR_IS_MAPPED (priv->encrypted_highlight))
    clutter_actor_paint (priv->encrypted_highlight);
  else if (CLUTTER_ACTOR_IS_MAPPED (priv->progress_bar))
    clutter_actor_paint (priv->progress_bar);

  clutter_actor_paint (CLUTTER_ACTOR (priv->table));

  if (priv->ac_list && CLUTTER_ACTOR_IS_MAPPED (priv->ac_list))
    {
      ClutterActorBox box;
      clutter_actor_get_allocation_box (CLUTTER_ACTOR (priv->ac_list), &box);
      cogl_clip_push (box.x1, box.y1, box.x2 - box.x1, box.y2 - box.y1);
      cogl_translate (0, -(box.y2 - box.y1) *
                         (1.0-priv->ac_list_anim_progress), 0);
      clutter_actor_paint (CLUTTER_ACTOR (priv->ac_list));
      cogl_clip_pop ();
    }
}

static void
mwb_radical_bar_pick (ClutterActor *actor, const ClutterColor *color)
{
  mwb_radical_bar_paint (actor);
}

static void
mwb_radical_bar_update_instructions (MwbRadicalBar *self)
{
  MwbRadicalBarPrivate *priv = self->priv;
  const gchar *text;

  if (CLUTTER_ACTOR_IS_VISIBLE (priv->ac_list))
    {
      if (mwb_ac_list_get_selection (MWB_AC_LIST (priv->ac_list)) != -1)
        text = _("Press enter to\n"
                 "visit site");
      else
        text = _("Press down to\n"
                 "explore list");

      nbtk_label_set_text (NBTK_LABEL (priv->instructions), text);

      /* Restore the natural width of the text */
      g_object_set (priv->instructions,
                    "natural-width-set", FALSE,
                    "min-width-set", FALSE,
                    NULL);

      clutter_actor_show (CLUTTER_ACTOR (priv->instructions));
    }
  else
    {
      /* Force the instructions to take up zero width then they are
         hidden */
      clutter_actor_set_width (CLUTTER_ACTOR (priv->instructions), 0);

      clutter_actor_hide (CLUTTER_ACTOR (priv->instructions));
    }
}

static void
mwb_radical_bar_ac_list_new_frame_cb (ClutterTimeline *timeline,
                                      guint            msecs,
                                      MwbRadicalBar   *self)
{
  MwbRadicalBarPrivate *priv = self->priv;
  priv->ac_list_anim_progress = clutter_timeline_get_progress (timeline);
  clutter_actor_queue_redraw (CLUTTER_ACTOR (self));
}

static void
mwb_radical_bar_ac_list_completed_cb (ClutterTimeline *timeline,
                                      MwbRadicalBar   *self)
{
  MwbRadicalBarPrivate *priv = self->priv;

  if (clutter_timeline_get_direction (timeline) == CLUTTER_TIMELINE_FORWARD)
    {
      priv->ac_list_anim_progress = 1.0;
    }
  else
    {
      priv->ac_list_anim_progress = 0.0;
      clutter_actor_hide (CLUTTER_ACTOR (priv->ac_list));
    }

  g_object_unref (timeline);
  priv->ac_list_timeline = NULL;

  mwb_radical_bar_update_instructions (self);

  clutter_actor_queue_redraw (CLUTTER_ACTOR (self));
}

static void
mwb_radical_bar_set_show_auto_complete (MwbRadicalBar *self, gboolean show)
{
  MwbRadicalBarPrivate *priv = self->priv;

  priv->show_ac_list = show;

  if (CLUTTER_ACTOR_IS_VISIBLE (priv->ac_list) != show)
    {
      if (!priv->ac_list_timeline)
        {
          priv->ac_list_timeline = clutter_timeline_new (150);
          g_signal_connect (priv->ac_list_timeline, "new-frame",
                            G_CALLBACK (mwb_radical_bar_ac_list_new_frame_cb),
                            self);
          g_signal_connect (priv->ac_list_timeline, "completed",
                            G_CALLBACK (mwb_radical_bar_ac_list_completed_cb),
                            self);
        }

      if (show)
        {
          const gchar *text = nbtk_entry_get_text (NBTK_ENTRY (priv->entry));
          clutter_actor_show (CLUTTER_ACTOR (priv->ac_list));
          mwb_ac_list_set_search_text (MWB_AC_LIST (priv->ac_list), text);
          mwb_ac_list_set_selection (MWB_AC_LIST (priv->ac_list), -1);

          clutter_timeline_set_direction (priv->ac_list_timeline,
                                          CLUTTER_TIMELINE_FORWARD);
        }
      else
        {
          clutter_timeline_set_direction (priv->ac_list_timeline,
                                          CLUTTER_TIMELINE_BACKWARD);
        }

      clutter_timeline_start (priv->ac_list_timeline);
      mwb_radical_bar_update_instructions (self);
    }
}

static void
mwb_radical_bar_activate_cb (ClutterText *text, MwbRadicalBar *self);

static void
mwb_radical_bar_complete_url (MwbRadicalBar *self,
                              const char    *suffix)
{
  const char *text;
  MwbRadicalBarPrivate *priv = self->priv;
  ClutterActor *ctext;
  gchar *buf;

  if (!suffix || (strlen (suffix) != 3))
    return;

  text = nbtk_entry_get_text (NBTK_ENTRY (priv->entry));

  if (!strncmp (text, "www.", 4) || strchr (text, ':'))
    return;

  buf = g_strconcat ("www.", text, ".", suffix, NULL);
  nbtk_entry_set_text (NBTK_ENTRY (priv->entry), buf);
  g_free (buf);
  ctext = nbtk_entry_get_clutter_text (NBTK_ENTRY (priv->entry));
  mwb_radical_bar_activate_cb (CLUTTER_TEXT (ctext), self);
}

static void
mwb_radical_bar_update_button (MwbRadicalBar *self)
{
  MwbRadicalBarPrivate *priv = self->priv;

  clutter_actor_set_name (CLUTTER_ACTOR (priv->button),
                          priv->loading ?
                          "url-button-stop" :
                          priv->text_changed ?
                          "url-button-play" :
                          "url-button-reload");
}

static void
mwb_radical_bar_select_all (MwbRadicalBar *radical_bar)
{
  ClutterText *text;
  gint length;
  MwbRadicalBarPrivate *priv = radical_bar->priv;

  text = CLUTTER_TEXT (nbtk_entry_get_clutter_text (NBTK_ENTRY (priv->entry)));
  length = strlen (clutter_text_get_text (text));
  clutter_text_set_selection (text, length, 0);
}

static void
mwb_radical_bar_show_uri (MwbRadicalBar *radical_bar)
{
  MwbRadicalBarPrivate *priv = radical_bar->priv;
  ClutterActor *ctext;
  const gchar *old_text;
  gboolean was_selected;

  /* If all of the text is selected then preserve that after the
     text is changed */
  ctext = nbtk_entry_get_clutter_text (NBTK_ENTRY (priv->entry));
  old_text = nbtk_entry_get_text (NBTK_ENTRY (priv->entry));
  was_selected =
    (clutter_text_get_selection_bound (CLUTTER_TEXT (ctext)) == 0 &&
     clutter_text_get_cursor_position (CLUTTER_TEXT (ctext)) ==
     strlen (old_text));

  nbtk_entry_set_text (NBTK_ENTRY (priv->entry),
                       priv->uri ? priv->uri : NULL);

  if (was_selected)
    mwb_radical_bar_select_all (radical_bar);

  /* Setting the text will cause the text changed callback to think
     the user is editing the URL. We don't want this so we need to set
     it back */
  priv->text_changed = FALSE;

  mwb_radical_bar_update_button (radical_bar);
}

static gboolean
mwb_radical_bar_captured_event (ClutterActor *actor,
                                ClutterEvent *event)
{
  ClutterKeyEvent *key_event;
  MwbRadicalBar *self = MWB_RADICAL_BAR (actor);
  MwbRadicalBarPrivate *priv = self->priv;

  if (event->type != CLUTTER_KEY_PRESS)
    return FALSE;
  key_event = (ClutterKeyEvent *)event;

  switch (key_event->keyval)
    {
    case CLUTTER_Escape:
      mwb_radical_bar_set_show_auto_complete (self, FALSE);

      /* If we have a URI then revert to that and give up focus so
         that the title will be shown if there is one */
      if (priv->uri && *priv->uri)
        {
          mwb_radical_bar_show_uri (self);

          if ((priv->show_title_state & MWB_RADICAL_BAR_ENTRY_HAS_FOCUS))
            {
              ClutterActor *parent;

              parent = clutter_actor_get_parent (actor);
              if (parent)
                clutter_actor_grab_key_focus (parent);
            }
        }
      return TRUE;

    case CLUTTER_Return:
      if ((key_event->modifier_state & MWB_UTILS_MODIFIERS_MASK) ==
          CLUTTER_CONTROL_MASK)
        mwb_radical_bar_complete_url (self, "com");
      else if ((key_event->modifier_state & MWB_UTILS_MODIFIERS_MASK) ==
               (CLUTTER_CONTROL_MASK | CLUTTER_SHIFT_MASK))
        mwb_radical_bar_complete_url (self, "org");
      else if ((key_event->modifier_state & MWB_UTILS_MODIFIERS_MASK) ==
               CLUTTER_SHIFT_MASK)
        mwb_radical_bar_complete_url (self, "net");
      else
        break;
      return TRUE;

    case CLUTTER_Down:
    case CLUTTER_Up:
      if (CLUTTER_ACTOR_IS_VISIBLE (CLUTTER_ACTOR (priv->ac_list)))
        {
          gint selection =
            mwb_ac_list_get_selection (MWB_AC_LIST (priv->ac_list));

          if (selection < 0)
            {
              mwb_ac_list_set_selection (MWB_AC_LIST (priv->ac_list), 0);
              mwb_radical_bar_update_instructions (self);
            }
          else if (key_event->keyval == CLUTTER_Down)
            {
              guint n_visible_entries =
                mwb_ac_list_get_n_visible_entries (MWB_AC_LIST (priv->ac_list));

              if (selection < n_visible_entries - 1)

                {
                  mwb_ac_list_set_selection (MWB_AC_LIST (priv->ac_list),
                                             selection + 1);
                  mwb_radical_bar_update_instructions (self);
                  return TRUE;
                }
            }
          else if (selection > 0)
            {
              mwb_ac_list_set_selection (MWB_AC_LIST (priv->ac_list),
                                         selection - 1);
              mwb_radical_bar_update_instructions (self);
              return TRUE;
            }
        }
      break;
    }

  if (CLUTTER_ACTOR_CLASS (mwb_radical_bar_parent_class)->captured_event)
    return CLUTTER_ACTOR_CLASS (mwb_radical_bar_parent_class)->
      captured_event (actor, event);

  return FALSE;
}

static void
mwb_radical_bar_map (ClutterActor *actor)
{
  MwbRadicalBarPrivate *priv = MWB_RADICAL_BAR (actor)->priv;

  CLUTTER_ACTOR_CLASS (mwb_radical_bar_parent_class)->map (actor);

  clutter_actor_map (CLUTTER_ACTOR (priv->table));
  clutter_actor_map (CLUTTER_ACTOR (priv->broken_highlight));
  clutter_actor_map (CLUTTER_ACTOR (priv->encrypted_highlight));
  clutter_actor_map (CLUTTER_ACTOR (priv->progress_bar));
  clutter_actor_map (CLUTTER_ACTOR (priv->ac_list));
}

static void
mwb_radical_bar_unmap (ClutterActor *actor)
{
  MwbRadicalBarPrivate *priv = MWB_RADICAL_BAR (actor)->priv;

  CLUTTER_ACTOR_CLASS (mwb_radical_bar_parent_class)->unmap (actor);

  clutter_actor_unmap (CLUTTER_ACTOR (priv->table));
  clutter_actor_unmap (CLUTTER_ACTOR (priv->broken_highlight));
  clutter_actor_unmap (CLUTTER_ACTOR (priv->encrypted_highlight));
  clutter_actor_unmap (CLUTTER_ACTOR (priv->progress_bar));
  clutter_actor_unmap (CLUTTER_ACTOR (priv->ac_list));
}

static void
mwb_radical_bar_go (MwbRadicalBar *self, const gchar *url)
{
  MwbRadicalBarPrivate *priv = self->priv;

  if (priv->ac_list)
    mwb_ac_list_increment_tld_score_for_url (MWB_AC_LIST (priv->ac_list), url);
}

static void
mwb_radical_bar_class_init (MwbRadicalBarClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  ClutterActorClass *actor_class = CLUTTER_ACTOR_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MwbRadicalBarPrivate));

  object_class->get_property = mwb_radical_bar_get_property;
  object_class->set_property = mwb_radical_bar_set_property;
  object_class->dispose = mwb_radical_bar_dispose;
  object_class->finalize = mwb_radical_bar_finalize;

  actor_class->allocate = mwb_radical_bar_allocate;
  actor_class->get_preferred_width = mwb_radical_bar_get_preferred_width;
  actor_class->get_preferred_height = mwb_radical_bar_get_preferred_height;
  actor_class->paint = mwb_radical_bar_paint;
  actor_class->pick = mwb_radical_bar_pick;
  actor_class->captured_event = mwb_radical_bar_captured_event;
  actor_class->map = mwb_radical_bar_map;
  actor_class->unmap = mwb_radical_bar_unmap;

  klass->go = mwb_radical_bar_go;

  g_object_class_install_property (object_class,
                                   PROP_ICON,
                                   g_param_spec_object ("icon",
                                                        "Icon",
                                                        "Icon actor.",
                                                        CLUTTER_TYPE_ACTOR,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_STATIC_NAME |
                                                        G_PARAM_STATIC_NICK |
                                                        G_PARAM_STATIC_BLURB));

  g_object_class_install_property (object_class,
                                   PROP_URI,
                                   g_param_spec_string ("uri",
                                                        "URI",
                                                        "URI to display when "
                                                        "the user is not "
                                                        "the text.",
                                                        "",
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_STATIC_NAME |
                                                        G_PARAM_STATIC_NICK |
                                                        G_PARAM_STATIC_BLURB));

  g_object_class_install_property (object_class,
                                   PROP_TITLE,
                                   g_param_spec_string ("title",
                                                        "Title",
                                                        "RadicalBar title.",
                                                        "",
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_STATIC_NAME |
                                                        G_PARAM_STATIC_NICK |
                                                        G_PARAM_STATIC_BLURB));

  g_object_class_install_property (object_class,
                                   PROP_LOADING,
                                   g_param_spec_boolean ("loading",
                                                         "Loading",
                                                         "Loading.",
                                                         FALSE,
                                                         G_PARAM_READWRITE |
                                                         G_PARAM_STATIC_NAME |
                                                         G_PARAM_STATIC_NICK |
                                                         G_PARAM_STATIC_BLURB));

  g_object_class_install_property (object_class,
                                   PROP_PROGRESS,
                                   g_param_spec_double ("progress",
                                                        "Progress",
                                                        "Load progress.",
                                                        0.0, 1.0, 0.0,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_STATIC_NAME |
                                                        G_PARAM_STATIC_NICK |
                                                        G_PARAM_STATIC_BLURB));

  g_object_class_install_property (object_class,
                                   PROP_FAKE_PROGRESS,
                                   g_param_spec_double ("fake-progress",
                                                        "Fake Progress",
                                                        "Fake load progress.",
                                                        0.0, 1.0, 0.0,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_STATIC_NAME |
                                                        G_PARAM_STATIC_NICK |
                                                        G_PARAM_STATIC_BLURB));

  g_object_class_install_property (object_class,
                                   PROP_PINNED,
                                   g_param_spec_boolean ("pinned",
                                                         "Pinned",
                                                         "Sets whether the pin "
                                                         "icon should be "
                                                         "pushed in.",
                                                         FALSE,
                                                         G_PARAM_READWRITE |
                                                         G_PARAM_STATIC_NAME |
                                                         G_PARAM_STATIC_NICK |
                                                         G_PARAM_STATIC_BLURB));

  g_object_class_install_property (object_class,
                                   PROP_SHOW_PIN,
                                   g_param_spec_boolean ("show-pin",
                                                         "Show pin",
                                                         "Sets whether to "
                                                         "show the pinned.",
                                                         FALSE,
                                                         G_PARAM_READWRITE |
                                                         G_PARAM_STATIC_NAME |
                                                         G_PARAM_STATIC_NICK |
                                                         G_PARAM_STATIC_BLURB));

  signals[GO] =
    g_signal_new ("go",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (MwbRadicalBarClass, go),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__STRING,
                  G_TYPE_NONE, 1, G_TYPE_STRING);

  signals[RELOAD] =
    g_signal_new ("reload",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (MwbRadicalBarClass, reload),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  signals[STOP] =
    g_signal_new ("stop",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (MwbRadicalBarClass, stop),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  signals[PIN_BUTTON_CLICKED] =
    g_signal_new ("pin-button-clicked",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (MwbRadicalBarClass, pin_button_clicked),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);
}

static void
mwb_radical_bar_activate_uri (MwbRadicalBar *self, const gchar *uri)
{
  mwb_radical_bar_set_uri (self, uri);
  mwb_radical_bar_show_uri (self);

  g_signal_emit (self, signals[GO], 0, uri);
}

static void
mwb_radical_bar_button_clicked_cb (NbtkButton *button, MwbRadicalBar *self)
{
  MwbRadicalBarPrivate *priv = self->priv;
  
  if (priv->loading)
    g_signal_emit (self, signals[STOP], 0);
  else
    {
      if (priv->text_changed)
        {
          const gchar *text = nbtk_entry_get_text (NBTK_ENTRY (priv->entry));
          mwb_radical_bar_activate_uri (self, text);
        }
      else
        g_signal_emit (self, signals[RELOAD], 0);
    }
}

static void
mwb_radical_bar_pin_button_clicked_cb (NbtkButton *button, MwbRadicalBar *self)
{
  g_signal_emit (self, signals[PIN_BUTTON_CLICKED], 0);
}

static void
mwb_radical_bar_update_pin_button (MwbRadicalBar *self)
{
  MwbRadicalBarPrivate *priv = self->priv;

  if (priv->show_pin)
    clutter_actor_show (CLUTTER_ACTOR (priv->pin_button));
  else
    clutter_actor_hide (CLUTTER_ACTOR (priv->pin_button));

  clutter_actor_set_name (CLUTTER_ACTOR (priv->pin_button),
                          priv->pinned ?
                          "pin-button-pinned" :
                          "pin-button-unpinned");
}

static void
mwb_radical_bar_text_changed_cb (ClutterText *text, MwbRadicalBar *self)
{
  MwbRadicalBarPrivate *priv = self->priv;

  if (!priv->text_changed)
    {
      priv->text_changed = TRUE;
      mwb_radical_bar_update_button (self);
    }

  if (CLUTTER_ACTOR_IS_VISIBLE (CLUTTER_ACTOR (priv->ac_list)))
    {
      const gchar *text = nbtk_entry_get_text (NBTK_ENTRY (priv->entry));
      mwb_ac_list_set_search_text (MWB_AC_LIST (priv->ac_list), text);
    }
}

static void
mwb_radical_bar_activate_cb (ClutterText *text, MwbRadicalBar *self)
{
  MwbRadicalBarPrivate *priv = self->priv;
  gint selection;

  selection = mwb_ac_list_get_selection (MWB_AC_LIST (priv->ac_list));

  if (CLUTTER_ACTOR_IS_VISIBLE (CLUTTER_ACTOR (priv->ac_list))
      && selection >= 0)
    {
      gchar *url = mwb_ac_list_get_entry_url (MWB_AC_LIST (priv->ac_list),
                                              selection);
      mwb_radical_bar_activate_uri (self, url);
      g_free (url);
    }
  else if (priv->text_changed)
    mwb_radical_bar_activate_uri (self, clutter_text_get_text (text));
  else
    g_signal_emit (self, signals[RELOAD], 0);

  mwb_radical_bar_set_show_auto_complete (self, FALSE);
}

static gboolean
mwb_radical_bar_key_press_event_cb (ClutterText *text,
                                    ClutterKeyEvent *event,
                                    MwbRadicalBar *self)
{
  if (event->unicode_value ||
      (event->keyval == CLUTTER_Down) ||
      (event->keyval == CLUTTER_Up))
    mwb_radical_bar_set_show_auto_complete (self, TRUE);

  return FALSE;
}

static void
mwb_radical_bar_set_show_title_state (MwbRadicalBar *self,
                                      MwbRadicalBarShowTitleState state)
{
  MwbRadicalBarPrivate *priv = self->priv;

  /* Only change the spindle position if the state has changed to or
     from zero */
  if (!!priv->show_title_state != !!state)
    clutter_actor_animate (priv->spindle, CLUTTER_EASE_OUT_SINE, 250,
                           "position", state ? 1.0 : 0.0,
                           NULL);

  priv->show_title_state = state;
}

static void
mwb_radical_bar_key_focus_in_cb (NbtkEntry *entry,
                                 MwbRadicalBar *self)
{
  MwbRadicalBarPrivate *priv = self->priv;

  mwb_radical_bar_set_show_title_state (self,
                                        priv->show_title_state |
                                        MWB_RADICAL_BAR_ENTRY_HAS_FOCUS);
}

static void
mwb_radical_bar_key_focus_out_cb (NbtkEntry *entry,
                                  MwbRadicalBar *self)
{
  MwbRadicalBarPrivate *priv = self->priv;

  mwb_radical_bar_set_show_title_state (self,
                                        priv->show_title_state &
                                        ~MWB_RADICAL_BAR_ENTRY_HAS_FOCUS);

  mwb_radical_bar_set_show_auto_complete (self, FALSE);
  mwb_radical_bar_update_instructions (self);
}

static void
mwb_radical_bar_enter_cb (ClutterActor *actor,
                          ClutterCrossingEvent *event,
                          MwbRadicalBar *self)
{
  MwbRadicalBarPrivate *priv = self->priv;
  MwbRadicalBarShowTitleState new_state = priv->show_title_state;

  if (actor == priv->spindle)
    new_state |= MWB_RADICAL_BAR_MOUSE_OVER_SPINDLE;
  else if (actor == (gpointer) priv->entry)
    new_state |= MWB_RADICAL_BAR_MOUSE_OVER_ENTRY;
  else
    /* This signal handler is only connected for these three actors so
       we can assume if we make it here then the ClutterText has been
       entered */
    new_state |= MWB_RADICAL_BAR_MOUSE_OVER_TEXT;

  mwb_radical_bar_set_show_title_state (self, new_state);
}

static void
mwb_radical_bar_leave_cb (ClutterActor *actor,
                          ClutterCrossingEvent *event,
                          MwbRadicalBar *self)
{
  MwbRadicalBarPrivate *priv = self->priv;
  MwbRadicalBarShowTitleState new_state = priv->show_title_state;

  if (actor == priv->spindle)
    new_state &= ~MWB_RADICAL_BAR_MOUSE_OVER_SPINDLE;
  else if (actor == (gpointer) priv->entry)
    new_state &= ~MWB_RADICAL_BAR_MOUSE_OVER_ENTRY;
  else
    /* This signal handler is only connected for these three actors so
       we can assume if we make it here then the ClutterText has been
       left */
    new_state &= ~MWB_RADICAL_BAR_MOUSE_OVER_TEXT;

  mwb_radical_bar_set_show_title_state (self, new_state);
}

static gboolean
mwb_radical_bar_spindle_button_press_cb (ClutterActor *spindle,
                                         ClutterButtonEvent *event,
                                         MwbRadicalBar *self)
{
  MwbRadicalBarPrivate *priv = self->priv;

  /* Give focus to the entry when the label is clicked */
  clutter_actor_grab_key_focus (CLUTTER_ACTOR (priv->entry));

  return FALSE;
}

static void
mwb_radical_bar_ac_list_activate_cb (MwbAcList *ac_list,
                                     MwbRadicalBar *self)
{
  MwbRadicalBarPrivate *priv = self->priv;
  gint selection;

  selection = mwb_ac_list_get_selection (MWB_AC_LIST (priv->ac_list));

  if (selection >= 0)
    {
      gchar *url = mwb_ac_list_get_entry_url (MWB_AC_LIST (priv->ac_list),
                                              selection);
      mwb_radical_bar_activate_uri (self, url);
      g_free (url);

      mwb_radical_bar_set_show_auto_complete (self, FALSE);
    }
}

static void
mwb_radical_bar_init (MwbRadicalBar *self)
{
  ClutterActor *text, *texture, *separator;

  MwbRadicalBarPrivate *priv = self->priv = RADICAL_BAR_PRIVATE (self);

  priv->table = nbtk_table_new ();
  priv->entry = nbtk_entry_new ("");
  priv->title = nbtk_label_new ("");
  clutter_actor_set_name (CLUTTER_ACTOR (priv->title),
                          "radical-bar-title");
  priv->instructions = nbtk_label_new ("");
  clutter_actor_set_name (CLUTTER_ACTOR (priv->instructions),
                          "radical-bar-instructions");
  priv->button = nbtk_button_new ();
  priv->pin_button = nbtk_button_new ();
  priv->lock_icon = nbtk_button_new ();
  priv->spinner = clutter_texture_new_from_file (PKGDATADIR "/spinner.png",
                                                 NULL);
  clutter_actor_hide (priv->spinner);

  clutter_actor_set_parent (CLUTTER_ACTOR (priv->table), CLUTTER_ACTOR (self));

  texture = clutter_texture_new_from_file (PKGDATADIR "/fail_red.png", NULL);
  priv->broken_highlight =
    nbtk_texture_frame_new (CLUTTER_TEXTURE (texture),
                            5, 5, 5, 5);
  clutter_actor_set_parent (priv->broken_highlight, CLUTTER_ACTOR (self));
  clutter_actor_hide (priv->broken_highlight);

  texture = clutter_texture_new_from_file (PKGDATADIR "/encrypted_yellow.png",
                                           NULL);
  priv->encrypted_highlight =
    nbtk_texture_frame_new (CLUTTER_TEXTURE (texture), 5, 5, 5, 5);
  clutter_actor_set_parent (priv->encrypted_highlight, CLUTTER_ACTOR (self));
  clutter_actor_hide (priv->encrypted_highlight);

  texture = clutter_texture_new_from_file (PKGDATADIR "/progress.png", NULL);
  priv->progress_bar =
    nbtk_texture_frame_new (CLUTTER_TEXTURE (texture),
                            5, 5, 5, 5);
  clutter_actor_set_parent (priv->progress_bar, CLUTTER_ACTOR (self));
  clutter_actor_hide (priv->progress_bar);

  priv->spindle = mwb_spindle_new ();
  clutter_actor_set_reactive (priv->spindle, TRUE);
  clutter_container_add (CLUTTER_CONTAINER (priv->spindle),
                         CLUTTER_ACTOR (priv->title),
                         CLUTTER_ACTOR (priv->entry),
                         /* Add a dummy actor to make the spindle triangular */
                         clutter_rectangle_new (),
                         NULL);
  priv->show_title_state = 0;

  text = nbtk_entry_get_clutter_text (NBTK_ENTRY (priv->entry));

  mwb_radical_bar_update_button (self);
  mwb_radical_bar_update_pin_button (self);

  g_signal_connect (priv->button, "clicked",
                    G_CALLBACK (mwb_radical_bar_button_clicked_cb), self);
  g_signal_connect (priv->pin_button, "clicked",
                    G_CALLBACK (mwb_radical_bar_pin_button_clicked_cb), self);

  g_signal_connect (text, "text-changed",
                    G_CALLBACK (mwb_radical_bar_text_changed_cb), self);
  g_signal_connect (text, "activate",
                    G_CALLBACK (mwb_radical_bar_activate_cb), self);
  g_signal_connect (text, "key-press-event",
                    G_CALLBACK (mwb_radical_bar_key_press_event_cb), self);
  g_signal_connect (text, "key-focus-in",
                    G_CALLBACK (mwb_radical_bar_key_focus_in_cb), self);
  g_signal_connect (text, "key-focus-out",
                    G_CALLBACK (mwb_radical_bar_key_focus_out_cb), self);
  g_signal_connect (priv->spindle, "button-press-event",
                    G_CALLBACK (mwb_radical_bar_spindle_button_press_cb), self);
  g_signal_connect (text, "enter-event",
                    G_CALLBACK (mwb_radical_bar_enter_cb), self);
  g_signal_connect (priv->entry, "enter-event",
                    G_CALLBACK (mwb_radical_bar_enter_cb), self);
  g_signal_connect (priv->spindle, "enter-event",
                    G_CALLBACK (mwb_radical_bar_enter_cb), self);
  g_signal_connect (text, "leave-event",
                    G_CALLBACK (mwb_radical_bar_leave_cb), self);
  g_signal_connect (priv->entry, "leave-event",
                    G_CALLBACK (mwb_radical_bar_leave_cb), self);
  g_signal_connect (priv->spindle, "leave-event",
                    G_CALLBACK (mwb_radical_bar_leave_cb), self);

  clutter_actor_set_reactive (text, TRUE);
  g_signal_connect (text, "button-press-event",
                    G_CALLBACK (mwb_utils_focus_on_click_cb),
                    GINT_TO_POINTER (FALSE));

  mwb_utils_table_add (NBTK_TABLE (priv->table), priv->spinner, 0, 0,
                       FALSE, FALSE, FALSE, FALSE);

  mwb_utils_table_add (NBTK_TABLE (priv->table), CLUTTER_ACTOR (priv->spindle),
                       0, 1, TRUE, TRUE, TRUE, TRUE);

  mwb_utils_table_add (NBTK_TABLE (priv->table),
                       CLUTTER_ACTOR (priv->lock_icon),
                       0, 2, FALSE, FALSE, FALSE, FALSE);

  mwb_utils_table_add (NBTK_TABLE (priv->table),
                       CLUTTER_ACTOR (priv->pin_button),
                       0, 3, FALSE, FALSE, FALSE, FALSE);

  mwb_utils_table_add (NBTK_TABLE (priv->table),
                       CLUTTER_ACTOR (priv->instructions), 0, 4, FALSE, TRUE,
                       FALSE, TRUE);

  separator = clutter_texture_new_from_file (PKGDATADIR "/entry-separator.png",
                                             NULL);
  mwb_utils_table_add (NBTK_TABLE (priv->table), separator, 0, 5, FALSE, FALSE,
                       FALSE, FALSE);
  mwb_utils_table_add (NBTK_TABLE (priv->table), CLUTTER_ACTOR (priv->button),
                       0, 6, FALSE, FALSE, FALSE, FALSE);

  priv->ac_list = mwb_ac_list_new ();
  priv->ac_list_activate_handler
    = g_signal_connect (priv->ac_list, "activate",
                        G_CALLBACK (mwb_radical_bar_ac_list_activate_cb),
                        self);
  clutter_actor_set_parent (CLUTTER_ACTOR (priv->ac_list),
                            CLUTTER_ACTOR (self));
  clutter_actor_hide (CLUTTER_ACTOR (priv->ac_list));

  mwb_radical_bar_update_instructions (self);
}

NbtkWidget*
mwb_radical_bar_new (void)
{
  return NBTK_WIDGET (g_object_new (MWB_TYPE_RADICAL_BAR, NULL));
}

void
mwb_radical_bar_focus (MwbRadicalBar *radical_bar)
{
  ClutterActor *text;
  MwbRadicalBarPrivate *priv = radical_bar->priv;
  
  text = nbtk_entry_get_clutter_text (NBTK_ENTRY (priv->entry));
  mwb_utils_focus_on_click_cb (text, NULL, GINT_TO_POINTER (TRUE));
  mwb_radical_bar_select_all (radical_bar);
}

void
mwb_radical_bar_set_uri (MwbRadicalBar *radical_bar, const gchar *uri)
{
  MwbRadicalBarPrivate *priv = radical_bar->priv;

  /* Store the URL regardless of whether we're going to update the
     entry so that we can use it later if the user presses Escape */
  g_free (priv->uri);
  priv->uri = g_strdup (uri);

  /* Only update the entry if the user wasn't in the middle of
     typing */
  if (!priv->text_changed)
    mwb_radical_bar_show_uri (radical_bar);

  g_object_notify (G_OBJECT (radical_bar), "uri");
}

void
mwb_radical_bar_set_title (MwbRadicalBar *radical_bar, const gchar *title)
{
  MwbRadicalBarPrivate *priv = radical_bar->priv;

  if (!title)
    title = "";

  nbtk_label_set_text (NBTK_LABEL (priv->title), title);

  g_object_notify (G_OBJECT (radical_bar), "title");
}

void
mwb_radical_bar_set_default_icon (MwbRadicalBar *radical_bar,
                                  ClutterActor  *icon)
{
  MwbRadicalBarPrivate *priv = radical_bar->priv;
  gboolean changed = !priv->icon || (priv->icon == priv->default_icon);

  if (icon)
    g_object_ref_sink (icon);

  if (priv->default_icon)
    g_object_unref (priv->default_icon);

  priv->default_icon = icon;

  if (changed)
    mwb_radical_bar_set_icon (radical_bar, NULL);
}

void
mwb_radical_bar_set_icon (MwbRadicalBar *radical_bar, ClutterActor *icon)
{
  MwbRadicalBarPrivate *priv = radical_bar->priv;

  /* passing NULL for icon will use default icon if available */

  if (priv->icon)
    clutter_container_remove_actor (CLUTTER_CONTAINER (priv->table),
                                    priv->icon);

  if (icon)
    priv->icon = icon;
  else
    priv->icon = priv->default_icon;

  if (priv->icon)
    mwb_utils_table_add (NBTK_TABLE (priv->table), priv->icon, 0, 0,
                         FALSE, FALSE, FALSE, FALSE);

  if (priv->loading && priv->icon)
    clutter_actor_hide (priv->icon);

  g_object_notify (G_OBJECT (radical_bar), "icon");
}

void
mwb_radical_bar_set_loading (MwbRadicalBar *radical_bar, gboolean loading)
{
  ClutterActorBox box;
  MwbRadicalBarPrivate *priv = radical_bar->priv;

  if (priv->loading == loading)
    return;

  priv->loading = loading;
  priv->fake_progress = priv->progress = 0;

  if (loading)
    {
      ClutterAnimation *anim;

      clutter_actor_show (priv->progress_bar);

      /* Enable spinner */
      if (priv->icon)
        clutter_actor_hide (priv->icon);

      clutter_actor_show (priv->spinner);
      clutter_actor_set_z_rotation_from_gravity (priv->spinner,
                                                 0.0,
                                                 CLUTTER_GRAVITY_CENTER);
      anim = clutter_actor_animate (priv->spinner, CLUTTER_LINEAR, 1000,
                                    "rotation-angle-z", 360.0,
                                    NULL);
      clutter_timeline_set_loop (clutter_animation_get_timeline (anim), TRUE);
    }
  else
    {
      clutter_actor_hide (priv->progress_bar);
      mwb_radical_bar_stop_fake_progress (radical_bar);

      /* Disable spinner */
      if (priv->icon)
        clutter_actor_show (priv->icon);

      clutter_actor_hide (priv->spinner);
      clutter_timeline_stop (
        clutter_animation_get_timeline (
          clutter_actor_get_animation (priv->spinner)));
    }

  clutter_actor_get_allocation_box (CLUTTER_ACTOR (radical_bar), &box);
  mwb_radical_bar_resize_progress (radical_bar, &box, 0);
  mwb_radical_bar_update_button (radical_bar);

  g_object_notify (G_OBJECT (radical_bar), "loading");
}

void
mwb_radical_bar_set_progress (MwbRadicalBar *radical_bar, gdouble progress)
{
  MwbRadicalBarPrivate *priv = radical_bar->priv;

  if ((priv->progress == progress) && priv->fake_progress_anim)
    return;

  /* FIXME: the numbers here for the fake progress indication are completely
   * arbitrary. They're just values that worked nicely on my system. We
   * ought to derive them somehow via some heuristic/algorithm, really...
   */
  if (progress > 0.0)
    {
      priv->fake_progress = priv->progress = CLAMP (progress, 0.0, 1.0);
      mwb_radical_bar_resize_progress (radical_bar, NULL, 0);
      g_object_notify (G_OBJECT (radical_bar), "progress");
    }
  else if (priv->progress > priv->fake_progress)
    priv->fake_progress = priv->progress;

  mwb_radical_bar_stop_fake_progress (radical_bar);

  if (priv->fake_progress < 0.9)
    {
      ClutterTimeline *timeline;
      ClutterInterval *interval;

      priv->fake_progress_anim = clutter_animation_new ();

      clutter_animation_set_object (priv->fake_progress_anim,
                                    G_OBJECT (radical_bar));
      clutter_animation_set_mode (priv->fake_progress_anim, CLUTTER_LINEAR);
      clutter_animation_set_duration (priv->fake_progress_anim, 30000);

      interval = clutter_interval_new (G_TYPE_DOUBLE, priv->fake_progress, 0.9);
      clutter_animation_bind_interval (priv->fake_progress_anim,
                                       "fake-progress",
                                       interval);

      timeline = clutter_animation_get_timeline (priv->fake_progress_anim);
      /*clutter_timeline_set_delay (timeline, 500);*/
      clutter_timeline_start (timeline);
    }

  clutter_actor_queue_redraw (CLUTTER_ACTOR (radical_bar));
}

void
mwb_radical_bar_set_pinned (MwbRadicalBar *radical_bar, gboolean pinned)
{
  MwbRadicalBarPrivate *priv = radical_bar->priv;

  if (priv->pinned != pinned)
    {
      priv->pinned = pinned;
      mwb_radical_bar_update_pin_button (radical_bar);
      g_object_notify (G_OBJECT (radical_bar), "pinned");
    }
}

void
mwb_radical_bar_set_show_pin (MwbRadicalBar *radical_bar, gboolean show_pin)
{
  MwbRadicalBarPrivate *priv = radical_bar->priv;

  if (priv->show_pin != show_pin)
    {
      priv->show_pin = show_pin;
      mwb_radical_bar_update_pin_button (radical_bar);
      g_object_notify (G_OBJECT (radical_bar), "show-pin");
    }
}

const gchar *
mwb_radical_bar_get_uri (MwbRadicalBar *radical_bar)
{
  return radical_bar->priv->uri;
}

const gchar *
mwb_radical_bar_get_title (MwbRadicalBar *radical_bar)
{
  return nbtk_label_get_text (NBTK_LABEL (radical_bar->priv->title));
}

ClutterActor *
mwb_radical_bar_get_icon (MwbRadicalBar *radical_bar)
{
  MwbRadicalBarPrivate *priv = radical_bar->priv;
  return priv->icon == priv->default_icon ? NULL : priv->icon;
}

gboolean
mwb_radical_bar_get_loading (MwbRadicalBar *radical_bar)
{
  return radical_bar->priv->loading;
}

gdouble
mwb_radical_bar_get_progress (MwbRadicalBar *radical_bar)
{
  return radical_bar->priv->progress;
}

gboolean
mwb_radical_bar_get_pinned (MwbRadicalBar *radical_bar)
{
  return radical_bar->priv->pinned;
}

gboolean
mwb_radical_bar_get_show_pin (MwbRadicalBar *radical_bar)
{
  return radical_bar->priv->show_pin;
}

guint
mwb_radical_bar_get_security (MwbRadicalBar *radical_bar)
{
  return radical_bar->priv->security;
}

MwbAcList *
mwb_radical_bar_get_ac_list (MwbRadicalBar *radical_bar)
{
  return MWB_AC_LIST (radical_bar->priv->ac_list);
}

