
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
#include "mpl-app-launches-store.h"

G_DEFINE_TYPE (MplAppLaunchContext, mpl_app_launch_context, G_TYPE_APP_LAUNCH_CONTEXT)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MPL_TYPE_APP_LAUNCH_CONTEXT, MplAppLaunchContextPrivate))

typedef struct
{
  MplAppLaunchesStore *store;
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

static char *
_get_startup_notify_id (GAppLaunchContext *context,
                        GAppInfo          *info,
                        GList             *files)
{
  MplAppLaunchContextPrivate *priv = GET_PRIVATE (context);
  char const  *executable;
  GError      *error = NULL;

  executable = g_app_info_get_executable (info);

  mpl_app_launches_store_add (priv->store, executable, &error);
  if (error)
  {
    g_warning ("%s : %s", G_STRLOC, error->message);
    g_clear_error (&error);
  }

  return mpl_gdk_windowing_get_startup_notify_id (context, info, files);
}

static void
_dispose (GObject *object)
{
  MplAppLaunchContextPrivate *priv = GET_PRIVATE (object);

  if (priv->store)
  {
    g_object_unref (priv->store);
    priv->store = NULL;
  }

  G_OBJECT_CLASS (mpl_app_launch_context_parent_class)->dispose (object);
}

static void
mpl_app_launch_context_class_init (MplAppLaunchContextClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GAppLaunchContextClass *context_class = G_APP_LAUNCH_CONTEXT_CLASS (klass);

  gobject_class->dispose = _dispose;

  context_class->get_display = _get_display;
  context_class->get_startup_notify_id = _get_startup_notify_id;
  context_class->launch_failed = mpl_gdk_windowing_launch_failed;

  g_type_class_add_private (klass, sizeof (MplAppLaunchContextPrivate));
}

static void
mpl_app_launch_context_init (MplAppLaunchContext *self)
{
  MplAppLaunchContextPrivate *priv = GET_PRIVATE (self);

  priv->store = mpl_app_launches_store_new ();
}

GAppLaunchContext *
mpl_app_launch_context_new (void)
{
  return g_object_new (MPL_TYPE_APP_LAUNCH_CONTEXT, NULL);
}

GAppLaunchContext *
mpl_app_launch_context_get_default (void)
{
  static GAppLaunchContext *_context = NULL;

  if (NULL == _context)
  {
    _context = mpl_app_launch_context_new ();
  }

  return _context;
}

