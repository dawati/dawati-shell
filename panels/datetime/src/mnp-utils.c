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

#include <config.h>
#include <string.h>
#include <time.h>
#include <libxml/xmlreader.h>

#include <glib/gi18n.h>
#define GWEATHER_I_KNOW_THIS_IS_UNSTABLE
#include <libgweather/gweather-xml.h>
#include <libgweather/gweather-location.h>
#include <libgweather/weather.h>
#include "system-timezone.h"
#include "mnp-utils.h"

GWeatherLocation *world = NULL;
WeatherLocation *gweather_location_to_weather_location (GWeatherLocation *gloc,
							const char *name);
struct _pass_data {
	char *key;
	const GWeatherLocation *location;
};

static gboolean 
search_locations_in_model (ClutterModel *model, ClutterModelIter *iter, struct _pass_data *pdata)
{
	char *clocation;
	GWeatherLocation *location;

	clutter_model_iter_get (iter, GWEATHER_LOCATION_ENTRY_COL_DISPLAY_NAME, &clocation,
				GWEATHER_LOCATION_ENTRY_COL_LOCATION, &location, -1);

	if (strcmp(clocation, pdata->key) == 0) {
		pdata->location = location;
		g_free (clocation);
		return FALSE;
	}

	g_free(clocation);

	return TRUE;
}

const GWeatherLocation *
mnp_utils_get_location_from_display (ClutterModel *store, const char *display)
{
	struct _pass_data pdata = { NULL, NULL};

	pdata.key = (char *)display;
	clutter_model_foreach (store, (ClutterModelForeachFunc)search_locations_in_model, &pdata);

	return pdata.location;
}

static gboolean 
search_locations_display_in_model (ClutterModel *model, ClutterModelIter *iter, struct _pass_data *pdata)
{
	char *clocation;
	GWeatherLocation *location;

	clutter_model_iter_get (iter, GWEATHER_LOCATION_ENTRY_COL_DISPLAY_NAME, &clocation,
				GWEATHER_LOCATION_ENTRY_COL_LOCATION, &location, -1);

	if (location == pdata->location) {
		pdata->key = clocation;
		return FALSE;
	}

	g_free(clocation);

	return TRUE;
}

char *
mnp_utils_get_display_from_location (ClutterModel *store, GWeatherLocation *location)
{
	struct _pass_data pdata = { NULL, NULL};

	pdata.location = location;
	clutter_model_foreach (store, (ClutterModelForeachFunc)search_locations_display_in_model, &pdata);

	return pdata.key;
}

static void
fill_location_entry_model (ClutterModel *store, GWeatherLocation *loc,
			   const char *parent_display_name,
			   const char *parent_compare_name,
			   const char *parent_sort_name)
{
    GWeatherLocation **children;
    char *display_name, *compare_name, *sort_name;
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
				       parent_compare_name,
				       parent_sort_name);
	}
	break;

    case GWEATHER_LOCATION_COUNTRY:
	/* Recurse, initializing the names to the country name */
	for (i = 0; children[i]; i++) {
	    fill_location_entry_model (store, children[i],
				       gweather_location_get_name (loc),
				       gweather_location_get_sort_name (loc),
				       gweather_location_get_sort_name (loc));
	}
	break;

    case GWEATHER_LOCATION_ADM1:
	/* Recurse, adding the ADM1 name to the country name */
	display_name = g_strdup_printf ("%s, %s", gweather_location_get_name (loc), parent_display_name);
	compare_name = g_strdup_printf ("%s, %s", gweather_location_get_sort_name (loc), parent_compare_name);

	for (i = 0; children[i]; i++) {
	    fill_location_entry_model (store, children[i],
				       display_name, compare_name, parent_sort_name);
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

	    	sort_name = g_strdup_printf ("%s:%s",
					    gweather_location_get_sort_name (loc),
					    parent_sort_name);

		clutter_model_prepend(store, 
				    GWEATHER_LOCATION_ENTRY_COL_LOCATION, children[i],
				    GWEATHER_LOCATION_ENTRY_COL_DISPLAY_NAME, display_name,
				    GWEATHER_LOCATION_ENTRY_COL_COMPARE_NAME, compare_name,
				    GWEATHER_LOCATION_ENTRY_COL_SORT_NAME, sort_name,
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
	    sort_name = g_strdup_printf ("%s:%s",
					    gweather_location_get_sort_name (loc),
					    parent_sort_name);
	    clutter_model_prepend(store, 
				GWEATHER_LOCATION_ENTRY_COL_LOCATION, children[0],
				GWEATHER_LOCATION_ENTRY_COL_DISPLAY_NAME, display_name,
				GWEATHER_LOCATION_ENTRY_COL_COMPARE_NAME, compare_name,
				GWEATHER_LOCATION_ENTRY_COL_SORT_NAME, sort_name,
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
	clutter_model_prepend(store, 		
			    GWEATHER_LOCATION_ENTRY_COL_LOCATION, loc,
			    GWEATHER_LOCATION_ENTRY_COL_DISPLAY_NAME, display_name,
			    GWEATHER_LOCATION_ENTRY_COL_COMPARE_NAME, compare_name,
			    GWEATHER_LOCATION_ENTRY_COL_SORT_NAME, parent_sort_name,
			    -1);

	g_free (display_name);
	g_free (compare_name);
	break;
    }

}

static char *
find_location_entry (GWeatherLocation *loc,
		     const char *city,
		     const char *country,
		     const char *parent_display_name)
{
    GWeatherLocation **children;
    int i;
    char *display_name;
    char *zone=NULL;

    children = gweather_location_get_children (loc);

    switch (gweather_location_get_level (loc)) {
    case GWEATHER_LOCATION_WORLD:
    case GWEATHER_LOCATION_REGION:
    case GWEATHER_LOCATION_ADM2:
	/* Ignore these levels of hierarchy; just recurse, passing on
	 * the names from the parent node.
	 */
	for (i = 0; children[i] && !zone; i++) {
	    zone = find_location_entry	(children[i], city, country, parent_display_name);
	}

	return zone;

    case GWEATHER_LOCATION_COUNTRY: {
	/* Recurse, initializing the names to the country name */
	char *lcountry = gweather_location_get_name (loc);

	if (g_ascii_strcasecmp (lcountry, country) == 0) {

		for (i = 0; children[i] && !zone; i++) {
			zone = find_location_entry (children[i], city, country, gweather_location_get_name (loc));
		}
	}

	return zone;
    }
    case GWEATHER_LOCATION_ADM1:
	/* Recurse, adding the ADM1 name to the country name */
	display_name = g_strdup_printf ("%s, %s", gweather_location_get_name (loc), parent_display_name);

	for (i = 0; children[i] && !zone; i++) {
	    zone = find_location_entry	(children[i], city, country, display_name);
	}
	g_free (display_name);
	
	return zone;

    case GWEATHER_LOCATION_CITY: {
	char *lcity = gweather_location_get_name (loc);

	if (g_ascii_strcasecmp (lcity, city) == 0) {
		if (children[0] && children[1]) {
			display_name = g_strdup_printf ("%s (%s), %s",
						gweather_location_get_name (loc),
						gweather_location_get_name (children[i]),
						parent_display_name);
			printf("display: %s\n", display_name);
			return display_name;
		} else if (children[0]) {
			display_name = g_strdup_printf ("%s, %s",
						gweather_location_get_name (loc),
						parent_display_name);
			printf("display: %s\n", display_name);
			return display_name;			
		}

	}
	return NULL;
    }
    case GWEATHER_LOCATION_WEATHER_STATION:
	break;
    }

    return NULL;
}


static gint 
sort_weather_model (ClutterModel *model, const GValue *a, const GValue *b, gpointer user_data)
{
	const char *loc1, *loc2;
	gchar **city1, **city2;
	int ret = 0;

	loc1 = g_value_get_string (a);
	loc2 = g_value_get_string (b);

	city1 = g_strsplit(loc1, ":", 2);
	city2 = g_strsplit(loc2, ":", 2);

	if (strcmp(city1[0], "paris") == 0) {
		if (strcmp(city1[1], "france") == 0)
			ret = -1;
		else 
			ret = strcmp(loc1, loc2);
	} else if (strcmp(city1[0], "london") == 0) {
		if (strcmp(city1[1], "united kingdom") == 0)
			ret = -1;
		else 
			ret = strcmp(loc1, loc2);
	} else if (strcmp(city2[0], "paris") == 0) {
		if (strcmp(city2[1], "france") == 0)
			ret = 1;
		else 
			ret = strcmp(loc1, loc2);
	} else if (strcmp(city2[0], "london") == 0) {
		if (strcmp(city2[1], "united kingdom") == 0)
			ret = 1;
		else 
			ret = strcmp(loc1, loc2);
	}
	else
		ret = strcmp(loc1, loc2);
	g_strfreev(city1);
	g_strfreev(city2);

	return ret;

}

char *
mnp_find_location (const char *city, const char *country)
{
	if (!world) {
		world = gweather_location_new_world (FALSE);
	}

	return find_location_entry (world, city, country, NULL);
}

ClutterModel *
mnp_get_world_timezones (void)
{
	ClutterModel *store;

	if (!world) {
		world = gweather_location_new_world (FALSE);
	}
	if (!world)
		return NULL;

	store = clutter_list_model_new (4, G_TYPE_STRING, "DisplayName",
                                  	G_TYPE_POINTER, "location",
					G_TYPE_STRING, "CompareName",
					G_TYPE_STRING, "SortName");


	fill_location_entry_model(store, world, NULL, NULL, NULL);
    	/* gweather_location_unref (world); */

	clutter_model_set_sort (store, 3, sort_weather_model, NULL, NULL);
	return (ClutterModel *)store;
}

/* Time formatting from gnome-panel */

static const char *
get_tzname (char *tzid)
{
        time_t now_t;
        struct tm now;

        setenv ("TZ", tzid, 1);
        tzset();

        now_t = time (NULL);
        localtime_r (&now_t, &now);

        if (now.tm_isdst > 0) {
                return tzname[1];
        } else {
                return tzname[0];
        }

	return NULL;
}

static void
clock_set_tz (char *tzone)
{
        time_t now_t;
        struct tm now;

        setenv ("TZ", tzone, 1);
        tzset();
}

#include <sys/time.h>

static void
clock_unset_tz (SystemTimezone *tzone)
{
        const char *env_timezone;

        env_timezone = system_timezone_get_env (tzone);

        if (env_timezone) {
                setenv ("TZ", env_timezone, 1);
        } else {
                unsetenv ("TZ");
        }
        tzset();
}

static glong
get_offset (const char *tzone, SystemTimezone *systz)
{
	glong offset = 0;
	time_t t;
	struct tm *tm;

        unsetenv ("TZ");
	t = time (NULL);
        tm = localtime (&t);

	if (tm->tm_isdst <= 0)
          goto done;

        setenv ("TZ", tzone, 1);
	t = time (NULL);
        tm = localtime (&t);
	if (tm->tm_isdst <= 0)
		offset = -3600;

done:
        clock_unset_tz (systz);
        return offset;
}

static void
clock_location_localtime (SystemTimezone *systz, char *tzone, struct tm *tm, time_t now)
{
	glong offset = get_offset (tzone, systz);
	time_t real_now = now + offset;

        clock_set_tz (tzone);

        localtime_r (&real_now, tm);

        clock_unset_tz (systz);
}

static MnpDateFormat *
format_time (struct tm   *now, 
             char        *tzname,
             gboolean  	  twelveh,
	     time_t 	 local_t,
	     gboolean 	 priority)
{
	char buf[256];
	char *format;
	struct tm local_now;
	time_t now_t;
	char *utf8;	
	MnpDateFormat *fmt = g_new0 (MnpDateFormat, 1);

	localtime_r (&local_t, &local_now);
	now_t = mktime(now);

	if (twelveh) {
		/* Translators: This is a strftime format string.
		 * It is used to display the time in 12-hours format
		 * (eg, like in the US: 8:10 am), when the local
		 * weekday differs from the weekday at the location
		 * (the %A expands to the weekday). The %p expands to
		 * am/pm. */
		format = _("%l:%M %p");
	}
	else {
		/* Translators: This is a strftime format string.
		 * It is used to display the time in 24-hours format
		 * (eg, like in France: 20:10), when the local
		 * weekday differs from the weekday at the location
		 * (the %A expands to the weekday). */
		format = _("%H:%M");
	}

	if (strftime (buf, sizeof (buf), format, now) <= 0) {
		strcpy (buf, "???");
	}
	utf8 = g_locale_to_utf8 (buf, -1, NULL, NULL, NULL);
	fmt->time = utf8;
	fmt->day = ((now->tm_hour >= 6) && (now->tm_hour < 19)) ? TRUE: FALSE;

	if (local_now.tm_wday != now->tm_wday) {
		/* Translators: This is a strftime format string.
		 * It is used to display in Aug 6 (Yesterday) */
		
		if (local_now.tm_wday > now->tm_wday)
			format = _("%b %-d (Yesterday)");
		else
			format = _("%b %-d (Tomorrow)");
	}
	else {
		/* Translators: This is a strftime format string.
		 * It is used to display in Aug 6 */
		if (priority) 
			format = _("%b %-d (current location)");
		else if (now_t == local_t) 
			format = _("%b %-d (local)");
		else
			format = _("%b %-d");
	}

	if (strftime (buf, sizeof (buf), format, now) <= 0) {
		strcpy (buf, "???");
	}

	utf8 = g_locale_to_utf8 (buf, -1, NULL, NULL, NULL);
	fmt->date = utf8;

	return fmt;
}

MnpDateFormat *
mnp_format_time_from_location (MnpZoneLocation *location, time_t time_now, gboolean tfh, gboolean priority)
{
	char *tzid;
	char *tzname;
	static SystemTimezone *systz = NULL; 
	struct tm now;
	MnpDateFormat *fmt;

	if (!systz)
		systz = system_timezone_new ();

	tzid = location->tzid;
	tzname = g_strdup (get_tzname(tzid));

	clock_location_localtime (systz, tzid, &now, time_now);
	fmt = format_time (&now, tzname, !tfh, time_now, priority);

	fmt->city = g_strdup(location->city);
	g_free(tzname);

	return fmt;
}

/* Keyfile data */
GPtrArray *
mnp_load_zones (void)
{
	GKeyFile *kfile;
	char *pfile;
	GPtrArray *zones = g_ptr_array_new ();
	pfile = g_build_filename(g_get_user_config_dir(), "panel-clock-zones.conf", NULL);

	kfile = g_key_file_new ();
	if (g_key_file_load_from_file(kfile, pfile, G_KEY_FILE_NONE, NULL)) {
		guint len=0, i;
		char **grps;
		
		grps = g_key_file_get_groups (kfile, &len);
		for (i=0; i<len; i++) {
			MnpZoneLocation *loc = g_new0(MnpZoneLocation, 1);

			loc->display = g_strdup (grps[i]);
			loc->city = g_key_file_get_string (kfile, grps[i], "city", NULL);
			loc->tzid = g_key_file_get_string (kfile, grps[i], "tzid", NULL);
			g_ptr_array_add (zones, loc);
		}
		
		g_strfreev(grps);
	}
	g_key_file_free(kfile);
	g_free (pfile);

	return zones;
}

void
mnp_save_zones (GPtrArray *zones)
{
	GKeyFile *kfile;
	char *pfile;
	char *contents;
	guint len=0;
	int i;

	pfile = g_build_filename(g_get_user_config_dir(), "panel-clock-zones.conf", NULL);
	
	kfile = g_key_file_new ();

	for (i=0; i<zones->len; i++) {
		MnpZoneLocation *loc = (MnpZoneLocation *)zones->pdata[i];
		g_key_file_set_string (kfile, loc->display, "city", loc->city);
		g_key_file_set_string (kfile, loc->display, "tzid", loc->tzid);

	}
	contents = g_key_file_to_data (kfile, &len, NULL);
	
	if(len) {
		g_file_set_contents (pfile, contents, len, NULL);
	}

	g_free(contents);
	g_key_file_free(kfile);
	g_free (pfile);

}
