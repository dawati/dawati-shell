#include <penge/penge-grid-view.h>
#include "moblin-netbook.h"
#include "moblin-netbook-mzone.h"

static void
_activated_cb (PengeGridView *view,
               NbtkTable     *table)
{
  clutter_actor_hide ((ClutterActor *)table);
}

ClutterActor *
make_mzone_grid (gint width)
{
  NbtkWidget *table;
  ClutterActor *grid;
  NbtkWidget *footer;
  NbtkWidget *up_button;
  NbtkPadding padding = MN_PADDING (4, 4, 4, 4);
  NbtkPadding bg_padding = MN_PADDING (0, 4, 36, 4);

  grid = g_object_new (PENGE_TYPE_GRID_VIEW,
                       NULL);
  clutter_actor_set_size (grid, 1024, 500);
  nbtk_widget_set_padding ((NbtkWidget *)grid, &padding);

  table = nbtk_table_new ();

  nbtk_widget_set_padding (NBTK_WIDGET (table), &bg_padding);

  /* footer with "up" button */
  footer = nbtk_table_new ();
  nbtk_widget_set_padding (footer, &padding);
  nbtk_widget_set_style_class_name (footer, "drop-down-footer");

  up_button = nbtk_button_new ();
  nbtk_widget_set_style_class_name (up_button, "drop-down-up-button");
  nbtk_table_add_actor (NBTK_TABLE (footer), CLUTTER_ACTOR (up_button), 0, 0);
  clutter_actor_set_size (CLUTTER_ACTOR (up_button), 23, 21);
  clutter_container_child_set (CLUTTER_CONTAINER (footer),
                               CLUTTER_ACTOR (up_button),
                               "keep-aspect-ratio", TRUE,
                               "x-align", 1.0,
                               NULL);
  g_signal_connect_swapped (up_button, "clicked", G_CALLBACK (clutter_actor_hide), table);
  g_signal_connect (grid, "activated", (GCallback)_activated_cb, table);

  /* add all the actors to the group */
  nbtk_table_add_actor (NBTK_TABLE (table), grid, 0, 0);
  nbtk_table_add_widget (NBTK_TABLE (table), footer, 1, 0);

  clutter_actor_set_width (CLUTTER_ACTOR (table), width);

  return CLUTTER_ACTOR (table);
}
