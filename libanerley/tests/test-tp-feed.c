#include <anerley/anerley-tp-feed.h>
#include <anerley/anerley-item.h>

#include <libmissioncontrol/mission-control.h>
#include <telepathy-glib/contact.h>

#include <glib.h>
#include <dbus/dbus-glib.h>

static void
_feed_items_added_cb (AnerleyTpFeed *feed,
                      GList         *added_items,
                      gpointer       userdata)
{
  GList *l;
  AnerleyItem *item;
  TpContact *contact;

  for (l = added_items; l; l = l->next)
  {
    item = (AnerleyItem *)l->data;
    g_object_get (item,
                  "contact",
                  &contact,
                  NULL);

    g_debug (G_STRLOC ": Added identifier: %s",
             tp_contact_get_identifier (contact));
  }
}

static void
_feed_items_removed_cb (AnerleyTpFeed *feed,
                        GList         *removed_items,
                        gpointer       userdata)
{
  GList *l;
  AnerleyItem *item;
  TpContact *contact;

  for (l = removed_items; l; l = l->next)
  {
    item = (AnerleyItem *)l->data;
    g_object_get (item,
                  "contact",
                  &contact,
                  NULL);

    g_debug (G_STRLOC ": Removed identifier: %s",
             tp_contact_get_identifier (contact));
  }
}


int
main (int    argc,
      char **argv)
{
  GMainLoop *main_loop;
  MissionControl *mc;
  McAccount *account;
  AnerleyTpFeed *feed;
  DBusGConnection *conn;

  if (argc < 2)
  {
    g_print ("Usage: ./test-tp-feed <account-name>\n");
    return 1;
  }

  g_type_init ();

  main_loop = g_main_loop_new (NULL, FALSE);
  conn = dbus_g_bus_get (DBUS_BUS_SESSION, NULL);
  mc = mission_control_new (conn);
  account = mc_account_lookup (argv[1]);
  feed = anerley_tp_feed_new (mc, account);
  g_signal_connect (feed,
                    "items-added",
                    _feed_items_added_cb,
                    NULL);
  g_signal_connect (feed,
                    "items-removed",
                    _feed_items_removed_cb,
                    NULL);

  g_main_loop_run (main_loop);

  return 0;
}

