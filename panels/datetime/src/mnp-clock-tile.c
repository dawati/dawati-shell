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


#include <config.h>
#include <glib.h>
#include <glib/gi18n-lib.h>

#include "mnp-clock-tile.h"
#include "mnp-utils.h"

#define FREE_DFMT(fmt) g_free(fmt->date); g_free(fmt->city); g_free(fmt->time); g_free(fmt);

struct _MnpClockTilePriv {
	/* Draggable properties */
	guint threshold;

	MxDragAxis axis;

	MxDragContainment containment;
	ClutterActorBox area;

	guint is_enabled : 1;
	MnpZoneLocation *loc;
	time_t time_now;

	MxLabel *date;
	MxLabel *time;
	MxLabel *city;

	MxButton *remove_button;
	TileRemoveFunc remove_cb;
	gpointer *remove_data;

	gfloat depth;
};

enum
{
  DRAG_PROP_0,

  DRAG_PROP_DRAG_THRESHOLD,
  DRAG_PROP_AXIS,
  DRAG_PROP_CONTAINMENT_TYPE,
  DRAG_PROP_CONTAINMENT_AREA,
  DRAG_PROP_ENABLED,
  DRAG_PROP_ACTOR,
};

static MxBoxLayoutClass *parent_class = NULL;

#if 0
static void
mnp_clock_tile_enable (MxDraggable *draggable)
{
}

static void
mnp_clock_tile_disable (MxDraggable *draggable)
{
}
#endif

static void
mnp_clock_tile_drag_begin (MxDraggable *draggable, gfloat event_x, gfloat event_y, gint event_button, ClutterModifierType  modifiers)
{
	ClutterActor *self = CLUTTER_ACTOR (draggable);
	ClutterActor *stage = clutter_actor_get_stage (self);
	gfloat orig_x, orig_y;
	MnpClockTile *tile = (MnpClockTile *)draggable;

	g_object_ref (self);

	tile->priv->depth = clutter_actor_get_depth (self);
	clutter_actor_get_transformed_position (self, &orig_x, &orig_y);
	clutter_actor_reparent (self, stage);
	clutter_actor_raise_top (self);
	clutter_actor_set_position (self, orig_x, orig_y);
	g_object_unref (self);

	clutter_actor_animate (self, CLUTTER_EASE_OUT_CUBIC, 250,
				"opacity", 150,
				NULL);
}

static void
mnp_clock_tile_drag_motion (MxDraggable *draggable, gfloat delta_x, gfloat delta_y)
{
	clutter_actor_move_by (CLUTTER_ACTOR (draggable), delta_x, delta_y);	
}

static void
mnp_clock_tile_drag_end (MxDraggable *draggable, gfloat event_x, gfloat event_y)
{
	ClutterActor *self = CLUTTER_ACTOR (draggable);
	MnpClockTile *tile = (MnpClockTile *)draggable;

	clutter_actor_animate (self, CLUTTER_EASE_OUT_CUBIC, 250,
                         	"opacity", 255,
                         	NULL);	
	tile->priv->depth = 0.0;
}

static void
mnp_clock_tile_finalize (GObject *object)
{
	G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
mnp_clock_tile_instance_init (GTypeInstance *instance, gpointer g_class)
{
}

static void
mnp_draggable_rectangle_parent_set (ClutterActor *actor,
                                ClutterActor *old_parent)
{
  /* ClutterActor *new_parent = clutter_actor_get_parent (actor); */

  /*g_debug ("%s: old_parent: %s, new_parent: %s (%s)",
           G_STRLOC,
           old_parent ? G_OBJECT_TYPE_NAME (old_parent) : "none",
           new_parent ? clutter_actor_get_name (new_parent) : "Unknown",
           new_parent ? G_OBJECT_TYPE_NAME (new_parent) : "none");*/
}

static void
mx_draggable_init (MxDraggableIface *klass)
{
	/*klass->enable = mnp_clock_tile_enable;
	klass->disable = mnp_clock_tile_disable; */
	klass->drag_begin = mnp_clock_tile_drag_begin;
	klass->drag_motion = mnp_clock_tile_drag_motion;
	klass->drag_end = mnp_clock_tile_drag_end;
}

static void
draggable_rectangle_set_property (GObject      *gobject,
                                  guint         prop_id,
                                  const GValue *value,
                                  GParamSpec   *pspec)
{
  MnpClockTile  *rect = (MnpClockTile *) gobject;

  switch (prop_id)
    {
    case DRAG_PROP_DRAG_THRESHOLD:
      rect->priv->threshold = g_value_get_uint (value);
      break;

    case DRAG_PROP_AXIS:
      rect->priv->axis = g_value_get_enum (value);
      break;

    case DRAG_PROP_CONTAINMENT_TYPE:
      rect->priv->containment = g_value_get_enum (value);
      break;

    case DRAG_PROP_CONTAINMENT_AREA:
      {
        ClutterActorBox *box = g_value_get_boxed (value);

        if (box)
          rect->priv->area = *box;
        else
          memset (&rect->priv->area, 0, sizeof (ClutterActorBox));
      }
      break;

    case DRAG_PROP_ENABLED:
      rect->priv->is_enabled = g_value_get_boolean (value);
      if (rect->priv->is_enabled)
        mx_draggable_enable (MX_DRAGGABLE (gobject));
      else
        mx_draggable_disable (MX_DRAGGABLE (gobject));
      break;

    case DRAG_PROP_ACTOR:
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
      break;
    }
}

static void
draggable_rectangle_get_property (GObject    *gobject,
                                  guint       prop_id,
                                  GValue     *value,
                                  GParamSpec *pspec)
{
  MnpClockTile *rect = (MnpClockTile *)gobject;

  switch (prop_id)
    {
    case DRAG_PROP_DRAG_THRESHOLD:
      g_value_set_uint (value, rect->priv->threshold);
      break;

    case DRAG_PROP_AXIS:
      g_value_set_enum (value, rect->priv->axis);
      break;

    case DRAG_PROP_CONTAINMENT_TYPE:
      g_value_set_enum (value, rect->priv->containment);
      break;

    case DRAG_PROP_CONTAINMENT_AREA:
      g_value_set_boxed (value, &rect->priv->area);
      break;

    case DRAG_PROP_ENABLED:
      g_value_set_boolean (value, rect->priv->is_enabled);
      break;

    case DRAG_PROP_ACTOR:
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
      break;
    }
}

static void
mnp_clock_tile_class_init (MnpClockTileClass *klass)
{
	GObjectClass *object_class;
	ClutterActorClass *actor_class = CLUTTER_ACTOR_CLASS (klass);

	parent_class = g_type_class_peek_parent (klass);
	object_class = (GObjectClass*) klass;
	object_class->finalize = mnp_clock_tile_finalize;
  	object_class->set_property = draggable_rectangle_set_property;
	object_class->get_property = draggable_rectangle_get_property;
	
	actor_class->parent_set = mnp_draggable_rectangle_parent_set;

	g_object_class_override_property (object_class,
                                    DRAG_PROP_DRAG_THRESHOLD,
                                    "drag-threshold");
	g_object_class_override_property (object_class,
                                    DRAG_PROP_AXIS,
                                    "axis");
	g_object_class_override_property (object_class,
                                    DRAG_PROP_CONTAINMENT_TYPE,
                                    "containment-type");
	g_object_class_override_property (object_class,
                                    DRAG_PROP_CONTAINMENT_AREA,
                                    "containment-area");
	g_object_class_override_property (object_class,
                                    DRAG_PROP_ENABLED,
                                    "enabled");
	g_object_class_override_property (object_class,
                                    DRAG_PROP_ACTOR,
                                    "drag-actor");	
}
GType
mnp_clock_tile_get_type (void)
{
	static GType type = 0;
	if (G_UNLIKELY(type == 0))
	{
		static const GTypeInfo info = 
		{
			sizeof (MnpClockTileClass),
			NULL,   /* base_init */
			NULL,   /* base_finalize */
			(GClassInitFunc) mnp_clock_tile_class_init,   /* class_init */
			NULL,   /* class_finalize */
			NULL,   /* class_data */
			sizeof (MnpClockTile),
			0,      /* n_preallocs */
			mnp_clock_tile_instance_init,    /* instance_init */
			NULL
		};


		static const GInterfaceInfo mx_draggable_info = 
		{
			(GInterfaceInitFunc) mx_draggable_init, /* interface_init */
			NULL,         /* interface_finalize */
			NULL          /* interface_data */
		};

		type = g_type_register_static (MX_TYPE_BOX_LAYOUT,
			"MnpClockTile",
			&info, 0);

		g_type_add_interface_static (type, MX_TYPE_DRAGGABLE,
			&mx_draggable_info);

	}
	return type;
}

static void
remove_tile (MxButton *button, MnpClockTile *tile)
{
	clutter_actor_hide((ClutterActor *)tile);
	tile->priv->remove_cb(tile, tile->priv->remove_data);
	clutter_actor_destroy ((ClutterActor *)tile);
}

void
mnp_clock_tile_set_remove_cb (MnpClockTile *tile, TileRemoveFunc func, gpointer data)
{
	tile->priv->remove_cb = func;
	tile->priv->remove_data = data;
}

static void
mnp_clock_construct (MnpClockTile *tile)
{
	MnpDateFormat *fmt;
	ClutterActor *box1, *box2, *label1, *label2, *label3;
	ClutterActor *icon;
	MnpClockTilePriv *priv = tile->priv;

	fmt = mnp_format_time_from_location (tile->priv->loc, tile->priv->time_now);

 

	label1 = mx_label_new (fmt->date);
	tile->priv->date = (MxLabel *)label1;
	clutter_actor_set_name (label1, "mnp-tile-date");

	label2 = mx_label_new (fmt->time);
	tile->priv->time = (MxLabel *)label2;
	clutter_actor_set_name (label2, "mnp-tile-time");

	label3 = mx_label_new (fmt->city);
	tile->priv->city = (MxLabel *)label3;
	clutter_actor_set_name (label3, "mnp-tile-city");
	clutter_actor_set_size (label3, 110, -1);

	box1 = mx_box_layout_new ();
	clutter_actor_set_name (box1, "mnp-tile-date-city");
	mx_box_layout_set_vertical ((MxBoxLayout *)box1, TRUE);
	mx_box_layout_set_pack_start ((MxBoxLayout *)box1, FALSE);

	clutter_container_add_actor ((ClutterContainer *)box1, label3);
	clutter_container_child_set ((ClutterContainer *)box1, label3,
				   	"expand", TRUE,
					"y-fill", FALSE,
                                   	NULL);
	
	clutter_container_add_actor ((ClutterContainer *)box1, label1);
	clutter_container_child_set ((ClutterContainer *)box1, label1,
				   	"expand", TRUE,
					"y-fill", FALSE,
                                   	NULL);

	mx_box_layout_set_vertical ((MxBoxLayout *)tile, FALSE);
	mx_box_layout_set_pack_start ((MxBoxLayout *)tile, FALSE);
	clutter_container_add_actor ((ClutterContainer *)tile, box1);
	clutter_container_child_set ((ClutterContainer *)tile, box1,
				   	"expand", TRUE,
					"x-fill", TRUE,
                                   	NULL);


	box1 = mx_box_layout_new ();
	mx_box_layout_set_vertical ((MxBoxLayout *)box1, TRUE);
	mx_box_layout_set_pack_start ((MxBoxLayout *)box1, FALSE);
	clutter_container_add_actor ((ClutterContainer *)box1, label2);
	clutter_container_child_set ((ClutterContainer *)box1, label2,
				   	"expand", TRUE,
                                   	NULL);
	
	clutter_container_add_actor ((ClutterContainer *)tile, box1);
	clutter_container_child_set ((ClutterContainer *)tile, box1,
				   	"expand", FALSE,
					"x-fill", FALSE,
                                   	NULL);
	FREE_DFMT(fmt);

	box2 = mx_box_layout_new ();
	clutter_actor_set_name (box2, "mnp-tile-remove-button");
	mx_box_layout_set_vertical ((MxBoxLayout *)box2, TRUE);
	mx_box_layout_set_pack_start ((MxBoxLayout *)box2, FALSE);	
 	
	priv->remove_button = (MxButton *)mx_button_new ();
	g_signal_connect (priv->remove_button, "clicked", G_CALLBACK(remove_tile), tile);
	clutter_container_add_actor ((ClutterContainer *)box2, (ClutterActor *)priv->remove_button);  	
  	clutter_container_child_set ((ClutterContainer *)box2, (ClutterActor *)priv->remove_button,
				   	"expand", FALSE,
					"y-fill", FALSE,
                                   	NULL);

	clutter_container_add_actor ((ClutterContainer *)tile, box2);
  	clutter_container_child_set ((ClutterContainer *)tile, box2,
				   	"expand", FALSE,
					"x-fill", FALSE,
                                   	NULL);

	mx_stylable_set_style_class (MX_STYLABLE (priv->remove_button),
                               			"TileRemoveButton");
  	icon = (ClutterActor *)mx_icon_new ();
  	mx_stylable_set_style_class (MX_STYLABLE (icon),
                               		"TileIcon");
  	mx_bin_set_child (MX_BIN (priv->remove_button),
                      		  (ClutterActor *)icon);
	
}

MnpClockTile *
mnp_clock_tile_new (MnpZoneLocation *location, time_t time_now)
{
	MnpClockTile *tile = g_object_new (MNP_TYPE_CLOCK_TILE, NULL);

	tile->priv = g_new0(MnpClockTilePriv, 1);
	tile->priv->is_enabled = 1;
  	tile->priv->threshold = 0;
	tile->priv->axis = 0;
  	tile->priv->is_enabled = TRUE;
	tile->priv->loc = location;
	tile->priv->time_now = time_now;

	mnp_clock_construct (tile);

	return tile;
}

void
mnp_clock_tile_refresh (MnpClockTile *tile, time_t now)
{
	MnpDateFormat *fmt;

	tile->priv->time_now = now;
	fmt = mnp_format_time_from_location (tile->priv->loc, tile->priv->time_now);

	mx_label_set_text ((MxLabel *)tile->priv->time, fmt->time);
	mx_label_set_text ((MxLabel *)tile->priv->date, fmt->date);

	FREE_DFMT(fmt);
}

MnpZoneLocation * 
mnp_clock_tile_get_location (MnpClockTile *tile)
{
	return tile->priv->loc;
}

