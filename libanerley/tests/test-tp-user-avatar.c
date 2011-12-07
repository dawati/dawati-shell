/*
 * Anerley - people feeds and widgets
 * Copyright (C) 2010, Intel Corporation.
 *
 * Authors: Danielle Madeley <danielle.madeley@collabora.co.uk>
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

#include <anerley/anerley-tp-user-avatar.h>

#include <clutter/clutter.h>

int
main (int    argc,
      char **argv)
{
//  GError *error = NULL;
//  gchar *path;
//  MxStyle *style;
  ClutterActor *stage, *avatar;

  clutter_init (&argc, &argv);

//  path = g_build_filename (PKG_DATA_DIR,
//                           "style.css",
//                           NULL);
//
//  style = mx_style_get_default ();
//
//  if (!mx_style_load_from_file (style,
//                                  path,
//                                  &error))
//  {
//    g_warning (G_STRLOC ": Error opening style: %s",
//               error->message);
//    g_clear_error (&error);
//  }
//
//  g_free (path);

  stage = clutter_stage_get_default ();
  avatar = anerley_tp_user_avatar_new ();

  clutter_container_add_actor (CLUTTER_CONTAINER (stage), avatar);
  clutter_actor_show_all (stage);

  clutter_main ();

  return 0;
}

