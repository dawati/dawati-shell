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
#include <gtk/gtk.h>
#include <gio/gdesktopappinfo.h>
#include <glib/gi18n.h>


#include <meego-panel/mpl-panel-clutter.h>
#include <meego-panel/mpl-panel-common.h>
#include <meego-panel/mpl-entry.h>

#include "mnp-world-clock.h"
#include "mnp-utils.h"
#include "mnp-button-item.h"

#include "mnp-clock-tile.h"
#include "mnp-clock-area.h"
#include <gconf/gconf-client.h>

#define SINGLE_DIV_LINE THEMEDIR "/single-div-line.png"

G_DEFINE_TYPE (MnpWorldClock, mnp_world_clock, MX_TYPE_BOX_LAYOUT)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MNP_TYPE_WORLD_CLOCK, MnpWorldClockPrivate))

#define TIMEOUT 250

enum {
	TIME_CHANGED,
	LAST_SIGNAL
};
static guint signals[LAST_SIGNAL] = { 0 };
	

typedef struct _MnpWorldClockPrivate MnpWorldClockPrivate;

struct _MnpWorldClockPrivate {
	MxEntry *search_location;
	MxListView *zones_list;
	ClutterModel *zones_model;
	ClutterActor *completion;
        ClutterActor *event_box;
	ClutterActor *entry_box;
	ClutterActor *tfh_clock;

	ClutterActor *add_location;

	ClutterActor *header_box;
	ClutterActor *widget_box;
	ClutterActor *launcher_box;

	ClutterActor *launcher;

	const char *search_text;

	MnpClockArea *area;

	GPtrArray *zones;
	guint completion_timeout;
	gboolean completion_inited;

	MplPanelClient *panel_client;
};

static void construct_completion (MnpWorldClock *world_clock);

static void
mnp_world_clock_dispose (GObject *object)
{
  /* MnpWorldClockPrivate *priv = GET_PRIVATE (object); */

  G_OBJECT_CLASS (mnp_world_clock_parent_class)->dispose (object);
}

static void
mnp_world_clock_finalize (GObject *object)
{
  /* MnpWorldClockPrivate *priv = GET_PRIVATE (object); */

  G_OBJECT_CLASS (mnp_world_clock_parent_class)->finalize (object);
}

static void
mnp_world_clock_class_init (MnpWorldClockClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MnpWorldClockPrivate));

  object_class->dispose = mnp_world_clock_dispose;
  object_class->finalize = mnp_world_clock_finalize;
  
  signals[TIME_CHANGED] =
	g_signal_new ("time-changed",
			G_OBJECT_CLASS_TYPE (object_class),
			G_SIGNAL_RUN_FIRST,      
			G_STRUCT_OFFSET (MnpWorldClockClass, time_changed),
			NULL, NULL,
			g_cclosure_marshal_VOID__VOID,
			G_TYPE_NONE, 0);
  
}


static void
mnp_world_clock_init (MnpWorldClock *self)
{
  MnpWorldClockPrivate *priv = GET_PRIVATE (self);

  priv->search_text = "";
  priv->zones = NULL;
  priv->completion_inited = FALSE;
}

static gboolean
start_search (MnpWorldClock *area)
{
	MnpWorldClockPrivate *priv = GET_PRIVATE (area);

	priv->completion_timeout = 0;

	if (!priv->completion_inited) {
		priv->completion_inited = TRUE;
		construct_completion (area);
	}
	priv->search_text = mx_entry_get_text (priv->search_location);

	if (!priv->search_text || (strlen(priv->search_text) < 3))
		clutter_actor_hide(priv->completion);
	
	if (priv->search_text && (strlen(priv->search_text) > 2))
		g_signal_emit_by_name (priv->zones_model, "filter-changed");
	if (priv->search_text && (strlen(priv->search_text) > 2) && (clutter_model_get_n_rows(priv->zones_model) > 0)) {
		clutter_actor_show(priv->completion);
		clutter_actor_raise_top (priv->completion);
	} else {
		clutter_actor_hide(priv->completion);
	}


	return FALSE;
}

static void
text_changed_cb (MxEntry *entry, GParamSpec *pspec, void *user_data)
{
	MnpWorldClockPrivate *priv = GET_PRIVATE (user_data);

	if (priv->completion_timeout != 0)
		g_source_remove(priv->completion_timeout);

	priv->completion_timeout = g_timeout_add (500, (GSourceFunc) start_search, user_data);

}

static char *
find_word (const char *full_name, const char *word, int word_len,
	   gboolean whole_word, gboolean is_first_word)
{
    char *p = (char *)full_name - 1;

    while ((p = strchr (p + 1, *word))) {
	if (strncmp (p, word, word_len) != 0)
	    continue;

	if (p > (char *)full_name) {
	    char *prev = g_utf8_prev_char (p);

	    /* Make sure p points to the start of a word */
	    if (g_unichar_isalpha (g_utf8_get_char (prev)))
		continue;

	    /* If we're matching the first word of the key, it has to
	     * match the first word of the location, city, state, or
	     * country. Eg, it either matches the start of the string
	     * (which we already know it doesn't at this point) or
	     * it is preceded by the string ", " (which isn't actually
	     * a perfect test. FIXME)
	     */
	    if (is_first_word) {
		if (prev == (char *)full_name || strncmp (prev - 1, ", ", 2) != 0)
		    continue;
	    }
	}

	if (whole_word && g_unichar_isalpha (g_utf8_get_char (p + word_len)))
	    continue;

	return p;
    }
    return NULL;
}

static gboolean 
filter_zone (ClutterModel *model, ClutterModelIter *iter, gpointer user_data)
{
	MnpWorldClockPrivate *priv = GET_PRIVATE (user_data);
	char *name, *name_mem;
	GWeatherLocation *loc;
	gboolean is_first_word = TRUE, match;
	int len;
	char *key, *skey;

	if (!priv->search_text || !*priv->search_text)
		return TRUE;

	skey = key = g_ascii_strdown(priv->search_text, -1);
	clutter_model_iter_get (iter, 
			GWEATHER_LOCATION_ENTRY_COL_COMPARE_NAME, &name_mem,
			GWEATHER_LOCATION_ENTRY_COL_LOCATION, &loc,
			-1);
	name = name_mem;

    	if (!loc) {
		g_free (name_mem);
		g_free (key);
		return FALSE;
    	}

    	/* All but the last word in KEY must match a full word from NAME,
     	 * in order (but possibly skipping some words from NAME).
     	 */
	len = strcspn (key, " ");
	while (key[len]) {
		name = find_word (name, key, len, TRUE, is_first_word);
		if (!name) {
	    		g_free (name_mem);
			g_free (skey);
	    		return FALSE;
		}

		key += len;
		while (*key && !g_unichar_isalpha (g_utf8_get_char (key)))
	    		key = g_utf8_next_char (key);
		while (*name && !g_unichar_isalpha (g_utf8_get_char (name)))
	    		name = g_utf8_next_char (name);

		len = strcspn (key, " ");
		is_first_word = FALSE;
    	}

	/* The last word in KEY must match a prefix of a following word in NAME */
	match = find_word (name, key, strlen (key), FALSE, is_first_word) != NULL;
	g_free (name_mem);
	g_free (skey);
	return match;
}

static void
clear_btn_clicked_cb (ClutterActor *button, MnpWorldClock *world_clock)
{
	MnpWorldClockPrivate *priv = GET_PRIVATE (world_clock);

  	mx_entry_set_text (priv->search_location, "");
  	clutter_actor_hide (priv->completion);
}

static void
add_location_clicked_cb (ClutterActor *button, MnpWorldClock *world_clock)
{
	MnpWorldClockPrivate *priv = GET_PRIVATE (world_clock);
	const GWeatherLocation *location;
	MnpClockTile *tile;
	MnpZoneLocation *loc = g_new0(MnpZoneLocation, 1);
	const GWeatherTimezone *tzone;

	mx_list_view_set_model (MX_LIST_VIEW (priv->zones_list), NULL);

	priv->search_text = NULL;
	g_signal_emit_by_name (priv->zones_model, "filter-changed");
	
	location = mnp_utils_get_location_from_display (priv->zones_model, mx_entry_get_text (priv->search_location));
	loc->display = g_strdup(mx_entry_get_text (priv->search_location));
	loc->city = g_strdup(gweather_location_get_city_name (location));
	tzone =  gweather_location_get_timezone (location);
	loc->tzid = g_strdup(gweather_timezone_get_tzid ((GWeatherTimezone *)tzone));


	g_ptr_array_add (priv->zones, loc);
	mnp_save_zones(priv->zones);

	mx_entry_set_text (priv->search_location, "");

	tile = mnp_clock_tile_new (loc, mnp_clock_area_get_time(priv->area));
	mnp_clock_area_add_tile (priv->area, tile);

	priv->search_text = "asd";
	g_signal_emit_by_name (priv->zones_model, "filter-changed");	
	mx_list_view_set_model (MX_LIST_VIEW (priv->zones_list), priv->zones_model);
	
	if (priv->zones->len >= 4)
		clutter_actor_hide (priv->entry_box);
}


static void 
mnp_completion_done (gpointer data, const char *zone)
{
	MnpWorldClock *clock = (MnpWorldClock *)data;
	MnpWorldClockPrivate *priv = GET_PRIVATE (clock);
	
	clutter_actor_hide (priv->completion);
	g_signal_handlers_block_by_func (priv->search_location, text_changed_cb, clock);
	mx_entry_set_text (priv->search_location, zone);
	g_signal_handlers_unblock_by_func (priv->search_location, text_changed_cb, clock);
	add_location_clicked_cb (NULL, clock);

}

static void
insert_in_ptr_array (GPtrArray *array, gpointer data, int pos)
{
	int i;

	g_ptr_array_add (array, NULL);
	for (i=array->len-2; i>=pos ; i--) {
		array->pdata[i+1] = array->pdata[i];
	}
	array->pdata[pos] = data;
}

static void
zone_reordered_cb (MnpZoneLocation *location, int new_pos, MnpWorldClock *clock)
{
	MnpWorldClockPrivate *priv = GET_PRIVATE (clock);
	char *cloc = location->display;
	int i;

	for (i=0; i<priv->zones->len; i++) {
		MnpZoneLocation *loc = (MnpZoneLocation *)priv->zones->pdata[i];
		if (strcmp(cloc, loc->display) == 0) {
			break;
		}
		
	}

	g_ptr_array_remove_index (priv->zones, i);
	insert_in_ptr_array (priv->zones, (gpointer) location, new_pos);

	mnp_save_zones (priv->zones);
}

static void
zone_removed_cb (MnpClockArea *area, char *display, MnpWorldClock *clock)
{
	MnpWorldClockPrivate *priv = GET_PRIVATE (clock);	
	int i;
	MnpZoneLocation *loc;

	for (i=0; i<priv->zones->len; i++) {
		loc = (MnpZoneLocation *)priv->zones->pdata[i];		
		if (strcmp(display, loc->display) == 0) {
			break;
		}
	}

	if (i != priv->zones->len) {
		g_ptr_array_remove_index (priv->zones, i);
		mnp_save_zones(priv->zones);
	}

	if (priv->zones->len < 4)
		clutter_actor_show (priv->entry_box);
	
}

static gboolean
event_box_clicked_cb (ClutterActor  *box,
                      ClutterEvent  *event,
                      MnpWorldClock *world_clock)
{
        MnpWorldClockPrivate *priv = GET_PRIVATE (world_clock);

        clutter_actor_hide (priv->completion);
        clutter_actor_hide (box);

        return TRUE;
}

static void
completion_show_cb (ClutterActor  *completion,
                    MnpWorldClock *world_clock)
{
        MnpWorldClockPrivate *priv = GET_PRIVATE (world_clock);

        clutter_actor_show (priv->event_box);
        clutter_actor_lower (priv->event_box, completion);
}

static void
completion_hide_cb (ClutterActor  *completion,
                    MnpWorldClock *world_clock)
{
        MnpWorldClockPrivate *priv = GET_PRIVATE (world_clock);
        clutter_actor_hide (priv->event_box);
}

static void
construct_completion (MnpWorldClock *world_clock)
{
        const ClutterColor transparent = { 0x00, 0x00, 0x00, 0x00 };

	ClutterActor *frame, *scroll, *view, *stage;
	MnpWorldClockPrivate *priv = GET_PRIVATE (world_clock);
	ClutterModel *model;
	MnpButtonItem *button_item;

	stage = clutter_stage_get_default ();
	
        /* Create an event-box to capture input when the completion list
         * displays.
         */
        priv->event_box = clutter_rectangle_new_with_color (&transparent);
        clutter_actor_set_size (priv->event_box,
                                clutter_actor_get_width (stage),
                                clutter_actor_get_height (stage));
        clutter_container_add_actor (CLUTTER_CONTAINER (stage),
                                     priv->event_box);
        clutter_actor_set_reactive (priv->event_box, TRUE);
        clutter_actor_hide (priv->event_box);
        g_signal_connect (priv->event_box, "button-press-event",
                          G_CALLBACK (event_box_clicked_cb), world_clock);

	model = mnp_get_world_timezones ();
	priv->zones_model = model;
	clutter_model_set_filter (model, filter_zone, world_clock, NULL);
	priv->search_text = "asd";

        frame = mx_frame_new ();
        clutter_actor_set_name (frame, "CompletionFrame");
        mx_bin_set_fill (MX_BIN (frame), TRUE, TRUE);

	scroll = mx_scroll_view_new ();
	clutter_actor_set_name (scroll, "CompletionScrollView");
        g_object_set (G_OBJECT (scroll), "clip-to-allocation", TRUE, NULL);
	clutter_actor_set_size (scroll, -1, 300);	
        mx_bin_set_child (MX_BIN (frame), scroll);
	clutter_container_add_actor ((ClutterContainer *)stage, frame);
      	clutter_actor_raise_top((ClutterActor *) frame);
	clutter_actor_set_position (frame, 19, 147);  
	clutter_actor_hide (frame);

	priv->completion = frame;
        g_signal_connect (priv->completion, "show",
                          G_CALLBACK (completion_show_cb), world_clock);
        g_signal_connect (priv->completion, "hide",
                          G_CALLBACK (completion_hide_cb), world_clock);

	view = mx_list_view_new ();
	clutter_actor_set_name (view, "CompletionListView");
	priv->zones_list = (MxListView *)view;

	clutter_container_add_actor (CLUTTER_CONTAINER (scroll), view);
	mx_list_view_set_model (MX_LIST_VIEW (view), model);

	button_item = mnp_button_item_new ((gpointer)world_clock, mnp_completion_done);
	mx_list_view_set_factory (MX_LIST_VIEW (view), (MxItemFactory *)button_item);
	mx_list_view_add_attribute (MX_LIST_VIEW (view), "label", 0);
}

static void
hour_clock_changed (ClutterActor *button, MnpWorldClock *world_clock)
{
/*	MnpWorldClockPrivate *priv = GET_PRIVATE (world_clock); */
	gboolean tfh;
	GConfClient *client = gconf_client_get_default();

	tfh = mx_button_get_toggled ((MxButton *)button);

	gconf_client_set_bool (client, "/apps/date-time-panel/24_h_clock", tfh, NULL);
	g_object_unref(client);
}

static void
construct_heading_and_top_area (MnpWorldClock *world_clock)
{
	MnpWorldClockPrivate *priv = GET_PRIVATE (world_clock);
	ClutterActor *box, *icon, *check_button, *text;
	ClutterActor *div;
	GConfClient *client = gconf_client_get_default ();
	gboolean tfh;

	/* Time title */
	box = mx_box_layout_new ();
	clutter_actor_set_name (box,
                               	"TimeTitleBox");
	mx_box_layout_set_orientation ((MxBoxLayout *)box, MX_ORIENTATION_HORIZONTAL);
	mx_box_layout_set_spacing ((MxBoxLayout *)box, 4);

  	icon = (ClutterActor *)mx_icon_new ();
  	mx_stylable_set_style_class (MX_STYLABLE (icon),
        	                       	"ClockIcon");
  	clutter_actor_set_size (icon, 36, 36);
	mx_box_layout_add_actor (MX_BOX_LAYOUT(box), icon, -1);
	clutter_container_child_set (CLUTTER_CONTAINER (box),
                               icon,
			       "expand", FALSE,
			       "x-fill", FALSE,
			       "y-fill", FALSE,
			       "y-align", MX_ALIGN_MIDDLE,
			       "x-align", MX_ALIGN_START,
                               NULL);

	text = mx_label_new_with_text (_("Time"));
	clutter_actor_set_name (text, "TimeTitle");

	mx_box_layout_add_actor (MX_BOX_LAYOUT(box), text, -1);
	clutter_container_child_set (CLUTTER_CONTAINER (box),
                               text,
			       "expand", TRUE,
			       "x-fill", TRUE,
			       "y-fill", FALSE,			       
			       "y-align", MX_ALIGN_MIDDLE,
			       "x-align", MX_ALIGN_START,			       
                               NULL);

	mx_box_layout_add_actor (MX_BOX_LAYOUT(world_clock), box, -1);	
	clutter_container_child_set (CLUTTER_CONTAINER (world_clock),
                               box,
                               "expand", FALSE,
			       "y-fill", FALSE,		
			       "x-fill", TRUE,
			       "x-align", MX_ALIGN_START,
                               NULL);

	/* Check box */
	box = mx_box_layout_new ();
	priv->widget_box = box;
	mx_box_layout_set_orientation ((MxBoxLayout *)box, MX_ORIENTATION_VERTICAL);
	mx_box_layout_set_spacing ((MxBoxLayout *)box, 6);
	clutter_actor_set_name (box, "TwelveHourButtonBox");

	box = mx_box_layout_new ();
	
	mx_box_layout_set_orientation ((MxBoxLayout *)box, MX_ORIENTATION_HORIZONTAL);
	mx_box_layout_set_spacing ((MxBoxLayout *)box, 4);
	
	tfh = gconf_client_get_bool (client, "/apps/date-time-panel/24_h_clock", NULL);
	g_object_unref(client);
	check_button = mx_button_new ();
	priv->tfh_clock = check_button;
	mx_button_set_is_toggle (MX_BUTTON (check_button), TRUE);
	mx_button_set_toggled(MX_BUTTON (check_button), tfh);

	g_signal_connect(priv->tfh_clock, "clicked", G_CALLBACK(hour_clock_changed), world_clock);
	mx_stylable_set_style_class (MX_STYLABLE (check_button),
                               		"check-box");
	clutter_actor_set_size ((ClutterActor *)check_button, 21, 21);
	mx_box_layout_add_actor (MX_BOX_LAYOUT(box), check_button, -1);
	clutter_container_child_set (CLUTTER_CONTAINER (box),
                               check_button,
                               "expand", FALSE,
			       "y-fill", FALSE,		
			       "x-fill", FALSE,
                               NULL);
	
	text = mx_label_new_with_text (_("24 hour clock"));

	clutter_actor_set_name (text, "HourClockLabel");
	mx_box_layout_add_actor (MX_BOX_LAYOUT(box), text, -1);
	clutter_container_child_set (CLUTTER_CONTAINER (box),
                               text,
                               "expand", TRUE,
			       "y-fill", FALSE,		
			       "x-fill", TRUE,	
                               NULL);

	mx_box_layout_add_actor (MX_BOX_LAYOUT(priv->widget_box), box, -1);	
	clutter_container_child_set (CLUTTER_CONTAINER (priv->widget_box),
                               box,
                               "expand", FALSE,
			       "y-fill", FALSE,		
			       "x-fill", TRUE,			       			       
                               NULL);	

	mx_box_layout_add_actor (MX_BOX_LAYOUT(world_clock), priv->widget_box, -1);	
	clutter_container_child_set (CLUTTER_CONTAINER (world_clock),
                               box,
                               "expand", FALSE,
			       "y-fill", FALSE,		
			       "x-fill", TRUE,			       			       
                               NULL);

/*	div = clutter_texture_new_from_file (SINGLE_DIV_LINE, NULL);
	mx_box_layout_add_actor (MX_BOX_LAYOUT(world_clock), div, -1);		
*/
}

static void
time_changed (MnpClockArea *area, MnpWorldClock *clock)
{
	g_signal_emit (clock, signals[TIME_CHANGED], 0);
}

static void
mnp_world_clock_construct (MnpWorldClock *world_clock)
{
	ClutterActor *entry, *box, *stage, *div;
	MxBoxLayout *table = (MxBoxLayout *)world_clock;
	gfloat width, height;
	MnpWorldClockPrivate *priv = GET_PRIVATE (world_clock);

	stage = clutter_stage_get_default ();

	mx_box_layout_set_orientation ((MxBoxLayout *)world_clock, MX_ORIENTATION_VERTICAL);
	mx_box_layout_set_spacing ((MxBoxLayout *)world_clock, 0);
	clutter_actor_set_name (CLUTTER_ACTOR(world_clock), "TimePane");

	construct_heading_and_top_area (world_clock);

	priv->completion_timeout = 0;

	/* Search Entry */

	box = mx_box_layout_new ();
	mx_box_layout_set_enable_animations ((MxBoxLayout *)box, TRUE);
	mx_box_layout_set_orientation ((MxBoxLayout *)box, MX_ORIENTATION_HORIZONTAL);
	mx_box_layout_set_spacing ((MxBoxLayout *)box, 4);

	priv->entry_box = box;
	mx_stylable_set_style_class (MX_STYLABLE(box), "ZoneSearchEntryBox");
	
	entry = mx_entry_new ();
	mx_stylable_set_style_class (MX_STYLABLE (entry), "ZoneSearchEntry");

	priv->search_location = (MxEntry *)entry;
	mx_entry_set_hint_text (MX_ENTRY (entry), _("Enter a country or city"));
	mx_entry_set_secondary_icon_from_file (MX_ENTRY (entry),
                                           THEMEDIR"/edit-clear.png");
  	g_signal_connect (entry, "secondary-icon-clicked",
                    	G_CALLBACK (clear_btn_clicked_cb), world_clock);

	g_signal_connect (G_OBJECT (entry),
                    "notify::text", G_CALLBACK (text_changed_cb), world_clock);
	
	clutter_actor_get_size (entry, &width, &height);
	clutter_actor_set_size (entry, width+10, -1);

	mx_box_layout_add_actor ((MxBoxLayout *)box, entry, 0);
	clutter_container_child_set (CLUTTER_CONTAINER (box),
                               entry,
                               "expand", FALSE,
			       "y-fill", FALSE,
			       "x-fill", FALSE,
			       "y-align", MX_ALIGN_MIDDLE,
			       "x-align", MX_ALIGN_START,
                               NULL);

	priv->add_location = mx_button_new ();
	mx_button_set_label ((MxButton *)priv->add_location, _("Add"));
	mx_stylable_set_style_class (MX_STYLABLE(priv->add_location), "ZoneSearchEntryAddButton");
	
	/* mx_box_layout_add_actor ((MxBoxLayout *)box, priv->add_location, 1); */
  	/* g_signal_connect (priv->add_location, "clicked",
                    	G_CALLBACK (add_location_clicked_cb), world_clock); */
	/*clutter_container_child_set (CLUTTER_CONTAINER (box),
                               priv->add_location,
                               "expand", FALSE,
			       "y-fill", FALSE,
			       "y-align", MX_ALIGN_MIDDLE,
                               NULL);*/
	

	mx_box_layout_add_actor (MX_BOX_LAYOUT(priv->widget_box), box, -1);
	clutter_container_child_set (CLUTTER_CONTAINER (priv->widget_box),
                               box,
                               "expand", FALSE,
			       "y-fill", FALSE,		
			       "x-fill", FALSE,
			       "x-align",  MX_ALIGN_START,
                               NULL);

	/* Clock Area */

	priv->area = mnp_clock_area_new ();
	g_signal_connect (priv->area, "time-changed", G_CALLBACK(time_changed), world_clock);
	clutter_actor_set_size ((ClutterActor *)priv->area, 300, -1);

	clutter_actor_set_reactive ((ClutterActor *)priv->area, TRUE);
	clutter_actor_set_name ((ClutterActor *)priv->area, "WorldClockArea");

	clutter_container_add_actor ((ClutterContainer *)stage, (ClutterActor *)priv->area);
	
	clutter_actor_lower_bottom ((ClutterActor *)priv->area);
	mx_droppable_enable ((MxDroppable *)priv->area);
	g_object_ref ((GObject *)priv->area);
	clutter_container_remove_actor ((ClutterContainer *)stage, (ClutterActor *)priv->area);
	mx_box_layout_add_actor ((MxBoxLayout *) table, (ClutterActor *)priv->area, -1);
	clutter_container_child_set (CLUTTER_CONTAINER (table),
                               (ClutterActor *)priv->area,
                               "expand", TRUE,
			       "y-fill", TRUE,
			       "x-fill", TRUE,
                               NULL);


	priv->zones = mnp_load_zones ();
	mnp_clock_area_refresh_time (priv->area, TRUE);
	mnp_clock_area_set_zone_remove_cb (priv->area, (ZoneRemovedFunc) zone_removed_cb, (gpointer)world_clock);
	mnp_clock_area_set_zone_reordered_cb (priv->area, (ClockZoneReorderedFunc) zone_reordered_cb, (gpointer)world_clock);

	if (priv->zones->len) {
		int i=0;

		for (i=0; i<priv->zones->len; i++) {
			MnpZoneLocation *loc = (MnpZoneLocation *)priv->zones->pdata[i];
			MnpClockTile *tile = mnp_clock_tile_new (loc, mnp_clock_area_get_time(priv->area));
			mnp_clock_area_add_tile (priv->area, tile);
		}
		if (priv->zones->len >= 4)
			clutter_actor_hide (priv->entry_box);
	}
	
/*	div = clutter_texture_new_from_file (SINGLE_DIV_LINE, NULL);
	mx_box_layout_add_actor (MX_BOX_LAYOUT(world_clock), div, -1);		
*/
	box = mx_box_layout_new ();
	clutter_actor_set_name (box, "DateTimeLauncherBox");
	priv->launcher_box = box;
	mx_box_layout_set_orientation ((MxBoxLayout *)box, MX_ORIENTATION_VERTICAL);
	mx_box_layout_set_spacing ((MxBoxLayout *)box, 6);
	
	priv->launcher = mx_button_new ();
	mx_button_set_label ((MxButton *) priv->launcher, _("Set Time & Date"));
	mx_stylable_set_style_class (MX_STYLABLE(priv->launcher), "DateTimeLauncherButton");

	mx_box_layout_add_actor ((MxBoxLayout *)box, priv->launcher, -1);
	clutter_container_child_set (CLUTTER_CONTAINER (box),
                               (ClutterActor *)priv->launcher,
                               "expand", FALSE,
			       "y-fill", FALSE,
			       "x-fill", FALSE,
			       "x-align", MX_ALIGN_END,
                               NULL);


	mx_box_layout_add_actor ((MxBoxLayout *)world_clock, box, -1);
}

ClutterActor *
mnp_world_clock_new (void)
{
  MnpWorldClock *panel = g_object_new (MNP_TYPE_WORLD_CLOCK, NULL);

  mnp_world_clock_construct (panel);

  return (ClutterActor *)panel;
}

static void
launch_date_config (ClutterActor *actor, MnpWorldClock *clock)
{
  	MnpWorldClockPrivate *priv = GET_PRIVATE (clock);

	if (priv->panel_client) {
		mpl_panel_client_hide(priv->panel_client);
		mpl_panel_client_launch_application (priv->panel_client, "/usr/bin/gnome-control-center system-config-date.desktop");
	}
}

static void
dropdown_show_cb (MplPanelClient *client,
                  gpointer        userdata)
{
  MnpWorldClockPrivate *priv = GET_PRIVATE (userdata); 

  mnp_clock_area_manual_update(priv->area);

}


static void
dropdown_hide_cb (MplPanelClient *client,
                  MnpWorldClock   *world_clock)
{
  MnpWorldClockPrivate *priv = GET_PRIVATE (world_clock);

  mx_entry_set_text (priv->search_location, "");
  clutter_actor_hide (priv->completion);	
}

void
mnp_world_clock_set_client (MnpWorldClock *clock, MplPanelClient *client)
{
  MnpWorldClockPrivate *priv = GET_PRIVATE (clock);

  priv->panel_client = client;
  g_signal_connect (priv->launcher, 
		    "clicked", 
		    G_CALLBACK(launch_date_config), 
		    clock);

  g_signal_connect (client,
                    "show-end",
                    (GCallback)dropdown_show_cb,
                    clock);

  g_signal_connect (client,
                    "hide-end",
                    (GCallback)dropdown_hide_cb,
                    clock);
}

