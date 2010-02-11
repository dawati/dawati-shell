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

MnpAlarmDialog*
mnp_alarm_dialog_new (void)
{
  return g_object_new (MNP_TYPE_ALARM_DIALOG, NULL);
}
