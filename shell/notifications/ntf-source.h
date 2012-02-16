/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */

/*
 * Copyright (c) 2010 Intel Corp.
 *
 * Author: Tomas Frydrych <tf@linux.intel.com>
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
 */

#ifndef _NTF_SOURCE_H
#define _NTF_SOURCE_H

#include <glib-object.h>

#include <clutter/clutter.h>
#include <meta/window.h>

G_BEGIN_DECLS

#define NTF_TYPE_SOURCE                         \
  (ntf_source_get_type())
#define NTF_SOURCE(obj)                         \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj),           \
                               NTF_TYPE_SOURCE, \
                               NtfSource))
#define NTF_SOURCE_CLASS(klass)                 \
  (G_TYPE_CHECK_CLASS_CAST ((klass),            \
                            NTF_TYPE_SOURCE,    \
                            NtfSourceClass))
#define NTF_IS_SOURCE(obj)                              \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj),                   \
                               NTF_TYPE_SOURCE))
#define NTF_IS_SOURCE_CLASS(klass)              \
  (G_TYPE_CHECK_CLASS_TYPE ((klass),            \
                            NTF_TYPE_SOURCE))
#define NTF_SOURCE_GET_CLASS(obj)               \
  (G_TYPE_INSTANCE_GET_CLASS ((obj),            \
                              NTF_TYPE_SOURCE,  \
                              NtfSourceClass))

typedef struct _NtfSource        NtfSource;
typedef struct _NtfSourceClass   NtfSourceClass;
typedef struct _NtfSourcePrivate NtfSourcePrivate;

struct _NtfSourceClass
{
  GObjectClass parent_class;

  void           (*closed)   (NtfSource *src);
  ClutterActor * (*get_icon) (NtfSource *src);
};

struct _NtfSource
{
  GObject parent;

  /*<private>*/
  NtfSourcePrivate *priv;
};

GType ntf_source_get_type (void) G_GNUC_CONST;

NtfSource    *ntf_source_new          (MetaWindow   *window);
NtfSource    *ntf_source_new_for_pid  (const gchar  *machine, gint pid);
const gchar  *ntf_source_get_id       (NtfSource    *src);
ClutterActor *ntf_source_get_icon     (NtfSource    *src);
MetaWindow   *ntf_source_get_window   (NtfSource *src);

/* The manipulate the global database */
NtfSource    *ntf_sources_find_for_id (const gchar *id);
void          ntf_sources_add         (NtfSource   *src);

G_END_DECLS

#endif /* _NTF_SOURCE_H */
