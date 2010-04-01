/*
 * Carrick - a connection panel for the Moblin Netbook
 * Copyright (C) 2010 Intel Corporation. All rights reserved.
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
 * Written by - Ross Burton <ross@linux.intel.com>
 */

#ifndef __CARRICK_BANNER_H__
#define __CARRICK_BANNER_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define CARRICK_TYPE_BANNER (carrick_banner_get_type())
#define CARRICK_BANNER(obj)                                             \
   (G_TYPE_CHECK_INSTANCE_CAST ((obj),                                  \
                                CARRICK_TYPE_BANNER,                    \
                                CarrickBanner))
#define CARRICK_BANNER_CLASS(klass)                                     \
   (G_TYPE_CHECK_CLASS_CAST ((klass),                                   \
                             CARRICK_TYPE_BANNER,                       \
                             CarrickBannerClass))
#define IS_CARRICK_BANNER(obj)                                          \
   (G_TYPE_CHECK_INSTANCE_TYPE ((obj),                                  \
                                CARRICK_TYPE_BANNER))
#define IS_CARRICK_BANNER_CLASS(klass)                                  \
   (G_TYPE_CHECK_CLASS_TYPE ((klass),                                   \
                             CARRICK_TYPE_BANNER))
#define CARRICK_BANNER_GET_CLASS(obj)                                   \
   (G_TYPE_INSTANCE_GET_CLASS ((obj),                                   \
                               CARRICK_TYPE_BANNER,                     \
                               CarrickBannerClass))

typedef struct _CarrickBannerPrivate CarrickBannerPrivate;
typedef struct _CarrickBanner      CarrickBanner;
typedef struct _CarrickBannerClass CarrickBannerClass;

struct _CarrickBanner {
  GtkLabel parent;

  CarrickBannerPrivate *priv;
};

struct _CarrickBannerClass {
  GtkLabelClass parent_class;
};

GType carrick_banner_get_type (void) G_GNUC_CONST;

GtkWidget *carrick_banner_new (void);

void carrick_banner_set_text (CarrickBanner *banner, const char *text);

G_END_DECLS

#endif /* __CARRICK_BANNER_H__ */
