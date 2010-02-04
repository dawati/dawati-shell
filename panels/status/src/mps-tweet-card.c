/*
 * Copyright (C) 2010 Intel Corporation.
 *
 * Author: Rob bradford <rob@linux.intel.com>
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

#include <gio/gio.h>

G_DEFINE_TYPE (MpsTweetCard, mps_tweet_card, MX_TYPE_BUTTON)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MPS_TYPE_TWEET_CARD, MpsTweetCardPrivate))

typedef struct _MpsTweetCardPrivate MpsTweetCardPrivate;

struct _MpsTweetCardPrivate {
  SwItem *item;
  ClutterActor *inner_table;

  ClutterActor *avatar_frame;
  ClutterActor *avatar;

  ClutterActor *content_label;
  ClutterActor *secondary_label;
};

enum
{
  PROP_0,
  PROP_ITEM
};

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
mps_tweet_card_class_init (MpsTweetCardClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GParamSpec *pspec;

  g_type_class_add_private (klass, sizeof (MpsTweetCardPrivate));

  object_class->get_property = mps_tweet_card_get_property;
  object_class->set_property = mps_tweet_card_set_property;
  object_class->dispose = mps_tweet_card_dispose;
  object_class->finalize = mps_tweet_card_finalize;

  pspec = g_param_spec_boxed ("item",
                              "Item",
                              "Item",
                              SW_TYPE_ITEM,
                              G_PARAM_READWRITE);
  g_object_class_install_property (object_class, PROP_ITEM, pspec);
}

void moblin_status_panel_hide (void);

static void
_button_clicked_cb (MxButton *button,
                    gpointer  userdata)
{
  MpsTweetCardPrivate *priv = GET_PRIVATE (button);
  GError *error = NULL;
  const gchar *uri;

  uri = sw_item_get_value (priv->item, "url");

  if (!uri)
    return;

  if (!g_app_info_launch_default_for_uri (uri,
                                          NULL,
                                          &error))
  {
    g_warning (G_STRLOC ": Error launching uri: %s",
               error->message);
    g_clear_error (&error);
  } else {
    moblin_status_panel_hide ();
  }
}

static void
mps_tweet_card_init (MpsTweetCard *self)
{
  MpsTweetCardPrivate *priv = GET_PRIVATE (self);
  ClutterActor *tmp_text;

  priv->inner_table = mx_table_new ();
  mx_table_set_col_spacing (MX_TABLE (priv->inner_table), 8);

  mx_bin_set_child (MX_BIN (self), priv->inner_table);
  mx_bin_set_fill (MX_BIN (self), TRUE, TRUE);

  priv->avatar_frame = mx_frame_new ();
  mx_stylable_set_style_class (MX_STYLABLE (priv->avatar_frame),
                               "mps-tweet-avatar-frame");
  priv->avatar = g_object_new (PENGE_TYPE_MAGIC_TEXTURE,
                               NULL);
  clutter_actor_set_size (priv->avatar, 48, 48);
  clutter_container_add_actor (CLUTTER_CONTAINER (priv->avatar_frame),
                               priv->avatar);
  mx_bin_set_fill (MX_BIN (priv->avatar_frame), TRUE, TRUE);

  mx_table_add_actor_with_properties (MX_TABLE (priv->inner_table),
                                      priv->avatar_frame,
                                      0, 0,
                                      "x-expand", FALSE,
                                      "y-expand", TRUE,
                                      "y-fill", FALSE,
                                      "row-span", 2,
                                      NULL);

  priv->content_label = mx_label_new ("");
  mx_stylable_set_style_class (MX_STYLABLE (priv->content_label),
                               "mps-tweet-content-label");

  tmp_text = mx_label_get_clutter_text (MX_LABEL (priv->content_label));
  clutter_text_set_line_wrap (CLUTTER_TEXT (tmp_text), TRUE),

  mx_table_add_actor_with_properties (MX_TABLE (priv->inner_table),
                                      priv->content_label,
                                      0, 1,
                                      "y-expand", TRUE,
                                      "y-align", 0.0,
                                      "x-expand", TRUE,
                                      "x-fill", TRUE,
                                      NULL);

  priv->secondary_label = mx_label_new ("");
  mx_stylable_set_style_class (MX_STYLABLE (priv->secondary_label),
                               "mps-tweet-secondary-label");
  mx_table_add_actor_with_properties (MX_TABLE (priv->inner_table),
                                      priv->secondary_label,
                                      1, 1,
                                      "y-expand", FALSE,
                                      "x-expand", TRUE,
                                      "x-fill", TRUE,
                                      NULL);

  g_signal_connect (self,
                    "clicked",
                    (GCallback)_button_clicked_cb,
                    NULL);
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
}

