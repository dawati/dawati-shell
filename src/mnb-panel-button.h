/* mnb-panel-button.h */
/*
 * Copyright (c) 2009 Intel Corp.
 *
 * Author: Thomas Wood <thomas@linux.intel.com>
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

#ifndef _MNB_PANEL_BUTTON
#define _MNB_PANEL_BUTTON

#include <glib-object.h>
#include <nbtk/nbtk.h>

G_BEGIN_DECLS

#define MNB_TYPE_PANEL_BUTTON mnb_panel_button_get_type()

#define MNB_PANEL_BUTTON(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), MNB_TYPE_PANEL_BUTTON, MnbPanelButton))

#define MNB_PANEL_BUTTON_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), MNB_TYPE_PANEL_BUTTON, MnbPanelButtonClass))

#define MNB_IS_PANEL_BUTTON(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MNB_TYPE_PANEL_BUTTON))

#define MNB_IS_PANEL_BUTTON_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), MNB_TYPE_PANEL_BUTTON))

#define MNB_PANEL_BUTTON_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), MNB_TYPE_PANEL_BUTTON, MnbPanelButtonClass))

typedef struct _MnbPanelButtonPrivate MnbPanelButtonPrivate;

typedef struct {
  NbtkButton parent;

  /*< private >*/
  MnbPanelButtonPrivate *priv;
} MnbPanelButton;

typedef struct {
  NbtkButtonClass parent_class;
} MnbPanelButtonClass;

GType mnb_panel_button_get_type (void);

NbtkWidget* mnb_panel_button_new (void);
void mnb_panel_button_set_reactive_area (MnbPanelButton  *button, gint x, gint y, gint width, gint height);

G_END_DECLS

#endif /* _MNB_PANEL_BUTTON */

