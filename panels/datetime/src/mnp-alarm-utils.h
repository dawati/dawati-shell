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

#ifndef _MNP_ALARM_UTILS
#define _MNP_ALARM_UTILS

typedef enum {
	MNP_SUNDAY = 1 << 0,
	MNP_MONDAY = 1 << 1,
	MNP_TUESDAY = 1 << 2,
	MNP_WEDNESDAY = 1 << 3,
	MNP_THURSDAY = 1 << 4,
	MNP_FRIDAY = 1 << 5,
	MNP_SATURDAY = 1 << 6

}MnpDay;

typedef enum {
	MNP_ALARM_NEVER=0,
	MNP_ALARM_EVERYDAY = MNP_SUNDAY|MNP_MONDAY|MNP_TUESDAY|MNP_WEDNESDAY|MNP_THURSDAY|MNP_FRIDAY|MNP_SATURDAY,
	MNP_ALARM_WORKWEEK = MNP_MONDAY|MNP_TUESDAY|MNP_WEDNESDAY|MNP_THURSDAY|MNP_FRIDAY, /* Mon-Fri*/
	MNP_ALARM_MONDAY = MNP_MONDAY,
	MNP_ALARM_TUESDAY = MNP_TUESDAY,
	MNP_ALARM_WEDNESDAY = MNP_WEDNESDAY,
	MNP_ALARM_THURSDAY = MNP_THURSDAY,
	MNP_ALARM_FRIDAY = MNP_FRIDAY,
	MNP_ALARM_SATURDAY = MNP_SATURDAY,
	MNP_ALARM_SUNDAY = MNP_SUNDAY
}MnpAlarmRecurrance;

typedef enum {
	MNP_SOUND_OFF=0,
	MNP_SOUND_BEEP,
	MNP_SOUND_MUSIC,
	MNP_SOUND_MESSAGE
}MnpAlarmSound;



typedef struct _MnpAlarmItem {
  int id;
  gboolean on_off;
  int hour;
  int minute;
  gboolean am_pm;
  int repeat;
  gboolean snooze;
  int sound;
}MnpAlarmItem;

#endif
