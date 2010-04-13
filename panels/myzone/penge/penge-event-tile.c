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


#include "penge-event-tile.h"
#include "penge-utils.h"

#include <libjana/jana.h>
#include <libjana-ecal/jana-ecal.h>

#include <string.h>

G_DEFINE_TYPE (PengeEventTile, penge_event_tile, MX_TYPE_BUTTON)

#define GET_PRIVATE_REAL(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), PENGE_TYPE_EVENT_TILE, PengeEventTilePrivate))


#define GET_PRIVATE(o) ((PengeEventTile *)o)->priv

struct _PengeEventTilePrivate {
  JanaEvent *event;
  JanaTime *time;
  JanaStore *store;

  ClutterActor *time_label;
  ClutterActor *summary_label;
  ClutterActor *calendar_indicator;

  ClutterActor *inner_table;

  ESource *source;
};

enum
{
  PROP_0,
  PROP_EVENT,
  PROP_TIME,
  PROP_STORE,
  PROP_MULTILINE_SUMMARY
};

static void penge_event_tile_update (PengeEventTile *tile);
static void penge_event_tile_set_store (PengeEventTile *tile,
                                        JanaStore      *store);

static void _source_changed_cb (ESource  *source,
                                gpointer  userdata);

static void
penge_event_tile_get_property (GObject *object, guint property_id,
                              GValue *value, GParamSpec *pspec)
{
  PengeEventTilePrivate *priv = GET_PRIVATE (object);

  switch (property_id) {
    case PROP_EVENT:
      g_value_set_object (value, priv->event);
      break;
    case PROP_TIME:
      g_value_set_object (value, priv->time);
      break;
    case PROP_STORE:
      g_value_set_object (value, priv->store);
      break;
    case PROP_MULTILINE_SUMMARY:
      {
        ClutterActor *tmp_text;

        tmp_text = mx_label_get_clutter_text (MX_LABEL (priv->summary_label));

        g_value_set_boolean (value,
                             !clutter_text_get_single_line_mode (CLUTTER_TEXT (tmp_text)));
      }
      break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
penge_event_tile_set_property (GObject *object, guint property_id,
                              const GValue *value, GParamSpec *pspec)
{
  PengeEventTilePrivate *priv = GET_PRIVATE (object);

  switch (property_id) {
    case PROP_EVENT:
      if (priv->event)
        g_object_unref (priv->event);

      priv->event = g_value_dup_object (value);

      penge_event_tile_update ((PengeEventTile *)object);
      break;
    case PROP_TIME:
      if (priv->time)
        g_object_unref (priv->time);

      priv->time = g_value_dup_object (value);

      penge_event_tile_update ((PengeEventTile *)object);
      break;
    case PROP_STORE:
      penge_event_tile_set_store ((PengeEventTile *)object,
                                  g_value_get_object (value));
      break;
    case PROP_MULTILINE_SUMMARY:
      {
        ClutterActor *tmp_text;

        tmp_text = mx_label_get_clutter_text (MX_LABEL (priv->summary_label));
        clutter_text_set_single_line_mode (CLUTTER_TEXT (tmp_text),
                                           !g_value_get_boolean (value));
        clutter_text_set_line_wrap (CLUTTER_TEXT (tmp_text),
                                    g_value_get_boolean (value));
      }
      break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
penge_event_tile_dispose (GObject *object)
{
  PengeEventTilePrivate *priv = GET_PRIVATE (object);

  if (priv->source)
  {
    g_signal_handlers_disconnect_by_func (priv->source,
                                          _source_changed_cb,
                                          object);
    g_object_unref (priv->source);
    priv->source = NULL;
  }

  if (priv->event)
  {
    g_object_unref (priv->event);
    priv->event = NULL;
  }

  if (priv->time)
  {
    g_object_unref (priv->time);
    priv->time = NULL;
  }

  if (priv->store)
  {
    g_object_unref (priv->store);
    priv->store = NULL;
  }

  G_OBJECT_CLASS (penge_event_tile_parent_class)->dispose (object);
}

static void
penge_event_tile_finalize (GObject *object)
{
  G_OBJECT_CLASS (penge_event_tile_parent_class)->finalize (object);
}

static void
penge_event_tile_class_init (PengeEventTileClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GParamSpec *pspec;

  g_type_class_add_private (klass, sizeof (PengeEventTilePrivate));

  object_class->get_property = penge_event_tile_get_property;
  object_class->set_property = penge_event_tile_set_property;
  object_class->dispose = penge_event_tile_dispose;
  object_class->finalize = penge_event_tile_finalize;

  pspec = g_param_spec_object ("event",
                               "The event",
                               "The event to show details of",
                               JANA_TYPE_EVENT,
                               G_PARAM_READWRITE);
  g_object_class_install_property (object_class, PROP_EVENT, pspec);

  pspec = g_param_spec_object ("time",
                               "The time now",
                               "The time now",
                               JANA_TYPE_TIME,
                               G_PARAM_READWRITE);
  g_object_class_install_property (object_class, PROP_TIME, pspec);

  pspec = g_param_spec_object ("store",
                               "The store.",
                               "The store this event came from.",
                               JANA_ECAL_TYPE_STORE,
                               G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
  g_object_class_install_property (object_class, PROP_STORE, pspec);

  pspec = g_param_spec_boolean ("multiline-summary",
                                "Multiple line summary",
                                "Whether the summary should be multiple lines",
                                FALSE,
                                G_PARAM_READWRITE);
  g_object_class_install_property (object_class, PROP_MULTILINE_SUMMARY, pspec);
}

static void
_button_clicked_cb (MxButton *button,
                    gpointer    userdata)
{
  PengeEventTilePrivate *priv = GET_PRIVATE (userdata);
  ECal *ecal;
  gchar *uid;
  gchar *command_line;

  g_object_get (priv->store, "ecal", &ecal, NULL);
  uid = jana_component_get_uid ((JanaComponent *)priv->event);

  command_line = g_strdup_printf ("evolution \"calendar:///?source-uid=%s&comp-uid=%s\"",
                                  e_source_peek_uid (priv->source),
                                  uid);
  g_free (uid);

  if (!penge_utils_launch_by_command_line ((ClutterActor *)button,
                                           command_line))
  {
    g_warning (G_STRLOC ": Error starting dates");
  } else{
    penge_utils_signal_activated ((ClutterActor *)userdata);
  }
}

static void
penge_event_tile_init (PengeEventTile *self)
{
  PengeEventTilePrivate *priv = GET_PRIVATE_REAL (self);
  ClutterActor *tmp_text;

  self->priv = priv;

  priv->inner_table = mx_table_new ();
  mx_bin_set_child (MX_BIN (self), (ClutterActor *)priv->inner_table);
  mx_bin_set_fill (MX_BIN (self), TRUE, TRUE);

  priv->calendar_indicator = clutter_cairo_texture_new (20, 20);
  clutter_actor_set_size (priv->calendar_indicator, 20, 20);

  priv->time_label = mx_label_new ();
  tmp_text = mx_label_get_clutter_text (MX_LABEL (priv->time_label));
  clutter_text_set_ellipsize (CLUTTER_TEXT (tmp_text), PANGO_ELLIPSIZE_NONE);
  mx_stylable_set_style_class (MX_STYLABLE (priv->time_label),
                               "PengeEventTimeLabel");

  priv->summary_label = mx_label_new ();
  mx_stylable_set_style_class (MX_STYLABLE (priv->summary_label),
                               "PengeEventSummary");
  tmp_text = mx_label_get_clutter_text (MX_LABEL (priv->summary_label));
  clutter_text_set_ellipsize (CLUTTER_TEXT (tmp_text), PANGO_ELLIPSIZE_END);
  clutter_text_set_single_line_mode (CLUTTER_TEXT (tmp_text), TRUE);

  /* Populate the table */
  mx_table_add_actor (MX_TABLE (priv->inner_table),
                      priv->calendar_indicator,
                      0,
                      0);
  mx_table_add_actor (MX_TABLE (priv->inner_table),
                      priv->time_label,
                      0,
                      1);
  mx_table_add_actor (MX_TABLE (priv->inner_table),
                      priv->summary_label,
                      0,
                      2);

  clutter_container_child_set (CLUTTER_CONTAINER (priv->inner_table),
                               priv->calendar_indicator,
                               "x-expand", FALSE,
                               "x-fill", FALSE,
                               "y-expand", FALSE,
                               "y-fill", FALSE,
                               NULL);
  clutter_container_child_set (CLUTTER_CONTAINER (priv->inner_table),
                               priv->time_label,
                               "x-expand", FALSE,
                               "x-fill", FALSE,
                               "y-expand", FALSE,
                               "y-fill", FALSE,
                               NULL);
  clutter_container_child_set (CLUTTER_CONTAINER (priv->inner_table),
                               priv->summary_label,
                               "x-expand", TRUE,
                               "x-fill", TRUE,
                               "y-expand", FALSE,
                               "y-fill", FALSE,
                               NULL);


  /* Setup spacing and padding */
  mx_table_set_column_spacing (MX_TABLE (priv->inner_table), 6);

  g_signal_connect (self,
                    "clicked",
                    (GCallback)_button_clicked_cb,
                    self);

  clutter_actor_set_reactive (CLUTTER_ACTOR (self), TRUE);
}

static void
penge_event_tile_update (PengeEventTile *tile)
{
  PengeEventTilePrivate *priv = GET_PRIVATE (tile);
  gchar *time_str;
  gchar *summary_str;
  JanaTime *t;
  gchar *p;

  if (!priv->event)
    return;

  if (priv->time)
  {
    t = jana_event_get_start (priv->event);

    /* Translate this time into local time */
    jana_time_set_offset (t, jana_time_get_offset (priv->time));

    if (jana_time_get_day (priv->time) != jana_time_get_day (t))
    {
      gchar *tmp_str;
      gchar *day_str;
      ClutterActor *tmp_text;

      tmp_str = jana_utils_strftime (t, "%a");
      day_str = g_utf8_strup (tmp_str, -1);
      g_free (tmp_str);

      time_str = jana_utils_strftime (t, "%H:%M");
      tmp_str = g_strdup_printf ("%s <span size=\"xx-small\">%s</span>",
                                 time_str,
                                 day_str);
      g_free (time_str);
      g_free (day_str);

      tmp_text = mx_label_get_clutter_text (MX_LABEL (priv->time_label));
      clutter_text_set_markup (CLUTTER_TEXT (tmp_text), tmp_str);
      g_free (tmp_str);
    } else {
      time_str = jana_utils_strftime (t, "%H:%M");
      mx_label_set_text (MX_LABEL (priv->time_label), time_str);
      g_free (time_str);
    }

    if (jana_time_get_day (priv->time) != jana_time_get_day (t))
    {
      mx_stylable_set_style_pseudo_class (MX_STYLABLE (priv->time_label),
                                          "past");
    } else {
      mx_stylable_set_style_pseudo_class (MX_STYLABLE (priv->time_label),
                                          NULL);
    }

    g_object_unref (t);
  }

  summary_str = jana_event_get_summary (priv->event);
  if (summary_str)
  {
    /* this hack is courtesy of Chris Lord, we look for a new line character
     * and if we find it replace it with \0. We need this because otherwise
     * new lines in our labels look funn
     */
    p = strchr (summary_str, '\n');
    if (p)
      *p = '\0';
    mx_label_set_text (MX_LABEL (priv->summary_label), summary_str);
    g_free (summary_str);
  } else {
    mx_label_set_text (MX_LABEL (priv->summary_label), "");
  }
}

gchar *
penge_event_tile_get_uid (PengeEventTile *tile)
{
  PengeEventTilePrivate *priv = GET_PRIVATE (tile);

  return jana_component_get_uid (JANA_COMPONENT (priv->event));
}

/* Copied from Clutter and modified to support weird 16 bit */
static gboolean
___clutter_color_from_string (ClutterColor *color,
                              const gchar  *str)
{
  PangoColor pango_color = { 0, };

  g_return_val_if_fail (color != NULL, FALSE);
  g_return_val_if_fail (str != NULL, FALSE);

  /* if the string contains a color encoded using the hexadecimal
   * notations (#rrggbbaa or #rgba) we attempt a rough pass at
   * parsing the color ourselves, as we need the alpha channel that
   * Pango can't retrieve.
   */
  if (str[0] == '#')
    {
      gint64 result;

      if (sscanf (str + 1, "%lx", &result))
        {
          gsize length = strlen (str);

          switch (length)
            {
            case 13: /* rrrrggggbbbb */
              color->red = ((result >> 40) & 0xff);
              color->green = ((result >> 24) & 0xff);
              color->blue = ((result >> 8) & 0xff);
              color->alpha = 0xff;

              return TRUE;

            case 9: /* rrggbbaa */
              color->red   = (result >> 24) & 0xff;
              color->green = (result >> 16) & 0xff;
              color->blue  = (result >>  8) & 0xff;

              color->alpha = result & 0xff;

              return TRUE;

            case 7: /* #rrggbb */
              color->red   = (result >> 16) & 0xff;
              color->green = (result >>  8) & 0xff;
              color->blue  = result & 0xff;

              color->alpha = 0xff;

              return TRUE;

            case 5: /* #rgba */
              color->red   = ((result >> 12) & 0xf);
              color->green = ((result >>  8) & 0xf);
              color->blue  = ((result >>  4) & 0xf);
              color->alpha = result & 0xf;

              color->red   = (color->red   << 4) | color->red;
              color->green = (color->green << 4) | color->green;
              color->blue  = (color->blue  << 4) | color->blue;
              color->alpha = (color->alpha << 4) | color->alpha;

              return TRUE;

            case 4: /* #rgb */
              color->red   = ((result >>  8) & 0xf);
              color->green = ((result >>  4) & 0xf);
              color->blue  = result & 0xf;

              color->red   = (color->red   << 4) | color->red;
              color->green = (color->green << 4) | color->green;
              color->blue  = (color->blue  << 4) | color->blue;

              color->alpha = 0xff;

              return TRUE;

            default:
              /* pass through to Pango */
              break;
            }
        }
    }
  
  /* Fall back to pango for named colors */
  if (pango_color_parse (&pango_color, str))
    {
      color->red   = pango_color.red;
      color->green = pango_color.green;
      color->blue  = pango_color.blue;

      color->alpha = 0xff;

      return TRUE;
    }

  return FALSE;
}

static void
__roundrect (cairo_t *cr,
             gdouble  x,
             gdouble  y,
             gdouble  radius,
             gdouble  size)
{
  cairo_move_to (cr, x, y + radius);
  cairo_arc (cr, x + radius, y + radius, radius, G_PI, G_PI + G_PI_2);
  cairo_arc (cr, x + size - radius, y + radius, radius, G_PI + G_PI_2, 2 * G_PI);
  cairo_arc (cr, x + size - radius, y + size - radius, radius, 0, G_PI_2);
  cairo_arc (cr, x + radius, y + size - radius, radius, G_PI_2, G_PI);
  cairo_close_path (cr);
}

static void
_update_calendar_indicator (PengeEventTile *tile)
{
  PengeEventTilePrivate *priv = GET_PRIVATE (tile);
  ClutterColor color, light_color;
  cairo_t *cr;
  cairo_pattern_t *pat;

  cr = clutter_cairo_texture_create (CLUTTER_CAIRO_TEXTURE (priv->calendar_indicator));
  ___clutter_color_from_string (&color, e_source_peek_color_spec (priv->source));

  clutter_color_lighten (&color, &light_color);

  __roundrect (cr, 0.5, 0.5, 3, 19);

  pat = cairo_pattern_create_linear (0, 3, 0, 16);
  cairo_pattern_add_color_stop_rgb (pat,
                                    0,
                                    light_color.red/255.0,
                                    light_color.green/255.0,
                                    light_color.blue/255.0);
  cairo_pattern_add_color_stop_rgb (pat,
                                    1,
                                    color.red/255.0,
                                    color.green/255.0,
                                    color.blue/255.0);

  cairo_set_source (cr, pat);
  cairo_fill (cr);
  cairo_pattern_destroy (pat);

  __roundrect (cr, 1.0, 1.0, 3, 18);

  clutter_color_lighten (&light_color, &color);

  cairo_set_line_width (cr, 1.5);
  cairo_set_source_rgb (cr, color.red/255.0, color.green/255.0, color.blue/255.0);
  cairo_stroke (cr);

  __roundrect (cr, 0.5, 0.5, 3, 19);

  cairo_set_line_width (cr, 1.0);
  cairo_set_source_rgb (cr, 0xad/255.0, 0xad/255.0, 0xad/255.0);
  cairo_stroke (cr);

  cairo_destroy(cr);
}

static void
_source_changed_cb (ESource *source,
                    gpointer userdata)
{
  PengeEventTile *tile = (PengeEventTile *)userdata;
  _update_calendar_indicator (tile);
}

static void
penge_event_tile_set_store (PengeEventTile *tile,
                            JanaStore      *store)
{
  PengeEventTilePrivate *priv = GET_PRIVATE (tile);
  ECal *ecal;

  priv->store = g_object_ref (store);

  g_object_get (store,
                "ecal", &ecal,
                NULL);

  priv->source = g_object_ref (e_cal_get_source (ecal));

  g_signal_connect (priv->source,
                    "changed",
                    (GCallback)_source_changed_cb,
                    tile);
  _update_calendar_indicator (tile);
}
