/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */

/* mpl-panel-windowless.h */
/*
 * Copyright (c) 2009, 2010 Intel Corp.
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

#ifndef _MPL_PANEL_WINDOWLESS
#define _MPL_PANEL_WINDOWLESS

#include "mpl-panel-client.h"

G_BEGIN_DECLS

#define MPL_TYPE_PANEL_WINDOWLESS mpl_panel_windowless_get_type()

#define MPL_PANEL_WINDOWLESS(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), MPL_TYPE_PANEL_WINDOWLESS, MplPanelWindowless))

#define MPL_PANEL_WINDOWLESS_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), MPL_TYPE_PANEL_WINDOWLESS, MplPanelWindowlessClass))

#define MPL_IS_PANEL_WINDOWLESS(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MPL_TYPE_PANEL_WINDOWLESS))

#define MPL_IS_PANEL_WINDOWLESS_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), MPL_TYPE_PANEL_WINDOWLESS))

#define MPL_PANEL_WINDOWLESS_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), MPL_TYPE_PANEL_WINDOWLESS, MplPanelWindowlessClass))

typedef struct _MplPanelWindowless        MplPanelWindowless;
typedef struct _MplPanelWindowlessClass MplPanelWindowlessClass;
typedef struct _MplPanelWindowlessPrivate MplPanelWindowlessPrivate;

/**
 * MplPanelWindowless:
 *
 * Panel object for non-interactive panels.
 */
struct _MplPanelWindowless
{
  /*<private>*/
  MplPanelClient parent;

  MplPanelWindowlessPrivate *priv;
};

/**
 * MplPanelWindowlessClass:
 *
 * Panel object for non-interactive panels.
 */
struct _MplPanelWindowlessClass
{
  /*<private>*/
  MplPanelClientClass parent_class;
};

GType mpl_panel_windowless_get_type (void);

MplPanelClient *mpl_panel_windowless_new   (const gchar *name,
                                            const gchar *tooltip,
                                            const gchar *stylesheet,
                                            const gchar *button_style,
                                            gboolean     with_toolbar_service);

G_END_DECLS

#endif /* _MPL_PANEL_WINDOWLESS */

