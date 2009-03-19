
#include <stdio.h>
#include <stdlib.h>

#include "mnb-launcher-entry.h"

int
main (int argc, char *argv[])
{
  GSList  *tree;
  GSList  *tree_iter;

  tree = mnb_launcher_tree_create ();
  for (tree_iter = tree; tree_iter; tree_iter = tree_iter->next)
    {
      MnbLauncherDirectory  *directory;
      GSList                *directory_iter;

      directory = (MnbLauncherDirectory *) tree_iter->data;
      printf ("%s\n", directory->name);

      for (directory_iter = directory->entries;
            directory_iter;
            directory_iter = directory_iter->next)
        {
          MnbLauncherEntry  *entry;

          entry = (MnbLauncherEntry *) directory_iter->data;
          printf ("  %s\n", mnb_launcher_entry_get_name (entry));
        }
    }

  mnb_launcher_tree_free (tree);

  return EXIT_SUCCESS;
}
