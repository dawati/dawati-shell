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


#include "anerley-feed.h"

static void
anerley_feed_base_init (gpointer g_class)
{
  static gboolean initialized = FALSE;

  if (!initialized) {
    /* create interface signals here. */
    initialized = TRUE;
    g_signal_new ("items-added",
                  ANERLEY_TYPE_FEED,
                  G_SIGNAL_RUN_FIRST,
                  0,
                  NULL,
                  NULL,
                  g_cclosure_marshal_VOID__POINTER,
                  G_TYPE_NONE,
                  1,
                  G_TYPE_POINTER);

    g_signal_new ("items-removed",
                  ANERLEY_TYPE_FEED,
                  G_SIGNAL_RUN_FIRST,
                  0,
                  NULL,
                  NULL,
                  g_cclosure_marshal_VOID__POINTER,
                  G_TYPE_NONE,
                  1,
                  G_TYPE_POINTER);

    g_signal_new ("items-changed",
                  ANERLEY_TYPE_FEED,
                  G_SIGNAL_RUN_FIRST,
                  0,
                  NULL,
                  NULL,
                  g_cclosure_marshal_VOID__POINTER,
                  G_TYPE_NONE,
                  1,
                  G_TYPE_POINTER);
  }
}

GType
anerley_feed_get_type (void)
{
  static GType type = 0;
  if (type == 0) {
    static const GTypeInfo info = {
      sizeof (AnerleyFeedInterface),
      anerley_feed_base_init,   /* base_init */
      NULL,

    };
    type = g_type_register_static (G_TYPE_INTERFACE,
      "AnerleyFeed", &info, 0);
    g_type_interface_add_prerequisite (type, G_TYPE_OBJECT);
  }
  return type;
}
