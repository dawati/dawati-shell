/* mwb-separator.h */
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

#ifndef _MWB_SEPARATOR_H
#define _MWB_SEPARATOR_H

#include <nbtk/nbtk.h>

G_BEGIN_DECLS

#define MWB_TYPE_SEPARATOR                                              \
  (mwb_separator_get_type())
#define MWB_SEPARATOR(obj)                                              \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj),                                   \
                               MWB_TYPE_SEPARATOR,                      \
                               MwbSeparator))
#define MWB_SEPARATOR_CLASS(klass)                                      \
  (G_TYPE_CHECK_CLASS_CAST ((klass),                                    \
                            MWB_TYPE_SEPARATOR,                         \
                            MwbSeparatorClass))
#define MWB_IS_SEPARATOR(obj)                                           \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj),                                   \
                               MWB_TYPE_SEPARATOR))
#define MWB_IS_SEPARATOR_CLASS(klass)                                   \
  (G_TYPE_CHECK_CLASS_TYPE ((klass),                                    \
                            MWB_TYPE_SEPARATOR))
#define MWB_SEPARATOR_GET_CLASS(obj)                                    \
  (G_TYPE_INSTANCE_GET_CLASS ((obj),                                    \
                              MWB_TYPE_SEPARATOR,                       \
                              MwbSeparatorClass))

typedef struct _MwbSeparator        MwbSeparator;
typedef struct _MwbSeparatorClass   MwbSeparatorClass;
typedef struct _MwbSeparatorPrivate MwbSeparatorPrivate;

struct _MwbSeparatorClass
{
  NbtkWidgetClass parent_class;
};

struct _MwbSeparator
{
  NbtkWidget parent;

  MwbSeparatorPrivate *priv;
};

GType mwb_separator_get_type (void) G_GNUC_CONST;

NbtkWidget *mwb_separator_new (void);

void mwb_separator_get_color (MwbSeparator *separator,
                              ClutterColor *color);

void mwb_separator_set_color (MwbSeparator       *separator,
                              const ClutterColor *color);

gfloat mwb_separator_get_line_width (MwbSeparator *separator);
void mwb_separator_set_line_width (MwbSeparator *separator,
                                   gfloat value);

gfloat mwb_separator_get_off_width (MwbSeparator *separator);
void mwb_separator_set_off_width (MwbSeparator *separator,
                                  gfloat value);

gfloat mwb_separator_get_on_width (MwbSeparator *separator);
void mwb_separator_set_on_width (MwbSeparator *separator,
                                 gfloat value);

G_END_DECLS

#endif /* _MWB_SEPARATOR_H */
