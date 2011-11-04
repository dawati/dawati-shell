/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */

/*
 * Copyright (C) 2010 Intel Corporation.
 *
 * Author: Srinivasa Ragavan <srini@linux.intel.com>
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
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <glib.h>
#include <glib/gi18n.h>
#include <gdk/gdk.h>
#include <mx/mx.h>
#include "mnp-alarm-manager.h"

static gboolean
idle_cb ()
{
	mnp_alarm_manager_new();

	return FALSE;
}

int
main (int argc, char *argv[])
{
	GError *error = NULL;

	mx_set_locale();

	if (!clutter_init_with_args (&argc, &argv, _("Dawati alarm notify"), NULL, NULL, &error)) {
		g_warning ("Unable to start dawati-alarm-notify: %s\n", error->message);
		g_error_free(error);
		return 0;
	}

	gdk_init(&argc, &argv);

	if (!g_thread_get_initialized ())
		g_thread_init (NULL);
	
	g_idle_add (idle_cb, NULL);
	clutter_main();

	return 0;
}
