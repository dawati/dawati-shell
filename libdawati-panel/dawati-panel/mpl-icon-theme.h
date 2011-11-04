
/*
 * Copyright (c) 2009 Intel Corporation.
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
 * Author: Robert Staudinger <robertx.staudinger@intel.com>
 */

#ifndef MPL_ICON_THEME_H
#define MPL_ICON_THEME_H

#include <gtk/gtk.h>

G_BEGIN_DECLS

gchar * mpl_icon_theme_lookup_icon_file (GtkIconTheme *theme,
                                         const gchar  *icon_name,
                                         gint          icon_size);

G_END_DECLS

#endif /* MPL_ICON_THEME_H */

