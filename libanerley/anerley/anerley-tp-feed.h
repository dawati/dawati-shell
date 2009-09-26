/*
 * Anerley - people feeds and widgets
 * Copyright (C) 2009, Intel Corporation.
 *
 * Authors: Rob Bradford <rob@linux.intel.com>
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


#ifndef _ANERLEY_TP_FEED
#define _ANERLEY_TP_FEED

#include <telepathy-glib/account.h>

#include <glib-object.h>

G_BEGIN_DECLS

#define ANERLEY_TYPE_TP_FEED anerley_tp_feed_get_type()

#define ANERLEY_TP_FEED(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), ANERLEY_TYPE_TP_FEED, AnerleyTpFeed))

#define ANERLEY_TP_FEED_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), ANERLEY_TYPE_TP_FEED, AnerleyTpFeedClass))

#define ANERLEY_IS_TP_FEED(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ANERLEY_TYPE_TP_FEED))

#define ANERLEY_IS_TP_FEED_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), ANERLEY_TYPE_TP_FEED))

#define ANERLEY_TP_FEED_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), ANERLEY_TYPE_TP_FEED, AnerleyTpFeedClass))

typedef struct {
  GObject parent;
} AnerleyTpFeed;

typedef struct {
  GObjectClass parent_class;
} AnerleyTpFeedClass;

GType anerley_tp_feed_get_type (void);

AnerleyTpFeed *anerley_tp_feed_new (TpAccount *account);

G_END_DECLS

#endif /* _ANERLEY_TP_FEED */
