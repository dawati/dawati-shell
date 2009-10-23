/*
 * Moblin-Web-Browser: The web browser for Moblin
 * Copyright (c) 2009, Intel Corporation.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU Lesser General Public License,
 * version 2.1, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef _MWB_RADICAL_BAR_H
#define _MWB_RADICAL_BAR_H

#include <glib-object.h>
#include <clutter/clutter.h>
#include <nbtk/nbtk.h>
#include "mwb-ac-list.h"

G_BEGIN_DECLS

#define MWB_TYPE_RADICAL_BAR mwb_radical_bar_get_type()

#define MWB_RADICAL_BAR(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
  MWB_TYPE_RADICAL_BAR, MwbRadicalBar))

#define MWB_RADICAL_BAR_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), \
  MWB_TYPE_RADICAL_BAR, MwbRadicalBarClass))

#define MWB_IS_RADICAL_BAR(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
  MWB_TYPE_RADICAL_BAR))

#define MWB_IS_RADICAL_BAR_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), \
  MWB_TYPE_RADICAL_BAR))

#define MWB_RADICAL_BAR_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
  MWB_TYPE_RADICAL_BAR, MwbRadicalBarClass))

typedef struct _MwbRadicalBarPrivate MwbRadicalBarPrivate;

typedef struct {
  NbtkWidget parent;
  
  MwbRadicalBarPrivate *priv;
} MwbRadicalBar;

typedef struct {
  NbtkWidgetClass parent_class;

  void (* go)                 (MwbRadicalBar *radical_bar, const gchar *url);
  void (* reload)             (MwbRadicalBar *radical_bar);
  void (* stop)               (MwbRadicalBar *radical_bar);
  void (* pin_button_clicked) (MwbRadicalBar *radical_bar);
} MwbRadicalBarClass;

GType mwb_radical_bar_get_type (void);

NbtkWidget *mwb_radical_bar_new (void);

void mwb_radical_bar_focus         (MwbRadicalBar *radical_bar);

void mwb_radical_bar_set_uri       (MwbRadicalBar *radical_bar, const gchar *uri);
void mwb_radical_bar_set_title     (MwbRadicalBar *radical_bar, const gchar *title);
void mwb_radical_bar_set_icon      (MwbRadicalBar *radical_bar, ClutterActor *icon);
void mwb_radical_bar_set_loading   (MwbRadicalBar *radical_bar, gboolean loading);
void mwb_radical_bar_set_progress  (MwbRadicalBar *radical_bar, gdouble progress);
void mwb_radical_bar_set_pinned    (MwbRadicalBar *radical_bar, gboolean pinned);
void mwb_radical_bar_set_show_pin  (MwbRadicalBar *radical_bar, gboolean show_pin);
void mwb_radical_bar_set_default_icon (MwbRadicalBar *radical_bar,
                                       ClutterActor  *icon);

const gchar  *mwb_radical_bar_get_uri       (MwbRadicalBar *radical_bar);
const gchar  *mwb_radical_bar_get_title     (MwbRadicalBar *radical_bar);
ClutterActor *mwb_radical_bar_get_icon      (MwbRadicalBar *radical_bar);
gboolean      mwb_radical_bar_get_loading   (MwbRadicalBar *radical_bar);
gdouble       mwb_radical_bar_get_progress  (MwbRadicalBar *radical_bar);
gboolean      mwb_radical_bar_get_pinned    (MwbRadicalBar *radical_bar);
gboolean      mwb_radical_bar_get_show_pin  (MwbRadicalBar *radical_bar);
guint         mwb_radical_bar_get_security  (MwbRadicalBar *radical_bar);
MwbAcList    *mwb_radical_bar_get_ac_list   (MwbRadicalBar *radical_bar);

G_END_DECLS

#endif /* _MWB_RADICAL_BAR_H */

