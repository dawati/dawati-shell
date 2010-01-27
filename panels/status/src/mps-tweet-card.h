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

#ifndef _MPS_TWEET_CARD
#define _MPS_TWEET_CARD

#include <glib-object.h>
#include <mx/mx.h>
#include <mojito-client/mojito-item.h>

G_BEGIN_DECLS

#define MPS_TYPE_TWEET_CARD mps_tweet_card_get_type()

#define MPS_TWEET_CARD(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), MPS_TYPE_TWEET_CARD, MpsTweetCard))

#define MPS_TWEET_CARD_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), MPS_TYPE_TWEET_CARD, MpsTweetCardClass))

#define MPS_IS_TWEET_CARD(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MPS_TYPE_TWEET_CARD))

#define MPS_IS_TWEET_CARD_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), MPS_TYPE_TWEET_CARD))

#define MPS_TWEET_CARD_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), MPS_TYPE_TWEET_CARD, MpsTweetCardClass))

typedef struct {
  MxButton parent;
} MpsTweetCard;

typedef struct {
  MxButtonClass parent_class;
} MpsTweetCardClass;

GType mps_tweet_card_get_type (void);

ClutterActor *mps_tweet_card_new (void);
void mps_tweet_card_set_item (MpsTweetCard *card,
                              MojitoItem   *item);
MojitoItem *mps_tweet_card_get_item (MpsTweetCard *card);

G_END_DECLS

#endif /* _MPS_TWEET_CARD */

