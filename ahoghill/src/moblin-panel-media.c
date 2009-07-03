#include <config.h>

#include <glib/gi18n.h>
#include <clutter/clutter.h>
#include <clutter/x11/clutter-x11.h>
#include <gdk/gdkx.h>
#include <gtk/gtk.h>
#include <nbtk/nbtk.h>
#include <bickley/bkl.h>

#include <moblin-panel/mpl-panel-clutter.h>
#include <moblin-panel/mpl-panel-common.h>
#include <ahoghill/ahoghill-grid-view.h>

static void
stage_width_notify_cb (ClutterActor     *stage,
                       GParamSpec       *pspec,
                       AhoghillGridView *view)
{
    guint width = clutter_actor_get_width (stage);

    clutter_actor_set_width (CLUTTER_ACTOR (view), width);
}

static void
stage_height_notify_cb (ClutterActor     *stage,
                        GParamSpec       *pspec,
                        AhoghillGridView *view)
{
    guint height = clutter_actor_get_height (stage);

    clutter_actor_set_height (CLUTTER_ACTOR (view), height);
}

static void
ahoghill_dismissed_cb (AhoghillGridView *view,
                       MplPanelClient   *panel)
{
    mpl_panel_client_request_hide (panel);
}

static void
panel_show_begin_cb (MplPanelClient   *panel,
                     AhoghillGridView *view)
{
}

static void
panel_show_end_cb (MplPanelClient   *panel,
                   AhoghillGridView *view)
{
    ahoghill_grid_view_focus (view);
}

static void
panel_hide_end_cb (MplPanelClient   *panel,
                   AhoghillGridView *view)
{
    ahoghill_grid_view_clear (view);
    ahoghill_grid_view_unfocus (view);
}

static gboolean _standalone = FALSE;

static GOptionEntry entries[] = {
  {"standalone", 's', 0, G_OPTION_ARG_NONE, &_standalone, "Do not embed into the mutter-moblin panel", NULL}
};

int
main (int    argc,
      char **argv)
{
    ClutterActor *stage;
    AhoghillGridView *gridview;
    GOptionContext *context;
    GError *error = NULL;

    g_thread_init (NULL);
    bkl_init ();

    context = g_option_context_new ("- mutter-moblin media panel");
    g_option_context_add_main_entries (context, entries, GETTEXT_PACKAGE);
    g_option_context_add_group (context, clutter_get_option_group_without_init ());
    g_option_context_add_group (context, gtk_get_option_group (TRUE));
    if (!g_option_context_parse (context, &argc, &argv, &error)) {
        g_critical ("%s %s", G_STRLOC, error->message);
        g_critical ("Starting in standalone mode.");
        g_clear_error (&error);
        _standalone = TRUE;
    }
    g_option_context_free (context);

    MPL_PANEL_CLUTTER_INIT_WITH_GTK (&argc, &argv);

    nbtk_texture_cache_load_cache (nbtk_texture_cache_get_default (),
                                   DATADIR "/icons/moblin/48x48/nbtk.cache");
    nbtk_texture_cache_load_cache (nbtk_texture_cache_get_default (),
                                   DATADIR "/icons/hicolor/48x48/nbtk.cache");
    nbtk_texture_cache_load_cache (nbtk_texture_cache_get_default (),
                                   DATADIR "/mutter-moblin/nbtk.cache");

    nbtk_texture_cache_load_cache (nbtk_texture_cache_get_default (),
                                   NBTK_CACHE);
    nbtk_style_load_from_file (nbtk_style_get_default (),
                               THEMEDIR "/panel.css", NULL);

    if (_standalone) {
        Window xwin;

        stage = clutter_stage_get_default ();
        clutter_actor_realize (stage);
        xwin = clutter_x11_get_stage_window (CLUTTER_STAGE (stage));

        MPL_PANEL_CLUTTER_SETUP_EVENTS_WITH_GTK_FOR_XID (xwin);

        gridview = g_object_new (AHOGHILL_TYPE_GRID_VIEW, NULL);
        clutter_container_add_actor (CLUTTER_CONTAINER (stage),
                                     CLUTTER_ACTOR (gridview));

        g_signal_connect (stage, "notify::width",
                          G_CALLBACK (stage_width_notify_cb), gridview);
        g_signal_connect (stage, "notify::height",
                          G_CALLBACK (stage_height_notify_cb), gridview);
        clutter_actor_set_size (stage, 1024, 768);
        clutter_actor_show (stage);
    } else {
        MplPanelClient *panel;

        /* All button styling goes in mutter-moblin.css for now,
         * don't pass out own stylesheet. */
        panel = mpl_panel_clutter_new (MPL_PANEL_MEDIA,
                                       _("media"),
                                       NULL, "media-button", TRUE);

        MPL_PANEL_CLUTTER_SETUP_EVENTS_WITH_GTK (panel);

        gridview = g_object_new (AHOGHILL_TYPE_GRID_VIEW, NULL);
        g_signal_connect (gridview, "dismiss",
                          G_CALLBACK (ahoghill_dismissed_cb), panel);

        g_signal_connect (panel, "show-begin",
                          G_CALLBACK (panel_show_begin_cb), gridview);
        g_signal_connect (panel, "show-end",
                          G_CALLBACK (panel_show_end_cb), gridview);
        g_signal_connect (panel, "hide-end",
                          G_CALLBACK (panel_hide_end_cb), gridview);

        stage = mpl_panel_clutter_get_stage (MPL_PANEL_CLUTTER (panel));
        g_signal_connect (stage, "notify::width",
                          G_CALLBACK (stage_width_notify_cb), gridview);
        g_signal_connect (stage, "notify::height",
                          G_CALLBACK (stage_height_notify_cb), gridview);

        clutter_container_add_actor (CLUTTER_CONTAINER (stage),
                                     CLUTTER_ACTOR (gridview));
    }

    clutter_main ();

    return 0;
}
