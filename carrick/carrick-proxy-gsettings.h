/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */
/*
 * carrick-proxy-to-gsettings - updates the global gnome proxy settings with
 * the service currently being use.
 *
 * Copyright © 2012 Intel Corporation.
 *
 * Author: Michael Wood <michael.g.wood@intel.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU Lesser General Public License,
 * version 2.1, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses>
 */

#include <glib.h>
#ifndef __CARICK_PROXY_GSETTINGS__

G_BEGIN_DECLS

void carrick_proxy_gsettings_update_proxy (void);

G_END_DECLS

#endif
