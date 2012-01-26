/*
 * Copyright © 2012 Intel Corp.
 *
 * Authors: Damien Lespiau <damien.lespiau@intel.com>
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

#ifndef __MPD_FS_PANE_H__
#define __MPD_FS_PANE_H__

#include <glib-object.h>
#include <mx/mx.h>

G_BEGIN_DECLS

#define MPD_TYPE_FS_PANEL mpd_fs_pane_get_type()

#define MPD_FS_PANE(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
  MPD_TYPE_FS_PANEL, MpdFsPane))

#define MPD_FS_PANE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), \
  MPD_TYPE_FS_PANEL, MpdFsPaneClass))

#define MPD_IS_FS_PANEL(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
  MPD_TYPE_FS_PANEL))

#define MPD_IS_FS_PANEL_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), \
  MPD_TYPE_FS_PANEL))

#define MPD_FS_PANE_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
  MPD_TYPE_FS_PANEL, MpdFsPaneClass))

typedef struct _MpdFsPane MpdFsPane;
typedef struct _MpdFsPaneClass MpdFsPaneClass;
typedef struct _MpdFsPanePrivate MpdFsPanePrivate;

struct _MpdFsPane
{
  MxBoxLayout parent;

  MpdFsPanePrivate *priv;
};

struct _MpdFsPaneClass
{
  MxBoxLayoutClass parent_class;
};

GType mpd_fs_pane_get_type (void) G_GNUC_CONST;

ClutterActor *mpd_fs_pane_new (void);

G_END_DECLS

#endif /* __MPD_FS_PANE_H__ */
