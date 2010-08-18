
/*
 * Copyright (c) 2010 Intel Corporation.
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
 *
 * Author: Rob Staudinger <robsta@linux.intel.com>
 */

#ifndef MPL_APP_LAUNCHES_STORE_PRIV_H
#define MPL_APP_LAUNCHES_STORE_PRIV_H

#include <meego-panel/mpl-app-launches-store.h>

bool
mpl_app_launches_store_open (MplAppLaunchesStore   *self,
                             bool                   for_writing,
                             GError               **error_out);

bool
mpl_app_launches_store_close (MplAppLaunchesStore  *self,
                              GError              **error_out);

#endif /* MPL_APP_LAUNCHES_STORE_PRIV_H */

