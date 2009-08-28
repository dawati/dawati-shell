/*
 *
 * Copyright (C) 2009, Intel Corporation.
 *
 * Authors: Rob Bradford <rob@linux.intel.com>
 * 
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU Lesser General Public License,
 * version 2.1, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
 * more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */

#ifndef _PENGE_MAGIC_LIST_VIEW
#define _PENGE_MAGIC_LIST_VIEW

#include <glib-object.h>
#include "penge-magic-container.h"

G_BEGIN_DECLS

#define PENGE_TYPE_MAGIC_LIST_VIEW penge_magic_list_view_get_type()

#define PENGE_MAGIC_LIST_VIEW(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), PENGE_TYPE_MAGIC_LIST_VIEW, PengeMagicListView))

#define PENGE_MAGIC_LIST_VIEW_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), PENGE_TYPE_MAGIC_LIST_VIEW, PengeMagicListViewClass))

#define PENGE_IS_MAGIC_LIST_VIEW(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PENGE_TYPE_MAGIC_LIST_VIEW))

#define PENGE_IS_MAGIC_LIST_VIEW_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), PENGE_TYPE_MAGIC_LIST_VIEW))

#define PENGE_MAGIC_LIST_VIEW_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), PENGE_TYPE_MAGIC_LIST_VIEW, PengeMagicListViewClass))

typedef struct {
  PengeMagicContainer parent;
} PengeMagicListView;

typedef struct {
  PengeMagicContainerClass parent_class;
} PengeMagicListViewClass;

GType penge_magic_list_view_get_type (void);

ClutterActor *penge_magic_list_view_new (void);
void penge_magic_list_view_set_model (PengeMagicListView *view,
                                      ClutterModel       *model);
void penge_magic_list_view_set_item_type (PengeMagicListView *view,
                                          GType               item_type);
void penge_magic_list_view_add_attribute (PengeMagicListView *view,
                                          const gchar        *property_name,
                                          gint                col);
void penge_magic_list_view_freeze (PengeMagicListView *view);
void penge_magic_list_view_thaw (PengeMagicListView *view);

G_END_DECLS

#endif /* _PENGE_MAGIC_LIST_VIEW */

