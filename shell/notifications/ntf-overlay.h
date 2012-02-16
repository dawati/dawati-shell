/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */

/*
 * Copyright (c) 2010 Intel Corp.
 *
 * Authors: Tomas Frydrych <tf@linux.intel.com>
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifndef _NTF_OVERLAY_H
#define _NTF_OVERLAY_H

#include <clutter/clutter.h>

#include "ntf-tray.h"

G_BEGIN_DECLS

#define NTF_TYPE_OVERLAY                                                \
   (ntf_overlay_get_type())
#define NTF_OVERLAY(obj)                                                \
   (G_TYPE_CHECK_INSTANCE_CAST ((obj),                                  \
                                NTF_TYPE_OVERLAY,                       \
                                NtfOverlay))
#define NTF_OVERLAY_CLASS(klass)                                        \
   (G_TYPE_CHECK_CLASS_CAST ((klass),                                   \
                             NTF_TYPE_OVERLAY,                          \
                             NtfOverlayClass))
#define NTF_IS_OVERLAY(obj)                                             \
   (G_TYPE_CHECK_INSTANCE_TYPE ((obj),                                  \
                                NTF_TYPE_OVERLAY))
#define NTF_IS_OVERLAY_CLASS(klass)                                     \
   (G_TYPE_CHECK_CLASS_TYPE ((klass),                                   \
                             NTF_TYPE_OVERLAY))
#define NTF_OVERLAY_GET_CLASS(obj)                                      \
   (G_TYPE_INSTANCE_GET_CLASS ((obj),                                   \
                               NTF_TYPE_OVERLAY,                        \
                               NtfOverlayClass))

typedef struct _NtfOverlay        NtfOverlay;
typedef struct _NtfOverlayClass   NtfOverlayClass;
typedef struct _NtfOverlayPrivate NtfOverlayPrivate;

struct _NtfOverlayClass
{
  ClutterActorClass parent_class;
};

struct _NtfOverlay
{
  ClutterActor parent;

  /*<private>*/
  NtfOverlayPrivate *priv;
};

GType ntf_overlay_get_type (void) G_GNUC_CONST;

ClutterActor *ntf_overlay_new                         (void);
NtfTray      *ntf_overlay_get_tray                    (gboolean urgent);
gboolean      ntf_overlay_urgent_notification_present (void);

G_END_DECLS

#endif /* _NTF_OVERLAY_H */
