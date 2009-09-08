/*
 * Dalston - power and volume applets for Moblin
 * Copyright (C) 2009, Intel Corporation.
 *
 * Authors: Rob Bradford <rob@linux.intel.com>
 * 
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU Lesser General Public License,
 * version 2.1, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
 * more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */


#include "dalston-power-applet.h"

#include <dalston/dalston-battery-monitor.h>
#include <dalston/dalston-brightness-slider.h>
#include <dalston/dalston-brightness-manager.h>
#include <dalston/dalston-button-monitor.h>
#include <libhal-power-glib/hal-power-proxy.h>
#include <gtk/gtk.h>
#include <nbtk/nbtk-gtk.h>
#include <glib/gi18n.h>
#include <libnotify/notify.h>

G_DEFINE_TYPE (DalstonPowerApplet, dalston_power_applet, G_TYPE_OBJECT)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), DALSTON_TYPE_POWER_APPLET, DalstonPowerAppletPrivate))

typedef struct _DalstonPowerAppletPrivate DalstonPowerAppletPrivate;

struct _DalstonPowerAppletPrivate {
  MplPanelClient *panel_client;

  DalstonBatteryMonitor *battery_monitor;
  DalstonBrightnessManager *brightness_manager;

  GtkWidget *main_hbox;
  GtkWidget *brightness_slider;
  GtkWidget *battery_image;
  GtkWidget *battery_primary_label;
  GtkWidget *battery_secondary_label;

  gboolean active;
};

typedef enum {
  BATTERY_ICON_STATE_UNKNOWN,
  BATTERY_ICON_STATE_UNKNOWN_ACTIVE,
  BATTERY_ICON_STATE_CHARGE_0,
  BATTERY_ICON_STATE_CHARGE_0_ACTIVE,
  BATTERY_ICON_STATE_CHARGE_25,
  BATTERY_ICON_STATE_CHARGE_25_ACTIVE,
  BATTERY_ICON_STATE_CHARGE_50,
  BATTERY_ICON_STATE_CHARGE_50_ACTIVE,
  BATTERY_ICON_STATE_CHARGE_75,
  BATTERY_ICON_STATE_CHARGE_75_ACTIVE,
  BATTERY_ICON_STATE_CHARGE_100,
  BATTERY_ICON_STATE_CHARGE_100_ACTIVE,
  BATTERY_ICON_STATE_AC_CONNECTED,
  BATTERY_ICON_STATE_AC_CONNECTED_ACTIVE
} BatteryIconState;


#define PKG_ICON_DIR PKG_DATA_DIR "/" "icons"

static const gchar *icon_names[] = {
  PKG_ICON_DIR "/" "dalston-power-applet-empty-normal.png",
  PKG_ICON_DIR "/" "dalston-power-applet-empty-active.png",
  PKG_ICON_DIR "/" "dalston-power-applet-empty-normal.png",
  PKG_ICON_DIR "/" "dalston-power-applet-empty-active.png",
  PKG_ICON_DIR "/" "dalston-power-applet-25-normal.png",
  PKG_ICON_DIR "/" "dalston-power-applet-25-active.png",
  PKG_ICON_DIR "/" "dalston-power-applet-50-normal.png",
  PKG_ICON_DIR "/" "dalston-power-applet-50-active.png",
  PKG_ICON_DIR "/" "dalston-power-applet-75-normal.png",
  PKG_ICON_DIR "/" "dalston-power-applet-75-active.png",
  PKG_ICON_DIR "/" "dalston-power-applet-full-normal.png",
  PKG_ICON_DIR "/" "dalston-power-applet-full-active.png",
  PKG_ICON_DIR "/" "dalston-power-applet-plugged-normal.png",
  PKG_ICON_DIR "/" "dalston-power-applet-plugged-active.png"
};


#define BATTERY_IMAGE_STATE_MISSING        "dalston-power-battery-missing.png"
#define BATTERY_IMAGE_STATE_CHARGE_0       "dalston-power-empty.png"
#define BATTERY_IMAGE_STATE_CHARGE_25      "dalston-power-25.png"
#define BATTERY_IMAGE_STATE_CHARGE_50      "dalston-power-50.png"
#define BATTERY_IMAGE_STATE_CHARGE_75      "dalston-power-75.png"
#define BATTERY_IMAGE_STATE_CHARGE_100     "dalston-power-full.png"


enum
{
  PROP_0,
  PROP_PANEL_CLIENT
};

static void
dalston_power_applet_get_property (GObject *object, guint property_id,
                              GValue *value, GParamSpec *pspec)
{
  DalstonPowerAppletPrivate *priv = GET_PRIVATE (object);

  switch (property_id) {
    case PROP_PANEL_CLIENT:
      g_value_set_object (value, priv->panel_client);
      break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
dalston_power_applet_set_property (GObject *object, guint property_id,
                              const GValue *value, GParamSpec *pspec)
{
  DalstonPowerAppletPrivate *priv = GET_PRIVATE (object);

  switch (property_id) {
    case PROP_PANEL_CLIENT:
      priv->panel_client = g_value_dup_object (value);
      break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
dalston_power_applet_dispose (GObject *object)
{
  DalstonPowerAppletPrivate *priv = GET_PRIVATE (object);

  if (priv->panel_client)
  {
    g_object_unref (priv->panel_client);
    priv->panel_client = NULL;
  }

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
  GParamSpec *pspec;

  g_type_class_add_private (klass, sizeof (DalstonPowerAppletPrivate));

  object_class->get_property = dalston_power_applet_get_property;
  object_class->set_property = dalston_power_applet_set_property;
  object_class->dispose = dalston_power_applet_dispose;
  object_class->finalize = dalston_power_applet_finalize;

  pspec = g_param_spec_object ("panel-client",
                               "Panel client",
                               "The panel client",
                               MPL_TYPE_PANEL_CLIENT,
                               G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
  g_object_class_install_property (object_class, PROP_PANEL_CLIENT, pspec);
}

static gchar *
dalston_power_applet_format_time_remaining (guint time_remaining)
{
  gchar *hour_string = NULL;
  gchar *minute_string = NULL;
  gchar *res;

  if (time_remaining >= 3600)
  {

    hour_string = g_strdup_printf (ngettext ("<b>%d</b> hour",
                                             "<b>%d</b> hours",
                                             time_remaining / 3600),
                                   time_remaining / 3600);

    if (((time_remaining % 3600) / 60) > 1)
    {
      minute_string = g_strdup_printf (ngettext ("<b>%d</b> minute",
                                                 "<b>%d</b> minutes",
                                                 (time_remaining % 3600) / 60),
                                       (time_remaining % 3600) / 60);

      res = g_strdup_printf ("%s %s", hour_string, minute_string);
      g_free (minute_string);
      g_free (hour_string);
      return res;
    } else {
      return hour_string;
    }
  } else {
    return g_strdup_printf (ngettext ("<b>%d</b> minute", 
                                      "<b>%d</b> minutes",
                                      (time_remaining / 60)),
                            (time_remaining / 60));
  }
}

typedef enum
{
  NOTIFICATION_30_PERCENT,
  NOTIFICATION_15_PERCENT,
  NOTIFICATION_10_PERCENT,
  NOTIFICATION_LAST
} NotificationLevel;

static const struct 
{
  const gchar *title;
  const gchar *message;
  const gchar *icon;
} messages[] = {
  { N_("Running low on battery"), N_("We've noticed that your battery is running a bit low. " \
                                     "If you can it would be a good idea to plug in and top up."), NULL },
  { N_("Getting close to empty"), N_("You're running quite low on battery. It'd be a good idea to save all your work " \
                                     "and plug in as soon as you can"), NULL },
  { N_("Danger!"), N_("Sorry, your computer is about to run out of battery. We're going to have to turn off now. " \
                       "Please save your work and hope to see you again soon."), NULL}
};

static void
dalston_power_applet_do_notification (DalstonPowerApplet *applet,
                                      NotificationLevel   level)
{
  NotifyNotification *note;
  GError *error = NULL;

  note = notify_notification_new (_(messages[level].title),
                                  _(messages[level].message),
                                  _(messages[level].icon),
                                  NULL);

  notify_notification_set_timeout (note, 10000);

  if (level == NOTIFICATION_10_PERCENT)
  {
    notify_notification_set_urgency (note, NOTIFY_URGENCY_CRITICAL);
  }

  if (!notify_notification_show (note,
                                 &error))
  {
    g_warning (G_STRLOC ": Error showing notification: %s",
               error->message);
    g_clear_error (&error);
  }

  g_object_unref (note);
}

static void
dalston_power_applet_do_shutdown (DalstonPowerApplet *applet)
{
  HalPowerProxy *power_proxy;

  power_proxy = hal_power_proxy_new ();
  hal_power_proxy_shutdown (power_proxy,
                            NULL,
                            NULL,
                            NULL);
  g_object_unref (power_proxy);
}

static gboolean
_shutdown_timeout_cb (gpointer userdata)
{
  DalstonPowerApplet *applet = DALSTON_POWER_APPLET (userdata);
  DalstonPowerAppletPrivate *priv = GET_PRIVATE (applet);
  DalstonBatteryMonitorState state;

  state = dalston_battery_monitor_get_state (priv->battery_monitor);

  if (state == DALSTON_BATTERY_MONITOR_STATE_DISCHARGING)
  {
    dalston_power_applet_do_shutdown (applet);
  }

  return FALSE;
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
  static gint last_notification_displayed = -1;
  gchar *description;

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
    mpl_panel_client_request_button_style (priv->panel_client, "state-plugged");
  } else if (percentage < 0) {
    mpl_panel_client_request_button_style (priv->panel_client, "state-missing");
  } else if (percentage < 20) {
    mpl_panel_client_request_button_style (priv->panel_client, "state-empty");
  } else if (percentage >= 20 && percentage < 35) {
    mpl_panel_client_request_button_style (priv->panel_client, "state-25");
  } else if (percentage >= 35 && percentage < 60) {
    mpl_panel_client_request_button_style (priv->panel_client, "state-50");
  } else if (percentage >= 60 && percentage < 90){
    mpl_panel_client_request_button_style (priv->panel_client, "state-75");
  } else {
    mpl_panel_client_request_button_style (priv->panel_client, "state-full");
  }

  if (percentage < 0) {
    gtk_image_set_from_file (GTK_IMAGE(priv->battery_image),
                             PKG_ICON_DIR "/" BATTERY_IMAGE_STATE_MISSING);
  } else if (percentage < 20) {
    gtk_image_set_from_file (GTK_IMAGE(priv->battery_image),
                             PKG_ICON_DIR "/" BATTERY_IMAGE_STATE_CHARGE_0);
  } else if (percentage >= 20 && percentage < 35) {
    gtk_image_set_from_file (GTK_IMAGE(priv->battery_image),
                             PKG_ICON_DIR "/" BATTERY_IMAGE_STATE_CHARGE_25);
  } else if (percentage >= 35 && percentage < 60){
    gtk_image_set_from_file (GTK_IMAGE(priv->battery_image),
                             PKG_ICON_DIR "/" BATTERY_IMAGE_STATE_CHARGE_50);
  } else if (percentage >= 60 && percentage < 90){
    gtk_image_set_from_file (GTK_IMAGE(priv->battery_image),
                             PKG_ICON_DIR "/" BATTERY_IMAGE_STATE_CHARGE_75);
  } else {
    gtk_image_set_from_file (GTK_IMAGE(priv->battery_image),
                             PKG_ICON_DIR "/" BATTERY_IMAGE_STATE_CHARGE_100);
  }

  if (state == DALSTON_BATTERY_MONITOR_STATE_OTHER)
  {
    if (percentage == 0)
    {
      gtk_label_set_markup (GTK_LABEL (priv->battery_primary_label),
                            _("Sorry, it looks like your battery is broken."));
    } else {
      gtk_label_set_markup (GTK_LABEL (priv->battery_primary_label),
                            _("Your battery is fully charged and you're ready to go."));
    }
  } else if (state == DALSTON_BATTERY_MONITOR_STATE_CHARGING) {
    description = g_strdup_printf (_("Your battery is charging. " \
                                     "It is about <b>%d</b>%% full."),
                                   percentage);
    gtk_label_set_markup (GTK_LABEL (priv->battery_primary_label),
                          description);
    g_free (description);
  } else if (state == DALSTON_BATTERY_MONITOR_STATE_DISCHARGING) {
    description = g_strdup_printf (_("Your battery is being used. " \
                                     "It is about <b>%d</b>%% full."),
                                   percentage);
    gtk_label_set_markup (GTK_LABEL (priv->battery_primary_label),
                          description);
    g_free (description);
  } else if (state == DALSTON_BATTERY_MONITOR_STATE_MISSING) {
      gtk_label_set_markup (GTK_LABEL (priv->battery_primary_label),
                            _("Sorry, you don't appear to have a battery "
                              "installed."));
  }

  if (state == DALSTON_BATTERY_MONITOR_STATE_DISCHARGING)
  {
    /* Do notifications at various levels */
    if (percentage > 0 && percentage < 10) {
      if (last_notification_displayed != NOTIFICATION_10_PERCENT)
      {
        dalston_power_applet_do_notification (applet, NOTIFICATION_10_PERCENT);
        last_notification_displayed = NOTIFICATION_10_PERCENT;

        g_timeout_add_seconds (60,
                               _shutdown_timeout_cb,
                               applet);
      }
    } else if (percentage < 15) {
      if (last_notification_displayed != NOTIFICATION_15_PERCENT)
      {
        dalston_power_applet_do_notification (applet, NOTIFICATION_15_PERCENT);
        last_notification_displayed = NOTIFICATION_15_PERCENT;
      }
    } else if (percentage < 20) {
      if (last_notification_displayed != NOTIFICATION_30_PERCENT)
      {
        dalston_power_applet_do_notification (applet, NOTIFICATION_30_PERCENT);
        last_notification_displayed = NOTIFICATION_30_PERCENT;
      }
    } else {
      /* Reset the notification */
      last_notification_displayed = -1;
    }
  }

#if 0
  g_debug (G_STRLOC ": Remaining time: %d. Remaining percentage: %d",
           time_remaining,
           percentage);

  g_debug (G_STRLOC ": State: %s",
           (state==0) ? "unknown" :
           (state==1) ? "charging" :
           (state==2) ? "discharging": "other");

  g_debug (G_STRLOC ": AC adapter: %s", ac_connected ? "yes" : "no");

#endif
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
  GtkWidget *label;
  GtkWidget *vbox;
  GtkWidget *frame;
  GtkWidget *hbox;
  gchar *str;

  /* The battery monitor that will pull in the battery state */
  priv->battery_monitor = g_object_new (DALSTON_TYPE_BATTERY_MONITOR,
                                        NULL);
  g_signal_connect (priv->battery_monitor,
                    "status-changed",
                    (GCallback)_battery_monitor_status_changed_cb,
                    self);

  /* Create the pane hbox */
  priv->main_hbox = gtk_hbox_new (FALSE, 4);
  gtk_container_set_border_width (GTK_CONTAINER (priv->main_hbox), 4);

#if 0
  frame = nbtk_gtk_frame_new ();
  gtk_box_pack_start (GTK_BOX (priv->main_hbox),
                      frame,
                      TRUE,
                      TRUE,
                      0);
  vbox = gtk_vbox_new (FALSE, 8);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 4);
  gtk_container_add (GTK_BIN (frame), vbox);

  str = g_strconcat ("<span font_desc=\"Liberation Sans Bold 18px\" foreground=\"#3e3e3e\">",
                     _("Display brightness"),
                     "</span>",
                     NULL);
  label = gtk_label_new (str);
  g_free (str);

  gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_box_pack_start (GTK_BOX (vbox),
                      label,
                      FALSE,
                      FALSE,
                      0);

  /* Brightness manager. We pass this into the slider when we create it */
  priv->brightness_manager = g_object_new (DALSTON_TYPE_BRIGHTNESS_MANAGER,
                                           NULL);
  hbox = gtk_hbox_new (FALSE, 8);
  gtk_box_pack_start (GTK_BOX (vbox),
                      hbox,
                      TRUE,
                      TRUE,
                      8);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 8);

  if (dalston_brightness_manager_is_controllable (priv->brightness_manager))
  {
    priv->brightness_slider = 
      dalston_brightness_slider_new (priv->brightness_manager);
    gtk_box_pack_start (GTK_BOX (hbox),
                        priv->brightness_slider,
                        TRUE,
                        TRUE,
                        8);
  } else {
    label = gtk_label_new (_("Sorry, we don't support modifying " \
                             "the brightness of your screen"));
    gtk_box_pack_start (GTK_BOX (hbox),
                        label,
                        TRUE,
                        FALSE,
                        8);

  }

#endif
  frame = nbtk_gtk_frame_new ();
  gtk_box_pack_start (GTK_BOX (priv->main_hbox),
                      frame,
                      FALSE,
                      FALSE,
                      0);
  battery_vbox = gtk_vbox_new (FALSE, 8);
  gtk_container_set_border_width (GTK_CONTAINER (battery_vbox), 8);
  gtk_container_add (GTK_BIN (frame), battery_vbox);

  priv->battery_image = gtk_image_new();
#if 0
  gtk_image_set_from_icon_name (priv->battery_image,
                                BATTERY_IMAGE_STATE_UNKNOWN,
                                GTK_ICON_SIZE_INVALID);
  gtk_image_set_pixel_size (priv->battery_image,
                            120);
#endif

  /* fix the size of the battery vbox */
  gtk_widget_set_size_request (battery_vbox, 240, -1);
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
  gtk_label_set_line_wrap (priv->battery_primary_label, TRUE);
  gtk_widget_set_size_request (priv->battery_primary_label, 220, -1);

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
dalston_power_applet_new (MplPanelClient *panel_client)
{
  return g_object_new (DALSTON_TYPE_POWER_APPLET,
                       "panel-client",
                       panel_client,
                       NULL);
}

GtkWidget *
dalston_power_applet_get_pane (DalstonPowerApplet *applet)
{
  DalstonPowerAppletPrivate *priv = GET_PRIVATE (applet);

  return priv->main_hbox;
}

void
dalston_power_applet_set_active (DalstonPowerApplet *applet,
                                 gboolean            active)
{
  DalstonPowerAppletPrivate *priv = GET_PRIVATE (applet);

  priv->active = active;

#if 0
  if (active)
  {
    /* TODO: Update the icon to be in the active state */
    dalston_brightness_manager_start_monitoring (priv->brightness_manager);
  } else {
    dalston_brightness_manager_stop_monitoring (priv->brightness_manager);
  }
#endif

  dalston_power_applet_update_battery_state (applet);
}
