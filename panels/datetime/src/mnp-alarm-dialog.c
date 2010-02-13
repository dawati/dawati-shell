/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */

/*
 * Copyright (C) 2010 Intel Corporation.
 *
 * Author: Srinivasa Ragavan <srini@linux.intel.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */


#include "mnp-alarm-dialog.h"
#include <glib/gi18n.h>
#include <gconf/gconf-client.h>

G_DEFINE_TYPE (MnpAlarmDialog, mnp_alarm_dialog, MX_TYPE_BOX_LAYOUT)

#define ALARM_DIALOG_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MNP_TYPE_ALARM_DIALOG, MnpAlarmDialogPrivate))

typedef struct _MnpAlarmDialogPrivate MnpAlarmDialogPrivate;

struct _MnpAlarmDialogPrivate
{
  ClutterActor *close_button;

  ClutterActor *on_off;

  ClutterActor *hour;
  ClutterActor *h_spin_up, *h_spin_down;

  ClutterActor *minute;
  ClutterActor *m_spin_up, *m_spin_down;
  ClutterActor *am_pm;

  ClutterActor *recur;
  ClutterActor *snooze;
  ClutterActor *sound;
  ClutterActor *delete;
  
  gint id;
};

struct _sound_table {
  gint id;
  const char *name;
} alarm_sound_table [] = {
  { MNP_SOUND_OFF, N_("OFF") },
  { MNP_SOUND_BEEP, N_("Beep") },
  { MNP_SOUND_MUSIC, N_("Music") },
  { MNP_SOUND_MESSAGE, N_("Message") }
};

struct _recur_table {
  gint id;
  const char *name;
} alarm_recur_table [] = {
  { MNP_ALARM_NEVER, N_("Never") },
  { MNP_ALARM_EVERYDAY, N_("Everyday") },
  { MNP_ALARM_WORKWEEK, N_("Monday - Friday") },
  { MNP_ALARM_MONDAY, N_("Monday") },
  { MNP_ALARM_TUESDAY, N_("Tuesday") },
  { MNP_ALARM_WEDNESDAY, N_("Wednesday") },
  { MNP_ALARM_THURSDAY, N_("Thursday") },
  { MNP_ALARM_FRIDAY, N_("Friday") },
  { MNP_ALARM_SATURDAY, N_("Satueday") },
  { MNP_ALARM_SUNDAY, N_("Sunday") }
};

static void
mnp_alarm_dialog_dispose (GObject *object)
{
  G_OBJECT_CLASS (mnp_alarm_dialog_parent_class)->dispose (object);
}

static void
mnp_alarm_dialog_finalize (GObject *object)
{
  G_OBJECT_CLASS (mnp_alarm_dialog_parent_class)->finalize (object);
}

static void
mnp_alarm_dialog_class_init (MnpAlarmDialogClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MnpAlarmDialogPrivate));

  object_class->dispose = mnp_alarm_dialog_dispose;
  object_class->finalize = mnp_alarm_dialog_finalize;
}

static void
mnp_alarm_dialog_init (MnpAlarmDialog *self)
{
}

static void
update_conf(MnpAlarmDialog *dialog)
{
  MnpAlarmDialogPrivate *priv = ALARM_DIALOG_PRIVATE(dialog);
  GConfClient *client = gconf_client_get_default();
  GSList *list = NULL, *tmp, *new_node = NULL;
  char *newdata;

  list = gconf_client_get_list (client,"/apps/date-time-panel/alarms", GCONF_VALUE_STRING, NULL);
  tmp = list;
  while(tmp) {
	char *data = (char *)tmp->data;
	int id, on_off, hour, min, am_pm, recur, snooze, sound;

	sscanf(data, "%d %d %d %d %d %d %d %d", &id, &on_off, &hour, &min, &am_pm, &recur, &snooze, &sound);

	if (id == priv->id) {
		new_node = tmp;
		break;
	}
		
  	tmp = tmp->next;
  }

  newdata = g_strdup_printf("%d %d %d %d %d %d %d %d\n", priv->id,
		   mx_button_get_checked(priv->on_off),
		   mx_spin_entry_get_value(priv->hour),
		   mx_spin_entry_get_value (priv->minute),
		   mx_toggle_get_active (priv->am_pm),
		   mx_combo_box_get_index (priv->recur),
		   mx_toggle_get_active(priv->snooze),
		   mx_combo_box_get_index(priv->sound));

  if (new_node) {
	g_free(new_node->data);
	new_node->data = newdata;
  } else {
	list = g_slist_append(list, newdata);
  } 

  gconf_client_set_list(client,"/apps/date-time-panel/alarms", GCONF_VALUE_STRING, list, NULL);

  g_slist_foreach(list, (GFunc)g_free, NULL);
  g_slist_free(list);

  g_object_unref (client);
}

static void
close_dialog (MxButton *btn, MnpAlarmDialog *dialog)
{
  MnpAlarmDialogPrivate *priv = ALARM_DIALOG_PRIVATE(dialog);

  update_conf(dialog);

  clutter_actor_destroy(dialog);
}

static void
construct_title_header (MnpAlarmDialog *dialog)
{
  MnpAlarmDialogPrivate *priv = ALARM_DIALOG_PRIVATE(dialog);
  ClutterActor *box = mx_box_layout_new ();
  ClutterActor *txt, *icon;

  mx_box_layout_set_pack_start ((MxBoxLayout *)box, FALSE);
  mx_box_layout_set_vertical ((MxBoxLayout *)box, FALSE);

  txt = mx_label_new (_("Alarm settings"));
  clutter_container_add_actor ((ClutterContainer *)box, txt);
  clutter_container_child_set ((ClutterContainer *)box, txt,
                                   "x-fill", TRUE,
                                   "y-fill", TRUE,
				   "expand", TRUE,
                                   NULL);

  priv->close_button = mx_button_new ();
  g_signal_connect(priv->close_button, "clicked", G_CALLBACK(close_dialog), dialog);
  mx_stylable_set_style_class (MX_STYLABLE (priv->close_button),
                               			"AlarmRemoveButton");
  icon = (ClutterActor *)mx_icon_new ();
  mx_stylable_set_style_class (MX_STYLABLE (icon),
                               	"AlarmDelIcon");
  mx_bin_set_child (MX_BIN (priv->close_button),
                      	(ClutterActor *)icon);
  
  clutter_container_add_actor ((ClutterContainer *)box, priv->close_button);
  clutter_container_child_set ((ClutterContainer *)box, priv->close_button,
                                   "x-fill", FALSE,
                                   "y-fill", TRUE,
                                   NULL);

  clutter_container_add_actor ((ClutterContainer *)dialog, box);
  clutter_container_child_set ((ClutterContainer *)dialog, box,
				   "expand", FALSE,
				   "x-fill", TRUE,
                                   NULL);

}

static void
construct_on_off_toggle (MnpAlarmDialog *dialog)
{
  MnpAlarmDialogPrivate *priv = ALARM_DIALOG_PRIVATE(dialog);
  ClutterActor *box = mx_box_layout_new ();
  ClutterActor *txt, *icon;

  mx_box_layout_set_pack_start ((MxBoxLayout *)box, FALSE);
  mx_box_layout_set_vertical ((MxBoxLayout *)box, FALSE);

  icon = (ClutterActor *)mx_icon_new ();
  mx_stylable_set_style_class (MX_STYLABLE (icon),
                               	"AlarmIcon");
  clutter_actor_set_size (icon, 16, 16);
  clutter_container_add_actor ((ClutterContainer *)box, icon);
  clutter_container_child_set ((ClutterContainer *)box, icon,
                                   "x-fill", FALSE,
                                   "y-fill", TRUE,
				   "expand", FALSE,
                                   NULL);
  
 
  txt = mx_label_new (_("ON"));
  clutter_container_add_actor ((ClutterContainer *)box, txt);
  clutter_container_child_set ((ClutterContainer *)box, txt,
                                   "x-fill", FALSE,
                                   "y-fill", TRUE,
				   "expand", TRUE,
                                   NULL);

  priv->on_off = mx_button_new ();
  mx_button_set_toggle_mode (MX_BUTTON (priv->on_off), TRUE);
  mx_bin_set_child (MX_BIN (priv->on_off),
                      	(ClutterActor *)box);
  mx_bin_set_fill (MX_BIN(priv->on_off), TRUE, TRUE);

  clutter_container_add_actor ((ClutterContainer *)dialog, priv->on_off);
  clutter_container_child_set ((ClutterContainer *)dialog, priv->on_off,
				   "expand", FALSE,
				   "x-fill", TRUE,
                                   NULL);
  
}

static void
construct_time_entry (MnpAlarmDialog *dialog)
{
  MnpAlarmDialogPrivate *priv = ALARM_DIALOG_PRIVATE(dialog);
  ClutterActor *box;

  box = mx_box_layout_new ();
  mx_box_layout_set_pack_start ((MxBoxLayout *)box, FALSE);
  mx_box_layout_set_vertical ((MxBoxLayout *)box, FALSE);
  mx_box_layout_set_spacing (box, 4);

  priv->hour = mx_spin_entry_new ();
  mx_spin_entry_set_cycle(priv->hour);
  mx_spin_entry_set_range (priv->hour, 1, 12);
  mx_spin_entry_set_value (priv->hour, 12);
  priv->minute = mx_spin_entry_new ();
  mx_spin_entry_set_cycle(priv->minute);
  mx_spin_entry_set_range (priv->minute, 0, 59);
  mx_spin_entry_set_value (priv->minute, 0);
  priv->am_pm = mx_toggle_new ();
  mx_toggle_set_active (priv->am_pm, TRUE);
  mx_stylable_set_style_class (MX_STYLABLE (priv->am_pm),
                               	"AmPmToggle");
 
  clutter_container_add_actor (box, priv->hour);
  clutter_container_child_set ((ClutterContainer *)box, priv->hour,
                                   "x-fill", FALSE,
                                   "y-fill", TRUE,
				   "expand", FALSE,
                                   NULL);

  
  clutter_container_add_actor (box, priv->minute);
  clutter_container_child_set ((ClutterContainer *)box, priv->minute,
                                   "x-fill", FALSE,
                                   "y-fill", TRUE,
				   "expand", FALSE,
                                   NULL);


  
  clutter_container_add_actor (box, priv->am_pm);
  clutter_container_child_set ((ClutterContainer *)box, priv->am_pm,
                                   "x-fill", FALSE,
                                   "y-fill", FALSE,
				   "expand", FALSE,
                                   NULL);

  
  clutter_container_add_actor ((ClutterContainer *)dialog, box);
  clutter_container_child_set ((ClutterContainer *)dialog, box,
				   "expand", FALSE,
				   "x-fill", TRUE,
                                   NULL);


}

static void
construct_recur_snooze_entry (MnpAlarmDialog *dialog)
{
  MnpAlarmDialogPrivate *priv = ALARM_DIALOG_PRIVATE(dialog);
  int i;
  ClutterActor *box, *label;

  priv->recur = mx_combo_box_new ();

  for (i=0; i< G_N_ELEMENTS(alarm_recur_table); i++) {
 	mx_combo_box_append_text(priv->recur, alarm_recur_table[i].name);
  }

  mx_combo_box_set_index (priv->recur, MNP_ALARM_NEVER);
  label = mx_label_new (_("Repeat"));

  box = mx_box_layout_new ();
  mx_box_layout_set_pack_start ((MxBoxLayout *)box, FALSE);
  mx_box_layout_set_vertical ((MxBoxLayout *)box, FALSE);
  mx_box_layout_set_spacing (box, 4);

  clutter_container_add_actor (box, label);
  clutter_container_child_set ((ClutterContainer *)box, label,
                                   "x-fill", FALSE,
                                   "y-fill", TRUE,
				   "expand", FALSE,
                                   NULL);
  clutter_container_add_actor (box, priv->recur);
  clutter_container_child_set ((ClutterContainer *)box, priv->recur,
                                   "x-fill", FALSE,
                                   "y-fill", TRUE,
				   "expand", FALSE,
                                   NULL);
  

  clutter_container_add_actor ((ClutterContainer *)dialog, box);
  clutter_container_child_set ((ClutterContainer *)dialog, box,
				   "expand", FALSE,
				   "x-fill", TRUE,
                                   NULL);
 
  priv->snooze = mx_toggle_new ();
  mx_toggle_set_active (priv->snooze, TRUE);
  mx_stylable_set_style_class (MX_STYLABLE (priv->snooze), "SnoozeToggle");

  label = mx_label_new (_("Snooze"));
  box = mx_box_layout_new ();
  mx_box_layout_set_pack_start ((MxBoxLayout *)box, FALSE);
  mx_box_layout_set_vertical ((MxBoxLayout *)box, FALSE);
  mx_box_layout_set_spacing (box, 4);

  clutter_container_add_actor (box, label);
  clutter_container_child_set ((ClutterContainer *)box, label,
                                   "x-fill", FALSE,
                                   "y-fill", TRUE,
				   "expand", FALSE,
                                   NULL);
  clutter_container_add_actor (box, priv->snooze);
  clutter_container_child_set ((ClutterContainer *)box, priv->snooze,
                                   "x-fill", FALSE,
                                   "y-fill", TRUE,
				   "expand", FALSE,
                                   NULL);
  

  clutter_container_add_actor ((ClutterContainer *)dialog, box);
  clutter_container_child_set ((ClutterContainer *)dialog, box,
				   "expand", FALSE,
				   "x-fill", TRUE,
                                   NULL);
  
}

static void
construct_sound_menu (MnpAlarmDialog *dialog)
{
  MnpAlarmDialogPrivate *priv = ALARM_DIALOG_PRIVATE(dialog);
  int i;
  ClutterActor *box, *label;
  
  priv->sound = mx_combo_box_new ();

  for (i=0; i<G_N_ELEMENTS(alarm_sound_table); i++)
	  mx_combo_box_append_text (priv->sound, alarm_sound_table[i].name);

  mx_combo_box_set_index (priv->sound, MNP_SOUND_BEEP);

  label = mx_label_new (_("Sound"));
  box = mx_box_layout_new ();
  mx_box_layout_set_pack_start ((MxBoxLayout *)box, FALSE);
  mx_box_layout_set_vertical ((MxBoxLayout *)box, FALSE);
  mx_box_layout_set_spacing (box, 4);

  clutter_container_add_actor (box, label);
  clutter_container_child_set ((ClutterContainer *)box, label,
                                   "x-fill", TRUE,
                                   "y-fill", TRUE,
				   "expand", FALSE,
                                   NULL);
  clutter_container_add_actor (box, priv->sound);
  clutter_container_child_set ((ClutterContainer *)box, priv->sound,
                                   "x-fill", FALSE,
                                   "y-fill", TRUE,
				   "expand", FALSE,
                                   NULL);
  

  clutter_container_add_actor ((ClutterContainer *)dialog, box);
  clutter_container_child_set ((ClutterContainer *)dialog, box,
				   "expand", FALSE,
				   "x-fill", TRUE,
                                   NULL);
 
  
}

static void
alarm_del (MxButton *btn, MnpAlarmDialog *dialog)
{
  MnpAlarmDialogPrivate *priv = ALARM_DIALOG_PRIVATE(dialog);
  GList *list, *tmp, *del_node;
  GConfClient *client;

  client = gconf_client_get_default();

  list = gconf_client_get_list (client,"/apps/date-time-panel/alarms", GCONF_VALUE_STRING, NULL);
  tmp = list;
  while(tmp) {
	char *data = (char *)tmp->data;
	int id, on_off, hour, min, am_pm, recur, snooze, sound;

	sscanf(data, "%d %d %d %d %d %d %d %d", &id, &on_off, &hour, &min, &am_pm, &recur, &snooze, &sound);

	if (id == priv->id) {
		del_node = tmp;
		break;
	}
		
  	tmp = tmp->next;
  }

  if (del_node) {
  	list = g_list_remove_link (list, del_node);
  	gconf_client_set_list(client,"/apps/date-time-panel/alarms", GCONF_VALUE_STRING, list, NULL);	
	g_free (del_node->data);
	g_free(del_node);
  }

  g_slist_foreach(list, (GFunc)g_free, NULL);
  g_slist_free(list);

  g_object_unref(client);
  clutter_actor_destroy(dialog);
}

static void
construct_alarm_delete (MnpAlarmDialog *dialog)
{
  MnpAlarmDialogPrivate *priv = ALARM_DIALOG_PRIVATE(dialog);
 
  priv->delete = mx_button_new ();
  mx_button_set_label (priv->delete, _("Delete alarm"));
  
  g_signal_connect (priv->delete, "clicked", G_CALLBACK(alarm_del), dialog);
  clutter_container_add_actor ((ClutterContainer *)dialog, priv->delete);
  clutter_container_child_set ((ClutterContainer *)dialog, priv->delete,
				   "expand", FALSE,
				   "x-fill", TRUE,
                                   NULL);
 
}

static void
mnp_alarm_dialog_construct (MnpAlarmDialog *dialog)
{
  MnpAlarmDialogPrivate *priv = ALARM_DIALOG_PRIVATE(dialog);
  
  clutter_actor_set_name (dialog, "new-alarm-dialog");
  mx_box_layout_set_pack_start ((MxBoxLayout *)dialog, FALSE);
  mx_box_layout_set_vertical ((MxBoxLayout *)dialog, TRUE);
  mx_box_layout_set_spacing ((MxBoxLayout *)dialog, 3);

  construct_title_header(dialog);
  construct_on_off_toggle(dialog);
  construct_time_entry(dialog);
  construct_recur_snooze_entry (dialog);
  construct_sound_menu (dialog);
  construct_alarm_delete (dialog);

  clutter_container_add_actor ((ClutterContainer *)clutter_stage_get_default(), dialog);
  clutter_actor_raise_top (dialog);
  clutter_actor_set_position (dialog, 300,105);
  clutter_actor_set_size (dialog, 300, 300);
  
  priv->id = g_random_int();
}

MnpAlarmDialog*
mnp_alarm_dialog_new (void)
{
  MnpAlarmDialog *dialog = g_object_new (MNP_TYPE_ALARM_DIALOG, NULL);

  mnp_alarm_dialog_construct(dialog);

  return dialog;
}
