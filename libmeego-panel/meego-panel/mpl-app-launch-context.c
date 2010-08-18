
/*
 * Copyright (c) 2010 Intel Corporation.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 *
 * Author: Rob Staudinger <robsta@linux.intel.com>
 */

#include "config.h"

#include <gdk/gdk.h>

#include "gdkapplaunchcontext-x11.h"
#include "mpl-app-launch-context.h"

G_DEFINE_TYPE (MplAppLaunchContext, mpl_app_launch_context, G_TYPE_APP_LAUNCH_CONTEXT)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MPL_TYPE_APP_LAUNCH_CONTEXT, MplAppLaunchContextPrivate))

typedef struct
{
  int dummy;
} MplAppLaunchContextPrivate;

static gchar *
_get_display (GAppLaunchContext *context,
              GAppInfo          *info,
              GList             *files)
{
  GdkDisplay *display;

  display = gdk_display_get_default ();

  return g_strdup (gdk_display_get_name (display));
}

static void
_dispose (GObject *object)
{
  G_OBJECT_CLASS (mpl_app_launch_context_parent_class)->dispose (object);
}

static void
mpl_app_launch_context_class_init (MplAppLaunchContextClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GAppLaunchContextClass *context_class = G_APP_LAUNCH_CONTEXT_CLASS (klass);

  gobject_class->dispose = _dispose;

  context_class->get_display = _get_display;
  context_class->get_startup_notify_id = mpl_gdk_windowing_get_startup_notify_id;
  context_class->launch_failed = mpl_gdk_windowing_launch_failed;

  g_type_class_add_private (klass, sizeof (MplAppLaunchContextPrivate));
}

static void
mpl_app_launch_context_init (MplAppLaunchContext *context)
{
}

GAppLaunchContext *
mpl_app_launch_context_new (void)
{
  return g_object_new (MPL_TYPE_APP_LAUNCH_CONTEXT, NULL);
}

