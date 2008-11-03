/*
 * Clutter.
 *
 * An OpenGL based 'interactive canvas' library.
 *
 * Authored By Matthew Allum  <mallum@openedhand.com>
 *
 * Copyright (C) 2008 OpenedHand
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef __MUTTER_GRID_H__
#define __MUTTER_GRID_H__

#include <clutter/clutter-actor.h>

G_BEGIN_DECLS

#define MUTTER_TYPE_GRID               (mutter_grid_get_type())
#define MUTTER_GRID(obj)                                       \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj),                        \
                               MUTTER_TYPE_GRID,               \
                               MutterGrid))
#define MUTTER_GRID_CLASS(klass)                               \
  (G_TYPE_CHECK_CLASS_CAST ((klass),                         \
                            MUTTER_TYPE_GRID,                  \
                            MutterGridClass))
#define MUTTER_IS_GRID(obj)                                    \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj),                        \
                               MUTTER_TYPE_GRID))
#define MUTTER_IS_GRID_CLASS(klass)                            \
  (G_TYPE_CHECK_CLASS_TYPE ((klass),                         \
                            MUTTER_TYPE_GRID))
#define MUTTER_GRID_GET_CLASS(obj)                             \
  (G_TYPE_INSTANCE_GET_CLASS ((obj),                         \
                              MUTTER_TYPE_GRID,                \
                              MutterGridClass))

typedef struct _MutterGrid        MutterGrid;
typedef struct _MutterGridClass   MutterGridClass;
typedef struct _MutterGridPrivate MutterGridPrivate;

struct _MutterGridClass
{
  ClutterActorClass parent_class;
};

struct _MutterGrid
{
  ClutterActor parent;

  MutterGridPrivate *priv;
};

GType mutter_grid_get_type (void) G_GNUC_CONST;

ClutterActor *mutter_grid_new                    (void);
void          mutter_grid_set_end_align          (MutterGrid    *self,
                                                gboolean     value);
gboolean      mutter_grid_get_end_align          (MutterGrid    *self);
void          mutter_grid_set_homogenous_rows    (MutterGrid    *self,
                                                gboolean     value);
gboolean      mutter_grid_get_homogenous_rows    (MutterGrid    *self);
void          mutter_grid_set_homogenous_columns (MutterGrid    *self,
                                                gboolean     value);
gboolean      mutter_grid_get_homogenous_columns (MutterGrid    *self);
void          mutter_grid_set_column_major       (MutterGrid    *self,
                                                gboolean     value);
gboolean      mutter_grid_get_column_major       (MutterGrid    *self);
void          mutter_grid_set_row_gap            (MutterGrid    *self,
                                                ClutterUnit  value);
ClutterUnit   mutter_grid_get_row_gap            (MutterGrid    *self);
void          mutter_grid_set_column_gap         (MutterGrid    *self,
                                                ClutterUnit  value);
ClutterUnit   mutter_grid_get_column_gap         (MutterGrid    *self);
void          mutter_grid_set_valign             (MutterGrid    *self,
                                                gdouble      value);
gdouble       mutter_grid_get_valign             (MutterGrid    *self);
void          mutter_grid_set_halign             (MutterGrid    *self,
                                                gdouble      value);
gdouble       mutter_grid_get_halign             (MutterGrid    *self);

G_END_DECLS

#endif /* __MUTTER_GRID_H__ */
