
/* 
   Copyright (C) 2007 Red Hat, Inc.

   The Gnome Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The Gnome Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the Gnome Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.

   Author: Rob Staudinger <robsta@linux.intel.com>
   This file was created from gdkapplaunchcontext-x11.c
*/

#include <gio/gio.h>

#ifndef MPL_GDKAPPLAUNCHCONTEXT_H
#define MPL_GDKAPPLAUNCHCONTEXT_H

char *
mpl_gdk_windowing_get_startup_notify_id (GAppLaunchContext *context,
                                         GAppInfo          *info, 
                                         GList             *files);

void
mpl_gdk_windowing_launch_failed (GAppLaunchContext *context, 
                                 const char        *startup_notify_id);

#endif /* MPL_GDKAPPLAUNCHCONTEXT_H */

