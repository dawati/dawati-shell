
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

#include <string.h>
#include <gtk/gtk.h>
#include "mpl-icon-theme.h"

#define FALLBACK_THEME  "hicolor"
#define ICON_PREFIX     "moblin"

/* Quite possible that we end up with duplicate entries here,
 * but that shouldn't matter because when we need this something's already
 * gone wrong and we're in fallback mode. */
#define FALLBACK_PATHS          \
          DATADIR "/icons",     \
          DATADIR "/pixmaps",   \
          "/usr/share/icons",   \
          "/usr/share/pixmaps", \
          NULL

#define FALLBACK_ICON       "applications-other"
#define FALLBACK_ICON_FILE  "/usr/share/icons/moblin/48x48/categories/applications-other.png"

static guint
get_n_parts (const gchar *icon_name)
{
  const gchar *p;
  guint        n_parts = 1;

  g_return_val_if_fail (icon_name, 0);
  g_return_val_if_fail (*icon_name, 0);

  for (p = icon_name; p && *p != '\0'; p++)
  {
    if (*p == '-')
      n_parts++;
  }

  return n_parts;
}

static gchar *
lookup_in_theme (GtkIconTheme  *theme,
                 gchar        **icon_names,
                 gint           icon_size)
{
  GIcon       *icon;
  GtkIconInfo *info;
  gchar       *icon_file = NULL;

  g_return_val_if_fail (theme, 0);
  g_return_val_if_fail (icon_names, 0);

  icon = g_themed_icon_new_from_names (icon_names, -1);
  g_return_val_if_fail (icon, 0);

  info = gtk_icon_theme_lookup_by_gicon (theme,
                                         icon,
                                         icon_size,
                                         GTK_ICON_LOOKUP_NO_SVG);
  g_object_unref (icon);
  if (info)
  {
    icon_file = g_strdup (gtk_icon_info_get_filename (info));
    gtk_icon_info_free (info);
  }

  return icon_file;
}

static gchar *
lookup_in_well_known_places (const gchar  **paths,
                             const gchar   *icon_name)
{
  const gchar **iter = paths;
  gchar        *icon_file = NULL;

  while (*iter)
  {
    gchar *path = g_build_filename (*iter, icon_name, NULL);
    if (g_file_test (path, G_FILE_TEST_IS_REGULAR))
    {
      icon_file = path;
      break;
    }
    g_free (path);
    iter++;
  }

  return icon_file;
}

gchar *
mpl_icon_theme_lookup_icon_file (GtkIconTheme *self,
                                 const gchar  *icon_name,
                                 gint          icon_size)
{
  gchar               **names;
  const gchar          *name;
  guint                 i = 0;
  gchar                *icon_file = NULL;

  g_return_val_if_fail (self, NULL);
  g_return_val_if_fail (icon_name, NULL);

  /* Shortcut absolute paths.
   * Used e.g. in ~/.local installed desktop files. */
  if (g_path_is_absolute (icon_name))
  {
    return g_strdup (icon_name);
  }

  /* Build names vector. */
  names = g_new0 (gchar *, get_n_parts (icon_name) * 2 + 1);
  name = icon_name;
  do {
    if (0 == g_strcmp0 (name, icon_name))
    {
      names[i++] = g_strdup_printf ("%s-%s", ICON_PREFIX, name);
      names[i++] = g_strdup (name);
    } else {
      names[i++] = g_strdup_printf ("%s%s", ICON_PREFIX, name);
      name += 1; /* skip leading "-" */
      names[i++] = g_strdup (name);
    }
  } while (NULL != (name = strchr (name, '-')));

  names[i] = NULL;

  /* Look up themed icon. */
  if (i > 0)
  {
    icon_file = lookup_in_theme (GTK_ICON_THEME (self),
                                 names,
                                 icon_size);
  }
  g_strfreev (names);

  /* Search well-known places. */
  if (NULL == icon_file &&
      g_str_has_suffix (icon_name, ".png"))
  {
    const gchar *paths[] = { FALLBACK_PATHS };
    icon_file = lookup_in_well_known_places ((const gchar **) paths,
                                             icon_name);
  }

  /* Lookup fallback icon. */
  if (NULL == icon_file)
  {
    GtkIconInfo *info = gtk_icon_theme_lookup_icon (GTK_ICON_THEME (self),
                                                    FALLBACK_ICON,
                                                    icon_size,
                                                    GTK_ICON_LOOKUP_NO_SVG |
                                                    GTK_ICON_LOOKUP_GENERIC_FALLBACK);
    if (info)
    {
      icon_file = g_strdup (gtk_icon_info_get_filename (info));
      gtk_icon_info_free (info);
    }
  }

  /* Use hardcoded icon. */
  if (NULL == icon_file)
  {
    icon_file = g_strdup (FALLBACK_ICON_FILE);
  }

  return icon_file;
}

