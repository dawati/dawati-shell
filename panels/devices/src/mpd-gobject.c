
/*
 * Copyright © 2010 Intel Corp.
 *
 * Authors: Rob Staudinger <robert.staudinger@intel.com>
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

#include "mpd-gobject.h"
#include "config.h"

/*
 * Disconnect signal handlers from object to self, unref object and NULL it.
 * This prevents stale handlers when object is a singleton or otherwise
 * referenced from multiple locations.
 */
void
mpd_gobject_detach (GObject   *self,
                    GObject  **object_ptr)
{
  g_return_if_fail (self);

  if (object_ptr && *object_ptr)
  {
    GObject *object = *object_ptr;
    int n_handlers = g_signal_handlers_disconnect_matched (object,
                                                           G_SIGNAL_MATCH_DATA,
                                                           0, 0, NULL, NULL,
                                                           self);
    g_debug ("Disconnected %i handlers on %s from %s",
             n_handlers,
             G_OBJECT_TYPE_NAME (self),
             G_OBJECT_TYPE_NAME (object));

    g_object_unref (object);
    *object_ptr = NULL;
  }
}

