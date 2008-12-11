
/*
 * Simple system tray test.
 */

#include <gtk/gtk.h>
#include <gdk/gdkx.h>

typedef struct
{
  guint32 type;
  guint16 button;
  guint16 count;
  gint16  x;
  gint16  y;
  guint32 time;
  guint32 modifiers;
} TrayEventData;

static GdkFilterReturn
client_message_handler (GdkXEvent *xevent, GdkEvent *event, gpointer data)
{
  GtkStatusIcon       *icon = GTK_STATUS_ICON (data);
  XClientMessageEvent *xev = (XClientMessageEvent *) xevent;
  TrayEventData       *td  = (TrayEventData*)&xev->data;

  g_debug ("Got click: button %d\n"
	   "             type %d\n"
	   "            count %d\n"
	   "                x %d\n"
	   "                y %d\n"
	   "        modifiers 0x%x\n\n",
	   td->button,
	   td->type,
	   td->count,
	   td->x,
	   td->y,
	   td->modifiers);

  if (td->type == GDK_BUTTON_PRESS)
    {
      switch (td->button)
	{
	case 1:
	  g_signal_emit_by_name (icon, "activate", 0);
	  break;
	case 3:
	  g_signal_emit_by_name (icon, "popup-menu", 0, td->button, td->time);
	  break;
	default:
	  break;
	}
    }
}

int
main (int argc, char *argv[])
{
  GtkStatusIcon *icon;

  gtk_init (&argc, &argv);

  icon = gtk_status_icon_new_from_stock (GTK_STOCK_INFO);

  gdk_add_client_message_filter (gdk_atom_intern ("MOBLIN_SYSTEM_TRAY_EVENT",
						  FALSE),
				 client_message_handler, icon);

  gtk_status_icon_set_visible (icon, TRUE);

  gtk_main ();

  return 0;
}
