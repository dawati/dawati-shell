#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <gconf/gconf-client.h>
#include <config.h>

#define MOBLIN_GCONF_DIR "/desktop/moblin"
#define IDLE_TIME_KEY "/desktop/moblin/suspend_idle_time"

enum
{
  TEN_MINUTES,
  TWENTY_MINUTES,
  THIRTY_MINUTES,
  FORTY_MINUTES,
  ONE_HOUR,
  NEVER,
  LAST_RANGE
};

static const gchar *slider_labels[LAST_RANGE] = { 
  N_("Ten minutes"),
  N_("Twenty minutes"),
  N_("Half an hour"),
  N_("40 minutes"),
  N_("An hour"),
  N_("Never")
};

static gchar *
_slider_format_value_cb (GtkScale *scale,
                         gdouble   value,
                         gpointer  userdata)
{
  return g_strdup (slider_labels[(int)value]);
}

static void
_idle_time_key_changed_cb (GConfClient *client,
                           guint        cnxn_id,
                           GConfEntry  *entry,
                           gpointer     userdata)
{
  GtkWidget *slider = GTK_WIDGET (userdata);
  gint idle_time;
  GError *error = NULL;
  gdouble slider_value;

  idle_time = gconf_client_get_int (client,
                                    IDLE_TIME_KEY,
                                    &error);

  if (error)
  {
    g_warning (G_STRLOC ": Error getting gconf key: %s",
               error->message);
    g_clear_error (&error);
    idle_time = -1;
  }

  if (idle_time == -1)
  {
    slider_value = (double)NEVER;
  } else if (idle_time == 10) {
    slider_value = (double)TEN_MINUTES;
  } else if (idle_time == 20) {
    slider_value = (double)TWENTY_MINUTES;
  } else if (idle_time == 30) {
    slider_value = (double)THIRTY_MINUTES;
  } else if (idle_time == 40) {
    slider_value = (double)FORTY_MINUTES;
  } else if (idle_time == 60) {
    slider_value = (double)ONE_HOUR;
  } else {
    slider_value = (double)NEVER;
  }

  gtk_range_set_value (GTK_RANGE (slider), slider_value);
}

gint
main (gint    argc,
      gchar **argv)
{
  GtkWidget *main_window;
  GtkWidget *main_frame;
  GtkWidget *label;
  GtkWidget *hbox;
  GtkWidget *slider;
  GtkWidget *inner_vbox;
  GtkWidget *vbox;
  GConfClient *client;
  guint notify_id;
  GError *error = NULL;

  g_set_application_name (_("Power Management Settings"));
  gtk_init (&argc, &argv);

  client = gconf_client_get_default ();

  main_window = gtk_dialog_new_with_buttons (_("Power Management Preferences"),
                                             NULL,
                                             GTK_DIALOG_MODAL,
                                             GTK_STOCK_CLOSE,
                                             GTK_RESPONSE_CLOSE,
                                             NULL);
  main_frame = gtk_frame_new (_("<b>Power Settings</b>"));
  gtk_frame_set_shadow_type (GTK_FRAME (main_frame),
                             GTK_SHADOW_NONE);
  label = gtk_frame_get_label_widget (GTK_FRAME (main_frame));
  gtk_label_set_use_markup (GTK_LABEL (label), TRUE);

  vbox = gtk_dialog_get_content_area (GTK_DIALOG (main_window));
  gtk_container_add (GTK_CONTAINER (vbox),
                     main_frame);

  hbox = gtk_hbox_new (FALSE, 6);
  label = gtk_label_new (_("Put computer to sleep when inactive for:"));
  gtk_box_pack_start (GTK_BOX (hbox),
                      label,
                      FALSE,
                      FALSE,
                      6);
  slider = gtk_hscale_new_with_range (0.0, 5.0, 1.0);
  g_signal_connect (slider,
                    "format-value",
                    (GCallback)_slider_format_value_cb,
                    NULL);
  gtk_scale_set_value_pos (GTK_SCALE (slider),
                           GTK_POS_BOTTOM);

  gtk_box_pack_start (GTK_BOX (hbox),
                      slider,
                      TRUE,
                      TRUE,
                      6);

  inner_vbox = gtk_vbox_new (FALSE, 6);
  gtk_box_pack_start (GTK_BOX (inner_vbox),
                      hbox,
                      FALSE,
                      FALSE,
                      6);

  gtk_container_add (GTK_CONTAINER (main_frame),
                     inner_vbox);

  gtk_container_set_border_width (GTK_CONTAINER (main_frame),
                                  12);
  gtk_container_set_border_width (GTK_CONTAINER (inner_vbox),
                                  12);
  gtk_widget_show_all (main_frame);
  gtk_widget_set_size_request (main_window, 600, -1);

  gconf_client_add_dir (client,
                        MOBLIN_GCONF_DIR,
                        GCONF_CLIENT_PRELOAD_NONE,
                        &error);

  if (error)
  {
    g_warning (G_STRLOC ": Error adding gconf directory: %s",
               error->message);
    g_clear_error (&error);
  }

  notify_id = gconf_client_notify_add (client,
                                       IDLE_TIME_KEY,
                                       _idle_time_key_changed_cb,
                                       slider,
                                       NULL,
                                       &error);

  if (error)
  {
    g_warning (G_STRLOC ": Error setting up gconf key notification: %s",
               error->message);
    g_clear_error (&error);
  }

  gconf_client_notify (client, IDLE_TIME_KEY);

  gtk_dialog_run (GTK_DIALOG (main_window));

  gconf_client_notify_remove (client, notify_id);
  g_object_unref (client);
}
