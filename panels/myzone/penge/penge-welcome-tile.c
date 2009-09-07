/*
 * Copyright (C) 2008 - 2009 Intel Corporation.
 *
 * Author: Rob Bradford <rob@linux.intel.com>
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

#include <glib/gi18n.h>

#include "penge-welcome-tile.h"

G_DEFINE_TYPE (PengeWelcomeTile, penge_welcome_tile, NBTK_TYPE_TABLE)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), PENGE_TYPE_WELCOME_TILE, PengeWelcomeTilePrivate))

#define TILE_WIDTH 170.0f
#define TILE_HEIGHT 115.0f

typedef struct _PengeWelcomeTilePrivate PengeWelcomeTilePrivate;

struct _PengeWelcomeTilePrivate {
  gpointer dummy;
};

static void
penge_welcome_tile_dispose (GObject *object)
{
  G_OBJECT_CLASS (penge_welcome_tile_parent_class)->dispose (object);
}

static void
penge_welcome_tile_finalize (GObject *object)
{
  G_OBJECT_CLASS (penge_welcome_tile_parent_class)->finalize (object);
}

/* Return reasonable tile size. If we don't do this then the huge potential
 * size of the text gives strange results with table.
 */
static void
penge_welcome_tile_get_preferred_width (ClutterActor *self,
                                        gfloat        for_height,
                                        gfloat       *min_width_p,
                                        gfloat       *natural_width_p)
{
  if (min_width_p)
    *min_width_p = TILE_WIDTH * 2;

  if (natural_width_p)
    *natural_width_p = TILE_WIDTH * 2;
}

static void
penge_welcome_tile_class_init (PengeWelcomeTileClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  ClutterActorClass *actor_class = CLUTTER_ACTOR_CLASS (klass);

  g_type_class_add_private (klass, sizeof (PengeWelcomeTilePrivate));

  object_class->dispose = penge_welcome_tile_dispose;
  object_class->finalize = penge_welcome_tile_finalize;

  actor_class->get_preferred_width = penge_welcome_tile_get_preferred_width;
}

static void
penge_welcome_tile_init (PengeWelcomeTile *tile)
{
  NbtkWidget *label;
  ClutterActor *tmp_text;

  nbtk_widget_set_style_class_name ((NbtkWidget *)tile, "PengeWelcomeTile");

  label = nbtk_label_new (_("<b>Welcome to Moblin 2.1 for Netbooks</b>"));
  clutter_actor_set_name ((ClutterActor *)label,
                          "penge-welcome-primary-text");
  tmp_text = nbtk_label_get_clutter_text (NBTK_LABEL (label));
  clutter_text_set_line_wrap (CLUTTER_TEXT (tmp_text), TRUE);
  clutter_text_set_line_wrap_mode (CLUTTER_TEXT (tmp_text),
                                   PANGO_WRAP_WORD_CHAR);
  clutter_text_set_use_markup (CLUTTER_TEXT (tmp_text),
                               TRUE);
  clutter_text_set_ellipsize (CLUTTER_TEXT (tmp_text),
                              PANGO_ELLIPSIZE_NONE);
  nbtk_table_add_actor_with_properties (NBTK_TABLE (tile),
                                        (ClutterActor *)label,
                                        0, 0,
                                        "x-expand", TRUE,
                                        "x-fill", TRUE,
                                        "y-expand", TRUE,
                                        "y-fill", TRUE,
                                        NULL);

  label = nbtk_label_new (_("As Moblin is a bit different to other computers, " \
                            "we've put together a couple of bits and pieces to " \
                            "help you find your way around. " \
                            "We hope you enjoy it, The Moblin Team."));
  clutter_actor_set_name ((ClutterActor *)label,
                          "penge-welcome-secondary-text");
  tmp_text = nbtk_label_get_clutter_text (NBTK_LABEL (label));
  clutter_text_set_line_wrap (CLUTTER_TEXT (tmp_text), TRUE);
  clutter_text_set_line_wrap_mode (CLUTTER_TEXT (tmp_text),
                                   PANGO_WRAP_WORD_CHAR);
  clutter_text_set_ellipsize (CLUTTER_TEXT (tmp_text),
                              PANGO_ELLIPSIZE_NONE);
  nbtk_table_add_actor_with_properties (NBTK_TABLE (tile),
                                        (ClutterActor *)label,
                                        1, 0,
                                        "x-expand", TRUE,
                                        "x-fill", TRUE,
                                        "y-expand", TRUE,
                                        "y-fill", TRUE,
                                        NULL);
}

ClutterActor *
penge_welcome_tile_new (void)
{
  return g_object_new (PENGE_TYPE_WELCOME_TILE, NULL);
}


