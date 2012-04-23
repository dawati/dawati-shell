/*
 * ntf-gio - Monitor volumes and create notifications
 *
 * Copyright © 2012 Intel Corporation.
 *
 * Authors: Michael Wood <michael.g.wood@intel.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU Lesser General Public License,
 * version 2.1, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses>
 */

#ifndef __NTF_GIO_H__
#define __NTF_GIO_H__

#include <glib-object.h>

G_BEGIN_DECLS

#define NTF_TYPE_GIO ntf_gio_get_type()

#define NTF_GIO(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
  NTF_TYPE_GIO, NtfGIO))

#define NTF_GIO_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), \
  NTF_TYPE_GIO, NtfGIOClass))

#define NTF_IS_GIO(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
  NTF_TYPE_GIO))

#define NTF_IS_GIO_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), \
  NTF_TYPE_GIO))

#define NTF_GIO_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
  NTF_TYPE_GIO, NtfGIOClass))

typedef struct _NtfGIO NtfGIO;
typedef struct _NtfGIOClass NtfGIOClass;
typedef struct _NtfGIOPrivate NtfGIOPrivate;

struct _NtfGIO
{
  GObject parent;

  NtfGIOPrivate *priv;
};

struct _NtfGIOClass
{
  GObjectClass parent_class;
};

GType ntf_gio_get_type (void) G_GNUC_CONST;

NtfGIO *ntf_gio_new (void);

G_END_DECLS

#endif /* __NTF_GIO__H__ */
