
#include <stdbool.h>
#include <stdlib.h>
#include <gtk/gtk.h>
#include <glib.h>
#include <gpm/gpm-brightness-xrandr.h>

int
main (int     argc,
      char  **argv)
{
  gboolean      down = FALSE;
  gboolean      up = FALSE;
  gboolean      get = FALSE;
  int           set_percentage = -1;
  GOptionEntry  options[] = {
    { "down", 'd', 0, G_OPTION_ARG_NONE, &down,
      "Turn brightness down", NULL },
    { "up", 'u', 0, G_OPTION_ARG_NONE, &up,
      "Turn brightness up", NULL },
    { "get", 'g', 0, G_OPTION_ARG_NONE, &get,
      "Get brightness", NULL },
    { "set", 's', 0, G_OPTION_ARG_INT, &set_percentage,
      "Set brightness", "percentage" },
    { NULL }
  };

  GpmBrightnessXRandR *brightness;
  GOptionContext      *context;
  GError              *error = NULL;
  gboolean             hw_changed = FALSE;
  gboolean             ret;

  context = g_option_context_new ("- Test xbacklight");
  g_option_context_add_main_entries (context, options, NULL);
  if (!g_option_context_parse (context, &argc, &argv, &error))
  {
    g_critical ("%s %s", G_STRLOC, error->message);
    g_clear_error (&error);
  }
  g_option_context_free (context);

  gtk_init (&argc, &argv);

  brightness = gpm_brightness_xrandr_new ();

  if (down)
  {
    ret = gpm_brightness_xrandr_down (brightness, &hw_changed);
    printf ("Down: %s (hardware changed: %s)\n",
            ret ? "succeeded" : "failed",
            hw_changed ? "true" : "false");
  }

  if (up)
  {
    ret = gpm_brightness_xrandr_up (brightness, &hw_changed);
    printf ("Up: %s (hardware changed: %s)\n",
            ret ? "succeeded" : "failed",
            hw_changed ? "true" : "false");
  }

  if (get)
  {
    guint percentage;
    ret = gpm_brightness_xrandr_get (brightness, &percentage);
    printf ("Get: %s, percentage %u%%\n",
            ret ? "succeeded" : "failed",
            percentage);
  }

  if (set_percentage != -1)
  {
    set_percentage = CLAMP (set_percentage, 0, 100);
    ret = gpm_brightness_xrandr_set (brightness, set_percentage, &hw_changed);
    printf ("Set: %s, percentage %u%% (hardware changed: %s)\n",
            ret ? "succeeded" : "failed",
            set_percentage,
            hw_changed ? "true" : "false");
  }

  g_object_unref (brightness);

  return EXIT_SUCCESS;
}

