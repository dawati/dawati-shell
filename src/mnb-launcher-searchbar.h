/*
 * Copyright (C) 2009 Intel Corporation
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * Written by: Robert Staudinger <robsta@openedhand.com>
 */

#ifndef __MNB_LAUNCHER_SEARCHBAR_H__
#define __MNB_LAUNCHER_SEARCHBAR_H__

#include <nbtk/nbtk-widget.h>

G_BEGIN_DECLS

#define MNB_TYPE_LAUNCHER_SEARCHBAR             \
        (mnb_launcher_searchbar_get_type ())
#define MNB_LAUNCHER_SEARCHBAR(obj)             \
        (G_TYPE_CHECK_INSTANCE_CAST ((obj), MNB_TYPE_LAUNCHER_SEARCHBAR, MnbLauncherSearchbar))
#define MNB_IS_LAUNCHER_SEARCHBAR(obj)          \
        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MNB_TYPE_LAUNCHER_SEARCHBAR))
#define MNB_LAUNCHER_SEARCHBAR_CLASS(klass)     \
        (G_TYPE_CHECK_CLASS_CAST ((klass), MNB_TYPE_LAUNCHER_SEARCHBAR, MnbLauncherSearchbarClass))
#define MNB_IS_LAUNCHER_SEARCHBAR_CLASS(klass)  \
        (G_TYPE_CHECK_CLASS_TYPE ((klass), MNB_TYPE_LAUNCHER_SEARCHBAR))
#define MNB_LAUNCHER_SEARCHBAR_GET_CLASS(obj)   \
        (G_TYPE_INSTANCE_GET_CLASS ((obj), MNB_TYPE_LAUNCHER_SEARCHBAR, MnbLauncherSearchbarClass))

typedef struct _MnbLauncherSearchbar              MnbLauncherSearchbar;
typedef struct _MnbLauncherSearchbarPrivate       MnbLauncherSearchbarPrivate;
typedef struct _MnbLauncherSearchbarClass         MnbLauncherSearchbarClass;

struct _MnbLauncherSearchbar
{
  /*< private >*/
  NbtkWidget parent_instance;

  MnbLauncherSearchbarPrivate *priv;
};

struct _MnbLauncherSearchbarClass
{
  NbtkWidgetClass parent_class;

  /* signals */
  void (* activated) (MnbLauncherSearchbar *self);
};

GType mnb_launcher_searchbar_get_type (void) G_GNUC_CONST;

NbtkWidget *          mnb_launcher_searchbar_new            (void);

G_END_DECLS

#endif /* __MNB_LAUNCHER_SEARCHBAR_H__ */
