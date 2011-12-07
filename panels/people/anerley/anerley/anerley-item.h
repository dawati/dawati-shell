/*
 * Anerley - people feeds and widgets
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


#ifndef _ANERLEY_ITEM
#define _ANERLEY_ITEM

#include <glib-object.h>

G_BEGIN_DECLS

#define ANERLEY_TYPE_ITEM anerley_item_get_type()

#define ANERLEY_ITEM(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), ANERLEY_TYPE_ITEM, AnerleyItem))

#define ANERLEY_ITEM_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), ANERLEY_TYPE_ITEM, AnerleyItemClass))

#define ANERLEY_IS_ITEM(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ANERLEY_TYPE_ITEM))

#define ANERLEY_IS_ITEM_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), ANERLEY_TYPE_ITEM))

#define ANERLEY_ITEM_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), ANERLEY_TYPE_ITEM, AnerleyItemClass))

typedef struct {
  GObject parent;
} AnerleyItem;

typedef struct {
  GObjectClass parent_class;
  void (*display_name_changed) (AnerleyItem *item);
  void (*avatar_path_changed) (AnerleyItem *item);
  void (*presence_changed) (AnerleyItem *item);
  void (*unread_messages_changed) (AnerleyItem *item, guint unread);
  const gchar * (*get_display_name) (AnerleyItem *item);
  const gchar * (*get_avatar_path) (AnerleyItem *item);
  const gchar * (*get_presence_status) (AnerleyItem *item);
  const gchar * (*get_presence_message) (AnerleyItem *item);
  const gchar * (*get_sortable_name) (AnerleyItem *item);
  guint (*get_unread_messages_count) (AnerleyItem *item);
  void (*activate) (AnerleyItem *item);
} AnerleyItemClass;

GType anerley_item_get_type (void);

const gchar *anerley_item_get_display_name (AnerleyItem *item);
const gchar *anerley_item_get_avatar_path (AnerleyItem *item);
const gchar *anerley_item_get_presence_status (AnerleyItem *item);
const gchar *anerley_item_get_presence_message (AnerleyItem *item);
const gchar *anerley_item_get_sortable_name (AnerleyItem *item);

void anerley_item_emit_display_name_changed (AnerleyItem *item);
void anerley_item_emit_avatar_path_changed (AnerleyItem *item);
void anerley_item_emit_presence_changed (AnerleyItem *item);
void anerley_item_emit_unread_messages_changed (AnerleyItem *item, guint unread);

void anerley_item_activate (AnerleyItem *item);

G_END_DECLS

#endif /* _ANERLEY_ITEM */

