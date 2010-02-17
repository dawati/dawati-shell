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


#include "mx-spin-entry.h"

G_DEFINE_TYPE (MxSpinEntry, mx_spin_entry, MX_TYPE_BOX_LAYOUT)

#define SPIN_ENTRY_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MX_TYPE_SPIN_ENTRY, MxSpinEntryPrivate))

typedef struct _MxSpinEntryPrivate MxSpinEntryPrivate;

struct _MxSpinEntryPrivate
{
	ClutterActor *entry;
	ClutterActor *up;
	ClutterActor *down;

	gint value;

	gboolean cycle;
	gint upper;
	gint lower;
};

static void
mx_spin_entry_dispose (GObject *object)
{
  G_OBJECT_CLASS (mx_spin_entry_parent_class)->dispose (object);
}

static void
mx_spin_entry_finalize (GObject *object)
{
  G_OBJECT_CLASS (mx_spin_entry_parent_class)->finalize (object);
}

static void
mx_spin_entry_class_init (MxSpinEntryClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MxSpinEntryPrivate));

  object_class->dispose = mx_spin_entry_dispose;
  object_class->finalize = mx_spin_entry_finalize;
}

static void
mx_spin_entry_init (MxSpinEntry *self)
{
}

static void
update_text (MxSpinEntry *spin)
{
  MxSpinEntryPrivate *priv = SPIN_ENTRY_PRIVATE(spin);
  char *txt;

  txt = g_strdup_printf("%.2d", priv->value);
  mx_entry_set_text((MxEntry *)priv->entry, txt);
  g_free(txt);  
}

static void
up_clicked (MxButton *button, MxSpinEntry *spin)
{
  MxSpinEntryPrivate *priv = SPIN_ENTRY_PRIVATE(spin);

  priv->value++;
  if (priv->value > priv->upper) {
	  if (priv->cycle)
	  	priv->value = priv->lower;
	  else
		priv->value--;
  }
  update_text(spin);
}

static void
down_clicked (MxButton *button, MxSpinEntry *spin)
{
  MxSpinEntryPrivate *priv = SPIN_ENTRY_PRIVATE(spin);

  priv->value--;
  if (priv->value < priv->lower) {
	  if (priv->cycle)
	  	priv->value = priv->upper;
	  else
		priv->value++;
  }

  update_text (spin);
}

static void
mx_spin_entry_construct (MxSpinEntry *spin)
{
  MxSpinEntryPrivate *priv = SPIN_ENTRY_PRIVATE(spin);
  ClutterActor *icon;

  mx_stylable_set_style_class (MX_STYLABLE (spin), "MxSpinEntry");
  mx_box_layout_set_pack_start ((MxBoxLayout *)spin, FALSE);
  mx_box_layout_set_vertical ((MxBoxLayout *)spin, TRUE);
  mx_box_layout_set_spacing ((MxBoxLayout *)spin, 0);

  priv->up = mx_button_new ();
  mx_stylable_set_style_class (MX_STYLABLE (priv->up), "UpButton");
  icon = mx_icon_new ();
  mx_stylable_set_style_class (MX_STYLABLE (icon), "UpIcon");
  mx_bin_set_child ((MxBin *)priv->up, icon);
  g_signal_connect (priv->up, "clicked", G_CALLBACK(up_clicked), spin);

  priv->down = mx_button_new ();
  mx_stylable_set_style_class (MX_STYLABLE (priv->down), "DownButton");
  icon = mx_icon_new ();
  mx_stylable_set_style_class (MX_STYLABLE (icon), "DownIcon");
  mx_bin_set_child ((MxBin *)priv->down, icon);
  g_signal_connect (priv->down, "clicked", G_CALLBACK(down_clicked), spin);


  priv->entry = mx_entry_new (NULL);
  mx_stylable_set_style_class (MX_STYLABLE (priv->entry), "SpinEntry");

  clutter_container_add_actor ((ClutterContainer *)spin, priv->up);
  clutter_container_add_actor ((ClutterContainer *)spin, priv->entry);
  clutter_container_add_actor ((ClutterContainer *)spin, priv->down);

  clutter_actor_set_size ((ClutterActor *)spin, 50, -1);
}

MxSpinEntry*
mx_spin_entry_new (void)
{
  MxSpinEntry *spin = g_object_new (MX_TYPE_SPIN_ENTRY, NULL);

  mx_spin_entry_construct (spin);

  return spin;
}

void
mx_spin_entry_set_range (MxSpinEntry *spin, gint lower, gint upper)
{
	MxSpinEntryPrivate *priv = SPIN_ENTRY_PRIVATE(spin);

	priv->lower = lower;
	priv->upper = upper;
	priv->value = lower;

	update_text (spin);
}

void
mx_spin_entry_set_cycle (MxSpinEntry *spin, gboolean cycle)
{
	MxSpinEntryPrivate *priv = SPIN_ENTRY_PRIVATE(spin);

	priv->cycle = cycle;
}

gint
mx_spin_entry_get_value (MxSpinEntry *spin)
{
	MxSpinEntryPrivate *priv = SPIN_ENTRY_PRIVATE(spin);

	return priv->value;
}

void
mx_spin_entry_set_value (MxSpinEntry *spin, gint value)
{
	MxSpinEntryPrivate *priv = SPIN_ENTRY_PRIVATE(spin);

	priv->value = value;
	if (value > priv->upper)
		value = priv->upper;
	else if (value < priv->lower)
		value = priv->lower;

	update_text(spin);
}
