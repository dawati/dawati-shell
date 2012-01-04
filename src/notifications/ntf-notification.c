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

#include <glib/gi18n.h>

#include "ntf-notification.h"

#define KEYSYM_KEY "ntf-notification-keysym"
#define KEYNAME_KEY "ntf-notification-keyname"

static gint subsytem_id = 0;
static GQuark keysym_quark = 0;
static GQuark keyname_quark = 0;

static void ntf_notification_focusable_init (MxFocusableIface *iface);

static void ntf_notification_dispose (GObject *object);
static void ntf_notification_finalize (GObject *object);
static void ntf_notification_constructed (GObject *object);
static void ntf_notification_get_property (GObject    *object,
                                           guint       property_id,
                                           GValue     *value,
                                           GParamSpec *pspec);
static void ntf_notification_set_property (GObject      *object,
                                           guint         property_id,
                                           const GValue *value,
                                           GParamSpec   *pspec);

G_DEFINE_TYPE_WITH_CODE (NtfNotification, ntf_notification, MX_TYPE_TABLE,
                         G_IMPLEMENT_INTERFACE (MX_TYPE_FOCUSABLE,
                                                ntf_notification_focusable_init));

#define NTF_NOTIFICATION_GET_PRIVATE(o) \
(G_TYPE_INSTANCE_GET_PRIVATE ((o), NTF_TYPE_NOTIFICATION, NtfNotificationPrivate))

struct _NtfNotificationPrivate
{
  NtfSource    *source;

  ClutterActor *summary;
  ClutterActor *body;
  ClutterActor *icon;
  ClutterActor *dismiss_button;
  ClutterActor *button_box;
  ClutterActor *title_box;

  gchar *focused_keyname;

  gint          id;
  gint          subsystem;

  gint          timeout;
  guint         timeout_id;
  GTimer       *timer;

  gulong source_closed_id;

  guint disposed          : 1;
  guint urgent            : 1;
  guint closed            : 1;
  guint no_dismiss_button : 1;
};

enum
{
  CLOSED,

  N_SIGNALS,
};

enum
{
  PROP_0,
  PROP_SOURCE,
  PROP_ID,
  PROP_SUBSYSTEM,
  PROP_NO_DISMISS_BUTTON,
};

static guint signals[N_SIGNALS] = {0};

static gboolean
ntf_notification_timeout_cb (NtfNotification *ntf)
{
  NtfNotificationPrivate *priv = ntf->priv;

  if (priv->closed)
    return FALSE;

  g_object_ref (ntf);
  g_signal_emit (ntf, signals[CLOSED], 0);
  g_object_unref (ntf);

  return FALSE;
}

static void
ntf_notification_show (ClutterActor *actor)
{
  NtfNotification        *ntf = (NtfNotification*) actor;
  NtfNotificationPrivate *priv = ntf->priv;

  if (priv->timeout > 0)
    {
      priv->timeout_id =
        g_timeout_add (priv->timeout, (GSourceFunc)ntf_notification_timeout_cb,
                       actor);
      if (!priv->timer)
        priv->timer = g_timer_new ();
      else
        g_timer_reset (priv->timer);
      g_timer_start (priv->timer);
    }

  CLUTTER_ACTOR_CLASS (ntf_notification_parent_class)->show (actor);
}

static gboolean
ntf_notification_enter_event (ClutterActor         *actor,
                              ClutterCrossingEvent *event)
{
  NtfNotificationPrivate *priv = NTF_NOTIFICATION (actor)->priv;

  if (priv->timeout > 0)
    {
      if (priv->timeout_id)
        {
          g_source_remove (priv->timeout_id);
          priv->timeout_id = 0;
        }
      g_timer_stop (priv->timer);
    }

  return
    CLUTTER_ACTOR_CLASS (ntf_notification_parent_class)->enter_event (actor,
                                                                      event);
}

static gboolean
ntf_notification_leave_event (ClutterActor         *actor,
                              ClutterCrossingEvent *event)
{
  NtfNotificationPrivate *priv = NTF_NOTIFICATION (actor)->priv;

  if (priv->timeout > 0)
    {
      gint remaining = (gint) priv->timeout -
        (gint) g_timer_elapsed (priv->timer, NULL) * 1000;

      priv->timeout_id =
        g_timeout_add (remaining,
                       (GSourceFunc) ntf_notification_timeout_cb,
                       actor);
      g_timer_continue (priv->timer);
    }

  return
    CLUTTER_ACTOR_CLASS (ntf_notification_parent_class)->leave_event (actor,
                                                                      event);
}

static void
ntf_notification_closed (NtfNotification *ntf)
{
  ntf->priv->closed = TRUE;
}

static void
ntf_notification_class_init (NtfNotificationClass *klass)
{
  GObjectClass      *object_class = (GObjectClass *)klass;
  ClutterActorClass *actor_class  = CLUTTER_ACTOR_CLASS (klass);

  keysym_quark = g_quark_from_static_string (KEYSYM_KEY);
  keyname_quark = g_quark_from_static_string (KEYNAME_KEY);

  g_type_class_add_private (klass, sizeof (NtfNotificationPrivate));

  object_class->dispose      = ntf_notification_dispose;
  object_class->finalize     = ntf_notification_finalize;
  object_class->constructed  = ntf_notification_constructed;
  object_class->get_property = ntf_notification_get_property;
  object_class->set_property = ntf_notification_set_property;

  actor_class->show          = ntf_notification_show;
  actor_class->enter_event   = ntf_notification_enter_event;
  actor_class->leave_event   = ntf_notification_leave_event;

  klass->closed              = ntf_notification_closed;

  g_object_class_install_property (object_class,
                                   PROP_SOURCE,
                                   g_param_spec_object ("source",
                                                      "NtfSource",
                                                      "NtfSource",
                                                      NTF_TYPE_SOURCE,
                                                      G_PARAM_READWRITE |
                                                      G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class,
                                   PROP_ID,
                                   g_param_spec_int ("id",
                                                   "id",
                                                   "implemnation specific id",
                                                   0,
                                                   G_MAXINT,
                                                   0,
                                                   G_PARAM_READWRITE |
                                                   G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class,
                                   PROP_SUBSYSTEM,
                                   g_param_spec_int ("subsystem",
                                                   "subsystem",
                                                   "subsystem id",
                                                   0,
                                                   G_MAXINT,
                                                   0,
                                                   G_PARAM_READWRITE |
                                                   G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class,
                                   PROP_NO_DISMISS_BUTTON,
                                   g_param_spec_boolean ("no-dismiss-button",
                                                       "No dismiss button",
                                                       "No dismiss button",
                                                       FALSE,
                                                       G_PARAM_READWRITE |
                                                       G_PARAM_CONSTRUCT_ONLY));

  signals[CLOSED]
    = g_signal_new ("closed",
                    G_OBJECT_CLASS_TYPE (klass),
                    G_SIGNAL_RUN_FIRST,
                    G_STRUCT_OFFSET (NtfNotificationClass, closed),
                    NULL, NULL,
                    g_cclosure_marshal_VOID__VOID,
                    G_TYPE_NONE, 0);
}

static void
ntf_notification_dismiss_cb (ClutterActor *button, NtfNotification *self)
{
  NtfNotificationPrivate *priv = self->priv;

  if (priv->closed)
    return;

  g_signal_emit (self, signals[CLOSED], 0);
}

static void
ntf_notification_constructed (GObject *object)
{
  NtfNotification        *self = (NtfNotification*) object;
  NtfNotificationPrivate *priv = self->priv;
  ClutterText            *txt;

  g_assert (self->priv->source);

  if (G_OBJECT_CLASS (ntf_notification_parent_class)->constructed)
    G_OBJECT_CLASS (ntf_notification_parent_class)->constructed (object);

  mx_stylable_set_style_class (MX_STYLABLE (self), "Notification");
  mx_table_set_column_spacing (MX_TABLE (self), 4);
  mx_table_set_row_spacing (MX_TABLE (self), 8);

  if (!priv->no_dismiss_button)
    {
      priv->dismiss_button = mx_button_new ();
      mx_button_set_label (MX_BUTTON (priv->dismiss_button), _("Dismiss"));

      g_signal_connect (priv->dismiss_button, "clicked",
                    G_CALLBACK (ntf_notification_dismiss_cb), self);
    }

  priv->title_box = mx_table_new ();
  mx_table_set_column_spacing (MX_TABLE (priv->title_box), 4);
  mx_table_add_actor (MX_TABLE (self), CLUTTER_ACTOR (priv->title_box), 0, 0);
  clutter_container_child_set (CLUTTER_CONTAINER (self),
                               CLUTTER_ACTOR (priv->title_box),
                               "y-expand", FALSE,
                               "x-expand", TRUE,
                               NULL);

  priv->summary = mx_label_new ();
  txt = CLUTTER_TEXT(mx_label_get_clutter_text(MX_LABEL(priv->summary)));
  clutter_text_set_line_alignment (txt, PANGO_ALIGN_LEFT);
  clutter_text_set_ellipsize (txt, PANGO_ELLIPSIZE_NONE);
  clutter_text_set_line_wrap (txt, TRUE);
  clutter_text_set_line_wrap_mode (txt, PANGO_WRAP_WORD_CHAR);

  mx_table_add_actor (MX_TABLE (priv->title_box), CLUTTER_ACTOR (priv->summary), 0, 1);
  clutter_container_child_set (CLUTTER_CONTAINER (priv->title_box),
                               CLUTTER_ACTOR (priv->summary),
                               "y-expand", TRUE,
                               "x-expand", TRUE,
                               "y-align", MX_ALIGN_MIDDLE,
                               "x-align", MX_ALIGN_START,
                               "y-fill", FALSE,
                               "x-fill", FALSE,
                               NULL);

  priv->body = mx_label_new ();
  mx_table_add_actor (MX_TABLE (self), CLUTTER_ACTOR (priv->body), 1, 0);
  txt = CLUTTER_TEXT(mx_label_get_clutter_text(MX_LABEL(priv->body)));
  clutter_text_set_line_alignment (txt, PANGO_ALIGN_LEFT);
  clutter_text_set_ellipsize (txt, PANGO_ELLIPSIZE_NONE);
  clutter_text_set_line_wrap (txt, TRUE);
  clutter_text_set_line_wrap_mode (txt, PANGO_WRAP_WORD_CHAR);
  clutter_text_set_use_markup (txt, TRUE);

  clutter_container_child_set (CLUTTER_CONTAINER (self),
                               CLUTTER_ACTOR (priv->body),
                               "y-expand", TRUE,
                               "x-expand", FALSE,
                               "y-fill", TRUE,
                               "x-fill", TRUE,
                               NULL);

  /* create the box for the buttons */
  priv->button_box = mx_grid_new ();
  mx_grid_set_line_alignment (MX_GRID (priv->button_box), MX_ALIGN_END);
  mx_grid_set_column_spacing (MX_GRID (priv->button_box), 7);
  mx_table_add_actor (MX_TABLE (self), CLUTTER_ACTOR (priv->button_box),
                        2, 0);

  if (priv->dismiss_button)
    {
      /* add the dismiss button to the button box */
      ntf_notification_add_button (self, priv->dismiss_button, "dismiss", 0);
    }

  mx_stylable_set_style_class (MX_STYLABLE (priv->summary),
                                    "NotificationSummary");
  mx_stylable_set_style_class (MX_STYLABLE (priv->body),
                                    "NotificationBody");
}

static void
ntf_notification_get_property (GObject    *object,
                               guint       property_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
  NtfNotification        *self = (NtfNotification*) object;
  NtfNotificationPrivate *priv = self->priv;

  switch (property_id)
    {
    case PROP_SOURCE:
      g_value_set_object (value, priv->source);
      break;
    case PROP_ID:
      g_value_set_int (value, priv->id);
      break;
    case PROP_SUBSYSTEM:
      g_value_set_int (value, priv->subsystem);
      break;
    case PROP_NO_DISMISS_BUTTON:
      g_value_set_boolean (value, priv->no_dismiss_button);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
ntf_notification_source_closed_cb (NtfSource *src, NtfNotification *ntf)
{
  NtfNotificationPrivate *priv = ntf->priv;

  priv->source = NULL;
  priv->source_closed_id = 0;

  if (priv->closed)
    return;

  g_object_ref (ntf);
  g_signal_emit (ntf, signals[CLOSED], 0);
  g_object_unref (ntf);
}

static void
ntf_notification_set_property (GObject      *object,
                               guint         property_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
  NtfNotification        *self = (NtfNotification*) object;
  NtfNotificationPrivate *priv = self->priv;

  switch (property_id)
    {
    case PROP_SOURCE:
      g_assert (!priv->source);
      priv->source = g_value_get_object (value);

      if (priv->source)
        {
          priv->source_closed_id =
            g_signal_connect (priv->source, "closed",
                              G_CALLBACK (ntf_notification_source_closed_cb),
                              self);
        }
      else
        priv->source_closed_id = 0;
      break;
    case PROP_ID:
      priv->id = g_value_get_int (value);
      break;
    case PROP_SUBSYSTEM:
      priv->subsystem = g_value_get_int (value);
      break;
    case PROP_NO_DISMISS_BUTTON:
      priv->no_dismiss_button = g_value_get_boolean (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
ntf_notification_init (NtfNotification *self)
{
  self->priv = NTF_NOTIFICATION_GET_PRIVATE (self);
}

static void
ntf_notification_dispose (GObject *object)
{
  NtfNotification        *self = (NtfNotification*) object;
  NtfNotificationPrivate *priv = self->priv;

  if (priv->disposed)
    return;

  priv->disposed = TRUE;

  if (priv->focused_keyname)
    {
      g_free (priv->focused_keyname);
      priv->focused_keyname = NULL;
    }

  if (priv->timer)
    {
      g_timer_destroy (priv->timer);
      priv->timer = NULL;
    }

  if (priv->timeout_id)
    {
      g_source_remove (priv->timeout_id);
      priv->timeout_id = 0;
    }

  if (priv->source_closed_id)
    {
      if (priv->source)
        {
          g_signal_handler_disconnect (priv->source, priv->source_closed_id);
          priv->source = NULL;
        }
      else
        g_warning ("Stale source 'closed' callback id");

      priv->source_closed_id = 0;
    }

  G_OBJECT_CLASS (ntf_notification_parent_class)->dispose (object);
}

static void
ntf_notification_finalize (GObject *object)
{
  G_OBJECT_CLASS (ntf_notification_parent_class)->finalize (object);
}

static GList *
ntf_notification_get_focus (NtfNotification *ntf, GList *children)
{
  NtfNotificationPrivate *priv = ntf->priv;
  GList *l = children;

  if (priv->focused_keyname)
    {
      for (l = children; l; l = g_list_next (l))
        {
          gchar *keyname = (gchar *) g_object_get_qdata (G_OBJECT (l->data),
                                                         keyname_quark);

          /* If we have a focused_keyname and the current child match,
             then we've found the focus. Otherwise, look of the one
             named "default". */
          if (priv->focused_keyname &&
              (!strcmp (priv->focused_keyname, keyname)))
            {
              return l;
            }
        }
    }

  return g_list_last (children);
}
static void
ntf_notification_save_focus (NtfNotification *ntf, MxFocusable *focusable)
{
  NtfNotificationPrivate *priv = ntf->priv;

  if (priv->focused_keyname)
    g_free (priv->focused_keyname);
  priv->focused_keyname = g_object_get_qdata (G_OBJECT (focusable),
                                              keyname_quark);
  if (priv->focused_keyname)
    priv->focused_keyname = g_strdup (priv->focused_keyname);
}

static MxFocusable *
ntf_notification_move_focus (MxFocusable      *focusable,
                             MxFocusDirection  direction,
                             MxFocusable      *from)
{
  NtfNotification *ntf = NTF_NOTIFICATION (focusable);
  NtfNotificationPrivate *priv = ntf->priv;
  GList *l, *children;

  children =
    clutter_container_get_children (CLUTTER_CONTAINER (priv->button_box));

  if (!children)
    return NULL;

  l = ntf_notification_get_focus (ntf, children);

  switch (direction)
    {
    case MX_FOCUS_DIRECTION_LEFT:
      if (l->prev)
        focusable = (MxFocusable *) l->prev->data;
      else
        focusable = (MxFocusable *) l->data;
      break;

    case MX_FOCUS_DIRECTION_RIGHT:
      if (l->next)
        focusable = (MxFocusable *) l->next->data;
      else
        focusable = (MxFocusable *) l->data;
      break;

    case MX_FOCUS_DIRECTION_NEXT:
      if (l->next)
        focusable = (MxFocusable *) l->next->data;
      else
        focusable = (MxFocusable *) children->data;
      break;

    case MX_FOCUS_DIRECTION_PREVIOUS:
      if (l->prev)
        focusable = (MxFocusable *) l->prev->data;
      else
        focusable = (MxFocusable *) g_list_last (children)->data;
      break;

    default:
    case MX_FOCUS_DIRECTION_DOWN:
    case MX_FOCUS_DIRECTION_UP:
      focusable = l->data;
      break;
    }

  ntf_notification_save_focus (ntf, focusable);

  focusable =
    mx_focusable_accept_focus (focusable,
                               mx_focus_hint_from_direction (direction));

  g_list_free (children);

  return focusable;
}

static MxFocusable *
ntf_notification_accept_focus (MxFocusable *focusable, MxFocusHint hint)
{
  NtfNotification *ntf = NTF_NOTIFICATION (focusable);
  NtfNotificationPrivate *priv = ntf->priv;
  GList *l, *children;

  children =
    clutter_container_get_children (CLUTTER_CONTAINER (priv->button_box));
  if (!children)
    return NULL;

  l = ntf_notification_get_focus (ntf, children);
  focusable = (MxFocusable *) l->data;

  ntf_notification_save_focus (ntf, focusable);

  g_list_free (children);

  if (focusable)
    return mx_focusable_accept_focus (focusable, hint);
  return NULL;
}

static void
ntf_notification_focusable_init (MxFocusableIface *iface)
{
  iface->move_focus = ntf_notification_move_focus;
  iface->accept_focus = ntf_notification_accept_focus;
}

/**
 * ntf_notification_get_source:
 * @ntf: #NtfNotification
 *
 * Returns the source associated with this notification.
 *
 * Return value: #NtfSource.
 */
NtfSource *
ntf_notification_get_source (NtfNotification *ntf)
{
  g_return_val_if_fail (NTF_IS_NOTIFICATION (ntf), NULL);

  return ntf->priv->source;
}

/**
 * ntf_notification_close:
 * @ntf: #NtfNotification
 *
 * Closes the notification (equivalent to user clickingo on the dismiss button.
 */
void
ntf_notification_close (NtfNotification *ntf)
{
  NtfNotificationPrivate *priv;

  g_return_if_fail (NTF_IS_NOTIFICATION (ntf));

  priv = ntf->priv;

  if (priv->closed)
    return;

  g_object_ref (ntf);
  g_signal_emit (ntf, signals[CLOSED], 0);
  g_object_unref (ntf);
}

/**
 * ntf_notification_add_button:
 * @ntf: #NtfNotification,
 * @button: #ClutterActor,
 * @keysym: key associated with this button, or None.
 *
 * Adds button to the notification button box.
 */
void
ntf_notification_add_button (NtfNotification *ntf,
                             ClutterActor    *button,
                             const gchar     *keyname,
                             KeySym           keysym)
{
  NtfNotificationPrivate *priv;

  g_return_if_fail (NTF_IS_NOTIFICATION (ntf) && CLUTTER_IS_ACTOR (button));

  priv = ntf->priv;

  clutter_container_add_actor (CLUTTER_CONTAINER (priv->button_box),
                               CLUTTER_ACTOR (button));

  if (keysym)
    {
      g_object_set_qdata (G_OBJECT (button), keysym_quark,
                          GINT_TO_POINTER (keysym));
    }

  if (keyname)
    {
      g_object_set_qdata_full (G_OBJECT (button), keyname_quark,
                               (gpointer) g_strdup (keyname),
                               g_free);
    }
}

/**
 * ntf_notification_remove_button:
 * @ntf: #NtfNotification,
 * @button: #ClutterActor
 *
 * Removes button from the notification button box.
 */
void
ntf_notification_remove_button (NtfNotification *ntf, ClutterActor *button)
{
  NtfNotificationPrivate *priv;
  gchar *keyname;

  g_return_if_fail (NTF_IS_NOTIFICATION (ntf) && CLUTTER_IS_ACTOR (button));

  priv = ntf->priv;

  /* Reset focused keyname if we remove the focused button */
  keyname = g_object_get_qdata (G_OBJECT (button), keyname_quark);
  if (!strcmp (keyname, priv->focused_keyname))
    {
      g_free (priv->focused_keyname);
      priv->focused_keyname = NULL;
    }

  clutter_container_remove_actor (CLUTTER_CONTAINER (priv->button_box),
                                  CLUTTER_ACTOR (button));
}

void
ntf_notification_remove_all_buttons (NtfNotification *ntf)
{
  NtfNotificationPrivate *priv;
  GList                  *l;

  g_return_if_fail (NTF_IS_NOTIFICATION (ntf));

  priv = ntf->priv;

  /* Reset focused keyname */
  if (priv->focused_keyname)
    {
      g_free (priv->focused_keyname);
      priv->focused_keyname = NULL;
    }

  /*
   * Remove all buttons, but hold onto the default button and prepend it
   * at the start.
   */
  if (priv->dismiss_button)
    g_object_ref (priv->dismiss_button);

  for (l = clutter_container_get_children(CLUTTER_CONTAINER (priv->button_box));
       l;
       l = g_list_delete_link (l, l))
    {
      clutter_container_remove_actor (CLUTTER_CONTAINER (priv->button_box),
                                      CLUTTER_ACTOR (l->data));
    }

  if (priv->dismiss_button)
    {
      clutter_container_add_actor (CLUTTER_CONTAINER (priv->button_box),
                                   priv->dismiss_button);

      g_object_unref (priv->dismiss_button);
    }
}

void
ntf_notification_set_summary   (NtfNotification *ntf, const gchar *text)
{
  NtfNotificationPrivate *priv;

  g_return_if_fail (NTF_IS_NOTIFICATION (ntf));

  priv = ntf->priv;

  mx_label_set_text (MX_LABEL(priv->summary), text);
}

void
ntf_notification_set_body (NtfNotification *ntf, const gchar *text)
{
  NtfNotificationPrivate *priv;

  g_return_if_fail (NTF_IS_NOTIFICATION (ntf));

  priv = ntf->priv;

  mx_label_set_text (MX_LABEL(priv->body), text);
  clutter_text_set_use_markup (CLUTTER_TEXT (mx_label_get_clutter_text(
                                             MX_LABEL(priv->body))),
                               TRUE);
}


void
ntf_notification_set_icon (NtfNotification *ntf, ClutterActor *icon)
{
  NtfNotificationPrivate *priv;

  g_return_if_fail (NTF_IS_NOTIFICATION (ntf) &&
                    (!icon || CLUTTER_IS_ACTOR (icon)));

  priv = ntf->priv;

  if (priv->icon)
    clutter_actor_destroy (priv->icon);

  priv->icon = NULL;

  if (icon)
    {
      clutter_container_child_set (CLUTTER_CONTAINER (priv->title_box),
                                   CLUTTER_ACTOR (priv->summary),
                                   "column", 1,
                                   NULL);

      mx_table_add_actor (MX_TABLE (priv->title_box), icon, 0, 0);
      clutter_container_child_set (CLUTTER_CONTAINER (priv->title_box),
                                   CLUTTER_ACTOR (icon),
                                   "y-expand", FALSE,
                                   "x-expand", FALSE,
                                   "x-align", MX_ALIGN_START,
                                   "x-fill", FALSE,
                                   "y-fill", FALSE,
                                   NULL);
    }
  else
    {
      clutter_container_child_set (CLUTTER_CONTAINER (priv->title_box),
                                   CLUTTER_ACTOR (priv->summary),
                                   "column", 0,
                                   NULL);
    }
}

/**
 * ntf_notification_get_id:
 * @ntf: #NtfNotification:
 *
 * Returns the numerical id of this notification as specified at construction
 * time. The meaning of the id is implementation-specific.
 */
gint
ntf_notification_get_id (NtfNotification *ntf)
{
  g_return_val_if_fail (NTF_IS_NOTIFICATION (ntf), 0);

  return ntf->priv->id;
}

/**
 * ntf_notification_get_urgent:
 * @ntf: #NtfNotification:
 *
 * Returns whether this notification is urgent.
 */
gboolean
ntf_notification_get_urgent (NtfNotification *ntf)
{
  g_return_val_if_fail (NTF_IS_NOTIFICATION (ntf), FALSE);

  return ntf->priv->urgent;
}

/**
 * ntf_notification_get_urgent:
 * @ntf: #NtfNotification:
 * @is_urgent: if %TRUE, set this notification as urgent.
 *
 * Set a notification is urgent.
 */
void
ntf_notification_set_urgent (NtfNotification *ntf, gboolean is_urgent)
{
  NtfNotificationPrivate *priv;

  g_return_if_fail (NTF_IS_NOTIFICATION (ntf));

  priv = ntf->priv;

  if (!priv->urgent != is_urgent)
    return;

  priv->urgent = is_urgent;

  if (is_urgent)
    {
      mx_stylable_set_style_class (MX_STYLABLE (priv->summary),
                                   "NotificationSummaryUrgent");
    }
  else
    {
      mx_stylable_set_style_class (MX_STYLABLE (priv->summary),
                                   "NotificationSummary");
    }
}

/**
 * ntf_notification_subsystem_id:
 *
 * Returns unique id that a given subsystem should use to set the subsystem
 * property of any #NtfNotifications it is creating. Together with the
 * notification id property, this can be used by the subsystem to look up
 * existing notifications in the tray.
 *
 * Each subsystem should only call this function once.
 *
 * Return value: numerical subsystem id.
 */
gint
ntf_notification_get_subsystem_id (void)
{
  return ++subsytem_id;
}

/**
 * ntf_notification_new:
 * @src: #NtfSource
 * @subsystem: subsystem to which notification belongs
 * @id: numerical id of the notification within its subsystem.
 * @no_dismiss_button: if %TRUE, no dismiss button is added.
 *
 * Creates a new notification for given subsystem; each subsystem should obtain
 * its id by initially calling ntf_notification_get_subsystem_id().
 */
NtfNotification *
ntf_notification_new (NtfSource *src,
                      gint       subsystem,
                      gint       id,
                      gboolean   no_dismiss_button)
{
  return g_object_new (NTF_TYPE_NOTIFICATION,
                       "source",             src,
                       "subsystem",          subsystem,
                       "id",                 id,
                       "reactive",           TRUE,
                       "no-dismiss-button",  no_dismiss_button,
                       NULL);
}

/**
 * ntf_notification_handle_key_event:
 * @ntf: #NtfNotification,
 * @event: #ClutterKeyEvent
 *
 * Calls the notification's key handler for the given event if installed.
 *
 * Return value: returns %TRUE if the event was handled, %FALSE otherwise.
 */
gboolean
ntf_notification_handle_key_event (NtfNotification *ntf,
                                   ClutterKeyEvent *event)
{
  NtfNotificationPrivate *priv;
  GList *l;

  g_return_val_if_fail (NTF_IS_NOTIFICATION (ntf), FALSE);

  priv = ntf->priv;

  l = clutter_container_get_children (CLUTTER_CONTAINER (priv->button_box));

  for (; l; l = l->next)
    {
      KeySym keysym = GPOINTER_TO_INT (g_object_get_qdata (G_OBJECT (l->data),
                                                           keysym_quark));

      if (keysym == event->keyval)
        {
          g_signal_emit_by_name (l->data, "clicked");
          return TRUE;
        }
    }

  return FALSE;
}

gint
ntf_notification_get_subsystem (NtfNotification *ntf)
{
  NtfNotificationPrivate *priv;

  g_return_val_if_fail (NTF_IS_NOTIFICATION (ntf), 0);

  priv = ntf->priv;

  return priv->subsystem;
}

void
ntf_notification_set_timeout (NtfNotification *ntf, guint timeout)
{
  NtfNotificationPrivate *priv;

  g_return_if_fail (NTF_IS_NOTIFICATION (ntf));

  priv = ntf->priv;

  priv->timeout = timeout;
}

gboolean
ntf_notification_is_closed (NtfNotification *ntf)
{
  NtfNotificationPrivate *priv;

  g_return_val_if_fail (NTF_IS_NOTIFICATION (ntf), 0);

  priv = ntf->priv;

  return (priv->closed != FALSE);
}

