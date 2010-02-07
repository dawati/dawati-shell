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

#include "mnp-clock-area.h"
#include "mnp-clock-tile.h"

struct _MnpClockAreaPriv {
	ClutterActor *background;

	guint is_enabled : 1;
	GList *clock_tiles;
	int position;
};

enum
{
  DROP_PROP_0,
  DROP_PROP_ENABLED
};

static const ClutterColor default_background_color = { 204, 204, 0, 255 };
static MxBoxLayoutClass *parent_class = NULL;
static void mx_droppable_init (MxDroppableIface *klass);

G_DEFINE_TYPE_WITH_CODE (MnpClockArea, mnp_clock_area, MX_TYPE_BOX_LAYOUT,
                         G_IMPLEMENT_INTERFACE (MX_TYPE_DROPPABLE,
                                                mx_droppable_init));

static void
mnp_clock_area_enable (MxDroppable *droppable)
{
}

static void
mnp_clock_area_disable (MxDroppable *droppable)
{
}

static gboolean
mnp_clock_area_accept_drop (MxDroppable *droppable, MxDraggable *draggable)
{
	return TRUE;
}

static void
mnp_clock_area_over_in (MxDroppable *droppable, MxDraggable *draggable)
{
  g_debug (G_STRLOC ": over-in");
  clutter_actor_animate (CLUTTER_ACTOR (droppable),
                         CLUTTER_EASE_IN_CUBIC,
                         250,
                         "opacity", 255,
                         NULL);
}

static void
mnp_clock_area_over_out (MxDroppable *droppable, MxDraggable *draggable)
{
  g_debug (G_STRLOC ": over-out");
  clutter_actor_animate (CLUTTER_ACTOR (droppable),
                         CLUTTER_EASE_IN_CUBIC,
                         250,
                         "opacity", 128,
                         NULL);
	
}

static void
mnp_clock_area_drop (MxDroppable *droppable, MxDraggable *draggable, gfloat event_x, gfloat event_y, gint button, ClutterModifierType modifiers)
{
  ClutterActor *self = CLUTTER_ACTOR (droppable);
  ClutterActor *child = CLUTTER_ACTOR (draggable);

  g_debug ("%s: dropped %s on '%s' (%s) at %.2f, %.2f",
           G_STRLOC,
           G_OBJECT_TYPE_NAME (draggable),
           clutter_actor_get_name (self),
           G_OBJECT_TYPE_NAME (droppable),
           event_x, event_y);

  g_object_ref (draggable);

  clutter_actor_reparent (child, self);
  clutter_actor_set_position (CLUTTER_ACTOR (draggable),
                              (event_x < 100) ? 50 : 100,
                              (event_y < 100) ? 50 : 100);

  g_object_unref (draggable);	
}

static void
mnp_clock_area_finalize (GObject *object)
{
	MnpClockArea *area = (MnpClockArea *)object;

	if (area->priv->background) {
		clutter_actor_destroy (area->priv->background);
		area->priv->background = NULL;
    	}


	G_OBJECT_CLASS(G_OBJECT_GET_CLASS(object))->finalize (object);
}
static void
mnp_clock_area_instance_init (GTypeInstance *instance, gpointer g_class)
{
}

static void
mx_droppable_init (MxDroppableIface *klass)
{
	//klass->enable = mnp_clock_area_enable;
	//klass->disable = mnp_clock_area_disable;
	//klass->accept_drop = mnp_clock_area_accept_drop;
	klass->over_in = mnp_clock_area_over_in;
	klass->over_out = mnp_clock_area_over_out;
	klass->drop = mnp_clock_area_drop;
}

static void
droppable_group_set_property (GObject      *gobject,
                              guint         prop_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  	MnpClockArea *group = (MnpClockArea *) gobject;

	switch (prop_id) {
	case DROP_PROP_ENABLED:
		group->priv->is_enabled = g_value_get_boolean (value);
		if (group->priv->is_enabled)
			mx_droppable_enable (MX_DROPPABLE (gobject));
		else
			mx_droppable_disable (MX_DROPPABLE (gobject));
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
		break;
	}
}

static void
droppable_group_get_property (GObject    *gobject,
                              guint       prop_id,
                              GValue     *value,
                              GParamSpec *pspec)
{
	MnpClockArea*group = (MnpClockArea *) gobject;

	switch (prop_id)
	{
	case DROP_PROP_ENABLED:
		g_value_set_boolean (value, group->priv->is_enabled);
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
		break;
    	}
}

static void
mnp_clock_area_class_init (MnpClockAreaClass *klass)
{
	GObjectClass *object_class;

	parent_class = g_type_class_peek_parent (klass);
	object_class = (GObjectClass*) klass;
	object_class->finalize = mnp_clock_area_finalize;

  	object_class->set_property = droppable_group_set_property;
  	object_class->get_property = droppable_group_get_property;

	g_object_class_override_property (object_class,
        	                            DROP_PROP_ENABLED,
                	                    "enabled");	
}

static void
mnp_clock_area_init (MnpClockArea *object)
{
}

#if 0
GType
mnp_clock_area_get_type (void)
{
	static GType type = 0;
	if (G_UNLIKELY(type == 0))
	{
		static const GTypeInfo info = 
		{
			sizeof (MnpClockAreaClass),
			NULL,   /* base_init */
			NULL,   /* base_finalize */
			(GClassInitFunc) mnp_clock_area_class_init,   /* class_init */
			NULL,   /* class_finalize */
			NULL,   /* class_data */
			sizeof (MnpClockArea),
			0,      /* n_preallocs */
			mnp_clock_area_instance_init,    /* instance_init */
			NULL
		};


		static const GInterfaceInfo mx_droppable_info = 
		{
			(GInterfaceInitFunc) mx_droppable_init, /* interface_init */
			NULL,         /* interface_finalize */
			NULL          /* interface_data */
		};

		type = g_type_register_static (MX_TYPE_BOX_LAYOUT,
			"MnpClockArea",
			&info, 0);

		g_type_add_interface_static (type, MX_TYPE_DROPPABLE,
			&mx_droppable_info);

	}
	return type;
}
#endif

MnpClockArea *
mnp_clock_area_new (void)
{
	MnpClockArea *area = g_object_new(MNP_TYPE_CLOCK_AREA, NULL);

	area->priv = g_new0(MnpClockAreaPriv, 1);
	area->priv->is_enabled = 1;
	area->priv->clock_tiles = NULL;
	area->priv->position = 55;
	mx_box_layout_set_vertical ((MxBoxLayout *)area, TRUE);
	mx_box_layout_set_pack_start ((MxBoxLayout *)area, TRUE);
	
	return area;
}

void
mnp_clock_area_add_tile (MnpClockArea *area, MnpClockTile *tile)
{

	clutter_actor_set_reactive (tile, TRUE);
	clutter_actor_set_name (tile, "clock-tile");
	clutter_container_add_actor (clutter_stage_get_default(), tile);
	mx_draggable_set_axis (MX_DRAGGABLE (tile), MX_Y_AXIS);
	mx_draggable_enable (tile);
	clutter_actor_set_size (tile, 200, 50);
	clutter_actor_set_position (tile, 6, area->priv->position);  
	area->priv->position += 55;
}
