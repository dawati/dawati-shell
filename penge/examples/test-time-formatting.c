/*
 * Copyright (C) 2008 - 2009 Intel Corporation.
 *
 * Author: Rob Bradford <rob@linux.intel.com>
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



#include <stdio.h>
#include <stdlib.h>
#include <penge/penge-utils.h>

int
main (int    argc,
      char **argv)
{
  gchar     *str;
  GTimeVal   now;
  GTimeVal   time;

  g_get_current_time (&now);

  time = now;
  time.tv_sec -= 30;
  str = penge_utils_format_time (&time);
  printf ("Minus 30 seconds: '%s'\n", str);
  g_free (str);

  time = now;
  time.tv_sec -= 30 * 60;
  str = penge_utils_format_time (&time);
  printf ("Minus 30 minutes: '%s'\n", str);
  g_free (str);

  time = now;
  time.tv_sec -= 60 * 60 * 2;
  str = penge_utils_format_time (&time);
  printf ("Minus 2 hours: '%s'\n", str);
  g_free (str);

  time = now;
  time.tv_sec -= 60 * 60 * 24;
  str = penge_utils_format_time (&time);
  printf ("Minus 1 day: '%s'\n", str);
  g_free (str);

  time = now;
  time.tv_sec -= 60 * 60 * 24 * 4;
  str = penge_utils_format_time (&time);
  printf ("Minus 4 days: '%s'\n", str);
  g_free (str);

  time = now;
  time.tv_sec -= 60 * 60 * 24 * 10;
  str = penge_utils_format_time (&time);
  printf ("Minus 10 days: '%s'\n", str);
  g_free (str);

  time = now;
  time.tv_sec -= 60 * 60 * 24 * 20;
  str = penge_utils_format_time (&time);
  printf ("Minus 20 days: '%s'\n", str);
  g_free (str);

  time = now;
  time.tv_sec -= 60 * 60 * 24 * 25;
  str = penge_utils_format_time (&time);
  printf ("Minus 25 days: '%s'\n", str);
  g_free (str);

  time = now;
  time.tv_sec -= 60 * 60 * 24 * 30 * 3;
  str = penge_utils_format_time (&time);
  printf ("Minus ~3 months: '%s'\n", str);
  g_free (str);

  time = now;
  time.tv_sec -= 60 * 60 * 24 * 30 * 11;
  str = penge_utils_format_time (&time);
  printf ("Minus ~11 months: '%s'\n", str);
  g_free (str);

  time = now;
  time.tv_sec -= 60 * 60 * 24 * 365 * 3;
  str = penge_utils_format_time (&time);
  printf ("Minus 3 years: '%s'\n", str);
  g_free (str);

  return EXIT_SUCCESS;
}
