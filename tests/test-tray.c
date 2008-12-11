/*
 * Simple system tray test.
 */

#include "../src/moblin-netbook-system-tray.h"

int
main (int argc, char *argv[])
{
  GtkStatusIcon *icon;

  gtk_init (&argc, &argv);

  icon = gtk_status_icon_new_from_stock (GTK_STOCK_INFO);

  gdk_add_client_message_filter (gdk_atom_intern (MOBLIN_SYSTEM_TRAY_EVENT,
						  FALSE),
				 mnbtk_client_message_handler, icon);

  gtk_status_icon_set_visible (icon, TRUE);

  gtk_main ();

  return 0;
}
