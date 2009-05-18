/*
 * libhal-panel-proxy - library for accessing HAL's LaptopPanel interface
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


#ifndef _HAL_PANEL_PROXY
#define _HAL_PANEL_PROXY

#include <glib-object.h>

G_BEGIN_DECLS

#define HAL_PANEL_TYPE_PROXY hal_panel_proxy_get_type()

#define HAL_PANEL_PROXY(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), HAL_PANEL_TYPE_PROXY, HalPanelProxy))

#define HAL_PANEL_PROXY_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), HAL_PANEL_TYPE_PROXY, HalPanelProxyClass))

#define HAL_PANEL_IS_PROXY(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), HAL_PANEL_TYPE_PROXY))

#define HAL_PANEL_IS_PROXY_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), HAL_PANEL_TYPE_PROXY))

#define HAL_PANEL_PROXY_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), HAL_PANEL_TYPE_PROXY, HalPanelProxyClass))

typedef struct {
  GObject parent;
} HalPanelProxy;

typedef struct {
  GObjectClass parent_class;
} HalPanelProxyClass;

GType hal_panel_proxy_get_type (void);

HalPanelProxy *hal_panel_proxy_new (const gchar *device_path);

typedef void (*HalPanelProxyGetBrightnessCallback) (HalPanelProxy *proxy,
                                                    gint           value,
                                                    const GError  *error,
                                                    GObject       *weak_object,
                                                    gpointer       userdata);

typedef void (*HalPanelProxySetBrightnessCallback) (HalPanelProxy *proxy,
                                                    const GError  *error,
                                                    GObject       *weak_object,
                                                    gpointer       userdata);

void hal_panel_proxy_get_brightness_async (HalPanelProxy                      *proxy,
                                           HalPanelProxyGetBrightnessCallback  cb,
                                           GObject                            *weak_object,
                                           gpointer                            userdata);

void hal_panel_proxy_set_brightness_async (HalPanelProxy                      *proxy,
                                           gint                                brightness,
                                           HalPanelProxySetBrightnessCallback  cb,
                                           GObject                            *weak_object,
                                           gpointer                            userdata);


G_END_DECLS

#endif /* _HAL_PANEL_PROXY */

