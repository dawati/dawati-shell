/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */

/*
 * Copyright (C) 2010 Intel Corporation.
 *
 * Author: Srinivasa Ragavan <srini@linux.intel.com>
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
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */


#ifndef _MX_SPIN_ENTRY_H
#define _MX_SPIN_ENTRY_H

#include <glib-object.h>
#include <mx/mx.h>

G_BEGIN_DECLS

#define MX_TYPE_SPIN_ENTRY mx_spin_entry_get_type()

#define MX_SPIN_ENTRY(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
  MX_TYPE_SPIN_ENTRY, MxSpinEntry))

#define MX_SPIN_ENTRY_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), \
  MX_TYPE_SPIN_ENTRY, MxSpinEntryClass))

#define MX_IS_SPIN_ENTRY(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
  MX_TYPE_SPIN_ENTRY))

#define MX_IS_SPIN_ENTRY_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), \
  MX_TYPE_SPIN_ENTRY))

#define MX_SPIN_ENTRY_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
  MX_TYPE_SPIN_ENTRY, MxSpinEntryClass))

typedef struct {
  MxBoxLayout parent;
} MxSpinEntry;

typedef struct {
  MxBoxLayoutClass parent_class;
} MxSpinEntryClass;

GType mx_spin_entry_get_type (void);

MxSpinEntry* mx_spin_entry_new (void);
void mx_spin_entry_set_range (MxSpinEntry *spin, gint lower, gint upper);
void mx_spin_entry_set_cycle (MxSpinEntry *spin, gboolean cycle);
void mx_spin_entry_set_value (MxSpinEntry *spin, gint value);
gint mx_spin_entry_get_value (MxSpinEntry *spin);

G_END_DECLS

#endif /* _MX_SPIN_ENTRY_H */
