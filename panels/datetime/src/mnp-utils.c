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

#include <libxml/xmlreader.h>

#define GWEATHER_I_KNOW_THIS_IS_UNSTABLE
#include <libgweather/gweather-xml.h>
#include <libgweather/gweather-location.h>
#include <libgweather/weather.h>

#include "mnp-utils.h"

WeatherLocation *gweather_location_to_weather_location (GWeatherLocation *gloc,
							const char *name);

static void
fill_location_entry_model (ClutterModel *store, GWeatherLocation *loc,
			   const char *parent_display_name,
			   const char *parent_compare_name)
{
    GWeatherLocation **children;
    char *display_name, *compare_name;
    int i;

    children = gweather_location_get_children (loc);

    switch (gweather_location_get_level (loc)) {
    case GWEATHER_LOCATION_WORLD:
    case GWEATHER_LOCATION_REGION:
    case GWEATHER_LOCATION_ADM2:
	/* Ignore these levels of hierarchy; just recurse, passing on
	 * the names from the parent node.
	 */
	for (i = 0; children[i]; i++) {
	    fill_location_entry_model (store, children[i],
				       parent_display_name,
				       parent_compare_name);
	}
	break;

    case GWEATHER_LOCATION_COUNTRY:
	/* Recurse, initializing the names to the country name */
	for (i = 0; children[i]; i++) {
	    fill_location_entry_model (store, children[i],
				       gweather_location_get_name (loc),
				       gweather_location_get_sort_name (loc));
	}
	break;

    case GWEATHER_LOCATION_ADM1:
	/* Recurse, adding the ADM1 name to the country name */
	display_name = g_strdup_printf ("%s, %s", gweather_location_get_name (loc), parent_display_name);
	compare_name = g_strdup_printf ("%s, %s", gweather_location_get_sort_name (loc), parent_compare_name);

	for (i = 0; children[i]; i++) {
	    fill_location_entry_model (store, children[i],
				       display_name, compare_name);
	}

	g_free (display_name);
	g_free (compare_name);
	break;

    case GWEATHER_LOCATION_CITY:
	if (children[0] && children[1]) {
	    /* If there are multiple (<location>) children, add a line
	     * for each of them.
	     */
	    for (i = 0; children[i]; i++) {
		display_name = g_strdup_printf ("%s (%s), %s",
						gweather_location_get_name (loc),
						gweather_location_get_name (children[i]),
						parent_display_name);
		compare_name = g_strdup_printf ("%s (%s), %s",
						gweather_location_get_sort_name (loc),
						gweather_location_get_sort_name (children[i]),
						parent_compare_name);

		clutter_model_append(store, 
				    GWEATHER_LOCATION_ENTRY_COL_LOCATION, children[i],
				    GWEATHER_LOCATION_ENTRY_COL_DISPLAY_NAME, display_name,
				    GWEATHER_LOCATION_ENTRY_COL_COMPARE_NAME, compare_name,
				    -1);

		g_free (display_name);
		g_free (compare_name);
	    }
	} else if (children[0]) {
	    /* Else there's only one location. This is a mix of the
	     * city-with-multiple-location case above and the
	     * location-with-no-city case below.
	     */
	    display_name = g_strdup_printf ("%s, %s",
					    gweather_location_get_name (loc),
					    parent_display_name);
	    compare_name = g_strdup_printf ("%s, %s",
					    gweather_location_get_sort_name (loc),
					    parent_compare_name);

	    clutter_model_append(store, 
				GWEATHER_LOCATION_ENTRY_COL_LOCATION, children[0],
				GWEATHER_LOCATION_ENTRY_COL_DISPLAY_NAME, display_name,
				GWEATHER_LOCATION_ENTRY_COL_COMPARE_NAME, compare_name,
				-1);

	    g_free (display_name);
	    g_free (compare_name);
	}
	break;

    case GWEATHER_LOCATION_WEATHER_STATION:
	/* <location> with no parent <city>, or <city> with a single
	 * child <location>.
	 */
	display_name = g_strdup_printf ("%s, %s",
					gweather_location_get_name (loc),
					parent_display_name);
	compare_name = g_strdup_printf ("%s, %s",
					gweather_location_get_sort_name (loc),
					parent_compare_name);
	clutter_model_append(store, 		
			    GWEATHER_LOCATION_ENTRY_COL_LOCATION, loc,
			    GWEATHER_LOCATION_ENTRY_COL_DISPLAY_NAME, display_name,
			    GWEATHER_LOCATION_ENTRY_COL_COMPARE_NAME, compare_name,
			    -1);

	g_free (display_name);
	g_free (compare_name);
	break;
    }

    gweather_location_free_children (loc, children);
}

ClutterModel *
mnp_get_world_timezones (void)
{
	GWeatherLocation *world;
	ClutterModel *store;

	world = gweather_location_new_world (FALSE);
	if (!world)
		return NULL;

	store = clutter_list_model_new (4, G_TYPE_STRING, "DisplayName",
                                  	G_TYPE_POINTER, "location",
					G_TYPE_STRING, "CompareName",
					G_TYPE_STRING, "SortName");


	fill_location_entry_model(store, world, NULL, NULL);
    	gweather_location_unref (world);

	return (ClutterModel *)store;
}
