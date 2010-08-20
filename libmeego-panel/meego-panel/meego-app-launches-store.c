
/*
 * Copyright (c) 2010 Intel Corporation.
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 *
 * Author: Rob Staudinger <robsta@linux.intel.com>
 */

#include <stdio.h>
#include <stdlib.h>

#include <gtk/gtk.h>

#include <meego-panel/mpl-app-launches-query.h>
#include <meego-panel/mpl-app-launches-store-priv.h>

static void
print_entry (char const *executable,
             time_t      last_launched,
             unsigned    n_launches)
{
  struct tm last_launched_tm;
  char str[64] = { 0, };

  localtime_r (&last_launched, &last_launched_tm);
  strftime (str, sizeof (str), "%Y-%m-%d %H:%M:%S", &last_launched_tm);
  printf ("%s\t%s\t%i\n", executable, str, n_launches);
}

static void
_store_changed_cb (MplAppLaunchesStore  *store,
                   void                 *data)
{
  puts ("store changed");
}

int
main (int     argc,
      char  **argv)
{
  char const *add = NULL;
  bool async = false;
  int  timestamp = 0;
  char const *lookup = NULL;
  char const **query = NULL;
  bool lock_exclusive = false;
  bool lock_shared = false;
  bool watch = false;
  bool dump = false;
  GOptionEntry _options[] = {
    { "add", 'a', 0, G_OPTION_ARG_STRING, (void **) &add,
      "Add launch of <executable> at current time to database", "<executable>" },
    { "async", 0, 0, G_OPTION_ARG_NONE, &async,
      "Asynchronous 'add' to database", NULL },
    { "timestamp", 't', 0, G_OPTION_ARG_INT, &timestamp,
      "Pass timestamp to 'add' instead of using current time", NULL },
    { "lookup", 'l', 0, G_OPTION_ARG_STRING, (void **) &lookup,
      "Lookup <executable> in database", "<executable>" },
    { G_OPTION_REMAINING, 'q', 0, G_OPTION_ARG_STRING_ARRAY, (void **) &query,
      "Lookup <executable> ... in database", "<executable> ..." },
    { "lock-exclusive", 'e', 0, G_OPTION_ARG_NONE, &lock_exclusive,
      "Write-lock database", NULL },
    { "lock-shared", 's', 0, G_OPTION_ARG_NONE, &lock_shared,
      "Read-lock database", NULL },
    { "watch", 'w', 0, G_OPTION_ARG_NONE, &watch,
      "Watch database for changes", NULL },
    { "dump", 'd', 0, G_OPTION_ARG_NONE, &dump,
      "Dump database", NULL },
    { NULL }
  };

  GOptionContext      *context;
  MplAppLaunchesStore *store;
  GError              *error = NULL;

  g_type_init ();

  context = g_option_context_new ("- Test app launches database");
  g_option_context_add_main_entries (context, _options, NULL);
  if (!g_option_context_parse (context, &argc, &argv, &error))
  {
    g_critical ("%s\n\t%s", G_STRLOC, error->message);
    return EXIT_FAILURE;
  }

  store = mpl_app_launches_store_new ();

  if (add)
  {
    if (async)
      mpl_app_launches_store_add_async (store, add, timestamp, &error);
    else
      mpl_app_launches_store_add (store, add, timestamp, &error);

    if (error)
    {
      g_critical ("%s\n\t%s", G_STRLOC, error->message);
      g_clear_error (&error);
    }

  } else if (dump) {

    mpl_app_launches_store_dump (store, &error);
    if (error)
    {
      g_critical ("%s\n\t%s", G_STRLOC, error->message);
      g_clear_error (&error);
    }

  } else if (lookup) {

    time_t last_launched;
    uint32_t n_launches;
    GError *error = NULL;
    bool ret;
    ret = mpl_app_launches_store_lookup (store,
                                         lookup,
                                         &last_launched,
                                         &n_launches,
                                         &error);
    if (ret)
    {
      print_entry (lookup, last_launched, n_launches);

    } else if (error) {

      g_warning ("%s\n\t%s", G_STRLOC, error->message);
      g_clear_error (&error);

    } else {

      printf ("\"%s\" has never been launched.\n", lookup);

    }
  } else if (query) {

    MplAppLaunchesQuery *store_query = mpl_app_launches_store_create_query (store);

    while (*query)
    {
      time_t last_launched;
      uint32_t n_launches;
      GError *error = NULL;
      bool ret;
      ret = mpl_app_launches_query_lookup (store_query,
                                           *query,
                                           &last_launched,
                                           &n_launches,
                                           &error);
      if (ret)
      {
        print_entry (*query, last_launched, n_launches);

      } else if (error) {

        g_warning ("%s\n\t%s", G_STRLOC, error->message);
        g_clear_error (&error);
      }
      query++;
    }

    g_object_unref (store_query);

  } else if (lock_exclusive) {

    GError *error = NULL;
    puts ("Write-locking database.\n"
          "This will block if a write-lock is already in place.");

    mpl_app_launches_store_open (store, true, &error);
    if (error)
    {
      g_warning ("%s\n\t%s", G_STRLOC, error->message);
      g_clear_error (&error);

    } else {

      puts ("Lock acquired, press <enter> to continue");
      getchar ();

      mpl_app_launches_store_close (store, &error);
      if (error)
      {
        g_warning ("%s\n\t%s", G_STRLOC, error->message);
        g_clear_error (&error);
      } else {
        puts ("Lock lifted");
      }
    }

  } else if (lock_shared) {

    GError *error = NULL;
    puts ("Read-locking database.\n"
          "This will block if a write-lock is already in place.");

    mpl_app_launches_store_open (store, false, &error);
    if (error)
    {
      g_warning ("%s\n\t%s", G_STRLOC, error->message);
      g_clear_error (&error);

    } else {

      puts ("Lock acquired, press <enter> to continue");
      getchar ();

      mpl_app_launches_store_close (store, &error);
      if (error)
      {
        g_warning ("%s\n\t%s", G_STRLOC, error->message);
        g_clear_error (&error);
      } else {
        puts ("Lock lifted");
      }
    }

  } else if (watch) {

    g_signal_connect (store, "changed",
                      G_CALLBACK (_store_changed_cb), NULL);
    gtk_main ();

  } else {

    char *help = g_option_context_get_help (context, true, NULL);
    puts (help);
    g_free (help);
  }

  g_option_context_free (context);
  g_object_unref (store);

  return EXIT_SUCCESS;
}

