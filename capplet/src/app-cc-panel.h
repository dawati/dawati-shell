/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2010 Intel Corp
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 */

#ifndef __APP_CC_PANEL_H__
#define __APP_CC_PANEL_H__

#include <gtk/gtk.h>
#include <libgnome-control-center-extension/cc-panel.h>

G_BEGIN_DECLS

#define APP_TYPE_CC_PANEL         (app_cc_panel_get_type ())
#define APP_CC_PANEL(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), APP_TYPE_CC_PANEL, AppCcPanel))
#define APP_CC_PANEL_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), APP_TYPE_CC_PANEL, AppCcPanelClass))
#define APP_IS_CC_PANEL(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), APP_TYPE_CC_PANEL))
#define APP_IS_CC_PANEL_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), APP_TYPE_CC_PANEL))
#define APP_CC_PANEL_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), APP_TYPE_CC_PANEL, AppCcPanelClass))

typedef struct _AppCcPanelPrivate AppCcPanelPrivate;

typedef struct
{
  CcPanel parent;
  AppCcPanelPrivate *priv;
} AppCcPanel;

typedef struct
{
  CcPanelClass parent_class;
} AppCcPanelClass;

GType app_cc_panel_get_type (void);

void app_cc_panel_register (GIOModule *module);

G_END_DECLS

#endif /* __APP_CC_PANEL_H__ */
