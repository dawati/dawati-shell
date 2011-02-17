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
 */

#include "mps-tweet-card.h"
#include "penge-magic-texture.h"
#include "penge-clickable-label.h"
#include <gio/gio.h>
#include <glib/gi18n.h>

G_DEFINE_TYPE (MpsTweetCard, mps_tweet_card, MX_TYPE_WIDGET)

#define GET_PRIVATE_REAL(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MPS_TYPE_TWEET_CARD, MpsTweetCardPrivate))
#define GET_PRIVATE(o) ((MpsTweetCard *)o)->priv

struct _MpsTweetCardPrivate {
  SwItem *item;
  ClutterActor *inner_table;

  ClutterActor *avatar_frame;
  ClutterActor *avatar;

  ClutterActor *content_label;
  ClutterActor *secondary_label;

  ClutterActor *button_box;
};

enum
{
  PROP_0,
  PROP_ITEM
};

enum
{
  REPLY_CLICKED_SIGNAL,
  RETWEET_CLICKED_SIGNAL,
  LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = {0, };

#define DEFAULT_AVATAR_PATH THEMEDIR "/avatar_icon.png"

static void
mps_tweet_card_get_property (GObject *object, guint property_id,
                              GValue *value, GParamSpec *pspec)
{
  MpsTweetCard *card = MPS_TWEET_CARD (object);

  switch (property_id) {
    case PROP_ITEM:
      g_value_set_boxed (value, mps_tweet_card_get_item (card));
      break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
mps_tweet_card_set_property (GObject *object, guint property_id,
                              const GValue *value, GParamSpec *pspec)
{
  MpsTweetCard *card = MPS_TWEET_CARD (object);

  switch (property_id) {
    case PROP_ITEM:
      mps_tweet_card_set_item (card, g_value_get_boxed (value));
      break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
mps_tweet_card_dispose (GObject *object)
{
  MpsTweetCardPrivate *priv = GET_PRIVATE (object);

  if (priv->item)
  {
    sw_item_unref (priv->item);
    priv->item = NULL;
  }

  G_OBJECT_CLASS (mps_tweet_card_parent_class)->dispose (object);
}

static void
mps_tweet_card_finalize (GObject *object)
{
  G_OBJECT_CLASS (mps_tweet_card_parent_class)->finalize (object);
}

static void
_reply_button_clicked_cb (MxButton     *button,
                          MpsTweetCard *card)
{
  g_signal_emit (card, signals[REPLY_CLICKED_SIGNAL], 0);
}

static void
_retweet_button_clicked_cb (MxButton     *button,
                            MpsTweetCard *card)
{
  g_signal_emit (card, signals[RETWEET_CLICKED_SIGNAL], 0);
}

static void
mps_tweet_card_constructed (GObject *object)
{
  MpsTweetCardPrivate *priv = GET_PRIVATE (object);

  if (g_str_equal (priv->item->service, "twitter"))
  {
    ClutterActor *button;
    ClutterActor *icon;

    button = mx_button_new ();
    icon = mx_icon_new ();
    mx_bin_set_child (MX_BIN (button), icon);
    mx_stylable_set_style_class (MX_STYLABLE (button),
                                 "mps-tweet-card-reply-button");

    clutter_container_add_actor (CLUTTER_CONTAINER (priv->button_box),
                                 button);

    g_signal_connect (button,
                      "clicked",
                      (GCallback)_reply_button_clicked_cb,
                      object);

    button = mx_button_new ();
    icon = mx_icon_new ();
    mx_bin_set_child (MX_BIN (button), icon);
    mx_stylable_set_style_class (MX_STYLABLE (button),
                                 "mps-tweet-card-retweet-button");

    clutter_container_add_actor (CLUTTER_CONTAINER (priv->button_box),
                                 button);
    g_signal_connect (button,
                      "clicked",
                      (GCallback)_retweet_button_clicked_cb,
                      object);
  }

  if (G_OBJECT_CLASS (mps_tweet_card_parent_class)->constructed)
    G_OBJECT_CLASS (mps_tweet_card_parent_class)->constructed (object);
}


static void
mps_tweet_card_map (ClutterActor *actor)
{
  MpsTweetCardPrivate *priv = GET_PRIVATE (actor);

  CLUTTER_ACTOR_CLASS (mps_tweet_card_parent_class)->map (actor);

  clutter_actor_map (priv->avatar_frame);
  clutter_actor_map (priv->content_label);
  clutter_actor_map (priv->secondary_label);
  clutter_actor_map (priv->button_box);
}

static void
mps_tweet_card_unmap (ClutterActor *actor)
{
  MpsTweetCardPrivate *priv = GET_PRIVATE (actor);

  CLUTTER_ACTOR_CLASS (mps_tweet_card_parent_class)->unmap (actor);

  clutter_actor_unmap (priv->avatar_frame);
  clutter_actor_unmap (priv->content_label);
  clutter_actor_unmap (priv->secondary_label);
  clutter_actor_unmap (priv->button_box);
}

static void
mps_tweet_card_paint (ClutterActor *actor)
{
  MpsTweetCardPrivate *priv = GET_PRIVATE (actor);

  CLUTTER_ACTOR_CLASS (mps_tweet_card_parent_class)->paint (actor);

  clutter_actor_paint (priv->avatar_frame);
  clutter_actor_paint (priv->content_label);
  clutter_actor_paint (priv->secondary_label);
  clutter_actor_paint (priv->button_box);
}

static void
mps_tweet_card_pick (ClutterActor       *actor,
                   const ClutterColor *color)
{
  MpsTweetCardPrivate *priv = GET_PRIVATE (actor);

  CLUTTER_ACTOR_CLASS (mps_tweet_card_parent_class)->pick (actor, color);

  clutter_actor_paint (priv->avatar_frame);
  clutter_actor_paint (priv->content_label);
  clutter_actor_paint (priv->secondary_label);
  clutter_actor_paint (priv->button_box);
}

#define COL_SPACING 8.0
#define ROW_SPACING 0.0

static void
mps_tweet_card_allocate (ClutterActor           *actor,
                         const ClutterActorBox  *box,
                         ClutterAllocationFlags  flags)
{
  MpsTweetCardPrivate *priv = GET_PRIVATE (actor);
  MxPadding padding;
  gfloat width, height;
  ClutterActorBox avatar_box;
  ClutterActorBox content_label_box;
  ClutterActorBox secondary_label_box;
  ClutterActorBox button_box_box;
  gfloat nat_h, min_h, nat_w, min_w;
  gfloat avail_height, avail_width;

  CLUTTER_ACTOR_CLASS (mps_tweet_card_parent_class)->allocate (actor,
                                                               box,
                                                               flags);

  mx_widget_get_padding (MX_WIDGET (actor), &padding);

  width = box->x2 - box->x1;
  height = box->y2 - box->y1;

  /* Avatar frame. Should be vertically centred in the available height */
  clutter_actor_get_preferred_size (priv->avatar_frame,
                                    &min_w,
                                    &min_h,
                                    &nat_w,
                                    &nat_h);

  avatar_box.x1 = padding.left;
  avatar_box.x2 = avatar_box.x1 + nat_w;
  avatar_box.y1 = (gint)((height - nat_h) / 2);
  avatar_box.y2 = avatar_box.y1 + nat_h;

  avail_height = avatar_box.y2 - avatar_box.y1;

  clutter_actor_allocate (priv->avatar_frame, &avatar_box, flags);

  clutter_actor_get_preferred_size (priv->button_box,
                                    &min_w,
                                    &min_h,
                                    &nat_w,
                                    &nat_h);

  /* Position the button box at the end of the row middle aligned relative */
  button_box_box.x2 = width - padding.right;
  button_box_box.x1 = button_box_box.x2 - nat_w;
  button_box_box.y1 = (gint)((height - nat_h) / 2);
  button_box_box.y2 = button_box_box.y1 + nat_h;

  clutter_actor_allocate (priv->button_box, &button_box_box, flags);

  /* Max widths for label are between the the avatar box and the buttons */
  avail_width = button_box_box.x1 - avatar_box.x2 - COL_SPACING * 2;

  /* Do secondary label first */
  clutter_actor_get_preferred_height (priv->secondary_label,
                                      avail_width,
                                      NULL,
                                      &nat_h);
  secondary_label_box.y2 = avatar_box.y2;
  secondary_label_box.y1 = avatar_box.y2 - nat_h;
  secondary_label_box.x1 = avatar_box.x2 + COL_SPACING;
  secondary_label_box.x2 = button_box_box.x1 - COL_SPACING;

  clutter_actor_allocate (priv->secondary_label, &secondary_label_box, flags);

  /* Then position primary in the space left */
  content_label_box.x1 = avatar_box.x2 + COL_SPACING;
  content_label_box.x2 = button_box_box.x1 - COL_SPACING;
  content_label_box.y1 = avatar_box.y1;
  content_label_box.y2 = secondary_label_box.y1 - ROW_SPACING;

  clutter_actor_allocate (priv->content_label, &content_label_box, flags);
}

static void
mps_tweet_card_class_init (MpsTweetCardClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  ClutterActorClass *actor_class = CLUTTER_ACTOR_CLASS (klass);
  GParamSpec *pspec;

  g_type_class_add_private (klass, sizeof (MpsTweetCardPrivate));

  object_class->get_property = mps_tweet_card_get_property;
  object_class->set_property = mps_tweet_card_set_property;
  object_class->dispose = mps_tweet_card_dispose;
  object_class->finalize = mps_tweet_card_finalize;
  object_class->constructed = mps_tweet_card_constructed;


  actor_class->paint = mps_tweet_card_paint;
  actor_class->pick = mps_tweet_card_pick;
  actor_class->map = mps_tweet_card_map;
  actor_class->unmap = mps_tweet_card_unmap;
  actor_class->allocate = mps_tweet_card_allocate;


  pspec = g_param_spec_boxed ("item",
                              "Item",
                              "Item",
                              SW_TYPE_ITEM,
                              G_PARAM_READWRITE | G_PARAM_CONSTRUCT);
  g_object_class_install_property (object_class, PROP_ITEM, pspec);

  signals[REPLY_CLICKED_SIGNAL] = g_signal_new ("reply-clicked",
                                                MPS_TYPE_TWEET_CARD,
                                                G_SIGNAL_RUN_FIRST,
                                                0,
                                                NULL,
                                                NULL,
                                                g_cclosure_marshal_VOID__VOID,
                                                G_TYPE_NONE,
                                                0);

  signals[RETWEET_CLICKED_SIGNAL] = g_signal_new ("retweet-clicked",
                                                  MPS_TYPE_TWEET_CARD,
                                                  G_SIGNAL_RUN_FIRST,
                                                  0,
                                                  NULL,
                                                  NULL,
                                                  g_cclosure_marshal_VOID__VOID,
                                                  G_TYPE_NONE,
                                                  0);
}

void meego_status_panel_hide (void);

static gboolean
_button_release_event_cb (ClutterActor *actor,
                          ClutterEvent *event,
                          gpointer      userdata)
{
  MpsTweetCardPrivate *priv = GET_PRIVATE (actor);
  GError *error = NULL;
  const gchar *uri;

  uri = sw_item_get_value (priv->item, "url");

  if (!uri)
    return TRUE;

  if (!g_app_info_launch_default_for_uri (uri,
                                          NULL,
                                          &error))
  {
    g_warning (G_STRLOC ": Error launching uri: %s",
               error->message);
    g_clear_error (&error);
  } else {
    meego_status_panel_hide ();
  }

  return TRUE;
}

static void
_label_url_clicked_cb (PengeClickableLabel *label,
                       const gchar         *url,
                       MpsTweetCard        *card)
{
  GError *error = NULL;

  if (!g_app_info_launch_default_for_uri (url,
                                          NULL,
                                          &error))
  {
    g_warning (G_STRLOC ": Error launching uri: %s",
               error->message);
    g_clear_error (&error);
  } else {
    meego_status_panel_hide ();
  }
}

static void
mps_tweet_card_init (MpsTweetCard *self)
{
  MpsTweetCardPrivate *priv = GET_PRIVATE_REAL (self);
  ClutterActor *tmp_text;

  self->priv = priv;

  /* Avatar frame & avatar */
  priv->avatar_frame = mx_frame_new ();
  mx_stylable_set_style_class (MX_STYLABLE (priv->avatar_frame),
                               "mps-tweet-avatar-frame");
  priv->avatar = g_object_new (PENGE_TYPE_MAGIC_TEXTURE,
                               NULL);
  clutter_actor_set_size (priv->avatar, 48, 48);
  clutter_container_add_actor (CLUTTER_CONTAINER (priv->avatar_frame),
                               priv->avatar);
  mx_bin_set_fill (MX_BIN (priv->avatar_frame), TRUE, TRUE);
  clutter_actor_set_parent (priv->avatar_frame,
                            CLUTTER_ACTOR (self));

  priv->content_label = penge_clickable_label_new (NULL);
  g_signal_connect (priv->content_label,
                    "url-clicked",
                    (GCallback)_label_url_clicked_cb,
                    self);
  mx_stylable_set_style_class (MX_STYLABLE (priv->content_label),
                               "mps-tweet-content-label");
  clutter_actor_set_parent (priv->content_label,
                            CLUTTER_ACTOR (self));

  tmp_text = mx_label_get_clutter_text (MX_LABEL (priv->content_label));
  clutter_text_set_line_wrap (CLUTTER_TEXT (tmp_text), TRUE),

  priv->secondary_label = mx_label_new ();
  mx_stylable_set_style_class (MX_STYLABLE (priv->secondary_label),
                               "mps-tweet-secondary-label");
  clutter_actor_set_parent (priv->secondary_label,
                            CLUTTER_ACTOR (self));

  g_signal_connect (self,
                    "button-release-event",
                    (GCallback)_button_release_event_cb,
                    NULL);

  priv->button_box = mx_box_layout_new ();
  mx_box_layout_set_orientation (MX_BOX_LAYOUT (priv->button_box),
                                 MX_ORIENTATION_VERTICAL);
  mx_box_layout_set_spacing (MX_BOX_LAYOUT (priv->button_box), 8);
  clutter_actor_set_parent (priv->button_box,
                            CLUTTER_ACTOR (self));

  clutter_actor_set_reactive (CLUTTER_ACTOR (self), TRUE);
}

ClutterActor *
mps_tweet_card_new (void)
{
  return g_object_new (MPS_TYPE_TWEET_CARD, NULL);
}

SwItem *
mps_tweet_card_get_item (MpsTweetCard *card)
{
  MpsTweetCardPrivate *priv = GET_PRIVATE (card);

  return priv->item;
}

static void
mps_tweet_card_set_time (MpsTweetCard *card)
{
  MpsTweetCardPrivate *priv = GET_PRIVATE (card);
  const gchar *place_fullname;
  gchar *time_str;
  gchar *secondary_msg;

  time_str = mx_utils_format_time (&(priv->item->date));

  place_fullname = sw_item_get_value (priv->item, "place_full_name");

  if (place_fullname)
  {
    /* When the tweet has a location associated with it then this string will
     * be <human readable time> from <human readable place name>
     *
     * e.g. A couple of hours ago from Aldgate, London
     */
    secondary_msg = g_strdup_printf (_("%s from %s"),
                                     time_str,
                                     place_fullname);
    mx_label_set_text (MX_LABEL (priv->secondary_label), secondary_msg);
    g_free (secondary_msg);
  } else {
    mx_label_set_text (MX_LABEL (priv->secondary_label), time_str);
  }

  g_free (time_str);
}

void
mps_tweet_card_set_item (MpsTweetCard *card,
                         SwItem       *item)
{
  MpsTweetCardPrivate *priv = GET_PRIVATE (card);
  const gchar *author_icon = NULL;
  const gchar *content = NULL;
  const gchar *author = NULL;
  gchar *combined_content;
  GError *error = NULL;
  ClutterActor *tmp_text;

  priv->item = sw_item_ref (item);

  author_icon = sw_item_get_value (item, "authoricon");

  if (!author_icon)
  {
    author_icon = DEFAULT_AVATAR_PATH;
  }

  if (!clutter_texture_set_from_file (CLUTTER_TEXTURE (priv->avatar),
                                      author_icon,
                                      &error))
  {
    g_critical (G_STRLOC ": Error setting avatar texture: %s",
                error->message);
    g_clear_error (&error);
  }

  content = sw_item_get_value (item, "content");
  author = sw_item_get_value (item, "author");

  combined_content = g_markup_printf_escaped("<b>%s</b> %s",
                                             author,
                                             content);

  tmp_text = mx_label_get_clutter_text (MX_LABEL (priv->content_label));
  clutter_text_set_markup (CLUTTER_TEXT (tmp_text),
                           combined_content);
  g_free (combined_content);

  mps_tweet_card_set_time (card);
}

void
mps_tweet_card_refresh (MpsTweetCard *card)
{
  mps_tweet_card_set_time (card);
}
