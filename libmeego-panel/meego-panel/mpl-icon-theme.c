
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
#define ICON_PREFIX     "netbook"

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
#define FALLBACK_ICON_FILE  "/usr/share/icons/netbook/48x48/categories/applications-other.png"

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

/**
 * mpl_icon_theme_lookup_icon_file:
 * @theme: #GtkIconTheme
 * @icon_name: name of the icon
 * @icon_size: size of the icon
 *
 * Looks up icon of given name and size in the supplied #GtkIconTheme,
 * prioritizing Meego-specific icons: if an icon exists that matches 'netbook-'
 * + icon_name, this is returned instead of an icon for the unprefixed name. If
 * the icon_name is an absolute path, no lookup is performed, and a copy of
 * icon_name is returned.
 *
 * Return value: path to the icon, or %NULL if suitable icon was not found in
 * the theme. The returned string must be freed with g_free() when no longer
 * needed.
 */
gchar *
mpl_icon_theme_lookup_icon_file (GtkIconTheme *theme,
                                 const gchar  *icon_name,
                                 gint          icon_size)
{
  GIcon *icon = NULL;
  gchar *icon_file = NULL;

  g_return_val_if_fail (theme, NULL);

  if (NULL == icon_name)
  {
    icon_name = FALLBACK_ICON;
  }

  /* Shortcut absolute paths.
   * Used e.g. in ~/.local installed desktop files. */
  if (g_path_is_absolute (icon_name))
  {
    return g_strdup (icon_name);
  }

  /* Look up with "netbook-" prefix as requested by hbons. */
  if (g_str_has_prefix (icon_name, ICON_PREFIX))
  {
    icon = g_themed_icon_new_with_default_fallbacks (icon_name);
  } else {

    /* +1 for the "meego-" prefixed name, +1 for NULL terminator. */
    guint n_parts = get_n_parts (icon_name);
    gchar **names = g_new0 (gchar *, n_parts + 2);
    gchar *name = (gchar *) icon_name;
    guint i = 0;

    do {
      if (i == 0)
      {
        names[i++] = g_strdup_printf ("%s-%s", ICON_PREFIX, name);
        names[i++] = g_strdup (name);
      } else {
        name += 1; /* skip leading "-" */
        names[i++] = g_strdup (name);
      }
    } while (NULL != (name = strchr (name, '-')));
    names[i] = NULL;

    icon = g_themed_icon_new_from_names (names, n_parts + 1);
    g_strfreev (names);
  }

  if (icon)
  {
    GtkIconInfo *info = gtk_icon_theme_lookup_by_gicon (GTK_ICON_THEME (theme),
                                                        icon,
                                                        icon_size,
                                                        GTK_ICON_LOOKUP_NO_SVG);
    if (info)
    {
      icon_file = g_strdup (gtk_icon_info_get_filename (info));
      gtk_icon_info_free (info);
    }
    g_object_unref (icon);
  }

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
    GtkIconInfo *info = gtk_icon_theme_lookup_icon (GTK_ICON_THEME (theme),
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

