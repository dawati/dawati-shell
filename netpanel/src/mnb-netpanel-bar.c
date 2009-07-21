/* mwb-netpanel-bar.c */
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

/* Borrowed from the moblin-web-browser project */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <gdk/gdkx.h>

#include "mnb-netpanel-bar.h"
#include "mwb-utils.h"
#include "mwb-ac-list.h"

G_DEFINE_TYPE (MnbNetpanelBar, mnb_netpanel_bar, MPL_TYPE_ENTRY)

#define NETPANEL_BAR_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MNB_TYPE_NETPANEL_BAR, \
   MnbNetpanelBarPrivate))

enum
{
  GO,

  LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0, };

struct _MnbNetpanelBarPrivate
{
  ClutterActorBox  box;

  NbtkWidget      *ac_list;
  guint            ac_list_activate_handler;
  ClutterTimeline *ac_list_timeline;
  gdouble          ac_list_anim_progress;
};

static void
mnb_netpanel_bar_dispose (GObject *object)
{
  MnbNetpanelBarPrivate *priv = MNB_NETPANEL_BAR (object)->priv;

  if (priv->ac_list_timeline)
    {
      clutter_timeline_stop (priv->ac_list_timeline);
      g_object_unref (priv->ac_list_timeline);
      priv->ac_list_timeline = NULL;
    }

  if (priv->ac_list)
    {
      g_signal_handler_disconnect (priv->ac_list,
                                   priv->ac_list_activate_handler);
      clutter_actor_unparent (CLUTTER_ACTOR (priv->ac_list));
      priv->ac_list = NULL;
    }

  G_OBJECT_CLASS (mnb_netpanel_bar_parent_class)->dispose (object);
}

static void
mnb_netpanel_bar_finalize (GObject *object)
{
  G_OBJECT_CLASS (mnb_netpanel_bar_parent_class)->finalize (object);
}

static void
mnb_netpanel_bar_allocate (ClutterActor           *actor,
                           const ClutterActorBox  *box,
                           ClutterAllocationFlags  flags)
{
  ClutterActorBox child_box;
  gfloat ac_preferred_height;

  MnbNetpanelBar *self = MNB_NETPANEL_BAR (actor);
  MnbNetpanelBarPrivate *priv = self->priv;

  CLUTTER_ACTOR_CLASS (mnb_netpanel_bar_parent_class)->
    allocate (actor, box, flags);

  priv->box = *box;

  if (priv->ac_list)
    {
      /* remove padding */
      child_box.x1 = 0;
      child_box.x2 = box->x2 - box->x1;
      clutter_actor_get_preferred_height (CLUTTER_ACTOR (priv->ac_list),
                                          child_box.x2 - child_box.x1,
                                          NULL,
                                          &ac_preferred_height);
      child_box.y1 = box->y2 - box->y1;
      child_box.y2 = child_box.y1 + ac_preferred_height;

      clutter_actor_allocate (CLUTTER_ACTOR (priv->ac_list), &child_box, flags);
    }
}

static void
mnb_netpanel_bar_paint (ClutterActor *actor)
{
  MnbNetpanelBarPrivate *priv = MNB_NETPANEL_BAR (actor)->priv;

  /* Chain up to get the background */
  CLUTTER_ACTOR_CLASS (mnb_netpanel_bar_parent_class)->paint (actor);

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
mnb_netpanel_bar_pick (ClutterActor *actor, const ClutterColor *color)
{
  mnb_netpanel_bar_paint (actor);
}

static void
mnb_netpanel_bar_ac_list_new_frame_cb (ClutterTimeline *timeline,
                                       guint            msecs,
                                       MnbNetpanelBar  *self)
{
  MnbNetpanelBarPrivate *priv = self->priv;
  priv->ac_list_anim_progress = clutter_timeline_get_progress (timeline);
  clutter_actor_queue_redraw (CLUTTER_ACTOR (self));
}

static void
mnb_netpanel_bar_ac_list_completed_cb (ClutterTimeline *timeline,
                                       MnbNetpanelBar  *self)
{
  MnbNetpanelBarPrivate *priv = self->priv;

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

  clutter_actor_queue_redraw (CLUTTER_ACTOR (self));
}

static void
mnb_netpanel_bar_set_show_auto_complete (MnbNetpanelBar *self,
                                         gboolean        show)
{
  MnbNetpanelBarPrivate *priv = self->priv;

  if (CLUTTER_ACTOR_IS_VISIBLE (priv->ac_list) != show)
    {
      if (!priv->ac_list_timeline)
        {
          priv->ac_list_timeline = clutter_timeline_new (150);
          g_signal_connect (priv->ac_list_timeline, "new-frame",
                            G_CALLBACK (mnb_netpanel_bar_ac_list_new_frame_cb),
                            self);
          g_signal_connect (priv->ac_list_timeline, "completed",
                            G_CALLBACK (mnb_netpanel_bar_ac_list_completed_cb),
                            self);
        }

      if (show)
        {
          const gchar *text = mpl_entry_get_text (MPL_ENTRY (self));
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
    }
}

static void
mnb_netpanel_bar_activate_cb (GObject *obj, MnbNetpanelBar *self);

static void
mnb_netpanel_bar_complete_url (MnbNetpanelBar *self,
                               const char     *suffix)
{
  const int MAX_LABEL = 63;  /* from RFC 1035 */
  char buf[MAX_LABEL + 8 + 1];
  const char *text;

  if (!suffix || (strlen (suffix) != 3))
    return;

  text = mpl_entry_get_text (MPL_ENTRY (self));

  if (!strncmp (text, "www.", 4) || strchr (text, ':'))
    return;

  if (strlen (text) <= MAX_LABEL) {
    sprintf (buf, "www.%s.%s", text, suffix);
    mpl_entry_set_text (MPL_ENTRY (self), buf);
    mnb_netpanel_bar_activate_cb (NULL, self);
  }
}

static gboolean
mnb_netpanel_bar_captured_event (ClutterActor *actor,
                                 ClutterEvent *event)
{
  ClutterKeyEvent *key_event;
  MnbNetpanelBar *self = MNB_NETPANEL_BAR (actor);
  MnbNetpanelBarPrivate *priv = self->priv;

  if (event->type != CLUTTER_KEY_PRESS)
    return FALSE;
  key_event = (ClutterKeyEvent *)event;

  if (CLUTTER_ACTOR_IS_VISIBLE (CLUTTER_ACTOR (priv->ac_list)))
    switch (key_event->keyval)
      {
      case CLUTTER_Escape:
        mnb_netpanel_bar_set_show_auto_complete (self, FALSE);
        return TRUE;

      case CLUTTER_Return:
        if ((key_event->modifier_state & MWB_UTILS_MODIFIERS_MASK) ==
            CLUTTER_CONTROL_MASK)
          mnb_netpanel_bar_complete_url (self, "com");
        else if ((key_event->modifier_state & MWB_UTILS_MODIFIERS_MASK) ==
                 (CLUTTER_CONTROL_MASK | CLUTTER_SHIFT_MASK))
          mnb_netpanel_bar_complete_url (self, "org");
        else if ((key_event->modifier_state & MWB_UTILS_MODIFIERS_MASK) ==
                 CLUTTER_SHIFT_MASK)
          mnb_netpanel_bar_complete_url (self, "net");
        else if (!key_event->modifier_state)
          mnb_netpanel_bar_activate_cb (NULL, self);
        return TRUE;

      case CLUTTER_Down:
      case CLUTTER_Up:
        {
          gint selection =
            mwb_ac_list_get_selection (MWB_AC_LIST (priv->ac_list));

          if (selection < 0)
            mwb_ac_list_set_selection (MWB_AC_LIST (priv->ac_list), 0);
          else if (key_event->keyval == CLUTTER_Down)
            {
              if (selection <
                  mwb_ac_list_get_n_entries (MWB_AC_LIST (priv->ac_list)) - 1)
                {
                  mwb_ac_list_set_selection (MWB_AC_LIST (priv->ac_list),
                                             selection + 1);
                  return TRUE;
                }
            }
          else if (selection > 0)
            {
              mwb_ac_list_set_selection (MWB_AC_LIST (priv->ac_list),
                                         selection - 1);
              return TRUE;
            }
        }
        break;
      }

  if (CLUTTER_ACTOR_CLASS (mnb_netpanel_bar_parent_class)->captured_event)
    return CLUTTER_ACTOR_CLASS (mnb_netpanel_bar_parent_class)->
      captured_event (actor, event);

  return FALSE;
}

static void
mnb_netpanel_bar_map (ClutterActor *actor)
{
  MnbNetpanelBarPrivate *priv = MNB_NETPANEL_BAR (actor)->priv;

  CLUTTER_ACTOR_CLASS (mnb_netpanel_bar_parent_class)->map (actor);

  clutter_actor_map (CLUTTER_ACTOR (priv->ac_list));
}

static void
mnb_netpanel_bar_unmap (ClutterActor *actor)
{
  MnbNetpanelBarPrivate *priv = MNB_NETPANEL_BAR (actor)->priv;

  CLUTTER_ACTOR_CLASS (mnb_netpanel_bar_parent_class)->unmap (actor);

  clutter_actor_unmap (CLUTTER_ACTOR (priv->ac_list));
}

static void
mnb_netpanel_bar_go (MnbNetpanelBar *self, const gchar *url)
{
  MnbNetpanelBarPrivate *priv = self->priv;

  if (priv->ac_list)
    mwb_ac_list_increment_tld_score_for_url (MWB_AC_LIST (priv->ac_list), url);
}

static void
mnb_netpanel_bar_class_init (MnbNetpanelBarClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  ClutterActorClass *actor_class = CLUTTER_ACTOR_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MnbNetpanelBarPrivate));

  object_class->dispose = mnb_netpanel_bar_dispose;
  object_class->finalize = mnb_netpanel_bar_finalize;

  actor_class->allocate = mnb_netpanel_bar_allocate;
  actor_class->paint = mnb_netpanel_bar_paint;
  actor_class->pick = mnb_netpanel_bar_pick;
  actor_class->captured_event = mnb_netpanel_bar_captured_event;
  actor_class->map = mnb_netpanel_bar_map;
  actor_class->unmap = mnb_netpanel_bar_unmap;

  klass->go = mnb_netpanel_bar_go;

  signals[GO] =
    g_signal_new ("go",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (MnbNetpanelBarClass, go),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__STRING,
                  G_TYPE_NONE, 1, G_TYPE_STRING);
}

static void
mnb_netpanel_bar_select_all (MnbNetpanelBar *self)
{
  NbtkWidget *entry;
  ClutterActor *actor;
  gint length;

  entry = mpl_entry_get_nbtk_entry (MPL_ENTRY (self));
  actor = nbtk_entry_get_clutter_text (NBTK_ENTRY (entry));
  length = strlen (clutter_text_get_text (CLUTTER_TEXT (actor)));
  clutter_text_set_selection (CLUTTER_TEXT (actor), 0, length);
}

static void
mnb_netpanel_bar_text_changed_cb (GObject        *obj,
                                  MnbNetpanelBar *self)
{
  MnbNetpanelBarPrivate *priv = self->priv;

  if (CLUTTER_ACTOR_IS_VISIBLE (CLUTTER_ACTOR (priv->ac_list)))
    {
      const gchar *text = mpl_entry_get_text (MPL_ENTRY (self));
      mwb_ac_list_set_search_text (MWB_AC_LIST (priv->ac_list), text);
    }
}

static void
mnb_netpanel_bar_activate_cb (GObject        *obj,
                              MnbNetpanelBar *self)
{
  MnbNetpanelBarPrivate *priv = self->priv;
  gint selection;

  selection = mwb_ac_list_get_selection (MWB_AC_LIST (priv->ac_list));

  if (CLUTTER_ACTOR_IS_VISIBLE (CLUTTER_ACTOR (priv->ac_list))
      && selection >= 0)
    {
      gchar *url = mwb_ac_list_get_entry_url (MWB_AC_LIST (priv->ac_list),
                                              selection);
      g_signal_emit (self, signals[GO], 0, url);
      g_free (url);
    }
  else
    g_signal_emit (self, signals[GO], 0, mpl_entry_get_text (MPL_ENTRY (self)));

  mnb_netpanel_bar_set_show_auto_complete (self, FALSE);
}

static gboolean
mnb_netpanel_bar_key_press_event_cb (GObject         *obj,
                                     ClutterKeyEvent *event,
                                     MnbNetpanelBar  *self)
{
  mnb_netpanel_bar_set_show_auto_complete (self, TRUE);
  return FALSE;
}

static void
mnb_netpanel_bar_key_focus_out_cb (GObject        *obj,
                                   MnbNetpanelBar *self)
{
  mnb_netpanel_bar_set_show_auto_complete (self, FALSE);
}

static void
mnb_netpanel_bar_ac_list_activate_cb (MwbAcList      *ac_list,
                                      MnbNetpanelBar *self)
{
  MnbNetpanelBarPrivate *priv = self->priv;
  gint selection;

  selection = mwb_ac_list_get_selection (MWB_AC_LIST (priv->ac_list));

  if (selection >= 0)
    {
      gchar *url = mwb_ac_list_get_entry_url (MWB_AC_LIST (priv->ac_list),
                                              selection);
      g_signal_emit (self, signals[GO], 0, url);
      g_free (url);

      mnb_netpanel_bar_set_show_auto_complete (self, FALSE);
    }
}

static void
mnb_netpanel_bar_init (MnbNetpanelBar *self)
{
  MnbNetpanelBarPrivate *priv = self->priv = NETPANEL_BAR_PRIVATE (self);
  NbtkWidget *entry;
  ClutterActor *actor;

  entry = mpl_entry_get_nbtk_entry (MPL_ENTRY (self));
  actor = nbtk_entry_get_clutter_text (NBTK_ENTRY (entry));

  g_signal_connect (actor, "text-changed",
                    G_CALLBACK (mnb_netpanel_bar_text_changed_cb), self);
  g_signal_connect (actor, "activate",
                    G_CALLBACK (mnb_netpanel_bar_activate_cb), self);
  g_signal_connect (entry, "key-press-event",
                    G_CALLBACK (mnb_netpanel_bar_key_press_event_cb), self);
  g_signal_connect (entry, "key-focus-out",
                    G_CALLBACK (mnb_netpanel_bar_key_focus_out_cb), self);

  clutter_actor_set_reactive (actor, TRUE);
  g_signal_connect (actor, "button-press-event",
                    G_CALLBACK (mwb_utils_focus_on_click_cb),
                    GINT_TO_POINTER (FALSE));

  priv->ac_list = mwb_ac_list_new ();
  priv->ac_list_activate_handler
    = g_signal_connect (priv->ac_list, "activate",
                        G_CALLBACK (mnb_netpanel_bar_ac_list_activate_cb),
                        self);
  clutter_actor_set_parent (CLUTTER_ACTOR (priv->ac_list),
                            CLUTTER_ACTOR (self));
  clutter_actor_hide (CLUTTER_ACTOR (priv->ac_list));
}

NbtkWidget*
mnb_netpanel_bar_new (const char *label)
{
  return NBTK_WIDGET (g_object_new (MNB_TYPE_NETPANEL_BAR,
                                    "label", label,
                                    NULL));
}

void
mnb_netpanel_bar_focus (MnbNetpanelBar *self)
{
  NbtkWidget *entry;
  ClutterActor *actor;

  entry = mpl_entry_get_nbtk_entry (MPL_ENTRY (self));
  actor = nbtk_entry_get_clutter_text (NBTK_ENTRY (entry));
  mwb_utils_focus_on_click_cb (actor, NULL, GINT_TO_POINTER (TRUE));
  mnb_netpanel_bar_select_all (self);
}
