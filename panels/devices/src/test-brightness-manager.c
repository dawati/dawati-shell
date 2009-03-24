#include <dalston/dalston-brightness-manager.h>

static void
_manager_num_levels_changed_cb (DalstonBrightnessManager *manager,
                                gint                      num_levels)
{
  g_debug (G_STRLOC ": Num levels changed: %d",
           num_levels);
}

int
main (int    argc,
      char **argv)
{
  GMainLoop *loop;
  DalstonBrightnessManager *manager;

  g_type_init ();
  loop = g_main_loop_new (NULL, TRUE);

  manager = dalston_brightness_manager_new ();

  g_signal_connect (manager,
                    "num-levels-changed",
                    _manager_num_levels_changed_cb,
                    NULL);

  g_main_loop_run (loop);
}
