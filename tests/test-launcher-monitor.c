
#include <stdio.h>
#include <stdlib.h>

#include <gtk/gtk.h>

#include "mnb-launcher-tree.h"

static void
launchers_change_cb (MnbLauncherMonitor *monitor,
                     const gchar        *message)
{
  printf ("%s() '%s'\n", __FUNCTION__, message);
}

/* This leaks. */
static gboolean
test_launcher_monitor (void)
{
  MnbLauncherTree     *tree;
  MnbLauncherMonitor  *monitor;

  tree = mnb_launcher_tree_create ();
  monitor = mnb_launcher_tree_create_monitor (tree,
                                              (MnbLauncherMonitorFunction) launchers_change_cb,
                                              "apps changed");
  mnb_launcher_tree_free (tree);

  return FALSE;
}

int
main (int     argc,
      char  **argv)
{
  gtk_init (&argc, &argv);

  test_launcher_monitor ();

  gtk_main ();

  return EXIT_SUCCESS;
}
