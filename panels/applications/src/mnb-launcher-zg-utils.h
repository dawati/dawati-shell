/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */

/*
 * Copyright (c) 2012 .Collabora
 *
 * Author: Seif Lotfy <seif.lotfy@collabora.co.uk>
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

#ifndef MNB_LAUNCHER_ZG_UTILS_H
#define MNB_LAUNCHER_ZG_UTILS_H

#include <glib.h>
#include <glib-object.h>

typedef struct {
    void (*callback)(GList *apps, gpointer user_data);
    gpointer data;
} mnb_launcher_zg_utils_cb_struct;

void mnb_launcher_zg_utils_send_launch_event(const gchar* executable, const gchar* title);

void mnb_launcher_zg_utils_get_most_used_apps(mnb_launcher_zg_utils_cb_struct *data);

#endif
