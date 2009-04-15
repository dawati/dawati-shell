#include <anerley/anerley-tp-feed.h>

#include <libmissioncontrol/mission-control.h>
#include <glib.h>
#include <dbus/dbus-glib.h>

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

  g_main_loop_run (main_loop);

  return 0;
}

