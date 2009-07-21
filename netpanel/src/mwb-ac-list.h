/* mwb-ac-list.h */
/*
 * Copyright (c) 2009 Intel Corp.
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

/* Borrowed from the moblin-web-browser project */

#ifndef _MWB_AC_LIST_H
#define _MWB_AC_LIST_H

#include <glib-object.h>
#include <clutter/clutter.h>
#include <nbtk/nbtk.h>

G_BEGIN_DECLS

#define MWB_TYPE_AC_LIST mwb_ac_list_get_type()

#define MWB_AC_LIST(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
  MWB_TYPE_AC_LIST, MwbAcList))

#define MWB_AC_LIST_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), \
  MWB_TYPE_AC_LIST, MwbAcListClass))

#define MWB_IS_AC_LIST(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
  MWB_TYPE_AC_LIST))

#define MWB_IS_AC_LIST_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), \
  MWB_TYPE_AC_LIST))

#define MWB_AC_LIST_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
  MWB_TYPE_AC_LIST, MwbAcListClass))

typedef struct _MwbAcListPrivate MwbAcListPrivate;

typedef struct {
  NbtkWidget parent;

  MwbAcListPrivate *priv;
} MwbAcList;

typedef struct {
  NbtkWidgetClass parent_class;

  void (* activate) (MwbAcList *ac_list);
} MwbAcListClass;

GType mwb_ac_list_get_type (void);

NbtkWidget *mwb_ac_list_new (void);

void mwb_ac_list_set_search_text (MwbAcList *self,
                                  const gchar *search_text);
const gchar *mwb_ac_list_get_search_text (MwbAcList *self);

void mwb_ac_list_set_selection (MwbAcList *self, gint selection);
gint mwb_ac_list_get_selection (MwbAcList *self);

guint mwb_ac_list_get_n_entries (MwbAcList *self);

gchar *mwb_ac_list_get_entry_url (MwbAcList *self, guint entry);

void mwb_ac_list_increment_tld_score_for_url (MwbAcList *self,
                                              const gchar *url);

G_END_DECLS

#endif /* _MWB_AC_LIST_H */
