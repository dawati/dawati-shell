
/*
 * Copyright (c) 2009 Intel Corp.
 *
 * Author: Robert Staudinger <robertx.staudinger@intel.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU Lesser General Public License,
 * version 2.1, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
 * more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <mx/mx.h>
#include "mnb-entry.h"

G_DEFINE_TYPE (MnbEntry, mnb_entry, MPL_TYPE_ENTRY)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MNB_TYPE_ENTRY, MnbEntryPrivate))

enum
{
  PROP_0,

  PROP_HAS_KEYBOARD_FOCUS
};

enum
{
  KEYNAV_EVENT,

  LAST_SIGNAL
};

typedef struct {
  gboolean has_keyboard_focus;
} MnbEntryPrivate;

static guint _signals[LAST_SIGNAL] = { 0, };

/*
 * MnbEntry
 */

static gboolean
_text_key_press_cb (ClutterActor     *actor,
                    ClutterKeyEvent  *event,
                    MplEntry         *entry)
{
  /* Some of the keys are swallowed, i.e. they don't move the
   * focus away from the entry. */
  switch (event->keyval)
  {
    case CLUTTER_Return:
    /* case CLUTTER_Left: */
    /* case CLUTTER_Up: */
    case CLUTTER_Down:
    /* case CLUTTER_Page_Up: */
    case CLUTTER_Page_Down:
    case CLUTTER_Tab:
        g_signal_emit (entry, _signals[KEYNAV_EVENT], 0, event->keyval);
        return TRUE;
    case CLUTTER_Right:
      if (-1 == clutter_text_get_cursor_position (CLUTTER_TEXT (actor)))
      {
        g_signal_emit (entry, _signals[KEYNAV_EVENT], 0, event->keyval);
        return TRUE;
      }
      break;
    default:
      ;
  }

  return FALSE;
}

static void
_get_property (GObject    *object,
               guint       property_id,
               GValue     *value,
               GParamSpec *pspec)
{
  switch (property_id)
  {
  case PROP_HAS_KEYBOARD_FOCUS:
    g_value_set_boolean (value,
                         mnb_entry_get_has_keyboard_focus (MNB_ENTRY (object)));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
_set_property (GObject      *object,
               guint         property_id,
               const GValue *value,
               GParamSpec   *pspec)
{
  switch (property_id) {
  case PROP_HAS_KEYBOARD_FOCUS:
    mnb_entry_set_has_keyboard_focus (MNB_ENTRY (object),
                                      g_value_get_boolean (value));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static gboolean
_captured_event (ClutterActor *actor,
                 ClutterEvent *event)
{
  MnbEntryPrivate *priv = GET_PRIVATE (actor);

  /* Swallow keyboard events while navigating outside the widget. */
  if (CLUTTER_KEY_PRESS == event->type &&
      !priv->has_keyboard_focus)
  {
    ClutterKeyEvent *key_event = (ClutterKeyEvent *) event;

    switch (key_event->keyval)
    {
      case CLUTTER_Return:
      case CLUTTER_Left:
      case CLUTTER_Up:
      case CLUTTER_Right:
      case CLUTTER_Down:
      case CLUTTER_Page_Up:
      case CLUTTER_Page_Down:
      case CLUTTER_Tab:
        g_signal_emit (actor, _signals[KEYNAV_EVENT], 0, key_event->keyval);
        return TRUE;
      default:
        ;
    }
  }

  return FALSE;
}

static void
mnb_entry_class_init (MnbEntryClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  ClutterActorClass *actor_class = CLUTTER_ACTOR_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MnbEntryPrivate));

  object_class->get_property = _get_property;
  object_class->set_property = _set_property;

  actor_class->captured_event = _captured_event;

  /* Properties */

  g_object_class_install_property (object_class,
                                   PROP_HAS_KEYBOARD_FOCUS,
                                   g_param_spec_boolean ("has-keyboard-focus",
                                                         "Keyboard focus",
                                                         "If the widget has keyboard focus",
                                                         FALSE,
                                                         G_PARAM_READWRITE));

  /* Signals */

  _signals[KEYNAV_EVENT] = g_signal_new ("keynav-event",
                                         G_TYPE_FROM_CLASS (object_class),
                                         G_SIGNAL_RUN_LAST,
                                         0, NULL, NULL,
                                         g_cclosure_marshal_VOID__UINT,
                                         G_TYPE_NONE, 1, G_TYPE_UINT);
}

static void
mnb_entry_init (MnbEntry *self)
{
  MxWidget      *entry;
  ClutterActor  *text;

  entry = mpl_entry_get_mx_entry (MPL_ENTRY (self));
  text = mx_entry_get_clutter_text (MX_ENTRY (entry));
  g_signal_connect (text, "key-press-event",
                    G_CALLBACK (_text_key_press_cb),
                    self);
}

ClutterActor *
mnb_entry_new (const char *label)
{
  return g_object_new (MNB_TYPE_ENTRY,
                       "label", label,
                       NULL);
}

gboolean
mnb_entry_get_has_keyboard_focus (MnbEntry *self)
{
  MnbEntryPrivate *priv = GET_PRIVATE (self);

  g_return_val_if_fail (MNB_IS_ENTRY (self), FALSE);

  return priv->has_keyboard_focus;
}

void
mnb_entry_set_has_keyboard_focus (MnbEntry *self,
                                  gboolean  keyboard_focus)
{
  MnbEntryPrivate *priv = GET_PRIVATE (self);

  g_return_if_fail (MNB_IS_ENTRY (self));

  if (keyboard_focus != priv->has_keyboard_focus)
  {
    MxEntry     *entry = (MxEntry *) mpl_entry_get_mx_entry (MPL_ENTRY (self));
    ClutterText *text = (ClutterText *) mx_entry_get_clutter_text (entry);

    priv->has_keyboard_focus = keyboard_focus;
    g_object_notify (G_OBJECT (self), "has-keyboard-focus");

    if (priv->has_keyboard_focus)
    {
      clutter_text_set_selection (text, 0, -1);
      clutter_text_set_cursor_visible (text, TRUE);
    } else {
      gint pos = clutter_text_get_cursor_position (text);
      clutter_text_set_selection_bound (text, pos);
      clutter_text_set_cursor_visible (text, FALSE);
    }
  }
}

