/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */

/* mnb-panel.h */
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

#ifndef _MNB_PANEL
#define _MNB_PANEL

#include <glib-object.h>
#include <dbus/dbus-glib.h>

#include <meego-panel/mpl-panel-common.h>

#include "mnb-input-manager.h"

G_BEGIN_DECLS

#define MNB_TYPE_PANEL mnb_panel_get_type()

#define MNB_PANEL(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), MNB_TYPE_PANEL, MnbPanel))

#define MNB_IS_PANEL(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MNB_TYPE_PANEL))

#define MNB_PANEL_GET_IFACE(obj)        (G_TYPE_INSTANCE_GET_INTERFACE ((obj), MNB_TYPE_PANEL, MnbPanelIface))

typedef struct _MnbPanel MnbPanel; /* dummy */

typedef struct {
  /*< private >*/
  GTypeInterface g_iface;

  /*< public >*/

  void          (*show)                (MnbPanel *panel);
  void          (*show_begin)          (MnbPanel *panel);
  void          (*show_completed)      (MnbPanel *panel);
  void          (*hide)                (MnbPanel *panel);
  void          (*hide_begin)          (MnbPanel *panel);
  void          (*hide_completed)      (MnbPanel *panel);
  void          (*request_button_style)(MnbPanel *panel, const gchar *style);
  void          (*request_button_state)(MnbPanel *panel, MnbButtonState state);
  void          (*request_tooltip)     (MnbPanel *panel, const gchar *tooltip);
  void          (*request_modality)    (MnbPanel *panel, gboolean modal);

  gboolean      (*is_mapped)           (MnbPanel *panel);
  gboolean      (*is_modal)            (MnbPanel *panel);

  const gchar * (*get_name)            (MnbPanel *panel);
  const gchar * (*get_stylesheet)      (MnbPanel *panel);
  const gchar * (*get_button_style)    (MnbPanel *panel);
  const gchar * (*get_tooltip)         (MnbPanel *panel);

  void          (*set_size)            (MnbPanel *panel,
                                        guint     width,
                                        guint     height);
  void          (*get_size)            (MnbPanel *panel,
                                        guint    *width,
                                        guint    *height);
  void          (*set_position)        (MnbPanel *panel,
                                        gint      x,
                                        gint      y);
  void          (*get_position)        (MnbPanel *panel,
                                        gint     *x,
                                        gint     *y);
  void          (*set_button)          (MnbPanel *panel, MxButton *button);
} MnbPanelIface;

/*
 * This enum defines the reason for showing or hiding the Toolbar, it is
 * ordered by priority, the higher the value, the higher priority.
 *
 * The underscored values are for internal use and are not to be passed into
 * the public API
 */
typedef enum {
  _MNB_SHOW_HIDE_UNSET   = 0,

  _MNB_SHOW_HIDE_PROGRAMMATIC,

  MNB_SHOW_HIDE_BY_DBUS,   /* via toolbar dbus API                          */
  MNB_SHOW_HIDE_BY_PANEL,  /* because panel hid itself (e.g., app launch)   */

  _MNB_SHOW_HIDE_USER_ACTION,

  MNB_SHOW_HIDE_BY_MOUSE,  /* mouse click or motion implying show/hide      */
  MNB_SHOW_HIDE_BY_KEY,    /* user used a key that implied show/hide        */

  _MNB_SHOW_HIDE_POLICY,
  MNB_SHOW_HIDE_POLICY     /* policy reasons, e.g., modal window popping up */
} MnbShowHideReason;

GType mnb_panel_get_type (void);

const gchar *    mnb_panel_get_name          (MnbPanel *panel);
const gchar *    mnb_panel_get_tooltip       (MnbPanel *panel);
const gchar *    mnb_panel_get_stylesheet    (MnbPanel *panel);
const gchar *    mnb_panel_get_button_style  (MnbPanel *panel);
void             mnb_panel_set_size          (MnbPanel *panel,
                                              guint     width,
                                              guint     height);
void             mnb_panel_get_size          (MnbPanel *panel,
                                              guint    *width,
                                              guint    *height);
void             mnb_panel_set_position      (MnbPanel *panel,
                                              gint      x,
                                              gint      y);
void             mnb_panel_get_position      (MnbPanel *panel,
                                              gint     *x,
                                              gint     *y);
void             mnb_panel_show              (MnbPanel *panel);
void             mnb_panel_hide              (MnbPanel *panel);
void             mnb_panel_hide_with_toolbar (MnbPanel *panel,
                                              MnbShowHideReason reason);

gboolean         mnb_panel_is_mapped         (MnbPanel *panel);
gboolean         mnb_panel_is_modal          (MnbPanel *panel);

void             mnb_panel_set_button        (MnbPanel   *panel,
                                              MxButton *button);

void             mnb_panel_ensure_size       (MnbPanel *panel);

G_END_DECLS

#endif /* _MNB_PANEL */

