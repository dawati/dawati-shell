#include <anerley/anerley-tp-feed.h>
#include <anerley/anerley-item.h>
#include <anerley/anerley-simple-grid-view.h>
#include <anerley/anerley-tile.h>
#include <anerley/anerley-aggregate-tp-feed.h>
#include <anerley/anerley-feed-model.h>

#include <clutter/clutter.h>
#include <libmissioncontrol/mission-control.h>
#include <telepathy-glib/contact.h>

#include <glib.h>
#include <dbus/dbus-glib.h>

static void
_entry_text_changed_cb (ClutterText  *text,
                        ClutterModel *model)
{
  const gchar *str;
  str = clutter_text_get_text (text);
  g_debug ("foo: %s", str);

  anerley_feed_model_set_filter_text ((AnerleyFeedModel *)model, str);
}

int
main (int    argc,
      char **argv)
{
  MissionControl *mc;
  AnerleyTpFeed *feed;
  DBusGConnection *conn;
  ClutterActor *stage;
  NbtkWidget *scroll_view;
  NbtkWidget *icon_view;
  ClutterModel *model;
  gchar *path;
  NbtkStyle *style;
  GError *error = NULL;
  NbtkWidget *table, *entry;
  ClutterActor *tmp;


  clutter_init (&argc, &argv);

  path = g_build_filename (PKG_DATA_DIR,
                           "style.css",
                           NULL);

  style = nbtk_style_get_default ();

  if (!nbtk_style_load_from_file (style,
                                  path,
                                  &error))
  {
    g_warning (G_STRLOC ": Error opening style: %s",
               error->message);
    g_clear_error (&error);
  }

  g_free (path);

  conn = dbus_g_bus_get (DBUS_BUS_SESSION, NULL);
  mc = mission_control_new (conn);
  feed = anerley_aggregate_tp_feed_new (mc);
  model = anerley_feed_model_new (feed);
  stage = clutter_stage_get_default ();
  icon_view = anerley_tile_view_new (model);

  table = nbtk_table_new ();
  entry = nbtk_entry_new (NULL);
  tmp = nbtk_entry_get_clutter_text (entry);
  g_signal_connect (tmp,
                    "text-changed",
                    _entry_text_changed_cb,
                    model);

  nbtk_table_add_actor_with_properties (NBTK_TABLE (table),
                                        entry,
                                        0,
                                        0,
                                        "x-fill",
                                        TRUE,
                                        "x-expand",
                                        TRUE,
                                        "y-expand",
                                        FALSE,
                                        NULL);

  scroll_view = nbtk_scroll_view_new ();
  clutter_container_add_actor (CLUTTER_CONTAINER (stage),
                               table);
  clutter_container_add_actor (CLUTTER_CONTAINER (scroll_view),
                               icon_view);
  clutter_actor_show_all (stage);
  nbtk_table_add_actor_with_properties (NBTK_TABLE (table),
                                        scroll_view,
                                        1,
                                        0,
                                        "x-fill",
                                        TRUE,
                                        "x-expand",
                                        TRUE,
                                        "y-expand",
                                        TRUE,
                                        "y-fill",
                                        TRUE,
                                        NULL);

  clutter_actor_set_size (table, 640, 480);
  clutter_main ();

  return 0;
}

