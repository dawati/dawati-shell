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

#include <gtk/gtk.h>
#include "carrick-banner.h"

struct _CarrickBannerPrivate {
  GdkColor colour;
};

#define GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), CARRICK_TYPE_BANNER, CarrickBannerPrivate))
G_DEFINE_TYPE (CarrickBanner, carrick_banner, GTK_TYPE_LABEL);

static void
carrick_banner_realize (GtkWidget *widget)
{
  CarrickBanner *banner = CARRICK_BANNER (widget);

  GTK_WIDGET_CLASS (carrick_banner_parent_class)->realize (widget);

  gdk_color_parse ("#d7d9d6", &banner->priv->colour);
  gdk_colormap_alloc_color (gtk_widget_get_colormap (widget),
                            &banner->priv->colour,
                            FALSE, TRUE);
}

static gboolean
carrick_banner_expose (GtkWidget *widget, GdkEventExpose *event)
{
  CarrickBanner *banner = CARRICK_BANNER (widget);
  GdkGC *gc;

  gc = gdk_gc_new (widget->window);
  gdk_gc_set_foreground (gc, &banner->priv->colour);

  gdk_draw_rectangle (widget->window, gc, TRUE,
                      event->area.x, event->area.y,
                      event->area.width, event->area.height);


  return GTK_WIDGET_CLASS (carrick_banner_parent_class)->expose_event (widget, event);
}

static void
carrick_banner_constructed (GObject *object)
{
  g_object_set (object,
                "xalign", 0.0f,
                "yalign", 0.5f,
                "xpad", 8,
                "ypad", 8,
                NULL);
}

static void
carrick_banner_class_init (CarrickBannerClass *klass)
{
    GObjectClass *o_class = (GObjectClass *)klass;
    GtkWidgetClass *w_class = (GtkWidgetClass *)klass;

    o_class->constructed = carrick_banner_constructed;
    w_class->realize = carrick_banner_realize;
    w_class->expose_event = carrick_banner_expose;

    g_type_class_add_private (klass, sizeof (CarrickBannerPrivate));
}

static void
carrick_banner_init (CarrickBanner *self)
{
  self->priv = GET_PRIVATE (self);
}

GtkWidget *
carrick_banner_new (void)
{
  return g_object_new (CARRICK_TYPE_BANNER, NULL);
}

void
carrick_banner_set_text (CarrickBanner *banner, const char *text)
{
  char *s;

  s = g_strconcat ("<big><b>", text, "</b></big>", NULL);
  gtk_label_set_markup (GTK_LABEL (banner), s);
  g_free (s);
}
