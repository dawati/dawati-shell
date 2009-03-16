#include <dalston/dalston-battery-monitor.h>

static void
_battery_monitor_status_changed_cb (DalstonBatteryMonitor *monitor,
                                    gpointer               userdata)
{
  g_debug (G_STRLOC ": Status changed");
  g_debug (G_STRLOC ": Remaining time: %d. Remaining percentage: %d",
           dalston_battery_monitor_get_time_remaining (monitor),
           dalston_battery_monitor_get_charge_percentage (monitor));
}

int
main (int    argc,
      char **argv)
{
  GMainLoop *loop;
  DalstonBatteryMonitor *monitor;

  g_type_init ();
  loop = g_main_loop_new (NULL, TRUE);

  monitor = g_object_new (DALSTON_TYPE_BATTERY_MONITOR,
                          NULL);

  g_signal_connect (monitor,
                    "status-changed",
                    _battery_monitor_status_changed_cb,
                    NULL);

  g_main_loop_run (loop);
}
