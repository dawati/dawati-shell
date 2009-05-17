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



#ifndef ANERLEY_FEED_H
#define ANERLEY_FEED_H

#include <glib-object.h>

#define ANERLEY_TYPE_FEED (anerley_feed_get_type ())
#define ANERLEY_FEED(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj),\
                           ANERLEY_TYPE_FEED, AnerleyFeed))
#define ANERLEY_IS_FEED(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj),\
                              ANERLEY_TYPE_FEED))
#define ANERLEY_FEED_GET_INTERFACE(inst) (G_TYPE_INSTANCE_GET_INTERFACE ((inst),\
                                          ANERLEY_TYPE_FEED, AnerleyFeedInterface))

typedef struct _AnerleyFeed AnerleyFeed;
typedef struct _AnerleyFeedInterface AnerleyFeedInterface;

struct _AnerleyFeedInterface {
  GTypeInterface parent;

  /* No vfuncs ? */
};

GType anerley_feed_get_type (void);

#endif /* ANERLEY_FEED_H */
