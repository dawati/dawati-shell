/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */

/*
 * Copyright (c) 2012 Intel Corp.
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 */

#include "mnb-statusbar.h"

#include <glib/gi18n.h>

#include "mnb-input-manager.h"

G_DEFINE_TYPE (MnbStatusbar, mnb_statusbar, MX_TYPE_BOX_LAYOUT)

#define STATUSBAR_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MNB_TYPE_STATUSBAR, MnbStatusbarPrivate))

#define TOOLBAR_TRIGGER_THRESHOLD         (1)
#define TOOLBAR_TRIGGER_ADJUSTMENT        (2)
#define TOOLBAR_TRIGGER_THRESHOLD_TIMEOUT (500)

enum
{
  PROP_0,
  PROP_MUTTER_PLUGIN,
  PROP_TOOLBAR
};

struct _MnbStatusbarPrivate
{
  MetaPlugin *plugin;
  MnbToolbar *toolbar;

  guint trigger_timeout_id;
  guint trigger_threshold;

  gboolean in_trigger_zone;

  guint timeout_id;
  ClutterActor *datetime;
};

static void
mnb_statusbar_stage_allocation_cb (ClutterActor *stage,
                                   GParamSpec   *pspec,
                                   MnbStatusbar *self)
{
  MnbStatusbarPrivate *priv = self->priv;
  ClutterActorBox      box;

  clutter_actor_get_allocation_box (stage, &box);
  clutter_actor_set_size (CLUTTER_ACTOR (self),
                          box.x2 - box.x1, STATUSBAR_HEIGHT);
  mnb_input_manager_push_actor (CLUTTER_ACTOR (self), MNB_INPUT_LAYER_TOP);
  dawati_netbook_set_struts (priv->plugin, -1, -1, STATUSBAR_HEIGHT, -1);
}

static void
mnb_statusbar_stage_show_cb (ClutterActor *stage,
                             MnbStatusbar *self)
{
  g_signal_connect (stage, "notify::allocation",
                    G_CALLBACK (mnb_statusbar_stage_allocation_cb),
                    self);
}

static gboolean
mnb_statusbar_trigger_toolbar_timeout_cb (gpointer data)
{
  MnbStatusbar *statusbar = MNB_STATUSBAR (data);

  mnb_toolbar_show (statusbar->priv->toolbar,
                    MNB_SHOW_HIDE_BY_MOUSE);
  statusbar->priv->trigger_timeout_id = 0;

  return FALSE;
}

#define POINTER_IN_ZONE(value) ((value) >= 0 && (value) <= priv->trigger_threshold)

static gboolean
mnb_statusbar_event_cb (MnbStatusbar *self,
                        ClutterEvent *event,
                        gpointer      data)
{
  MnbStatusbarPrivate *priv = self->priv;
  gboolean             show_toolbar;

  if (!(event->type == CLUTTER_ENTER ||
        event->type == CLUTTER_LEAVE ||
        event->type == CLUTTER_MOTION))
    {
      /* g_debug (G_STRLOC " leaving early"); */
      return FALSE;
    }

  /*
   * This is when we want to show the toolbar:
   *
   *  a) we got an enter event on stage,
   *
   *    OR
   *
   *  b) we got a leave event on stage at the very top of the screen (when the
   *     pointer is at the position that coresponds to the top of the window,
   *     it is considered to have left the window; when the user slides pointer
   *     to the top, we get an enter event immediately followed by a leave
   *     event).
   *
   *  In all cases, only if the toolbar is not already visible.
   */
  show_toolbar =  (event->type == CLUTTER_ENTER &&
                   POINTER_IN_ZONE (event->crossing.y));
  show_toolbar |= (event->type == CLUTTER_LEAVE &&
                   POINTER_IN_ZONE (event->crossing.y));
  show_toolbar |= (event->type == CLUTTER_MOTION &&
                   POINTER_IN_ZONE (event->motion.y));

  if (show_toolbar)
    {
      if (!mnb_toolbar_is_visible (priv->toolbar) &&
          !mnb_toolbar_in_show_transition (priv->toolbar))
        {
          /*
           * If any fullscreen apps are present, then bail out.
           */
          if (dawati_netbook_fullscreen_apps_present (priv->plugin))
            return FALSE;

          /*
           * Only do this once; if the timeout is already installed,
           * we wait (see bug 3949)
           */
          if (!priv->trigger_timeout_id)
            {
              /*
               * Increase sensitivity -- increasing size of the
               * trigger zone while the timeout reduces the effect of
               * a shaking hand.
               */
              priv->trigger_threshold = TOOLBAR_TRIGGER_ADJUSTMENT;

              priv->trigger_timeout_id =
                g_timeout_add (TOOLBAR_TRIGGER_THRESHOLD_TIMEOUT,
                               mnb_statusbar_trigger_toolbar_timeout_cb,
                               self);
            }
        }
      priv->in_trigger_zone = TRUE;
    }
  else
    {
      if (priv->trigger_timeout_id)
        {
          /*
           * Pointer left us before the required timeout triggered; clean up.
           */
          priv->trigger_threshold = TOOLBAR_TRIGGER_THRESHOLD;
          g_source_remove (priv->trigger_timeout_id);
          priv->trigger_timeout_id = 0;
        }
      else if (mnb_toolbar_is_visible (priv->toolbar)/*  && */
               /* !priv->waiting_for_panel_hide */)
        {
          priv->trigger_threshold = TOOLBAR_TRIGGER_THRESHOLD;
          mnb_toolbar_hide (priv->toolbar, MNB_SHOW_HIDE_BY_MOUSE);
        }

      priv->in_trigger_zone = FALSE;
    }

  return FALSE;
}

static void
mnb_statusbar_myzone_clicked_cb (MxButton     *button,
                                 MnbStatusbar *self)
{
  g_print ("Myzone clicked\n");
}

static void
mnb_statusbar_update_datetime (MnbStatusbar *self)
{
  MnbStatusbarPrivate *priv = self->priv;
  GDateTime *datetime = g_date_time_new_now_local ();
  gchar *time_str;
  GConfClient *client = gconf_client_get_default ();
  gboolean c24h = gconf_client_get_bool (client, MNB_24H_KEY, NULL);

  if (c24h)
    {

      /* translators: translate this to a suitable 24 hourt time
       * format for your locale showing only hours and minutes. For
       * available format specifiers see
       * http://www.opengroup.org/onlinepubs/007908799/xsh/strftime.html
       */
      time_str = g_date_time_format (datetime, _("%A %B %e, %Y - %H:%M"));
    }
  else
    {
      /* translators: translate this to a suitable default time format for
       * your locale showing only hours and minutes. For available format
       * specifiers see
       * http://www.opengroup.org/onlinepubs/007908799/xsh/strftime.html
       */
      time_str = g_date_time_format (datetime, _("%A %B %e, %Y - %l:%M %P"));
    }

  mx_button_set_label (MX_BUTTON (priv->datetime), time_str);

  g_date_time_unref (datetime);
  g_free (time_str);
}

static gboolean
mnb_statusbar_timeout_cb (MnbStatusbar *self)
{
  mnb_statusbar_update_datetime (self);

  return TRUE;
}

static gboolean
mnb_statusbar_initial_timeout_cb (MnbStatusbar *self)
{
  MnbStatusbarPrivate *priv = self->priv;

  priv->timeout_id = g_timeout_add_seconds (60,
                                            (GSourceFunc) mnb_statusbar_timeout_cb,
                                            self);
  mnb_statusbar_update_datetime (self);

  return FALSE;
}

static void
mnb_statusbar_allocate (ClutterActor           *actor,
                        const ClutterActorBox  *box,
                        ClutterAllocationFlags  flags)
{
  MnbStatusbarPrivate *priv = MNB_STATUSBAR (actor)->priv;
  ClutterActorBox child_box;
  gfloat cwidth;

  clutter_actor_get_preferred_width (priv->datetime, -1, NULL, &cwidth);

  child_box.x1 = (gint) ((box->x2 - box->x1) / 2 - cwidth / 2);
  child_box.y1 = box->y1;
  child_box.x2 = child_box.x1 + cwidth;
  child_box.y2 = box->y2;

  clutter_actor_allocate (priv->datetime, &child_box, flags);

  CLUTTER_ACTOR_CLASS (mnb_statusbar_parent_class)->allocate (actor, box, flags);
}

static void
mnb_statusbar_paint (ClutterActor *actor)
{
  MnbStatusbarPrivate *priv = MNB_STATUSBAR (actor)->priv;

  CLUTTER_ACTOR_CLASS (mnb_statusbar_parent_class)->paint (actor);

  clutter_actor_paint (CLUTTER_ACTOR (priv->datetime));
}

static void
mnb_statusbar_get_property (GObject    *object,
                            guint       property_id,
                            GValue     *value,
                            GParamSpec *pspec)
{
  MnbStatusbarPrivate *priv = MNB_STATUSBAR (object)->priv;

  switch (property_id)
    {
    case PROP_MUTTER_PLUGIN:
      g_value_set_object (value, priv->plugin);
      break;

    case PROP_TOOLBAR:
      g_value_set_object (value, priv->toolbar);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
mnb_statusbar_set_property (GObject      *object,
                            guint         property_id,
                            const GValue *value,
                            GParamSpec   *pspec)
{
  MnbStatusbarPrivate *priv = MNB_STATUSBAR (object)->priv;

  switch (property_id)
    {
    case PROP_MUTTER_PLUGIN:
      priv->plugin = g_value_get_object (value);
      break;

    case PROP_TOOLBAR:
      priv->toolbar = g_value_get_object (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
mnb_statusbar_dispose (GObject *object)
{
  MnbStatusbarPrivate *priv = MNB_STATUSBAR (object)->priv;

  if (priv->timeout_id)
    {
      g_source_remove (priv->timeout_id);
      priv->timeout_id = 0;
    }

  if (priv->trigger_timeout_id)
    {
      g_source_remove (priv->trigger_timeout_id);
      priv->trigger_timeout_id = 0;
    }

  if (priv->datetime)
    {
      clutter_actor_destroy (priv->datetime);
      priv->datetime = NULL;
    }

  G_OBJECT_CLASS (mnb_statusbar_parent_class)->dispose (object);
}

static void
mnb_statusbar_finalize (GObject *object)
{
  G_OBJECT_CLASS (mnb_statusbar_parent_class)->finalize (object);
}

static void
mnb_statusbar_constructed (GObject *object)
{
  MnbStatusbarPrivate *priv   = MNB_STATUSBAR (object)->priv;

  if (G_OBJECT_CLASS (mnb_statusbar_parent_class)->constructed)
    G_OBJECT_CLASS (mnb_statusbar_parent_class)->finalize (object);

  g_signal_connect (meta_plugin_get_stage (priv->plugin),
                    "show", G_CALLBACK (mnb_statusbar_stage_show_cb),
                    object);

  clutter_actor_set_reactive (CLUTTER_ACTOR (object), TRUE);
}

static void
mnb_statusbar_class_init (MnbStatusbarClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  ClutterActorClass *actor_class = CLUTTER_ACTOR_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MnbStatusbarPrivate));

  object_class->constructed = mnb_statusbar_constructed;
  object_class->get_property = mnb_statusbar_get_property;
  object_class->set_property = mnb_statusbar_set_property;
  object_class->dispose = mnb_statusbar_dispose;
  object_class->finalize = mnb_statusbar_finalize;

  actor_class->allocate = mnb_statusbar_allocate;
  actor_class->paint = mnb_statusbar_paint;

  g_object_class_install_property (object_class,
                                   PROP_MUTTER_PLUGIN,
                                   g_param_spec_object ("mutter-plugin",
                                                        "Mutter Plugin",
                                                        "Mutter Plugin",
                                                        META_TYPE_PLUGIN,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class,
                                   PROP_TOOLBAR,
                                   g_param_spec_object ("toolbar",
                                                        "Toolbar",
                                                        "Toolbar",
                                                        MNB_TYPE_TOOLBAR,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));
}

static void
mnb_statusbar_init (MnbStatusbar *self)
{
  MnbStatusbarPrivate *priv;
  GDateTime *datetime;

  priv = self->priv = STATUSBAR_PRIVATE (self);

  g_signal_connect (self,
                    "event",
                    G_CALLBACK (mnb_statusbar_event_cb),
                    NULL);

  priv->datetime = mx_button_new ();
  clutter_actor_set_name (priv->datetime, "statusbar-date-button");
  clutter_actor_set_parent (priv->datetime, CLUTTER_ACTOR (self));

  mnb_statusbar_update_datetime (self);

  datetime = g_date_time_new_now_local ();
  g_timeout_add_seconds (60 - g_date_time_get_second (datetime),
                         (GSourceFunc) mnb_statusbar_initial_timeout_cb,
                         self);
  g_date_time_unref (datetime);
}

ClutterActor *
mnb_statusbar_new (MetaPlugin *plugin, MnbToolbar *toolbar)
{
  return g_object_new (MNB_TYPE_STATUSBAR,
                       "mutter-plugin", plugin,
                       "toolbar", toolbar,
                       /* FIXME: Why does this crashes gobject???
                        *        64bits weirdness??
                        */
                       /* "orientation", MX_ORIENTATION_HORIZONTAL, */
                       NULL);
}
