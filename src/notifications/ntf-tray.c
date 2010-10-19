/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */

/*
 * Copyright (c) 2010 Intel Corp.
 *
 * Author: Tomas Frydrych <tf@linux.intel.com>
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

#include <glib/gi18n.h>

#include "../meego-netbook.h"
#include "../mnb-input-manager.h"

#include "ntf-tray.h"
#include "mnb-notification-gtk.h"

#define CLUSTER_WIDTH 320
#define FADE_DURATION 300

static void ntf_tray_dismiss_all_cb (ClutterActor *button, NtfTray *tray);
static void ntf_tray_dispose (GObject *object);
static void ntf_tray_finalize (GObject *object);
static void ntf_tray_constructed (GObject *object);

G_DEFINE_TYPE (NtfTray, ntf_tray, MX_TYPE_WIDGET);

#define NTF_TRAY_GET_PRIVATE(o) \
(G_TYPE_INSTANCE_GET_PRIVATE ((o), NTF_TYPE_TRAY, NtfTrayPrivate))

struct _NtfTrayPrivate
{
  ClutterActor *notifiers;
  ClutterActor *control;
  ClutterActor *control_text;
  ClutterActor *lowlight;
  gint          n_notifiers;
  ClutterActor *active_notifier;

  ClutterActor *pending_removed;  /* notification pending removal on anim */
  gboolean      anim_lock;

  guint disposed : 1;
};

enum
{
  N_SIGNALS,
};

enum
{
  PROP_0,
};

/* static guint signals[N_SIGNALS] = {0}; */

static void
ntf_tray_paint (ClutterActor *actor)
{
  NtfTrayPrivate *priv = NTF_TRAY (actor)->priv;

  if (CLUTTER_ACTOR_IS_MAPPED (priv->control))
    clutter_actor_paint (CLUTTER_ACTOR(priv->control));

  if (priv->notifiers && CLUTTER_ACTOR_IS_MAPPED (priv->notifiers))
    clutter_actor_paint (CLUTTER_ACTOR(priv->notifiers));
}

static void
ntf_tray_pick (ClutterActor       *actor,
               const ClutterColor *color)
{
  CLUTTER_ACTOR_CLASS (ntf_tray_parent_class)->pick (actor, color);

  ntf_tray_paint (actor);
}

static void
ntf_tray_map (ClutterActor *actor)
{
  NtfTrayPrivate *priv = NTF_TRAY (actor)->priv;

  CLUTTER_ACTOR_CLASS (ntf_tray_parent_class)->map (actor);

  if (priv->notifiers)
    clutter_actor_map (CLUTTER_ACTOR (priv->notifiers));
}

static void
ntf_tray_unmap (ClutterActor *actor)
{
  NtfTrayPrivate *priv = NTF_TRAY (actor)->priv;

  CLUTTER_ACTOR_CLASS (ntf_tray_parent_class)->unmap (actor);

  if (CLUTTER_ACTOR_IS_MAPPED (priv->control))
    clutter_actor_unmap (CLUTTER_ACTOR (priv->control));

  if (priv->notifiers)
    clutter_actor_unmap (CLUTTER_ACTOR (priv->notifiers));
}

static void
ntf_tray_get_preferred_width (ClutterActor *actor,
                              gfloat        for_height,
                              gfloat       *min_width,
                              gfloat       *natural_width)
{
  *min_width     = CLUSTER_WIDTH;
  *natural_width = CLUSTER_WIDTH;
}

static void
ntf_tray_get_preferred_height (ClutterActor *actor,
                               gfloat        for_width,
                               gfloat       *min_height,
                               gfloat       *natural_height)
{
  NtfTrayPrivate *priv = NTF_TRAY (actor)->priv;

  *min_height = 0;
  *natural_height = 0;

  if (priv->notifiers)
    {
      gfloat m_height, p_height;

      clutter_actor_get_preferred_height (CLUTTER_ACTOR (priv->notifiers),
                                          CLUSTER_WIDTH, &m_height, &p_height);

      *min_height += m_height;
      *natural_height += p_height;
    }

  if (priv->control && CLUTTER_ACTOR_IS_MAPPED (priv->control))
    {
      gfloat m_height, p_height;

      clutter_actor_get_preferred_height (CLUTTER_ACTOR (priv->control),
                                          CLUSTER_WIDTH, &m_height, &p_height);

      *min_height += m_height - 30;
      *natural_height += p_height - 30;
    }
}

static void
ntf_tray_allocate (ClutterActor          *actor,
                   const ClutterActorBox *box,
                   ClutterAllocationFlags flags)
{
  NtfTrayPrivate    *priv = NTF_TRAY (actor)->priv;
  ClutterActorClass *klass;
  gfloat             m_height = 0.0, p_height = 0.0;

  klass = CLUTTER_ACTOR_CLASS (ntf_tray_parent_class);

  klass->allocate (actor, box, flags);

  if (priv->notifiers)
    {
      ClutterActorBox notifier_box = { 0, };

      clutter_actor_get_preferred_height (CLUTTER_ACTOR (priv->notifiers),
                                          CLUSTER_WIDTH, &m_height, &p_height);

      notifier_box.x2 = box->x2 - box->x1;
      notifier_box.y2 = p_height;

      clutter_actor_allocate (CLUTTER_ACTOR(priv->notifiers),
                              &notifier_box, flags);
    }

  if (priv->control && CLUTTER_ACTOR_IS_MAPPED (priv->control))
    {
      ClutterActorBox control_box;

      control_box.x1 = 0.0;
      control_box.y1 = p_height - 30;

      clutter_actor_get_preferred_height (CLUTTER_ACTOR (priv->control),
                                          CLUSTER_WIDTH, &m_height, &p_height);

      control_box.x2 = box->x2 - box->x1;
      control_box.y2 = control_box.y1 + p_height;

      clutter_actor_allocate (CLUTTER_ACTOR(priv->control),
                              &control_box, flags);
    }
}

static gboolean
ntf_tray_key_press_event (ClutterActor *actor, ClutterKeyEvent *event)
{
  NtfTray        *tray   = NTF_TRAY (actor);
  NtfTrayPrivate *priv   = tray->priv;
  gboolean        retval = FALSE;

  switch (event->keyval)
    {
    case CLUTTER_Escape:
      ntf_tray_dismiss_all_cb (NULL, tray);
      retval = TRUE;
      break;
    default:
      {
        GList *notifiers;
        GList *last;

        notifiers =
          clutter_container_get_children (CLUTTER_CONTAINER (priv->notifiers));

        last = g_list_last (notifiers);

        if (last)
          {
            NtfNotification *ntf = NTF_NOTIFICATION (last->data);

            ntf_notification_handle_key_event (ntf, event);
            retval = TRUE;
          }

        g_list_free (notifiers);
      }
      break;
    }

  return retval;
}

static void
ntf_tray_class_init (NtfTrayClass *klass)
{
  GObjectClass      *object_class = (GObjectClass *)klass;
  ClutterActorClass *actor_class  = CLUTTER_ACTOR_CLASS (klass);

  g_type_class_add_private (klass, sizeof (NtfTrayPrivate));

  object_class->dispose             = ntf_tray_dispose;
  object_class->finalize            = ntf_tray_finalize;
  object_class->constructed         = ntf_tray_constructed;

  actor_class->key_press_event      = ntf_tray_key_press_event;
  actor_class->paint                = ntf_tray_paint;
  actor_class->pick                 = ntf_tray_pick;
  actor_class->map                  = ntf_tray_map;
  actor_class->unmap                = ntf_tray_unmap;
  actor_class->allocate             = ntf_tray_allocate;
  actor_class->get_preferred_height = ntf_tray_get_preferred_height;
  actor_class->get_preferred_width  = ntf_tray_get_preferred_width;
}

static void
ntf_tray_dismiss_all_foreach (ClutterActor *notifier)
{
  g_signal_emit_by_name (notifier, "closed", 0);
}

static void
ntf_tray_dismiss_all_cb (ClutterActor *button, NtfTray *tray)
{
  NtfTrayPrivate *priv = tray->priv;

  clutter_actor_hide (CLUTTER_ACTOR (tray));

  clutter_container_foreach (CLUTTER_CONTAINER (priv->notifiers),
                             (ClutterCallback)ntf_tray_dismiss_all_foreach,
                             NULL);
}

static void
ntf_tray_constructed (GObject *object)
{
  NtfTray        *self = (NtfTray*) object;
  ClutterActor   *actor = (ClutterActor*) self;
  NtfTrayPrivate *priv = self->priv;
  ClutterActor   *button;

  if (G_OBJECT_CLASS (ntf_tray_parent_class)->constructed)
    G_OBJECT_CLASS (ntf_tray_parent_class)->constructed (object);

  priv->notifiers = clutter_group_new ();

  clutter_actor_set_parent (priv->notifiers, actor);

  /* 'Overflow' control */
  priv->control = mx_table_new ();

  mx_stylable_set_style_class (MX_STYLABLE (priv->control),
                               "notification-control");

  button = mx_button_new ();
  mx_button_set_label (MX_BUTTON (button), _("Dismiss All"));
  mx_table_add_actor (MX_TABLE (priv->control), button, 0, 1);

  g_signal_connect (button, "clicked",
                    G_CALLBACK (ntf_tray_dismiss_all_cb), self);

  priv->control_text = mx_label_new ();
  mx_table_add_actor (MX_TABLE (priv->control),
                        CLUTTER_ACTOR (priv->control_text), 0, 0);

  clutter_actor_set_width (priv->control, CLUSTER_WIDTH);

  clutter_actor_set_parent (priv->control, actor);

  clutter_actor_hide (priv->control);

  clutter_actor_set_reactive (priv->notifiers, TRUE);
  clutter_actor_set_reactive (actor, TRUE);

  mnb_input_manager_push_actor (actor, MNB_INPUT_LAYER_TOP);
}

static void
ntf_tray_init (NtfTray *self)
{
  self->priv = NTF_TRAY_GET_PRIVATE (self);
}

static void
ntf_tray_dispose (GObject *object)
{
  NtfTray        *self = (NtfTray*) object;
  NtfTrayPrivate *priv = self->priv;

  if (priv->disposed)
    return;

  priv->disposed = TRUE;

  G_OBJECT_CLASS (ntf_tray_parent_class)->dispose (object);
}

static void
ntf_tray_finalize (GObject *object)
{
  G_OBJECT_CLASS (ntf_tray_parent_class)->finalize (object);
}

static void
ntf_tray_hide_ntf_completed_cb (ClutterAnimation *anim, NtfTray *tray)
{
  NtfTrayPrivate *priv = tray->priv;
  ClutterActor   *actor = CLUTTER_ACTOR (clutter_animation_get_object (anim));

  clutter_actor_destroy (actor);

  /* Hide ourselves if nothing left to show */
  if (priv->n_notifiers == 0)
    clutter_actor_hide (CLUTTER_ACTOR (tray));
}

static void
ntf_tray_control_hide_completed_cb (ClutterAnimation *anim,
                                    NtfTray          *tray)
{
  NtfTrayPrivate *priv = tray->priv;

  clutter_actor_hide (priv->control);
}

static void
ntf_tray_notification_closed_cb (NtfNotification *ntf, NtfTray *tray)
{
  NtfTrayPrivate   *priv = tray->priv;
  ClutterActor     *ntfa = CLUTTER_ACTOR (ntf);
  ClutterAnimation *anim;

  priv->n_notifiers--;

  if (priv->n_notifiers < 0)
    {
      g_warning ("Bug in notifier accounting, attempting to fix");
      priv->n_notifiers = 0;
    }

  /* fade out closed notifier */
  if (ntfa == priv->active_notifier)
    {
      anim = clutter_actor_animate (CLUTTER_ACTOR (ntfa),
                                    CLUTTER_EASE_IN_SINE,
                                    FADE_DURATION,
                                    "opacity", 0,
                                    NULL);

      g_signal_connect_after (anim,
                        "completed",
                        G_CALLBACK
                        (ntf_tray_hide_ntf_completed_cb),
                        tray);
    }
  else
    {
      clutter_actor_destroy (ntfa);
    }

  /* Fade in newer notifier from below stack */
  if (ntfa == priv->active_notifier && priv->n_notifiers > 0)
    {
      gint prev_height, new_height;

      prev_height = clutter_actor_get_height (ntfa);

      priv->active_notifier =
        clutter_group_get_nth_child (CLUTTER_GROUP (priv->notifiers),
                                     1); /* Next, not 0 */

      if (priv->active_notifier)
        {
          clutter_actor_set_opacity (priv->active_notifier, 0);
          clutter_actor_show (CLUTTER_ACTOR (priv->active_notifier));
          clutter_actor_animate (CLUTTER_ACTOR (priv->active_notifier),
                                 CLUTTER_EASE_IN_SINE,
                                 FADE_DURATION,
                                 "opacity", 0xff,
                                 NULL);

          new_height = clutter_actor_get_height (priv->active_notifier);

          if (prev_height != new_height && priv->n_notifiers > 1)
            {
              gfloat new_y;

              new_y = clutter_actor_get_y (priv->control)
                - (prev_height - new_height);

              clutter_actor_animate (priv->control,
                                     CLUTTER_EASE_IN_SINE,
                                     FADE_DURATION,
                                     "y", new_y,
                                     NULL);
            }
        }
    }

  if (priv->n_notifiers == 0)
    {
      priv->active_notifier = NULL;
      mnb_notification_gtk_hide ();
    }
  else if (priv->n_notifiers == 1)
    {
      /* slide the control out of view */
      ClutterAnimation *anim;

      anim = clutter_actor_animate (priv->control,
                                    CLUTTER_EASE_IN_SINE,
                                    FADE_DURATION,
                                    "opacity", 0x0,
                                    "y",
                                    clutter_actor_get_height (priv->active_notifier)
                                    - clutter_actor_get_height (priv->control),
                                    NULL);

      g_signal_connect_after (anim,
                        "completed",
                        G_CALLBACK
                        (ntf_tray_control_hide_completed_cb),
                        tray);
    }
  else
    {
      /* Just Update control text */
      gchar *msg;
      msg = g_strdup_printf (_("%i pending messages"), priv->n_notifiers);
      mx_label_set_text (MX_LABEL (priv->control_text), msg);
      g_free (msg);
    }
}

void
ntf_tray_add_notification (NtfTray *tray, NtfNotification *ntf)
{
  NtfTrayPrivate   *priv;
  ClutterActor     *ntfa;
  ClutterAnimation *anim;
  MutterPlugin     *plugin;

  g_return_if_fail (NTF_IS_TRAY (tray) && NTF_IS_NOTIFICATION (ntf));

  priv   = tray->priv;
  ntfa   = CLUTTER_ACTOR (ntf);
  plugin = meego_netbook_get_plugin_singleton ();

  if (meego_netbook_compositor_disabled (plugin))
    {
      mnb_notification_gtk_show ();
    }

  g_signal_connect (ntf, "closed",
                    G_CALLBACK (ntf_tray_notification_closed_cb),
                    tray);

  clutter_container_add_actor (CLUTTER_CONTAINER (priv->notifiers), ntfa);

  clutter_actor_set_width (ntfa, CLUSTER_WIDTH);

  priv->n_notifiers++;

  if (priv->n_notifiers == 1)
    {
      /* May have been previously hidden */
      clutter_actor_show (CLUTTER_ACTOR (tray));

      /* show just the single notification */
      priv->active_notifier = ntfa;
      clutter_actor_set_opacity (ntfa, 0);

      clutter_actor_animate (ntfa,
                             CLUTTER_EASE_IN_SINE,
                             FADE_DURATION,
                             "opacity", 0xff,
                             NULL);
     }
  else if (priv->n_notifiers == 2)
    {
      /* slide the control into view */
      mx_label_set_text (MX_LABEL(priv->control_text),
                         _("1 pending message"));

      clutter_actor_show (priv->control);

      clutter_actor_set_opacity (priv->control, 0);
      clutter_actor_set_y (priv->control,
              clutter_actor_get_height (priv->active_notifier)
                 - clutter_actor_get_height (priv->control) - 30);

      anim = clutter_actor_animate (priv->control,
                                    CLUTTER_EASE_IN_SINE,
                                    FADE_DURATION,
                                    "opacity", 0xff,
                                    "y", clutter_actor_get_height
                                    (priv->active_notifier)- 30,
                                    NULL);
    }
  else
    {
      /* simply update the control */
      gchar *msg;

      msg = g_strdup_printf (_("%i pending messages"), priv->n_notifiers-1);

      mx_label_set_text (MX_LABEL (priv->control_text), msg);

      g_free (msg);
    }
}

NtfNotification *
ntf_tray_find_notification (NtfTray *tray, gint subsystem, gint id)
{
  NtfTrayPrivate  *priv;
  GList           *notifiers, *l;
  NtfNotification *ntf = NULL;

  g_return_val_if_fail (NTF_IS_TRAY (tray), NULL);

  priv = tray->priv;

  notifiers =
    clutter_container_get_children (CLUTTER_CONTAINER (priv->notifiers));

  for (l = notifiers; l; l = l->next)
    {
      gint s = ntf_notification_get_subsystem (NTF_NOTIFICATION (l->data));
      gint i = ntf_notification_get_id (NTF_NOTIFICATION (l->data));

      if (subsystem == s && id == i)
        {
          ntf = l->data;
          break;
        }
    }

  g_list_free (notifiers);

  return ntf;
}

NtfTray *
ntf_tray_new (void)
{
  return g_object_new (NTF_TYPE_TRAY,
                       "show-on-set-parent", FALSE,
                       NULL);
}
