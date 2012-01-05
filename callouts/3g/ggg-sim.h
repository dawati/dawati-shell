/*
 * Carrick - a connection panel for the Dawati Netbook
 * Copyright (C) 2011 Intel Corporation. All rights reserved.
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
 * Written by - Jussi Kukkonen <jussi.kukkonen@intel.com>
 */

#ifndef __GGG_SIM_H__
#define __GGG_SIM_H__

#include <gio/gio.h>

#define OFONO_SERVICE             "org.ofono"
#define OFONO_MANAGER_PATH        "/"
#define OFONO_MANAGER_INTERFACE OFONO_SERVICE ".Manager"
#define OFONO_MODEM_INTERFACE OFONO_SERVICE ".Modem"
#define OFONO_CONNMAN_INTERFACE OFONO_SERVICE ".ConnectionManager"
#define OFONO_CONTEXT_INTERFACE OFONO_SERVICE ".ConnectionContext"

/* This is an abstraction of a ofono Modem + ConnectionContext */
typedef struct Sim {
  GDBusProxy *modem_proxy;
  GDBusProxy *context_proxy;

  char *device_name;

  char *mnc;
  char *mcc;

  const char *apn;
  const char *username;
  const char *password;
} Sim;


void sim_free (Sim *sim);
Sim* sim_new (const char *modem_path);
gboolean sim_get_plan (Sim *sim,
                       char **name,
                       char **apn,
                       char **username,
                       char **password);
gboolean sim_set_plan (Sim *sim,
                       const char *name,
                       const char *apn,
                       const char *username,
                       const char *password);
gboolean sim_remove_plan (Sim *sim);

#endif /* __GGG_SIM_H__ */
