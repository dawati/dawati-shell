
#include <stdio.h>
#include <stdlib.h>

#include <gtk/gtk.h>

#include "mnb-launcher-tree.h"

static void
launchers_change_cb (MnbLauncherMonitor *monitor,
                     MnbLauncherTree    *tree)
{
  GList *directories;

  printf ("%s()\n", __FUNCTION__);

  /* GMenu requires walking the tree in order to keep the monitor active. */
  directories = mnb_launcher_tree_list_entries (tree);
  mnb_launcher_tree_free_entries (directories);
}

int
main (int     argc,
      char  **argv)
{
  MnbLauncherTree     *tree;
  MnbLauncherMonitor  *monitor;
  GList               *directories;

  gtk_init (&argc, &argv);

  tree = mnb_launcher_tree_create ();
  directories = mnb_launcher_tree_list_entries (tree);
  monitor = mnb_launcher_tree_create_monitor (tree,
                                              (MnbLauncherMonitorFunction) launchers_change_cb,
                                              tree);
  mnb_launcher_tree_free_entries (directories);

  gtk_main ();

  mnb_launcher_tree_free (tree);

  return EXIT_SUCCESS;
}
