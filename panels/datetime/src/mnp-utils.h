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

#include <clutter/clutter.h>
#include <mx/mx.h>

#define GWEATHER_I_KNOW_THIS_IS_UNSTABLE
#include <libgweather/gweather-xml.h>
#include <libgweather/gweather-location.h>
#include <libgweather/weather.h>

#ifndef _MNP_UTILS_H
#define _MNP_UTILS_H

enum
{
    GWEATHER_LOCATION_ENTRY_COL_DISPLAY_NAME = 0,
    GWEATHER_LOCATION_ENTRY_COL_LOCATION,
    GWEATHER_LOCATION_ENTRY_COL_COMPARE_NAME,
    GWEATHER_LOCATION_ENTRY_COL_SORT_NAME,
    GWEATHER_LOCATION_ENTRY_NUM_COLUMNS
};

typedef struct _mnp_date_fmt {
	char *date;
	char *city;
	char *time;
	gboolean day;
}MnpDateFormat;

typedef struct _mnp_zone_location {
	char *display;
	char *city;
	char *tzid;
}MnpZoneLocation;


ClutterModel * mnp_get_world_timezones (void);
const GWeatherLocation * mnp_utils_get_location_from_display (ClutterModel *store, const char *display);
GPtrArray * mnp_load_zones (void);
void mnp_save_zones (GPtrArray *zones);
MnpDateFormat * mnp_format_time_from_location (MnpZoneLocation *location, time_t time_now, gboolean tfh);
char * mnp_utils_get_display_from_location (ClutterModel *store, GWeatherLocation *location);


#endif 
