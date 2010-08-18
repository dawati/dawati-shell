
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

#include "mpl-app-launches-query.h"
#include "mpl-app-launches-store-priv.h"

G_DEFINE_TYPE (MplAppLaunchesQuery, mpl_app_launches_query, G_TYPE_OBJECT)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MPL_TYPE_APP_LAUNCHES_QUERY, MplAppLaunchesQueryPrivate))

enum
{
  PROP_0,
  PROP_STORE
};

typedef struct
{
  MplAppLaunchesStore *store;
} MplAppLaunchesQueryPrivate;

static void
_get_property (GObject    *object,
               unsigned    property_id,
               GValue     *value,
               GParamSpec *pspec)
{
  /* MplAppLaunchesQueryPrivate *priv = GET_PRIVATE (object); */

  switch (property_id)
  {
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
_set_property (GObject      *object,
               unsigned      property_id,
               const GValue *value,
               GParamSpec   *pspec)
{
  MplAppLaunchesQueryPrivate *priv = GET_PRIVATE (object);

  switch (property_id)
  {
  case PROP_STORE:
    /* Construct-only */
    priv->store = g_value_dup_object (value);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
_constructed (GObject *object)
{
  MplAppLaunchesQueryPrivate *priv = GET_PRIVATE (object);

  mpl_app_launches_store_open (priv->store, false, NULL);
}

static void
_dispose (GObject *object)
{
  MplAppLaunchesQueryPrivate *priv = GET_PRIVATE (object);

  if (priv->store)
  {
    mpl_app_launches_store_close (priv->store, NULL);
    g_object_unref (priv->store);
  }

  G_OBJECT_CLASS (mpl_app_launches_query_parent_class)->dispose (object);
}

static void
mpl_app_launches_query_class_init (MplAppLaunchesQueryClass *klass)
{
  GObjectClass  *object_class = G_OBJECT_CLASS (klass);
  GParamFlags    param_flags;

  g_type_class_add_private (klass, sizeof (MplAppLaunchesQueryPrivate));

  object_class->get_property = _get_property;
  object_class->set_property = _set_property;
  object_class->constructed = _constructed;
  object_class->dispose = _dispose;

  /* Properties */

  param_flags = G_PARAM_WRITABLE | G_PARAM_STATIC_STRINGS;

  g_object_class_install_property (object_class,
                                   PROP_STORE,
                                   g_param_spec_object ("store",
                                                        "Store",
                                                        "Database store",
                                                        MPL_TYPE_APP_LAUNCHES_STORE,
                                                        param_flags |
                                                        G_PARAM_CONSTRUCT_ONLY));
}

static void
mpl_app_launches_query_init (MplAppLaunchesQuery *self)
{
}

/*
 * Look up executable.
 */
bool
mpl_app_launches_query_lookup (MplAppLaunchesQuery   *self,
                               char const            *executable,
                               time_t                *last_launched_out,
                               uint32_t              *n_launches_out,
                               GError               **error)
{
  MplAppLaunchesQueryPrivate *priv = GET_PRIVATE (self);

  g_return_val_if_fail (MPL_IS_APP_LAUNCHES_QUERY (self), false);

  return mpl_app_launches_store_lookup (priv->store,
                                        executable,
                                        last_launched_out,
                                        n_launches_out,
                                        error);
}

