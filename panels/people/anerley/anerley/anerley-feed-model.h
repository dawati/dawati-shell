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


#ifndef _ANERLEY_FEED_MODEL
#define _ANERLEY_FEED_MODEL

#include <glib-object.h>
#include <anerley/anerley-feed.h>
#include <clutter/clutter.h>

G_BEGIN_DECLS

#define ANERLEY_TYPE_FEED_MODEL anerley_feed_model_get_type()

#define ANERLEY_FEED_MODEL(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), ANERLEY_TYPE_FEED_MODEL, AnerleyFeedModel))

#define ANERLEY_FEED_MODEL_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), ANERLEY_TYPE_FEED_MODEL, AnerleyFeedModelClass))

#define ANERLEY_IS_FEED_MODEL(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ANERLEY_TYPE_FEED_MODEL))

#define ANERLEY_IS_FEED_MODEL_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), ANERLEY_TYPE_FEED_MODEL))

#define ANERLEY_FEED_MODEL_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), ANERLEY_TYPE_FEED_MODEL, AnerleyFeedModelClass))

typedef struct {
  ClutterListModel parent;
} AnerleyFeedModel;

typedef struct {
  ClutterListModelClass parent_class;
} AnerleyFeedModelClass;

GType anerley_feed_model_get_type (void);

ClutterModel *anerley_feed_model_new (AnerleyFeed *feed);
void anerley_feed_model_set_filter_text (AnerleyFeedModel *model,
                                         const gchar      *filter_text);
G_END_DECLS

#endif /* _ANERLEY_FEED_MODEL */

