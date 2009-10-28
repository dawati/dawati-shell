/*
 * Copyright (C) 2008 - 2009 Intel Corporation.
 *
 * Author: Rob Bradford <rob@linux.intel.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU Lesser General Public License,
 * version 2.1, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
 * more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
 */


#include "penge-date-tile.h"

#include <libjana/jana.h>

G_DEFINE_TYPE (PengeDateTile, penge_date_tile, NBTK_TYPE_TABLE)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), PENGE_TYPE_DATE_TILE, PengeDateTilePrivate))

typedef struct _PengeDateTilePrivate PengeDateTilePrivate;

struct _PengeDateTilePrivate {
    JanaTime *time;
    NbtkWidget *day_label;
    NbtkWidget *date_label;
};

enum
{
  PROP_0,
  PROP_TIME
};

static void penge_date_tile_update (PengeDateTile *tile);

static void
penge_date_tile_get_property (GObject *object, guint property_id,
                              GValue *value, GParamSpec *pspec)
{
  PengeDateTilePrivate *priv = GET_PRIVATE (object);

  switch (property_id) {
    case PROP_TIME:
      g_value_set_object (value, priv->time);
      break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
penge_date_tile_set_property (GObject *object, guint property_id,
                              const GValue *value, GParamSpec *pspec)
{
  PengeDateTilePrivate *priv = GET_PRIVATE (object);

  switch (property_id) {
    case PROP_TIME:
      if (priv->time)
        g_object_unref (priv->time);
      priv->time = g_value_dup_object (value);

      penge_date_tile_update ((PengeDateTile *)object);
      break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
penge_date_tile_dispose (GObject *object)
{
  PengeDateTilePrivate *priv = GET_PRIVATE (object);

  if (priv->time)
  {
    g_object_unref (priv->time);
    priv->time = NULL;
  }

  G_OBJECT_CLASS (penge_date_tile_parent_class)->dispose (object);
}

static void
penge_date_tile_finalize (GObject *object)
{
  G_OBJECT_CLASS (penge_date_tile_parent_class)->finalize (object);
}

static void
penge_date_tile_class_init (PengeDateTileClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GParamSpec *pspec;

  g_type_class_add_private (klass, sizeof (PengeDateTilePrivate));

  object_class->get_property = penge_date_tile_get_property;
  object_class->set_property = penge_date_tile_set_property;
  object_class->dispose = penge_date_tile_dispose;
  object_class->finalize = penge_date_tile_finalize;

  pspec = g_param_spec_object ("time",
                               "Time to show",
                               "The time object to show the day and date for.",
                               JANA_TYPE_TIME,
                               G_PARAM_READWRITE);
  g_object_class_install_property (object_class, PROP_TIME, pspec);
}

static void
penge_date_tile_init (PengeDateTile *self)
{
  PengeDateTilePrivate *priv = GET_PRIVATE (self);

  priv->day_label = nbtk_label_new ("Day");
  nbtk_widget_set_style_class_name (priv->day_label,
                                    "PengeDayLabel");
  priv->date_label = nbtk_label_new ("XX");
  nbtk_widget_set_style_class_name (priv->date_label,
                                    "PengeDateLabel");


  nbtk_table_add_actor (NBTK_TABLE (self),
                        (ClutterActor *)priv->day_label,
                        0,
                        0);
  nbtk_table_add_actor (NBTK_TABLE (self),
                        (ClutterActor *)priv->date_label,
                        1,
                        0);
  clutter_container_child_set (CLUTTER_CONTAINER (self),
                               (ClutterActor *)priv->date_label,
                               "y-expand",
                               TRUE,
                               "x-expand",
                               TRUE,
                               NULL);

  clutter_actor_set_reactive ((ClutterActor *)self, TRUE);
}

static void
penge_date_tile_update (PengeDateTile *tile)
{
  PengeDateTilePrivate *priv = GET_PRIVATE (tile);
  gchar *tmp_str;

  g_return_if_fail (tile != NULL);

  tmp_str = jana_utils_strftime (priv->time, "%e");
  tmp_str = g_strstrip(tmp_str);
  nbtk_label_set_text (NBTK_LABEL (priv->date_label), tmp_str);
  g_free (tmp_str);

  tmp_str = jana_utils_strftime (priv->time, "%A");
  nbtk_label_set_text (NBTK_LABEL (priv->day_label), tmp_str);
  g_free (tmp_str);
}
