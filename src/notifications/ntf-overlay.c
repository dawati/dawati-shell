/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */

/*
 * Copyright (c) 2010 Intel Corp.
 *
 * Authors: Tomas Frydrych <tf@linux.intel.com>
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "../meego-netbook.h"
#include "../mnb-input-manager.h"
#include "ntf-overlay.h"
#include "ntf-libnotify.h"
#include "ntf-wm.h"

/*
 * MeeGo 1.1 Notification Framework
 *
 * The Ntf* notification framework is a complete rewrite of how notifications
 * are handled, with two aims:
 *
 * (1) Make the notification handling self-contained, with all implementation
 *     details completely hidden from the rest of the plugin; this avoids the
 *     serious pitfall of the previous implemtation where some aspects of the
 *     notifications (such as positioning and input management) were handled on
 *     the plugin level, and difficult to maintain.
 *
 * (2) Create an overall framework that is easily extensible to add new types
 *     of notifications; the old mechanism was explicitely tied to the libnotify
 *     notifications, with the 'demands-attention' notifications bolted on the
 *     top through an uggly hack. The new framework allows for any number of
 *     different notification subsystems to be connected in.
 *
 * To give credit where due, the overall structure of the Ntf framework was
 * inspired the the notification handling code in Gnome Shell.
 *
 * The framework consists of four major components that are agnostic of any
 * specific notification protocol; these are NtfOverlay, NtfSource,
 * NtfNotification and NtfTray. These components are used by any number of
 * notification subsystems, each of which handles the gory details of a specific
 * notitication transport mechanism.
 *
 *
 * NtfOverlay
 * ==========
 *
 * NtfOverlay is a top level ClutterActor which represents the entire
 * notification system, and provides the public API for the plugin. The public
 * API is minimal, it providing only a constructor, and a method to check for
 * the presence of urgent notifications. NtfOverlay is implemented as a
 * singleton.
 *
 *
 * NtfSource
 * =========
 *
 * NtfSource is a object representing single source of notifications, and each
 * notification is associated with a source. Having each notification associated
 * with a source allows notifications to be organized based on their source.
 *
 * The framework makes no assumptions about what constitutes a source; the
 * default NftSource implementation considers source to be an application
 * defined by a window and/or pid, but subsystems can implement NtfSource
 * subclasses to suit their needs (e.g., instant messaging subsystem might want
 * to have a source that represents a single contact).
 *
 * The source object provides an API to access per-source resources, at present
 * this is limited to an source-specific icon.
 *
 *
 * NtfNotification
 * ===============
 *
 * NtfNotification is a ClutterActor that represents a single message, with API
 * allowing to set the summary, body and icon of the message. Each message has a
 * default 'dismiss' button; additional buttons can be added using the
 * NtfNotification API.
 *
 *
 * NtfTray
 * =======
 *
 * NtfTray represents a message board on which individual notifications are
 * placed, proving controls to navigate and dismiss messages en mass. The MeeGo
 * framework has two separate trays, one for normal notifications and one for
 * urgent notifications.
 *
 *
 * ntf-libnotify
 * =============
 *
 * ntf-libnotify is a notification subsystem that handles messages delivered
 * through the standard notification protocol, using the notification store
 * implementation (taken over from MeeGo 1.0).
 *
 *
 * ntf-wm
 * ======
 *
 * ntf-wm is a notification subsystem that handless 'demands-attention' messages
 * that application dispatch via the EWMH demands-attention and WM urgency-hint
 * protocols.
 *
 *
 * MnbNotificationGtk
 * ==================
 *
 * MnbNotificaitonGtk is a simple gtk-based notifier used when the UX Shell is
 * not available (i.e., the compositor is not running); it is taken over from
 * the MeeGo 1.0 notification system.
 */

static NtfOverlay *self__ = NULL;

static void ntf_overlay_dispose (GObject *object);
static void ntf_overlay_finalize (GObject *object);
static void ntf_overlay_constructed (GObject *object);

G_DEFINE_TYPE (NtfOverlay, ntf_overlay, CLUTTER_TYPE_ACTOR);

#define NTF_OVERLAY_GET_PRIVATE(o) \
(G_TYPE_INSTANCE_GET_PRIVATE ((o), NTF_TYPE_OVERLAY, NtfOverlayPrivate))

struct _NtfOverlayPrivate
{
  NtfTray      *tray_normal;
  NtfTray      *tray_urgent;
  ClutterActor *lowlight;

  gulong        stage_allocation_id;

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
ntf_overlay_paint (ClutterActor *actor)
{
  NtfOverlayPrivate *priv = NTF_OVERLAY (actor)->priv;

  if (CLUTTER_ACTOR_IS_MAPPED (priv->tray_normal))
    clutter_actor_paint (CLUTTER_ACTOR(priv->tray_normal));

  if (CLUTTER_ACTOR_IS_MAPPED (priv->lowlight))
    clutter_actor_paint (CLUTTER_ACTOR(priv->lowlight));

  if (CLUTTER_ACTOR_IS_MAPPED (priv->tray_urgent))
    clutter_actor_paint (CLUTTER_ACTOR(priv->tray_urgent));
}

static void
ntf_overlay_pick (ClutterActor *actor, const ClutterColor *color)
{
  CLUTTER_ACTOR_CLASS (ntf_overlay_parent_class)->pick (actor, color);

  ntf_overlay_paint (actor);
}

static void
ntf_overlay_map (ClutterActor *actor)
{
  CLUTTER_ACTOR_CLASS (ntf_overlay_parent_class)->map (actor);

  /*
   * We do not map any children here, since they are not initially visible.
   */
}

static void
ntf_overlay_unmap (ClutterActor *actor)
{
  NtfOverlayPrivate *priv = NTF_OVERLAY (actor)->priv;

  CLUTTER_ACTOR_CLASS (ntf_overlay_parent_class)->unmap (actor);

  if (CLUTTER_ACTOR_IS_MAPPED (priv->tray_normal))
    clutter_actor_unmap (CLUTTER_ACTOR (priv->tray_normal));

  if (CLUTTER_ACTOR_IS_MAPPED (priv->tray_normal))
    clutter_actor_unmap (CLUTTER_ACTOR (priv->tray_normal));

  if (CLUTTER_ACTOR_IS_MAPPED (priv->lowlight))
    clutter_actor_unmap (CLUTTER_ACTOR (priv->lowlight));
}

static void
ntf_overlay_get_preferred_width (ClutterActor *actor,
                                 gfloat        for_height,
                                 gfloat       *min_width,
                                 gfloat       *natural_width)
{
  MutterPlugin *plugin = meego_netbook_get_plugin_singleton ();
  gint          screen_width;
  gint          screen_height;

  mutter_plugin_query_screen_size (plugin, &screen_width, &screen_height);

  *min_width     = screen_width;
  *natural_width = screen_width;
}

static void
ntf_overlay_get_preferred_height (ClutterActor *actor,
                               gfloat        for_width,
                               gfloat       *min_height,
                               gfloat       *natural_height)
{
  MutterPlugin *plugin = meego_netbook_get_plugin_singleton ();
  gint          screen_width;
  gint          screen_height;

  mutter_plugin_query_screen_size (plugin, &screen_width, &screen_height);

  *min_height     = screen_height;
  *natural_height = screen_height;
}

static void
ntf_overlay_allocate (ClutterActor          *actor,
                      const ClutterActorBox *box,
                      ClutterAllocationFlags flags)
{
  NtfOverlayPrivate *priv = NTF_OVERLAY (actor)->priv;
  ClutterActorClass *klass;
  gfloat             my_width, my_height;
  ClutterActor      *tna = CLUTTER_ACTOR (priv->tray_normal);
  ClutterActor      *tua = CLUTTER_ACTOR (priv->tray_urgent);
  ClutterActor      *lowlight = priv->lowlight;

  klass = CLUTTER_ACTOR_CLASS (ntf_overlay_parent_class);

  klass->allocate (actor, box, flags);

  my_width  = box->x2 - box->x1;
  my_height = box->y2 - box->y1;

  {
    ClutterActorBox tray_box;
    gfloat p_width, p_height, m_width, m_height;

    clutter_actor_get_preferred_width  (tna, -1.0, &m_width, &p_width);
    clutter_actor_get_preferred_height (tna, p_width, &m_height, &p_height);

    tray_box.x1 = my_width  - p_width;
    tray_box.y1 = my_height - p_height;
    tray_box.x2 = tray_box.x1 + p_width;
    tray_box.y2 = tray_box.y1 + p_height;

    clutter_actor_allocate (tna, &tray_box, flags);
  }

  {
    ClutterActorBox tray_box;
    gfloat p_width, p_height, m_width, m_height;

    clutter_actor_get_preferred_width  (tua, -1.0, &m_width, &p_width);
    clutter_actor_get_preferred_height (tua, p_width, &m_height, &p_height);

    tray_box.x1 = (gint)((my_width  - p_width)  / 2.0);
    tray_box.y1 = (gint)((my_height - p_height) / 2.0);
    tray_box.x2 = tray_box.x1 + p_width;
    tray_box.y2 = tray_box.y1 + p_height;

    clutter_actor_allocate (tua, &tray_box, flags);
  }

  {
    ClutterActorBox lowlight_box;

    lowlight_box.x1 = 0.0;
    lowlight_box.y1 = 0.0;
    lowlight_box.x2 = my_width;
    lowlight_box.y2 = my_height;

    clutter_actor_allocate (lowlight, &lowlight_box, flags);
  }
}

static GObject*
ntf_overlay_constructor (GType                  type,
                         guint                  n_construct_params,
                         GObjectConstructParam *construct_params)
{
  if (!self__)
    {
      self__ = (NtfOverlay*)
        G_OBJECT_CLASS (ntf_overlay_parent_class)->constructor (type,
                                                             n_construct_params,
                                                             construct_params);
    }
  else
    g_object_ref (self__);

  return (GObject*)self__;
}

static void
ntf_overlay_stage_allocation_cb (GObject      *object,
                                 GParamSpec   *spec,
                                 ClutterActor *overlay)
{
  clutter_actor_queue_relayout (overlay);
}

static void
ntf_overlay_parent_set (ClutterActor *overlay, ClutterActor *old_parent)
{
  NtfOverlayPrivate *priv   = NTF_OVERLAY (overlay)->priv;
  ClutterActorClass *klass  = CLUTTER_ACTOR_CLASS (ntf_overlay_parent_class);
  ClutterActor      *parent = clutter_actor_get_parent (overlay);

  if (priv->stage_allocation_id)
    {
      g_signal_handler_disconnect (old_parent, priv->stage_allocation_id);
      priv->stage_allocation_id = 0;
    }

  if (klass->parent_set)
    klass->parent_set (overlay, old_parent);

  if (parent)
    {
      priv->stage_allocation_id =
        g_signal_connect (parent, "notify::allocation",
                          G_CALLBACK (ntf_overlay_stage_allocation_cb),
                          overlay);
    }
}

static void
ntf_overlay_class_init (NtfOverlayClass *klass)
{
  GObjectClass      *object_class = (GObjectClass *)klass;
  ClutterActorClass *actor_class  = CLUTTER_ACTOR_CLASS (klass);

  g_type_class_add_private (klass, sizeof (NtfOverlayPrivate));

  object_class->constructor         = ntf_overlay_constructor;
  object_class->dispose             = ntf_overlay_dispose;
  object_class->finalize            = ntf_overlay_finalize;
  object_class->constructed         = ntf_overlay_constructed;

  actor_class->paint                = ntf_overlay_paint;
  actor_class->pick                 = ntf_overlay_pick;
  actor_class->map                  = ntf_overlay_map;
  actor_class->unmap                = ntf_overlay_unmap;
  actor_class->allocate             = ntf_overlay_allocate;
  actor_class->get_preferred_height = ntf_overlay_get_preferred_height;
  actor_class->get_preferred_width  = ntf_overlay_get_preferred_width;
  actor_class->parent_set           = ntf_overlay_parent_set;
}

static void
ntf_overlay_urgent_tray_show_cb (ClutterActor *tray, NtfOverlay *self)
{
  NtfOverlayPrivate *priv = self->priv;

  clutter_actor_show (priv->lowlight);
}

static void
ntf_overlay_urgent_tray_hide_cb (ClutterActor *tray, NtfOverlay *self)
{
  NtfOverlayPrivate *priv = self->priv;

  clutter_actor_hide (priv->lowlight);
}

static gboolean
ntf_overlay_lowlight_button_event_cb (ClutterActor       *actor,
                                      ClutterButtonEvent *event,
                                      NtfOverlay         *overlay)
{
  /*
   * Swallow all button events.
   */
  return TRUE;
}


static void
ntf_overlay_constructed (GObject *object)
{
  NtfOverlay        *self  = (NtfOverlay*) object;
  NtfOverlayPrivate *priv  = self->priv;
  ClutterActor      *actor = (ClutterActor*) object;
  ClutterColor       low_clr = {0, 0, 0, 0x7f};

  if (G_OBJECT_CLASS (ntf_overlay_parent_class)->constructed)
    G_OBJECT_CLASS (ntf_overlay_parent_class)->constructed (object);

  priv->tray_normal = ntf_tray_new ();
  clutter_actor_set_parent (CLUTTER_ACTOR (priv->tray_normal), actor);

  priv->tray_urgent = ntf_tray_new ();
  clutter_actor_set_parent (CLUTTER_ACTOR (priv->tray_urgent), actor);

  priv->lowlight = clutter_rectangle_new_with_color (&low_clr);
  clutter_actor_set_parent (CLUTTER_ACTOR (priv->lowlight), actor);
  mnb_input_manager_push_actor (priv->lowlight, MNB_INPUT_LAYER_TOP);
  clutter_actor_hide (priv->lowlight);

  /*
   * The urgent tray shows/hides itself when it contains something / is empty.
   * We need to show the lowlight any time it is visible.
   */
  g_signal_connect (priv->tray_urgent, "show",
                    G_CALLBACK (ntf_overlay_urgent_tray_show_cb),
                    self);

  g_signal_connect (priv->tray_urgent, "hide",
                    G_CALLBACK (ntf_overlay_urgent_tray_hide_cb),
                    self);

  g_signal_connect (priv->lowlight, "button-press-event",
                    G_CALLBACK (ntf_overlay_lowlight_button_event_cb),
                    self);

  g_signal_connect (priv->lowlight, "button-release-event",
                    G_CALLBACK (ntf_overlay_lowlight_button_event_cb),
                    self);
}

static void
ntf_overlay_init (NtfOverlay *self)
{
  self->priv = NTF_OVERLAY_GET_PRIVATE (self);

  /*
   * We do not want to force relayout when allocating.
   */
  CLUTTER_ACTOR_SET_FLAGS (self, CLUTTER_ACTOR_NO_LAYOUT);

  /*
   * Initialize notificaiton subsystems.
   */
  ntf_libnotify_init ();
  ntf_wm_init ();
}

static void
ntf_overlay_dispose (GObject *object)
{
  NtfOverlay        *self = (NtfOverlay*) object;
  NtfOverlayPrivate *priv = self->priv;

  if (priv->disposed)
    return;

  priv->disposed = TRUE;

  clutter_actor_destroy (priv->lowlight);
  clutter_actor_destroy (CLUTTER_ACTOR (priv->tray_normal));
  clutter_actor_destroy (CLUTTER_ACTOR (priv->tray_urgent));

  G_OBJECT_CLASS (ntf_overlay_parent_class)->dispose (object);
}

static void
ntf_overlay_finalize (GObject *object)
{
  G_OBJECT_CLASS (ntf_overlay_parent_class)->finalize (object);
}

NtfTray *
ntf_overlay_get_tray (gboolean urgent)
{
  NtfOverlayPrivate *priv;

  g_return_val_if_fail (self__, NULL);

  priv = self__->priv;

  if (urgent)
    return priv->tray_urgent;
  else
    return priv->tray_normal;
}

ClutterActor *
ntf_overlay_new (void)
{
  return g_object_new (NTF_TYPE_OVERLAY, NULL);
}

gboolean
ntf_overlay_urgent_notification_present (void)
{
  NtfOverlayPrivate *priv;

  g_return_val_if_fail (self__, FALSE);

  priv = self__->priv;

  if (CLUTTER_ACTOR_IS_MAPPED (priv->tray_urgent))
    return TRUE;

  return FALSE;
}
