
#include <stdio.h>
#include <stdlib.h>

#include <clutter/clutter.h>
#include <gtk/gtk.h>
#include <gio/gdesktopappinfo.h>
#include <nbtk/nbtk.h>
#include "mnb-drop-down.h"
#include "moblin-netbook.h"
#include "moblin-netbook-launcher.h"
#include "moblin-netbook-chooser.h"

/* Fake plugin implementation to satisfy the linker. */

GType moblin_netbook_plugin_get_type (void);
gboolean hide_panel (MutterPlugin *plugin);

GType
moblin_netbook_plugin_get_type (void)
{
  return 0;
}

void
mnb_drop_down_hide_with_toolbar (MnbDropDown *dropdown)
{
}

/* Fake dropdown implementation to satisfy the linker. */

GType
mnb_drop_down_get_type (void)
{
  return 0;
}

NbtkWidget *
mnb_drop_down_new (MutterPlugin *plugin)
{
  return NULL;
}

void
mnb_drop_down_set_child (MnbDropDown *drop_down, ClutterActor *child)
{
  ;
}

gboolean
moblin_netbook_launch_application_from_desktop_file (const  gchar *desktop,
                                                     GList        *files,
                                                     gint          workspace,
                                                     gboolean      no_chooser)
{
  GAppInfo *app;
  GAppLaunchContext *ctx;
  GError *error = NULL;
  gboolean retval = TRUE;

  g_return_val_if_fail (desktop, FALSE);

  app = G_APP_INFO (g_desktop_app_info_new_from_filename (desktop));

  if (!app)
    {
      g_warning ("Failed to create GAppInfo for file %s", desktop);
      return FALSE;
    }

  ctx = G_APP_LAUNCH_CONTEXT (gdk_app_launch_context_new ());

  retval = g_app_info_launch (app, files, ctx, &error);

  if (error)
    {
      g_warning ("Failed to lauch %s (%s)",
#if GLIB_CHECK_VERSION(2,20,0)
                 g_app_info_get_commandline (app),
#else
                 g_app_info_get_name (app),
#endif
                 error->message);

      g_error_free (error);
    }

  g_object_unref (ctx);
  g_object_unref (app);

  return retval;
}

int
main (int argc, char *argv[])
{
  ClutterActor *stage;
  ClutterActor *launcher;

  gtk_init (&argc, &argv);
  clutter_init (&argc, &argv);

  nbtk_style_load_from_file (nbtk_style_get_default (),
                             "../theme/panel.css", NULL);

  stage = clutter_stage_get_default ();
  clutter_actor_set_size (stage, 800, 600);

  launcher = mnb_launcher_new (800, 600);
  clutter_container_add_actor (CLUTTER_CONTAINER (stage), launcher);

  clutter_actor_show (stage);

  clutter_main ();

  return EXIT_SUCCESS;
}
