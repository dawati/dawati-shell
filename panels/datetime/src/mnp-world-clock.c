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

#include <gtk/gtk.h>
#include <gio/gdesktopappinfo.h>
#include <glib/gi18n.h>


#include <moblin-panel/mpl-panel-clutter.h>
#include <moblin-panel/mpl-panel-common.h>
#include <moblin-panel/mpl-entry.h>

#include "mnp-world-clock.h"
#include "mnp-utils.h"
#include "mnp-button-item.h"

#include "mnp-clock-tile.h"
#include "mnp-clock-area.h"


G_DEFINE_TYPE (MnpWorldClock, mnp_world_clock, MX_TYPE_TABLE)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MNP_TYPE_WORLD_CLOCK, MnpWorldClockPrivate))

#define TIMEOUT 250


typedef struct _MnpWorldClockPrivate MnpWorldClockPrivate;

struct _MnpWorldClockPrivate {
	MplPanelClient *panel_client;

	MxEntry *search_location;
	MxListView *zones_list;
	ClutterModel *zones_model;
	ClutterActor *scroll;

	ClutterActor *add_location;

	const char *search_text;

	MnpClockArea *area;

	GPtrArray *zones;
};

static void
mnp_world_clock_dispose (GObject *object)
{
  MnpWorldClockPrivate *priv = GET_PRIVATE (object);

  if (priv->panel_client)
  {
    g_object_unref (priv->panel_client);
    priv->panel_client = NULL;
  }

  G_OBJECT_CLASS (mnp_world_clock_parent_class)->dispose (object);
}

static void
mnp_world_clock_finalize (GObject *object)
{
  MnpWorldClockPrivate *priv = GET_PRIVATE (object);

  G_OBJECT_CLASS (mnp_world_clock_parent_class)->finalize (object);
}

static void
mnp_world_clock_class_init (MnpWorldClockClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MnpWorldClockPrivate));

  object_class->dispose = mnp_world_clock_dispose;
  object_class->finalize = mnp_world_clock_finalize;
}


static void
mnp_world_clock_init (MnpWorldClock *self)
{
  MnpWorldClockPrivate *priv = GET_PRIVATE (self);

  priv->search_text = "";
  priv->zones = NULL;
}

static void
text_changed_cb (MxEntry *entry, GParamSpec *pspec, void *user_data)
{
	MnpWorldClockPrivate *priv = GET_PRIVATE (user_data);

	priv->search_text = mx_entry_get_text (entry);
	g_signal_emit_by_name (priv->zones_model, "filter-changed");
	if (priv->search_text && *priv->search_text)
		clutter_actor_show(priv->scroll);
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
	char *key;

	if (!priv->search_text || !*priv->search_text)
		return TRUE;

	key = g_ascii_strdown(priv->search_text, -1);
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
			g_free (key);
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
	g_free (key);
	return match;
}

static void
clear_btn_clicked_cb (ClutterActor *button, MnpWorldClock *world_clock)
{
	MnpWorldClockPrivate *priv = GET_PRIVATE (world_clock);

  	mx_entry_set_text (priv->search_location, "");
  	clutter_actor_hide (priv->scroll);
}

static void
add_location_clicked_cb (ClutterActor *button, MnpWorldClock *world_clock)
{
	MnpWorldClockPrivate *priv = GET_PRIVATE (world_clock);
	const GWeatherLocation *location;
	GWeatherTimezone *zone;
	MnpClockTile *tile;

	priv->search_text = NULL;
	g_signal_emit_by_name (priv->zones_model, "filter-changed");
	
	location = mnp_utils_get_location_from_display (priv->zones_model, mx_entry_get_text (priv->search_location));
	g_ptr_array_add (priv->zones, g_strdup(mx_entry_get_text (priv->search_location)));
	mnp_save_zones(priv->zones);

	mx_entry_set_text (priv->search_location, "");

	tile = mnp_clock_tile_new (location);
	mnp_clock_area_add_tile (priv->area, tile);
}

static void 
mnp_completion_done (gpointer data, const char *zone)
{
	MnpWorldClock *clock = (MnpWorldClock *)data;
	MnpWorldClockPrivate *priv = GET_PRIVATE (clock);
	
	clutter_actor_hide (priv->scroll);
	g_signal_handlers_block_by_func (priv->search_location, text_changed_cb, clock);
	mx_entry_set_text (priv->search_location, zone);
	g_signal_handlers_unblock_by_func (priv->search_location, text_changed_cb, clock);

}

static void
mnp_world_clock_construct (MnpWorldClock *world_clock)
{
	ClutterActor *entry, *scroll, *view, *box, *stage;
	MxTable *table = (MxTable *)world_clock;
	gfloat width, height;
	ClutterModel *model;
	MnpWorldClockPrivate *priv = GET_PRIVATE (world_clock);
	MnpButtonItem *button_item;

	stage = clutter_stage_get_default ();

	mx_table_set_col_spacing (MX_TABLE (table), 1);
	mx_table_set_row_spacing (MX_TABLE (table), 1);

	box = mx_box_layout_new ();
	mx_box_layout_set_vertical ((MxBoxLayout *)box, FALSE);
	mx_box_layout_set_pack_start ((MxBoxLayout *)box, FALSE);

	entry = mx_entry_new ("");
	priv->search_location = (MxEntry *)entry;
	mx_entry_set_hint_text (MX_ENTRY (entry), _("Enter a country or city"));
	mx_entry_set_secondary_icon_from_file (MX_ENTRY (entry),
                                           THEMEDIR"/edit-clear.png");
  	g_signal_connect (entry, "secondary-icon-clicked",
                    	G_CALLBACK (clear_btn_clicked_cb), world_clock);

	/* FIXME: Don't exceed beyond parent */
	clutter_actor_get_size (entry, &width, &height);
	clutter_actor_set_size (entry, width+10, -1);
	g_signal_connect (G_OBJECT (entry),
                    "notify::text", G_CALLBACK (text_changed_cb), world_clock);
	
	clutter_container_add_actor ((ClutterContainer *)box, entry);
	
	priv->add_location = mx_button_new ();
	mx_button_set_label ((MxButton *)priv->add_location, _("Add"));
	clutter_container_add_actor ((ClutterContainer *)box, priv->add_location);
  	g_signal_connect (priv->add_location, "clicked",
                    	G_CALLBACK (add_location_clicked_cb), world_clock);

	mx_table_add_actor (MX_TABLE (table), box, 0, 0);
	clutter_container_child_set (CLUTTER_CONTAINER (table),
                               box,
                               "x-expand", FALSE, "y-expand", FALSE,
                               NULL);

	clutter_actor_set_position ((ClutterActor *)table, 6, 5);
	model = mnp_get_world_timezones ();
	priv->zones_model = model;
	clutter_model_set_filter (model, filter_zone, world_clock, NULL);

	scroll = mx_scroll_view_new ();
	clutter_actor_set_name (scroll, "completion-scroll-bin");
	priv->scroll = scroll;
	clutter_actor_set_size (scroll, width-20, -1);	
	mx_table_add_actor (MX_TABLE (table), scroll, 1, 0);
	clutter_container_child_set (CLUTTER_CONTAINER (table),
                               scroll,
                               "x-expand", FALSE, "y-expand", FALSE,
                               NULL);
	clutter_actor_hide (scroll);

	view = mx_list_view_new ();
	clutter_actor_set_name (view, "completion-list-view");
	priv->zones_list = (MxListView *)view;

	clutter_container_add_actor (CLUTTER_CONTAINER (scroll), view);
	mx_list_view_set_model (MX_LIST_VIEW (view), model);

	button_item = mnp_button_item_new ((gpointer)world_clock, mnp_completion_done);
	mx_list_view_set_factory (MX_LIST_VIEW (view), (MxItemFactory *)button_item);
	mx_list_view_add_attribute (MX_LIST_VIEW (view), "label", 0);

	priv->area = mnp_clock_area_new ();
	clutter_actor_get_size (box, &width, &height);

	clutter_actor_set_size (priv->area, width+20, 500);
	clutter_actor_set_position (priv->area, 6, 50);  
	clutter_actor_set_reactive (priv->area, TRUE);
	clutter_actor_set_name (priv->area, "clock-area");

	clutter_container_add_actor (stage, priv->area);
	clutter_actor_lower_bottom (priv->area);
	mx_droppable_enable (priv->area);

	priv->zones = mnp_load_zones ();
	if (priv->zones->len) {
		int i=0;

		for (i=0; i<priv->zones->len; i++) {
			GWeatherLocation *location = mnp_utils_get_location_from_display (priv->zones_model, priv->zones->pdata[i]);
			MnpClockTile *tile = mnp_clock_tile_new (location);
			mnp_clock_area_add_tile (priv->area, tile);
		}
	}
}

ClutterActor *
mnp_world_clock_new (void)
{
  MnpWorldClock *panel = g_object_new (MNP_TYPE_WORLD_CLOCK, NULL);

  mnp_world_clock_construct (panel);

  return (ClutterActor *)panel;
}

static void
dropdown_show_cb (MplPanelClient *client,
                  gpointer        userdata)
{
  //MnpWorldClockPrivate *priv = GET_PRIVATE (userdata);

  /* give focus to the actor */
  /*clutter_actor_grab_key_focus (priv->entry);*/
}


static void
dropdown_hide_cb (MplPanelClient *client,
                  gpointer        userdata)
{
  //MnpWorldClockPrivate *priv = GET_PRIVATE (userdata);

  /* Reset search. */
  /* mpl_entry_set_text (MPL_ENTRY (priv->entry), ""); */
}

void
mnp_world_clock_set_panel_client (MnpWorldClock *world_clock,
                                   MplPanelClient *panel_client)
{
  MnpWorldClockPrivate *priv = GET_PRIVATE (world_clock);

  priv->panel_client = g_object_ref (panel_client);

  g_signal_connect (panel_client,
                    "show-end",
                    (GCallback)dropdown_show_cb,
                    world_clock);

  g_signal_connect (panel_client,
                    "hide-end",
                    (GCallback)dropdown_hide_cb,
                    world_clock);
}


