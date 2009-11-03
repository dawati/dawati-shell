/*
 * Moblin-Web-Browser: The web browser for Moblin
 * Copyright (c) 2009, Intel Corporation.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU Lesser General Public License,
 * version 2.1, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gtk/gtk.h>
#include "mwb-utils.h"

gboolean
mwb_utils_focus_on_click_cb (ClutterActor       *actor,
                             ClutterButtonEvent *event,
                             gpointer            swallow_event)
{
  clutter_actor_grab_key_focus (actor);
  return GPOINTER_TO_INT (swallow_event);
}

inline void
mwb_utils_table_add (NbtkTable    *table,
                     ClutterActor *child,
                     gint          row,
                     gint          col,
                     gboolean      x_expand,
                     gboolean      x_fill,
                     gboolean      y_expand,
                     gboolean      y_fill)
{
  nbtk_table_add_actor_with_properties (table, child, row, col,
                                        "x-expand", x_expand,
                                        "x-fill", x_fill,
                                        "y-expand", y_expand,
                                        "y-fill", y_fill,
                                        NULL);
}

CoglHandle
mwb_utils_image_to_texture (const guint8 *data,
                            guint data_len,
                            GError **error)
{
  GdkPixbuf *pixbuf;
  GInputStream *input_stream
    = g_memory_input_stream_new_from_data (data, data_len, NULL);
  ClutterActor *texture = COGL_INVALID_HANDLE;

  if ((pixbuf = gdk_pixbuf_new_from_stream_at_scale (input_stream,
                                                     16, 16,
                                                     TRUE,
                                                     NULL,
                                                     error)))
    {
      texture = cogl_texture_new_from_data (gdk_pixbuf_get_width (pixbuf),
                                            gdk_pixbuf_get_height (pixbuf),
                                            0,
                                            gdk_pixbuf_get_has_alpha (pixbuf)
                                            ? COGL_PIXEL_FORMAT_RGBA_8888
                                            : COGL_PIXEL_FORMAT_RGB_888,
                                            COGL_PIXEL_FORMAT_ANY,
                                            gdk_pixbuf_get_rowstride (pixbuf),
                                            gdk_pixbuf_get_pixels (pixbuf));

      g_object_unref (pixbuf);
    }

  g_object_unref (input_stream);

  return texture;
}

gboolean
mwb_utils_actor_has_focus (ClutterActor *actor)
{
  ClutterActor *stage = clutter_actor_get_stage (actor);

  if (stage && (clutter_stage_get_key_focus (CLUTTER_STAGE (stage)) == actor))
    return TRUE;
  else
    return FALSE;
}

void
mwb_utils_show_popup (NbtkPopup *popup)
{
  ClutterAnimation *animation;
  ClutterActor *actor = CLUTTER_ACTOR (popup);

  if (!CLUTTER_ACTOR_IS_VISIBLE (actor))
    {
      g_object_set (G_OBJECT (popup),
                    "scale-gravity", CLUTTER_GRAVITY_NORTH,
                    NULL);
      clutter_actor_set_scale (actor, 1.0, 0.0);
      clutter_actor_set_opacity (actor, 0x0);
      clutter_actor_show (actor);
    }

  animation =
    clutter_actor_animate (actor, CLUTTER_EASE_IN_QUAD, 100,
                           "scale-y", 1.0,
                           "opacity", 0xff,
                           NULL);
  g_signal_handlers_disconnect_by_func (animation, clutter_actor_hide, actor);
}

GdkPixbuf *
mwb_utils_pixbuf_new_from_stock (const gchar *icon_name, gint size)
{
  GdkPixbuf *pixbuf;

  GError *error = NULL;
  GtkIconTheme *icon_theme = gtk_icon_theme_get_default ();

  pixbuf = gtk_icon_theme_load_icon (icon_theme,
                                     icon_name,
                                     size,
                                     GTK_ICON_LOOKUP_USE_BUILTIN |
                                     GTK_ICON_LOOKUP_GENERIC_FALLBACK |
                                     GTK_ICON_LOOKUP_FORCE_SIZE,
                                     &error);
  if (!pixbuf)
    {
      if (error)
        {
          g_warning ("Error retrieving icon: %s", error->message);
          g_error_free (error);
        }
      return NULL;
    }

  return pixbuf;
}

GdkCursor *
mwb_utils_cursor_new_from_stock (const gchar *icon_name)
{
  gint width, height;
  GdkPixbuf *pixbuf;

  gtk_icon_size_lookup (GTK_ICON_SIZE_MENU, &width, &height);
  pixbuf = mwb_utils_pixbuf_new_from_stock (icon_name, MIN (width, height));

  if (pixbuf)
    return gdk_cursor_new_from_pixbuf (gdk_display_get_default (),
                                       pixbuf,
                                       0, 0);
  else
    return NULL;
}

gchar*
mwb_utils_places_db_get_filename()
{
  GKeyFile *keys;
  gboolean is_relative = TRUE;
  gboolean is_default = FALSE;
  gchar *profile_ini;
  gchar **groups;
  gchar *result = NULL;
  gchar *path = NULL;
  guint i, length = 0;

  profile_ini = g_build_filename(g_get_home_dir(),
                                 MWB_PROFILES_INI,
                                 NULL);
  if (!g_file_test (profile_ini, G_FILE_TEST_EXISTS))
    {
      /* Browser profile does not created yet */
      g_free (profile_ini);
      return NULL;
    }

  keys = g_key_file_new ();
  if (!g_key_file_load_from_file (keys, profile_ini, 0, NULL))
    {
      g_warning ("[netpanel] Failed to load keys from profile config: %s", profile_ini);
      g_free (profile_ini);
      g_key_file_free (keys);
      return NULL;
    }

  groups = g_key_file_get_groups (keys, &length);
  for (i = 0; groups[i]; i++)
    {
      if (g_strcmp0(groups[i], "General"))
        {
          is_default = g_key_file_get_boolean(keys, groups[i], "Default", NULL);
          is_relative = g_key_file_get_boolean(keys, groups[i], "IsRelative", NULL);
          if (is_default || i == length-1)
            {
              path = g_key_file_get_string(keys, groups[i], "Path", NULL);
              break;
            }
        }
    }

  if (is_relative)
    result = g_build_filename(g_get_home_dir (), MWB_PROFILES_BASE, 
                              path, MWB_PLACES_SQLITE, NULL);
  else
    result = g_build_filename(path, MWB_PLACES_SQLITE, NULL); 

  /* Ensure we return a valid db path */
  if (!g_file_test (result, G_FILE_TEST_EXISTS))
    {
      g_free (result);
      result = NULL;
    }

  g_strfreev (groups);
  g_key_file_free (keys);
  g_free (profile_ini);
  g_free (path);

  return result;
}

int
mwb_utils_places_db_connect(const gchar *places_db, sqlite3 **dbcon)
{
  if (!places_db)
    return -1;

  g_assert (dbcon != NULL);
  gint rc = sqlite3_open(places_db, dbcon);
  if (rc)
    {
      g_warning ("[netpanel] unable to open places db: %s, places=%s\n",
                 sqlite3_errmsg(*dbcon), places_db);
      sqlite3_close(*dbcon);
      *dbcon = NULL;
      return -1;
    }
  sqlite3_busy_timeout(*dbcon, 5000);

  return 0;
}

void 
mwb_utils_places_db_close(sqlite3 *dbcon)
{
    sqlite3_close(dbcon);
}
