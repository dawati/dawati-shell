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
mnp_alarm_dialog_construct (MnpAlarmDialog *dialog)
{
  MnpAlarmDialogPrivate *priv = ALARM_DIALOG_PRIVATE(dialog);
  
  clutter_actor_set_name (dialog, "new-alarm-dialog");
  mx_box_layout_set_pack_start ((MxBoxLayout *)dialog, FALSE);
  mx_box_layout_set_vertical ((MxBoxLayout *)dialog, TRUE);
  
  construct_title_header(dialog);

  clutter_container_add_actor ((ClutterContainer *)clutter_stage_get_default(), dialog);
  clutter_actor_set_position (dialog, 500,200);
  clutter_actor_set_size (dialog, 250, 250);
}

MnpAlarmDialog*
mnp_alarm_dialog_new (void)
{
  MnpAlarmDialog *dialog = g_object_new (MNP_TYPE_ALARM_DIALOG, NULL);

  mnp_alarm_dialog_construct(dialog);

  return dialog;
}
