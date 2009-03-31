#include "dalston-power-applet.h"

#include <dalston/dalston-battery-monitor.h>
#include <dalston/dalston-brightness-slider.h>
#include <dalston/dalston-brightness-manager.h>
#include <gtk/gtk.h>

#include <glib/gi18n.h>

G_DEFINE_TYPE (DalstonPowerApplet, dalston_power_applet, G_TYPE_OBJECT)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), DALSTON_TYPE_POWER_APPLET, DalstonPowerAppletPrivate))

typedef struct _DalstonPowerAppletPrivate DalstonPowerAppletPrivate;

struct _DalstonPowerAppletPrivate {
  GtkStatusIcon *status_icon;
  DalstonBatteryMonitor *battery_monitor;
  DalstonBrightnessManager *brightness_manager;

  GtkWidget *main_hbox;
  GtkWidget *brightness_slider;
  GtkWidget *battery_image;
  GtkWidget *battery_primary_label;
  GtkWidget *battery_secondary_label;
};

#define BATTERY_ICON_STATE_UNKNOWN        "dalston-power-applet-empty-normal.png"
#define BATTERY_ICON_STATE_CHARGE_0       "dalston-power-applet-empty-normal.png"
#define BATTERY_ICON_STATE_CHARGE_25      "dalston-power-applet-25-normal.png"
#define BATTERY_ICON_STATE_CHARGE_50      "dalston-power-applet-50-normal.png"
#define BATTERY_ICON_STATE_CHARGE_75      "dalston-power-applet-75-normal.png"
#define BATTERY_ICON_STATE_CHARGE_100     "dalston-power-applet-full-normal.png"
#define BATTERY_ICON_STATE_AC_CONNECTED   "dalston-power-applet-plugged-normal.png"

#define BATTERY_IMAGE_STATE_UNKNOWN        "dalston-power-applet-empty.svg"
#define BATTERY_IMAGE_STATE_CHARGE_0       "dalston-power-applet-empty.svg"
#define BATTERY_IMAGE_STATE_CHARGE_25      "dalston-power-applet-25.svg"
#define BATTERY_IMAGE_STATE_CHARGE_50      "dalston-power-applet-50.svg"
#define BATTERY_IMAGE_STATE_CHARGE_75      "dalston-power-75.svg"
#define BATTERY_IMAGE_STATE_CHARGE_100     "dalston-power-full.svg"

#define PKG_ICON_DIR PKG_DATA_DIR "/" "icons"

static void
dalston_power_applet_get_property (GObject *object, guint property_id,
                              GValue *value, GParamSpec *pspec)
{
  switch (property_id) {
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
dalston_power_applet_set_property (GObject *object, guint property_id,
                              const GValue *value, GParamSpec *pspec)
{
  switch (property_id) {
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
dalston_power_applet_dispose (GObject *object)
{
  G_OBJECT_CLASS (dalston_power_applet_parent_class)->dispose (object);
}

static void
dalston_power_applet_finalize (GObject *object)
{
  G_OBJECT_CLASS (dalston_power_applet_parent_class)->finalize (object);
}

static void
dalston_power_applet_class_init (DalstonPowerAppletClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (DalstonPowerAppletPrivate));

  object_class->get_property = dalston_power_applet_get_property;
  object_class->set_property = dalston_power_applet_set_property;
  object_class->dispose = dalston_power_applet_dispose;
  object_class->finalize = dalston_power_applet_finalize;
}

static gchar *
dalston_power_applet_format_time_remaining (guint time_remaining)
{
  if (time_remaining >= 3600)
  {
    /* x hours y minutes */
    return g_strdup_printf (_("<b>%d</b> hours <b>%d</b> minutes"),
                            time_remaining / 3600,
                            (time_remaining % 3600) / 60);
  } else {
    return g_strdup_printf (_("<b>%d</b> minutes"),
                            (time_remaining / 60));
  }
}

static void
dalston_power_applet_update_battery_state (DalstonPowerApplet *applet)
{
  DalstonPowerAppletPrivate *priv = GET_PRIVATE (applet);
  gint time_remaining;
  gint percentage;
  DalstonBatteryMonitorState state;
  gboolean ac_connected = FALSE;
  gchar *label_text;

  time_remaining =
    dalston_battery_monitor_get_time_remaining (priv->battery_monitor);
  percentage =
    dalston_battery_monitor_get_charge_percentage (priv->battery_monitor);
  state =
    dalston_battery_monitor_get_state (priv->battery_monitor);
  ac_connected =
    dalston_battery_monitor_get_ac_connected (priv->battery_monitor);

  if (ac_connected)
  {
    gtk_status_icon_set_from_file (priv->status_icon,
                                   PKG_ICON_DIR "/" BATTERY_ICON_STATE_AC_CONNECTED);
  } else if (percentage < 0) {
    gtk_status_icon_set_from_file (priv->status_icon,
                                   PKG_ICON_DIR "/" BATTERY_ICON_STATE_UNKNOWN);
  } else if (percentage < 25) {
    gtk_status_icon_set_from_file (priv->status_icon,
                                   PKG_ICON_DIR "/" BATTERY_ICON_STATE_CHARGE_0);

  } else if (percentage >= 25 && percentage < 50) {
    gtk_status_icon_set_from_file (priv->status_icon,
                                   PKG_ICON_DIR "/" BATTERY_ICON_STATE_CHARGE_25);

  } else if (percentage >= 50 && percentage < 75){
    gtk_status_icon_set_from_file (priv->status_icon,
                                   PKG_ICON_DIR "/" BATTERY_ICON_STATE_CHARGE_50);

  } else if (percentage >= 75 && percentage < 100){
    gtk_status_icon_set_from_file (priv->status_icon,
                                   PKG_ICON_DIR "/" BATTERY_ICON_STATE_CHARGE_75);
  } else {
    gtk_status_icon_set_from_file (priv->status_icon,
                                   PKG_ICON_DIR "/" BATTERY_ICON_STATE_CHARGE_100);
  }

  if (percentage < 0) {
    gtk_image_set_from_file (GTK_IMAGE(priv->battery_image),
                             PKG_ICON_DIR "/" BATTERY_IMAGE_STATE_UNKNOWN);
  } else if (percentage < 25) {
    gtk_image_set_from_file (GTK_IMAGE(priv->battery_image),
                             PKG_ICON_DIR "/" BATTERY_IMAGE_STATE_CHARGE_0);
  } else if (percentage >= 25 && percentage < 50) {
    gtk_image_set_from_file (GTK_IMAGE(priv->battery_image),
                             PKG_ICON_DIR "/" BATTERY_IMAGE_STATE_CHARGE_25);
  } else if (percentage >= 50 && percentage < 75){
    gtk_image_set_from_file (GTK_IMAGE(priv->battery_image),
                             PKG_ICON_DIR "/" BATTERY_IMAGE_STATE_CHARGE_50);
  } else if (percentage >= 75 && percentage < 100){
    gtk_image_set_from_file (GTK_IMAGE(priv->battery_image),
                             PKG_ICON_DIR "/" BATTERY_IMAGE_STATE_CHARGE_75);
  } else {
    gtk_image_set_from_file (GTK_IMAGE(priv->battery_image),
                             PKG_ICON_DIR "/" BATTERY_IMAGE_STATE_CHARGE_100);
  }

  if (time_remaining== 0 && state == DALSTON_BATTERY_MONITOR_STATE_OTHER)
  {
    gtk_label_set_markup (GTK_LABEL (priv->battery_primary_label), _("Fully charged"));
  } else {
    label_text = dalston_power_applet_format_time_remaining (time_remaining);
    gtk_label_set_markup (GTK_LABEL (priv->battery_primary_label), label_text);
    g_free (label_text);

    if (state == DALSTON_BATTERY_MONITOR_STATE_DISCHARGING)
    {
      gtk_label_set_markup (GTK_LABEL (priv->battery_secondary_label), 
                            _("of battery power remain at current usage"));
    } else if (state == DALSTON_BATTERY_MONITOR_STATE_CHARGING) {
      gtk_label_set_markup (GTK_LABEL (priv->battery_secondary_label), 
                            _("until battery is charged"));
    }
  }

  g_debug (G_STRLOC ": Remaining time: %d. Remaining percentage: %d",
           time_remaining,
           percentage);

  g_debug (G_STRLOC ": State: %s",
           (state==0) ? "unknown" :
           (state==1) ? "charging" :
           (state==2) ? "discharging": "other");

  g_debug (G_STRLOC ": AC adapter: %s", ac_connected ? "yes" : "no");
}

static void
_battery_monitor_status_changed_cb (DalstonBatteryMonitor *monitor,
                                    gpointer               userdata)
{
  g_debug (G_STRLOC ": Status changed");

  dalston_power_applet_update_battery_state ((DalstonPowerApplet *)userdata);
}

static gboolean
_update_on_init_idle_cb (gpointer userdata)
{
  dalston_power_applet_update_battery_state ((DalstonPowerApplet *)userdata);

  return FALSE;
}

static void
dalston_power_applet_init (DalstonPowerApplet *self)
{
  DalstonPowerAppletPrivate *priv = GET_PRIVATE (self);
  GtkWidget *battery_vbox;

  /* The battery monitor that will pull in the battery state */
  priv->battery_monitor = g_object_new (DALSTON_TYPE_BATTERY_MONITOR,
                                        NULL);
  g_signal_connect (priv->battery_monitor,
                    "status-changed",
                    _battery_monitor_status_changed_cb,
                    self);

  /* Brightness manager. We pass this into the slider when we create it */
  priv->brightness_manager = g_object_new (DALSTON_TYPE_BRIGHTNESS_MANAGER,
                                           NULL);

  /* The status icon that will go into the toolbar */
  priv->status_icon = gtk_status_icon_new ();

#if 0
  gtk_status_icon_set_from_icon_name (priv->status_icon,
                                      BATTERY_ICON_STATE_UNKNOWN);
#endif

  /* Create the pane hbox */
  priv->main_hbox = gtk_hbox_new (FALSE, 8);
  priv->brightness_slider = 
    dalston_brightness_slider_new (priv->brightness_manager);
  gtk_box_pack_start (GTK_BOX (priv->main_hbox),
                      priv->brightness_slider,
                      TRUE,
                      TRUE,
                      8);
  battery_vbox = gtk_vbox_new (FALSE, 8);
  gtk_box_pack_start (GTK_BOX (priv->main_hbox),
                      battery_vbox,
                      FALSE,
                      FALSE,
                      8);
  priv->battery_image = gtk_image_new();
#if 0
  gtk_image_set_from_icon_name (priv->battery_image,
                                BATTERY_IMAGE_STATE_UNKNOWN,
                                GTK_ICON_SIZE_INVALID);
  gtk_image_set_pixel_size (priv->battery_image,
                            120);
#endif

  gtk_box_pack_start (GTK_BOX (battery_vbox),
                      priv->battery_image,
                      TRUE,
                      FALSE,
                      8);
  priv->battery_primary_label = gtk_label_new ("");
  gtk_box_pack_start (GTK_BOX (battery_vbox),
                      priv->battery_primary_label,
                      TRUE,
                      FALSE,
                      8);
  priv->battery_secondary_label = gtk_label_new ("");
  gtk_box_pack_start (GTK_BOX (battery_vbox),
                      priv->battery_secondary_label,
                      TRUE,
                      FALSE,
                      8);
  gtk_widget_show_all (priv->main_hbox);

  /* Do an idle update of the UI */
  g_idle_add (_update_on_init_idle_cb, self);
}

DalstonPowerApplet *
dalston_power_applet_new (void)
{
  return g_object_new (DALSTON_TYPE_POWER_APPLET, NULL);
}

GtkStatusIcon *
dalston_power_applet_get_status_icon (DalstonPowerApplet *applet)
{
  DalstonPowerAppletPrivate *priv = GET_PRIVATE (applet);

  return priv->status_icon;
}

GtkWidget *
dalston_power_applet_get_pane (DalstonPowerApplet *applet)
{
  DalstonPowerAppletPrivate *priv = GET_PRIVATE (applet);

  return priv->main_hbox;
}
