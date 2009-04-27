
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
