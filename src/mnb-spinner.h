/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */

/* mnb-spinner.h */
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

#ifndef _MNB_SPINNER
#define _MNB_SPINNER

#include <mx/mx.h>

G_BEGIN_DECLS

#define MNB_TYPE_SPINNER mnb_spinner_get_type()

#define MNB_SPINNER(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), MNB_TYPE_SPINNER, MnbSpinner))

#define MNB_SPINNER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), MNB_TYPE_SPINNER, MnbSpinnerClass))

#define MNB_IS_SPINNER(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MNB_TYPE_SPINNER))

#define MNB_IS_SPINNER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), MNB_TYPE_SPINNER))

#define MNB_SPINNER_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), MNB_TYPE_SPINNER, MnbSpinnerClass))

typedef struct _MnbSpinnerPrivate MnbSpinnerPrivate;

typedef struct {
  MxWidget parent;

  /*< private >*/
  MnbSpinnerPrivate *priv;
} MnbSpinner;

typedef struct {
  MxWidgetClass parent_class;
} MnbSpinnerClass;

GType mnb_spinner_get_type (void);

ClutterActor* mnb_spinner_new (void);

void mnb_spinner_start (MnbSpinner *spinner);
void mnb_spinner_stop  (MnbSpinner *spinner);

G_END_DECLS

#endif /* _MNB_SPINNER */

