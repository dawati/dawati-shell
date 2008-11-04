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

#ifndef __NUTTER_GRID_H__
#define __NUTTER_GRID_H__

#include <clutter/clutter-actor.h>

G_BEGIN_DECLS

#define NUTTER_TYPE_GRID               (nutter_grid_get_type())
#define NUTTER_GRID(obj)                                       \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj),                        \
                               NUTTER_TYPE_GRID,               \
                               NutterGrid))
#define NUTTER_GRID_CLASS(klass)                               \
  (G_TYPE_CHECK_CLASS_CAST ((klass),                         \
                            NUTTER_TYPE_GRID,                  \
                            NutterGridClass))
#define NUTTER_IS_GRID(obj)                                    \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj),                        \
                               NUTTER_TYPE_GRID))
#define NUTTER_IS_GRID_CLASS(klass)                            \
  (G_TYPE_CHECK_CLASS_TYPE ((klass),                         \
                            NUTTER_TYPE_GRID))
#define NUTTER_GRID_GET_CLASS(obj)                             \
  (G_TYPE_INSTANCE_GET_CLASS ((obj),                         \
                              NUTTER_TYPE_GRID,                \
                              NutterGridClass))

typedef struct _NutterGrid        NutterGrid;
typedef struct _NutterGridClass   NutterGridClass;
typedef struct _NutterGridPrivate NutterGridPrivate;

struct _NutterGridClass
{
  ClutterActorClass parent_class;
};

struct _NutterGrid
{
  ClutterActor parent;

  NutterGridPrivate *priv;
};

GType nutter_grid_get_type (void) G_GNUC_CONST;

ClutterActor *nutter_grid_new                    (void);
void          nutter_grid_set_end_align          (NutterGrid    *self,
						  gboolean     value);
gboolean      nutter_grid_get_end_align          (NutterGrid    *self);
void          nutter_grid_set_homogenous_rows    (NutterGrid    *self,
						  gboolean     value);
gboolean      nutter_grid_get_homogenous_rows    (NutterGrid    *self);
void          nutter_grid_set_homogenous_columns (NutterGrid    *self,
						  gboolean     value);
gboolean      nutter_grid_get_homogenous_columns (NutterGrid    *self);
void          nutter_grid_set_column_major       (NutterGrid    *self,
						  gboolean     value);
gboolean      nutter_grid_get_column_major       (NutterGrid    *self);
void          nutter_grid_set_row_gap            (NutterGrid    *self,
						  ClutterUnit  value);
ClutterUnit   nutter_grid_get_row_gap            (NutterGrid    *self);
void          nutter_grid_set_column_gap         (NutterGrid    *self,
						  ClutterUnit  value);
ClutterUnit   nutter_grid_get_column_gap         (NutterGrid    *self);
void          nutter_grid_set_valign             (NutterGrid    *self,
						  gdouble      value);
gdouble       nutter_grid_get_valign             (NutterGrid    *self);
void          nutter_grid_set_halign             (NutterGrid    *self,
						  gdouble      value);
gdouble       nutter_grid_get_halign             (NutterGrid    *self);

void          nutter_grid_set_max_size           (NutterGrid    *self,
						  guint          width,
						  guint          height);

G_END_DECLS

#endif /* __NUTTER_GRID_H__ */
