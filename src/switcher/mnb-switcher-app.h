/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */

/*
 * Moblin Netbook
 * Copyright © 2009, Intel Corporation.
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

#ifndef _MNB_SWITCHER_APP
#define _MNB_SWITCHER_APP

#include <nbtk/nbtk.h>
#include "mnb-switcher.h"
#include "mnb-switcher-item.h"

G_BEGIN_DECLS

/*
 * MnbSwitcherApp
 *
 * A MnbSwitcherItem subclass represening a single application thumb in the switcher.
 *
 */
#define MNB_TYPE_SWITCHER_APP                 (mnb_switcher_app_get_type ())
#define MNB_SWITCHER_APP(obj)                 (G_TYPE_CHECK_INSTANCE_CAST ((obj), MNB_TYPE_SWITCHER_APP, MnbSwitcherApp))
#define MNB_IS_SWITCHER_APP(obj)              (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MNB_TYPE_SWITCHER_APP))
#define MNB_SWITCHER_APP_CLASS(klass)         (G_TYPE_CHECK_CLASS_CAST ((klass), MNB_TYPE_SWITCHER_APP, MnbSwitcherAppClass))
#define MNB_IS_SWITCHER_APP_CLASS(klass)      (G_TYPE_CHECK_CLASS_TYPE ((klass), MNB_TYPE_SWITCHER_APP))
#define MNB_SWITCHER_APP_GET_CLASS(obj)       (G_TYPE_INSTANCE_GET_CLASS ((obj), MNB_TYPE_SWITCHER_APP, MnbSwitcherAppClass))

typedef struct _MnbSwitcherApp               MnbSwitcherApp;
typedef struct _MnbSwitcherAppPrivate        MnbSwitcherAppPrivate;
typedef struct _MnbSwitcherAppClass          MnbSwitcherAppClass;

struct _MnbSwitcherApp
{
  /*< private >*/
  MnbSwitcherItem parent_instance;

  MnbSwitcherAppPrivate *priv;
};

struct _MnbSwitcherAppClass
{
  /*< private >*/
  MnbSwitcherItemClass parent_class;
};

GType mnb_switcher_app_get_type (void);

MnbSwitcherApp *mnb_switcher_app_new                 (MnbSwitcher    *switcher,
                                                      MutterWindow   *mw);
MutterWindow   *mnb_switcher_app_get_window          (MnbSwitcherApp *self);
ClutterActor   *mnb_switcher_app_get_pre_drag_parent (MnbSwitcherApp *self);

G_END_DECLS

#endif /* _MNB_SWITCHER_APP */
