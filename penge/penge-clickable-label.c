/*
 * Copyright (C) 2010 Intel Corporation.
 *
 * Author: Rob Bradford <rob@linux.intel.com>
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
 *
 */

/* Strongly inspired by Emmanuele Bassi's 'tweet' */

#include "penge-clickable-label.h"

/* for pointer cursor support */
#include <clutter/x11/clutter-x11.h>
#include <X11/Xlib.h>
#include <X11/cursorfont.h>

G_DEFINE_TYPE (PengeClickableLabel, penge_clickable_label, MX_TYPE_LABEL)

#define GET_PRIVATE_REAL(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), PENGE_TYPE_CLICKABLE_LABEL, PengeClickableLabelPrivate))

#define GET_PRIVATE(o) ((PengeClickableLabel *)o)->priv

struct _PengeClickableLabelPrivate {
  GRegex *url_regex;
  GArray *matches;
};

static const char tweet_url_label_regex[] = "\\b https?://[\\S]+?(?=\\.?(\\s|$))";

typedef struct
{
  gint start, end;
} _UrlLabelMatch;

enum
{
  URL_CLICKED,
  LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0, };

static void
penge_clickable_label_get_property (GObject *object, guint property_id,
                              GValue *value, GParamSpec *pspec)
{
  switch (property_id) {
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
penge_clickable_label_set_property (GObject *object, guint property_id,
                              const GValue *value, GParamSpec *pspec)
{
  switch (property_id) {
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
penge_clickable_label_dispose (GObject *object)
{
  PengeClickableLabelPrivate *priv = GET_PRIVATE (object);

  if (priv->url_regex)
  {
    g_regex_unref (priv->url_regex);
    priv->url_regex = NULL;
  }

  G_OBJECT_CLASS (penge_clickable_label_parent_class)->dispose (object);
}

static void
penge_clickable_label_finalize (GObject *object)
{
  PengeClickableLabelPrivate *priv = GET_PRIVATE (object);

  g_array_free (priv->matches, TRUE);

  G_OBJECT_CLASS (penge_clickable_label_parent_class)->finalize (object);
}

static void
_update_attributes_from_matches (PengeClickableLabel *label,
                                 _UrlLabelMatch      *match_in)
{
  PengeClickableLabelPrivate *priv = GET_PRIVATE (label);
  PangoAttrList *attrs;
  PangoAttribute *attr;
  ClutterActor *text;
  const gchar *str;
  gint i = 0;

  text = mx_label_get_clutter_text (MX_LABEL (label));
  str = clutter_text_get_text (CLUTTER_TEXT (text));

  attrs = pango_attr_list_new ();

  for (i = 0; i < priv->matches->len; i++)
  {
    _UrlLabelMatch *match = &g_array_index (priv->matches,
                                            _UrlLabelMatch,
                                            i);

    if (match == match_in)
    {
      attr = pango_attr_foreground_new ((0xac * 0xffff) / 0xff,
                                       (0x30 * 0xffff) / 0xff,
                                       (0xe0 * 0xffff) / 0xff);
    } else {
      attr = pango_attr_foreground_new ((0x30 * 0xffff) / 0xff,
                                       (0xac * 0xffff) / 0xff,
                                       (0xe0 * 0xffff) / 0xff);
    }
    attr->start_index = match->start;
    attr->end_index = match->end;
    pango_attr_list_change (attrs, attr);
  }

  clutter_text_set_attributes (CLUTTER_TEXT (text), attrs);
  pango_attr_list_unref (attrs);
}

static void
_text_changed_notify_cb (ClutterText         *text,
                         GParamSpec          *pspec,
                         PengeClickableLabel *label)
{
  PengeClickableLabelPrivate *priv = GET_PRIVATE (label);

  /* Clear any existing matches */
  g_array_set_size (priv->matches, 0);

  if (priv->url_regex)
  {
    GMatchInfo *match_info;
    const gchar *str;

    str = clutter_text_get_text (text);

    /* Find each URL and keep track of its location */
    g_regex_match (priv->url_regex, str, 0, &match_info);
    while (g_match_info_matches (match_info))
    {
      _UrlLabelMatch match;

      if (g_match_info_fetch_pos (match_info, 0, &match.start, &match.end))
        g_array_append_val (priv->matches, match);

      g_match_info_next (match_info, NULL);
    }
    g_match_info_free (match_info);
  }

  /* If there is at least one URL then make sure the actor is reactive
     so we can detect when the cursor moves over it */
  if (priv->matches->len > 0)
  {
    clutter_actor_set_reactive (CLUTTER_ACTOR (label), TRUE);
    clutter_actor_set_reactive (CLUTTER_ACTOR (text), TRUE);
  }

  _update_attributes_from_matches (label, NULL);
}

static gboolean
_button_release_event_cb (ClutterActor        *actor,
                          ClutterEvent        *event,
                          PengeClickableLabel *label)
{
  PengeClickableLabelPrivate *priv = GET_PRIVATE (label);
  ClutterButtonEvent *bevent = (ClutterButtonEvent *)event;
  gfloat layout_x, layout_y;
  PangoLayout *layout;
  const gchar *str;
  gint i = 0, index = 0;

  clutter_actor_transform_stage_point (actor,
                                       bevent->x,
                                       bevent->y,
                                       &layout_x,
                                       &layout_y);


  layout = clutter_text_get_layout (CLUTTER_TEXT (actor));
  str = clutter_text_get_text (CLUTTER_TEXT (actor));

  if (pango_layout_xy_to_index (layout,
                                layout_x * PANGO_SCALE,
                                layout_y * PANGO_SCALE,
                                &index,
                                NULL))
  {
    /* Check whether that byte index is covered by any of the
       URL matches */
    for (i = 0; i < priv->matches->len; i++)
    {
      _UrlLabelMatch *match;
      match = &g_array_index (priv->matches, _UrlLabelMatch, i);

      if (index >= match->start && index < match->end)
      {
        gchar *url_text = g_strndup (str + match->start,
                                     match->end - match->start);
        g_signal_emit (label, signals[URL_CLICKED], 0, url_text);
        g_free (url_text);
        return TRUE;
      }
    }
  }

  return FALSE;
}


static gboolean
_motion_event_cb (ClutterActor        *actor,
                  ClutterEvent        *event,
                  PengeClickableLabel *label)
{
  PengeClickableLabelPrivate *priv = GET_PRIVATE (label);
  ClutterButtonEvent *bevent = (ClutterButtonEvent *)event;
  gfloat layout_x, layout_y;
  PangoLayout *layout;
  const gchar *str;
  gint i = 0, index = 0;
  Display *dpy;
  ClutterActor *stage;
  Window win;

  static Cursor hand = None;

  dpy = clutter_x11_get_default_display ();
  stage = clutter_actor_get_stage (actor);
  win = clutter_x11_get_stage_window (CLUTTER_STAGE (stage));

  if (hand == None)
    hand = XCreateFontCursor (dpy, XC_hand2);


  clutter_actor_transform_stage_point (actor,
                                       bevent->x,
                                       bevent->y,
                                       &layout_x,
                                       &layout_y);


  layout = clutter_text_get_layout (CLUTTER_TEXT (actor));
  str = clutter_text_get_text (CLUTTER_TEXT (actor));

  if (pango_layout_xy_to_index (layout,
                                layout_x * PANGO_SCALE,
                                layout_y * PANGO_SCALE,
                                &index,
                                NULL))
  {
    /* Check whether that byte index is covered by any of the
       URL matches */
    for (i = 0; i < priv->matches->len; i++)
    {
      _UrlLabelMatch *match;
      match = &g_array_index (priv->matches, _UrlLabelMatch, i);

      if (index >= match->start && index < match->end)
      {
        XDefineCursor (dpy, win, hand);

        _update_attributes_from_matches (label, match);
        return FALSE;
      }
    }
  }

  XUndefineCursor (dpy, win);
  _update_attributes_from_matches (label, NULL);
  return FALSE;
}

static gboolean
_leave_event_cb (ClutterActor        *actor,
                  ClutterEvent        *event,
                  PengeClickableLabel *label)
{
  Display *dpy;
  ClutterActor *stage;
  Window win;

  dpy = clutter_x11_get_default_display ();
  stage = clutter_actor_get_stage (actor);
  win = clutter_x11_get_stage_window (CLUTTER_STAGE (stage));

  /* Remove cursor */
  XUndefineCursor (dpy, win);

  _update_attributes_from_matches (label, NULL);
  return FALSE;
}



static void
penge_clickable_label_constructed (GObject *object)
{
  ClutterActor *text;

  text = mx_label_get_clutter_text (MX_LABEL (object));

  g_signal_connect (text,
                    "notify::text",
                    (GCallback)_text_changed_notify_cb,
                    object);
  g_object_notify (G_OBJECT (text), "text");

  g_signal_connect (text,
                    "button-release-event",
                    (GCallback)_button_release_event_cb,
                    object);

  g_signal_connect (text,
                    "motion-event",
                    (GCallback)_motion_event_cb,
                    object);

  g_signal_connect (text,
                    "leave-event",
                    (GCallback)_leave_event_cb,
                    object);

  if (G_OBJECT_CLASS (penge_clickable_label_parent_class)->constructed)
    G_OBJECT_CLASS (penge_clickable_label_parent_class)->constructed (object);
}


static void
penge_clickable_label_class_init (PengeClickableLabelClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (PengeClickableLabelPrivate));

  object_class->get_property = penge_clickable_label_get_property;
  object_class->set_property = penge_clickable_label_set_property;
  object_class->dispose = penge_clickable_label_dispose;
  object_class->finalize = penge_clickable_label_finalize;
  object_class->constructed = penge_clickable_label_constructed;

  signals[URL_CLICKED] = g_signal_new ("url-clicked",
                                       PENGE_TYPE_CLICKABLE_LABEL,
                                       G_SIGNAL_RUN_FIRST,
                                       0,
                                       0,
                                       NULL,
                                       g_cclosure_marshal_VOID__STRING,
                                       G_TYPE_NONE,
                                       1,
                                       G_TYPE_STRING);
}

static void
penge_clickable_label_init (PengeClickableLabel *self)
{
  PengeClickableLabelPrivate *priv = GET_PRIVATE_REAL (self);
  GError *error = NULL;

  self->priv = priv;

  priv->url_regex = g_regex_new (tweet_url_label_regex,
                                 G_REGEX_CASELESS
                                 | G_REGEX_EXTENDED
                                 | G_REGEX_NO_AUTO_CAPTURE,
                                 0, &error);

  if (priv->url_regex == NULL)
  {
    g_critical ("Compilation of URL matching regex failed: %s",
                error->message);
    g_clear_error (&error);
  }

  priv->matches = g_array_new (FALSE, FALSE, sizeof (_UrlLabelMatch));
}

ClutterActor *
penge_clickable_label_new (const gchar *text)
{
  if (!text)
    text = "";

  return g_object_new (PENGE_TYPE_CLICKABLE_LABEL,
                       "text", text,
                       NULL);
}

