/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */

/*
 * Copyright (c) 2012 Intel Corp.
 *
 * Author: Lionel Landwerlin <lionel.g.landwerlin@linux.intel.com>
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
 */

#include <clutter/clutter.h>
#include <mx/mx.h>

#include "mnb-spinner.h"

int
main (int argc, char *argv[])
{
  ClutterActor *stage, *spinner;
  MxStyle *style;

  if (!clutter_init (&argc, &argv))
    return -1;

  style = mx_style_get_default ();
  mx_style_load_from_file (style,
                           THEMEDIR "/mutter-dawati.css",
                           NULL);
  mx_style_load_from_file (mx_style_get_default (),
                           THEMEDIR "/shared/shared.css",
                           NULL);

  stage = clutter_stage_new ();
  clutter_actor_set_size (stage, 1024, 768);
  clutter_actor_show (stage);

  spinner = mnb_spinner_new ();
  mnb_spinner_start (MNB_SPINNER (spinner));
  clutter_actor_add_child (stage, spinner);

  clutter_main ();

  return 0;
}
