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

#ifndef _NTF_TRAY_H
#define _NTF_TRAY_H

#include <mx/mx.h>

#include "ntf-notification.h"

G_BEGIN_DECLS

#define NTF_TYPE_TRAY                                                   \
   (ntf_tray_get_type())
#define NTF_TRAY(obj)                                                   \
   (G_TYPE_CHECK_INSTANCE_CAST ((obj),                                  \
                                NTF_TYPE_TRAY,                          \
                                NtfTray))
#define NTF_TRAY_CLASS(klass)                                           \
   (G_TYPE_CHECK_CLASS_CAST ((klass),                                   \
                             NTF_TYPE_TRAY,                             \
                             NtfTrayClass))
#define NTF_IS_TRAY(obj)                                                \
   (G_TYPE_CHECK_INSTANCE_TYPE ((obj),                                  \
                                NTF_TYPE_TRAY))
#define NTF_IS_TRAY_CLASS(klass)                                        \
   (G_TYPE_CHECK_CLASS_TYPE ((klass),                                   \
                             NTF_TYPE_TRAY))
#define NTF_TRAY_GET_CLASS(obj)                                         \
   (G_TYPE_INSTANCE_GET_CLASS ((obj),                                   \
                               NTF_TYPE_TRAY,                           \
                               NtfTrayClass))

typedef struct _NtfTray        NtfTray;
typedef struct _NtfTrayClass   NtfTrayClass;
typedef struct _NtfTrayPrivate NtfTrayPrivate;

struct _NtfTrayClass
{
  MxWidgetClass parent_class;
};

struct _NtfTray
{
  MxWidget parent;

  /*<private>*/
  NtfTrayPrivate *priv;
};

GType ntf_tray_get_type (void) G_GNUC_CONST;

NtfTray         *ntf_tray_new               (void);
void             ntf_tray_add_notification  (NtfTray         *tray,
                                             NtfNotification *ntf);
NtfNotification *ntf_tray_find_notification (NtfTray         *tray,
                                             gint             subsystem,
                                             gint             id);

G_END_DECLS

#endif /* _NTF_TRAY_H */
