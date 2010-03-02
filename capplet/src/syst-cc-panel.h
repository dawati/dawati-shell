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

#ifndef __SYST_CC_PANEL_H__
#define __SYST_CC_PANEL_H__

#include <gtk/gtk.h>
#include <libgnome-control-center-extension/cc-panel.h>

G_BEGIN_DECLS

#define SYST_TYPE_CC_PANEL         (syst_cc_panel_get_type ())
#define SYST_CC_PANEL(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), SYST_TYPE_CC_PANEL, SystCcPanel))
#define SYST_CC_PANEL_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), SYST_TYPE_CC_PANEL, SystCcPanelClass))
#define SYST_IS_CC_PANEL(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), SYST_TYPE_CC_PANEL))
#define SYST_IS_CC_PANEL_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), SYST_TYPE_CC_PANEL))
#define SYST_CC_PANEL_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), SYST_TYPE_CC_PANEL, SystCcPanelClass))

typedef struct _SystCcPanelPrivate SystCcPanelPrivate;

typedef struct
{
        CcPanel              parent;
        SystCcPanelPrivate *priv;
} SystCcPanel;

typedef struct
{
        CcPanelClass parent_class;
} SystCcPanelClass;

GType syst_cc_panel_get_type (void);

void syst_cc_panel_register (GIOModule *module);

G_END_DECLS

#endif /* __SYST_CC_PANEL_H__ */
