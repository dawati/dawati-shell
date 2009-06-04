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


#ifndef _ANERLEY_EBOOK_FEED
#define _ANERLEY_EBOOK_FEED

#include <glib-object.h>
#include <libebook/e-book.h>

G_BEGIN_DECLS

#define ANERLEY_TYPE_EBOOK_FEED anerley_ebook_feed_get_type()

#define ANERLEY_EBOOK_FEED(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), ANERLEY_TYPE_EBOOK_FEED, AnerleyEBookFeed))

#define ANERLEY_EBOOK_FEED_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), ANERLEY_TYPE_EBOOK_FEED, AnerleyEBookFeedClass))

#define ANERLEY_IS_EBOOK_FEED(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ANERLEY_TYPE_EBOOK_FEED))

#define ANERLEY_IS_EBOOK_FEED_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), ANERLEY_TYPE_EBOOK_FEED))

#define ANERLEY_EBOOK_FEED_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), ANERLEY_TYPE_EBOOK_FEED, AnerleyEBookFeedClass))

typedef struct {
  GObject parent;
} AnerleyEBookFeed;

typedef struct {
  GObjectClass parent_class;
} AnerleyEBookFeedClass;

GType anerley_ebook_feed_get_type (void);

AnerleyEBookFeed *anerley_ebook_feed_new (EBook *book);

G_END_DECLS

#endif /* _ANERLEY_EBOOK_FEED */


