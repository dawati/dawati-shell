/*
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
 */

#ifndef _PENGE_PEOPLE_MODEL
#define _PENGE_PEOPLE_MODEL

#include <glib-object.h>
#include <clutter/clutter.h>
#include <mojito-client/mojito-client-view.h>

G_BEGIN_DECLS

#define PENGE_TYPE_PEOPLE_MODEL penge_people_model_get_type()

#define PENGE_PEOPLE_MODEL(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), PENGE_TYPE_PEOPLE_MODEL, PengePeopleModel))

#define PENGE_PEOPLE_MODEL_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), PENGE_TYPE_PEOPLE_MODEL, PengePeopleModelClass))

#define PENGE_IS_PEOPLE_MODEL(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PENGE_TYPE_PEOPLE_MODEL))

#define PENGE_IS_PEOPLE_MODEL_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), PENGE_TYPE_PEOPLE_MODEL))

#define PENGE_PEOPLE_MODEL_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), PENGE_TYPE_PEOPLE_MODEL, PengePeopleModelClass))

typedef struct {
  ClutterListModel parent;
} PengePeopleModel;

typedef struct {
  ClutterListModelClass parent_class;
} PengePeopleModelClass;

GType penge_people_model_get_type (void);

ClutterModel *penge_people_model_new (MojitoClientView *view);

G_END_DECLS

#endif /* _PENGE_PEOPLE_MODEL */

