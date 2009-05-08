#include <libhal-power-glib/hal-power-proxy.h>

static void
_power_proxy_shutdown_cb (HalPowerProxy *proxy,
                          const GError  *error,
                          GObject       *weak_object,
                          gpointer       userdata)
{
  if (error)
  {
    g_warning (G_STRLOC ": Error when shutting down: %s",
               error->message);
  }
}

int
main (int    argc,
      char **argv)
{
  GMainLoop *loop;
  HalPowerProxy *proxy;

  g_type_init ();

  loop = g_main_loop_new (NULL, TRUE);
  proxy = hal_power_proxy_new ();
  hal_power_proxy_shutdown (proxy,
                            _power_proxy_shutdown_cb,
                            NULL,
                            NULL);
  g_main_loop_run (loop);
}
