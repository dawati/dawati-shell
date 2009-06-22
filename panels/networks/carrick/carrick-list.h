/*
 * Carrick - a connection panel for the Moblin Netbook
 * Copyright (C) 2009 Intel Corporation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License version
 * 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 *
 * Written by - Joshua Lock <josh@linux.intel.com>
 *
 */

#ifndef _CARRICK_LIST_H
#define _CARRICK_LIST_H

#include <gtk/gtk.h>
#include <gconnman/gconnman.h>

G_BEGIN_DECLS

#define CARRICK_TYPE_LIST carrick_list_get_type()

#define CARRICK_LIST(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
  CARRICK_TYPE_LIST, CarrickList))

#define CARRICK_LIST_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), \
  CARRICK_TYPE_LIST, CarrickListClass))

#define CARRICK_IS_LIST(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
  CARRICK_TYPE_LIST))

#define CARRICK_IS_LIST_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), \
  CARRICK_TYPE_LIST))

#define CARRICK_LIST_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
  CARRICK_TYPE_LIST, CarrickListClass))

typedef struct {
  GtkVBox parent;
} CarrickList;

typedef struct {
  GtkVBoxClass parent_class;
} CarrickListClass;

GType carrick_list_get_type (void);

GtkWidget* carrick_list_new (void);
void carrick_list_add_item (CarrickList *list, GtkWidget *item);
GtkWidget *carrick_list_find_service_item (CarrickList *list,
                                           CmService   *service);
void carrick_list_sort_list (CarrickList *list);

G_END_DECLS

#endif /* _CARRICK_LIST_H */
