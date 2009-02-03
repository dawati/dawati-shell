/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */

/*
 * Moblin Netbook
 *
 * Copyright © 2009, Intel Corporation.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU Lesser General Public License,
 * version 2.1, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU Lesser General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 51 Franklin St
 * - Fifth Floor, Boston, MA 02110-1301 USA
 *
 */
#ifndef _MNB_SWITCHER
#define _MNB_SWITCHER

#include <glib-object.h>
#include <mutter-plugin.h>
#include "mnb-drop-down.h"

G_BEGIN_DECLS

#define MNB_TYPE_SWITCHER mnb_switcher_get_type()

#define MNB_SWITCHER(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), MNB_TYPE_SWITCHER, MnbSwitcher))

#define MNB_SWITCHER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), MNB_TYPE_SWITCHER, MnbSwitcherClass))

#define MNB_IS_SWITCHER(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MNB_TYPE_SWITCHER))

#define MNB_IS_SWITCHER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), MNB_TYPE_SWITCHER))

#define MNB_SWITCHER_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), MNB_TYPE_SWITCHER, MnbSwitcherClass))

typedef struct _MnbSwitcherPrivate MnbSwitcherPrivate;

typedef struct {
  MnbDropDown parent;

  /*< private >*/
  MnbSwitcherPrivate *priv;
} MnbSwitcher;

typedef struct {
  MnbDropDownClass parent_class;
} MnbSwitcherClass;

GType mnb_switcher_get_type (void);

NbtkWidget* mnb_switcher_new (MutterPlugin *plugin);

void mnb_switcher_select_window (MnbSwitcher *switcher, MetaWindow *meta_win);
void mnb_switcher_activate_selection (MnbSwitcher *switcher, gboolean close,
                                      guint timestamp);

G_END_DECLS

#endif /* _MNB_SWITCHER */

