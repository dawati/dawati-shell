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

#ifndef __MWB_SPINDLE_H__
#define __MWB_SPINDLE_H__

#include <clutter/clutter.h>

G_BEGIN_DECLS

#define MWB_TYPE_SPINDLE                                                \
  (mwb_spindle_get_type())
#define MWB_SPINDLE(obj)                                                \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj),                                   \
                               MWB_TYPE_SPINDLE,                        \
                               MwbSpindle))
#define MWB_SPINDLE_CLASS(klass)                                        \
  (G_TYPE_CHECK_CLASS_CAST ((klass),                                    \
                            MWB_TYPE_SPINDLE,                           \
                            MwbSpindleClass))
#define MWB_IS_SPINDLE(obj)                                             \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj),                                   \
                               MWB_TYPE_SPINDLE))
#define MWB_IS_SPINDLE_CLASS(klass)                                     \
  (G_TYPE_CHECK_CLASS_TYPE ((klass),                                    \
                            MWB_TYPE_SPINDLE))
#define MWB_SPINDLE_GET_CLASS(obj)                                      \
  (G_TYPE_INSTANCE_GET_CLASS ((obj),                                    \
                              MWB_TYPE_SPINDLE,                         \
                              MwbSpindleClass))

typedef struct _MwbSpindle        MwbSpindle;
typedef struct _MwbSpindleClass   MwbSpindleClass;
typedef struct _MwbSpindlePrivate MwbSpindlePrivate;

struct _MwbSpindleClass
{
  ClutterActorClass parent_class;
};

struct _MwbSpindle
{
  ClutterActor parent;

  MwbSpindlePrivate *priv;
};

GType mwb_spindle_get_type (void) G_GNUC_CONST;

ClutterActor *mwb_spindle_new (void);

void mwb_spindle_set_position (MwbSpindle *spindle, gdouble position);
gdouble mwb_spindle_get_position (MwbSpindle *spindle);

G_END_DECLS

#endif /* __MWB_SPINDLE_H__ */
