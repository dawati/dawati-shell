
/*
 * Simple system tray test.
 */

#include <gtk/gtk.h>
#include <gdk/gdkx.h>

static GdkFilterReturn
client_message_handler (GdkXEvent *xevent, GdkEvent *event, gpointer data)
{
  XClientMessageEvent *xev = (XClientMessageEvent *) xevent;

  g_print("Got click: button %d\n"
	  "             type %d\n"
	  "            count %d\n"
	  "                x %d\n"
	  "                y %d\n"
	  "        modifiers 0x%x\n\n",
	  xev->data.l[1] & 0xffff,
	  xev->data.l[0],
	  (xev->data.l[1] & 0xffff0000) >> 16,
	  xev->data.l[2],
	  xev->data.l[3],
	  xev->data.l[4]);
}

int
main (int argc, char *argv[])
{
  GtkStatusIcon *icon;

  gtk_init (&argc, &argv);

  icon = gtk_status_icon_new_from_stock (GTK_STOCK_INFO);

  gdk_add_client_message_filter (gdk_atom_intern ("MOBLIN_SYSTEM_TRAY_EVENT",
						  FALSE),
				 client_message_handler, NULL);

  gtk_status_icon_set_visible (icon, TRUE);

  gtk_main ();

  return 0;
}
