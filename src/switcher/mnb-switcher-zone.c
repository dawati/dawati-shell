/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */

/*
 * Moblin Netbook
 * Copyright © 2009, Intel Corporation.
 *
 * Author: Tomas Frydrych <tf@linux.intel.com>
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 */

#include <clutter/x11/clutter-x11.h>

#include "mnb-switcher-zone.h"
#include "../mnb-panel.h"

struct _MnbSwitcherZonePrivate
{
  MnbSwitcher  *switcher;

  gint          index;

  ClutterActor *table;             /* the inner content area     */
  ClutterActor *label;             /* the label area             */
  ClutterActor *text;              /* the text withing the label */
  gchar        *label_text;        /* (custom) label text        */

  gboolean      without_label : 1; /* do not construct label     */
  gboolean      active        : 1; /* we are currently selected  */
  gboolean      text_set      : 1; /* custom text set            */
  gboolean      pageable      : 1; /* this zone is pageable      */
  gboolean      has_items     : 1; /* this zone has items        */
  gboolean      constructed   : 1; /* construction completed     */
};

enum
{
  PROP_0 = 0,

  PROP_SWITCHER,
  PROP_INDEX,
  PROP_LABEL_TEXT,
  PROP_WITHOUT_LABEL,
  PROP_ACTIVE,
  PROP_PAGEABLE,
  PROP_HAS_ITEMS
};

G_DEFINE_ABSTRACT_TYPE (MnbSwitcherZone, mnb_switcher_zone, MX_TYPE_TABLE);

#define MNB_SWITCHER_ZONE_GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MNB_TYPE_SWITCHER_ZONE,\
                                MnbSwitcherZonePrivate))

static void
mnb_switcher_zone_dispose (GObject *object)
{
  G_OBJECT_CLASS (mnb_switcher_zone_parent_class)->dispose (object);
}

static void
mnb_switcher_zone_finalize (GObject *object)
{
  MnbSwitcherZonePrivate *priv = MNB_SWITCHER_ZONE (object)->priv;

  g_free (priv->label_text);

  G_OBJECT_CLASS (mnb_switcher_zone_parent_class)->finalize (object);
}

void
mnb_switcher_zone_set_state (MnbSwitcherZone      *zone,
                             MnbSwitcherZoneState  state)
{
  MnbSwitcherZoneClass   *klass = MNB_SWITCHER_ZONE_GET_CLASS (zone);
  MnbSwitcherZonePrivate *priv  = zone->priv;
  const gchar            *zone_name  = klass->zone_style  (zone, state);
  const gchar            *label_name = klass->label_style (zone, state);
  const gchar            *text_name  = klass->text_style  (zone, state);

  clutter_actor_set_name (priv->table, zone_name);

  if (priv->label)
    clutter_actor_set_name (priv->label, label_name);

  if (priv->text)
    clutter_actor_set_name (priv->text, text_name);
}


static void
mnb_switcher_zone_set_property (GObject      *gobject,
                                guint         prop_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
  MnbSwitcherZone        *zone = MNB_SWITCHER_ZONE (gobject);
  MnbSwitcherZonePrivate *priv = zone->priv;

  switch (prop_id)
    {
    case PROP_SWITCHER:
      priv->switcher = g_value_get_object (value);
      break;
    case PROP_WITHOUT_LABEL:
      priv->without_label = g_value_get_boolean (value);
      break;
    case PROP_ACTIVE:
      priv->active = g_value_get_boolean (value);
      break;
    case PROP_PAGEABLE:
      mnb_switcher_zone_set_pageable (zone, g_value_get_boolean (value));
      break;
    case PROP_HAS_ITEMS:
      mnb_switcher_zone_set_has_items (zone, g_value_get_boolean (value));
      break;
    case PROP_INDEX:
      mnb_switcher_zone_set_index (zone, g_value_get_int (value));
      break;
    case PROP_LABEL_TEXT:
      {
        g_free (priv->label_text);
        priv->label_text = g_value_dup_string (value);

        if (priv->text)
          mx_label_set_text (MX_LABEL (priv->text), priv->label_text);

        if (priv->label_text)
          priv->text_set = TRUE;
        else
          priv->text_set = FALSE;
      }
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
      break;
    }
}

static void
mnb_switcher_zone_get_property (GObject    *gobject,
                                guint       prop_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
  MnbSwitcherZonePrivate *priv = MNB_SWITCHER_ZONE (gobject)->priv;

  switch (prop_id)
    {
    case PROP_SWITCHER:
      g_value_set_object (value, priv->switcher);
      break;
    case PROP_WITHOUT_LABEL:
      g_value_set_boolean (value, priv->without_label);
      break;
    case PROP_ACTIVE:
      g_value_set_boolean (value, priv->active);
      break;
    case PROP_PAGEABLE:
      g_value_set_boolean (value, priv->pageable);
      break;
    case PROP_HAS_ITEMS:
      g_value_set_boolean (value, priv->has_items);
      break;
    case PROP_INDEX:
      g_value_set_int (value, priv->index);
      break;
    case PROP_LABEL_TEXT:
      g_value_set_string (value, priv->label_text);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
      break;
    }
}

static void
mnb_switcher_zone_constructed (GObject *self)
{
  MnbSwitcherZoneClass   *klass      = MNB_SWITCHER_ZONE_GET_CLASS (self);
  MnbSwitcherZone        *zone       = MNB_SWITCHER_ZONE (self);
  MnbSwitcherZonePrivate *priv       = zone->priv;
  MnbSwitcherZoneState    state      = MNB_SWITCHER_ZONE_NORMAL;
  const gchar            *zone_name;
  const gchar            *zone_class;
  ClutterActor           *table;

  g_assert (priv->index >= 0);

  priv->table = table = (ClutterActor*)mx_table_new ();
  mx_table_add_actor (MX_TABLE (self), table, 1, 0);
  mx_table_set_row_spacing (MX_TABLE (self), 4);

  if (priv->active)
    state = MNB_SWITCHER_ZONE_ACTIVE;

  zone_name = klass->zone_style (zone, state);
  clutter_actor_set_name (table, zone_name);

  zone_class = klass->zone_class (zone);
  mx_widget_set_style_class_name (MX_WIDGET (table), zone_class);

  /*
   * Now construct the label area
   */
  if (!priv->without_label)
    {
      ClutterActor *label;
      ClutterActor *text;
      gchar        *s;
      const gchar  *label_name;
      const gchar  *text_name;
      const gchar  *label_class;
      const gchar  *text_class;

      label_name  = klass->label_style (zone, state);
      text_name   = klass->text_style  (zone, state);
      label_class = klass->label_class (zone);
      text_class  = klass->text_class (zone);

      if (priv->label_text)
        s = g_strdup (priv->label_text);
      else
        s = g_strdup_printf ("%d", priv->index + 1);

      priv->label = label = CLUTTER_ACTOR (mx_frame_new ());
      priv->text  = text  = (ClutterActor*)mx_label_new (s);

      mx_widget_set_style_class_name (MX_WIDGET (text), text_class);
      clutter_actor_set_name (CLUTTER_ACTOR (text), text_name);

      mx_bin_set_child (MX_BIN (label), CLUTTER_ACTOR (text));

      mx_widget_set_style_class_name (MX_WIDGET (label), label_class);
      clutter_actor_set_name (CLUTTER_ACTOR (label), label_name);

      mx_table_add_actor (MX_TABLE (self), label, 0, 0);
      clutter_container_child_set (CLUTTER_CONTAINER (self),
                                   CLUTTER_ACTOR (label),
                                   "y-expand", FALSE,
                                   "x-fill", TRUE,
                                   NULL);
    }

  priv->constructed = TRUE;
}

/*
 * The default styles for the internal 'zone' (the area where the application
 * items live)
 */
static const gchar *
mnb_switcher_zone_zone_style (MnbSwitcherZone *zone, MnbSwitcherZoneState state)
{
  const gchar *zone_name;

  switch (state)
    {
    default:
    case MNB_SWITCHER_ZONE_NORMAL:
      zone_name = "";
      break;
    case MNB_SWITCHER_ZONE_ACTIVE:
      zone_name = "switcher-workspace-active";
      break;
    case MNB_SWITCHER_ZONE_HOVER:
      zone_name = "switcher-workspace-new-over";
      break;
    }

  return zone_name;
}

/*
 * Default styles for the label area
 */
static const gchar *
mnb_switcher_zone_label_style (MnbSwitcherZone     *zone,
                               MnbSwitcherZoneState state)
{
  const gchar *label_name;

  switch (state)
    {
    default:
    case MNB_SWITCHER_ZONE_NORMAL:
      label_name = "";
      break;
    case MNB_SWITCHER_ZONE_ACTIVE:
      label_name = "workspace-title-active";
      break;
    case MNB_SWITCHER_ZONE_HOVER:
      label_name = "workspace-title-new-over";
      break;
    }

  return label_name;
}

/*
 * Default styles for the label text
 */
static const gchar *
mnb_switcher_zone_text_style (MnbSwitcherZone     *zone,
                              MnbSwitcherZoneState state)
{
  const gchar *text_name;

  switch (state)
    {
    default:
    case MNB_SWITCHER_ZONE_NORMAL:
      text_name = "";
      break;
    case MNB_SWITCHER_ZONE_ACTIVE:
      text_name = "workspace-title-label-active";
      break;
    case MNB_SWITCHER_ZONE_HOVER:
      text_name = "workspace-title-label-over";
      break;
    }

  return text_name;
}

/*
 * Handling of button release on the zone -- we switch to the corresponding
 * workspace.
 */
static gboolean
mnb_switcher_zone_button_release (ClutterActor       *actor,
                                  ClutterButtonEvent *event)
{
  MnbSwitcherZonePrivate *priv      = MNB_SWITCHER_ZONE (actor)->priv;
  MutterPlugin           *plugin    = moblin_netbook_get_plugin_singleton ();
  MetaScreen             *screen    = mutter_plugin_get_screen (plugin);
  guint32                 timestamp = clutter_x11_get_current_event_time ();
  MetaWorkspace          *workspace;

  if (mnb_switcher_get_dnd_in_progress (priv->switcher))
    return FALSE;

  workspace = meta_screen_get_workspace_by_index (screen, priv->index);

  if (!workspace)
    {
      g_warning ("No workspace specified, %s:%d\n", __FILE__, __LINE__);
      return FALSE;
    }

  mnb_panel_hide_with_toolbar (MNB_PANEL (priv->switcher));

  mnb_switcher_end_kbd_grab (priv->switcher);

  meta_workspace_activate (workspace, timestamp);

  return FALSE;
}

/*
 * The default css class for the internal 'zone' component.
 */
const gchar *
mnb_switcher_zone_zone_class (MnbSwitcherZone *zone)
{
  return "switcher-workspace";
}

/*
 * The default css class for the internal 'label' component.
 */
const gchar *
mnb_switcher_zone_label_class (MnbSwitcherZone *zone)
{
  return "workspace-title";
}

/*
 * The default css class for the text within the label component
 */
const gchar *
mnb_switcher_zone_text_class (MnbSwitcherZone *zone)
{
  return "workspace-title-label";
}

static void
mnb_switcher_zone_class_init (MnbSwitcherZoneClass *klass)
{
  GObjectClass         *object_class = G_OBJECT_CLASS (klass);
  ClutterActorClass    *actor_class  = CLUTTER_ACTOR_CLASS (klass);
  MnbSwitcherZoneClass *zone_class   = MNB_SWITCHER_ZONE_CLASS (klass);

  object_class->dispose              = mnb_switcher_zone_dispose;
  object_class->finalize             = mnb_switcher_zone_finalize;
  object_class->get_property         = mnb_switcher_zone_get_property;
  object_class->set_property         = mnb_switcher_zone_set_property;
  object_class->constructed          = mnb_switcher_zone_constructed;

  actor_class->button_release_event  = mnb_switcher_zone_button_release;

  zone_class->zone_style             = mnb_switcher_zone_zone_style;
  zone_class->label_style            = mnb_switcher_zone_label_style;
  zone_class->text_style             = mnb_switcher_zone_text_style;
  zone_class->zone_class             = mnb_switcher_zone_zone_class;
  zone_class->label_class            = mnb_switcher_zone_label_class;
  zone_class->text_class             = mnb_switcher_zone_text_class;

  g_type_class_add_private (klass, sizeof (MnbSwitcherZonePrivate));

  g_object_class_install_property (object_class,
                                   PROP_SWITCHER,
                                   g_param_spec_object ("switcher",
                                                        "Switcher",
                                                        "Switcher",
                                                        MNB_TYPE_SWITCHER,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  /*
   * Indicates that the zone should be constructed without the label area.
   */
  g_object_class_install_property (object_class,
                                   PROP_WITHOUT_LABEL,
                                   g_param_spec_boolean ("without-label",
                                                         "without label",
                                                         "without label",
                                                         FALSE,
                                                         G_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class,
                                   PROP_ACTIVE,
                                   g_param_spec_boolean ("active",
                                                         "active",
                                                         "active",
                                                         FALSE,
                                                         G_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT));

  /*
   * Indicates that the zone is pageable, i.e., it can be selected as a single
   * whole in the switcher. Defaults to FALSE. The MnbSwitcherZoneApps subclass,
   * for example, is not pageable, but the forthcoming myzone class will be.
   */
  g_object_class_install_property (object_class,
                                   PROP_PAGEABLE,
                                   g_param_spec_boolean ("pageable",
                                                         "pageable",
                                                         "pageable",
                                                         FALSE,
                                                         G_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT));

  /*
   * The zone contains navigable items (i.e., items that should be iterated one
   * one with the Alt+Tab key).
   */
  g_object_class_install_property (object_class,
                                   PROP_HAS_ITEMS,
                                   g_param_spec_boolean ("has-items",
                                                         "has-items",
                                                         "has-items",
                                                         FALSE,
                                                         G_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT));

  /*
   * Custom text for the label; by default the label is the number of the zone
   * in the switcher; use this property to override.
   */
  g_object_class_install_property (object_class,
                                   PROP_LABEL_TEXT,
                                   g_param_spec_string ("label-text",
                                                        "Lable Text",
                                                        "Lable Text",
                                                        NULL,
                                                        G_PARAM_WRITABLE |
                                                        G_PARAM_CONSTRUCT));

  /*
   * The zone index, i.e., its position in the switcher, 0-based.
   */
  g_object_class_install_property (object_class,
                                   PROP_INDEX,
                                   g_param_spec_int ("index",
                                                     "Index",
                                                     "Index",
                                                     G_MININT,
                                                     G_MAXINT,
                                                     -1,
                                                     G_PARAM_READWRITE |
                                                     G_PARAM_CONSTRUCT));
}

static void
mnb_switcher_zone_init (MnbSwitcherZone *self)
{
  self->priv = MNB_SWITCHER_ZONE_GET_PRIVATE (self);

  /*
   * Initialize the index to -1; we will assert on it being >= 0 in the
   * constructor, ensuring that valid index has been set during prior to
   * construction via the constrution-time property.
   */
  self->priv->index = -1;
}

/*
 * Reset the state of this zone to what is appropriate to our current
 * circumstance.
 */
void
mnb_switcher_zone_reset_state (MnbSwitcherZone *zone)
{
  MnbSwitcherZonePrivate *priv = zone->priv;

  mnb_switcher_zone_set_state (zone,
                               priv->active ?
                               MNB_SWITCHER_ZONE_ACTIVE :
                               MNB_SWITCHER_ZONE_NORMAL);
}

/*
 * Returns the index of this zone (it's position in the switcher).
 */
gint
mnb_switcher_zone_get_index (MnbSwitcherZone *zone)
{
  MnbSwitcherZonePrivate *priv = zone->priv;

  return priv->index;
}

/*
 * Get the next item after the provided current item.
 */
MnbSwitcherItem *
mnb_switcher_zone_get_next_item (MnbSwitcherZone *zone,
                                 MnbSwitcherItem *current)
{
  MnbSwitcherZoneClass *klass = MNB_SWITCHER_ZONE_GET_CLASS (zone);

  if (klass->next_item)
    return klass->next_item (zone, current);

  return NULL;
}

/*
 * Get item before the current item.
 */
MnbSwitcherItem *
mnb_switcher_zone_get_prev_item (MnbSwitcherZone *zone,
                                 MnbSwitcherItem *current)
{
  MnbSwitcherZoneClass *klass = MNB_SWITCHER_ZONE_GET_CLASS (zone);

  if (klass->prev_item)
    return klass->prev_item (zone, current);

  return NULL;
}

/*
 * Select this item.
 */
gboolean
mnb_switcher_zone_select_item (MnbSwitcherZone *zone, MnbSwitcherItem *item)
{
  MnbSwitcherZonePrivate *priv  = zone->priv;
  MnbSwitcherZoneClass   *klass = MNB_SWITCHER_ZONE_GET_CLASS (zone);

  if (klass->select_item)
    {
      if (klass->select_item (zone, item))
        {
          priv->active = TRUE;
          mnb_switcher_zone_set_state (zone, MNB_SWITCHER_ZONE_ACTIVE);
          g_object_notify (G_OBJECT (zone), "active");
          return TRUE;
        }

      g_warning (G_STRLOC " Item %s in zone %s could not be selected",
                 item ? G_OBJECT_TYPE_NAME (item) : "nil",
                 zone ? G_OBJECT_TYPE_NAME (zone) : "nil");

      return FALSE;
    }

  g_warning ("Object of type %s does not implement select_item()",
             G_OBJECT_TYPE_NAME (zone));
  return FALSE;
}

/*
 * Select this zone.
 *
 * NB: this is only possible for zones that are pageable.
 */
gboolean
mnb_switcher_zone_select (MnbSwitcherZone *zone)
{
  MnbSwitcherZonePrivate *priv   = zone->priv;
  MnbSwitcherZoneClass   *klass  = MNB_SWITCHER_ZONE_GET_CLASS (zone);
  gboolean                retval = FALSE;

  if (!mnb_switcher_zone_is_pageable (zone))
    {
      g_warning (G_STRLOC " only pageable zones can be selected");
      return FALSE;
    }

  /*
   * If the zone provides a class method, we call it first; if not,
   * we fall back on the default behaviour, which is to set the state to
   * selected.
   */
  if (klass->select)
    retval = klass->select (zone);
  else
    {
      retval = TRUE;
      mnb_switcher_zone_set_state (zone, MNB_SWITCHER_ZONE_ACTIVE);
    }

  if (retval)
    {
      priv->active = TRUE;
      g_object_notify (G_OBJECT (zone), "active");
    }

  return retval;
}

/*
 * Unselect all items on this zone.
 */
void
mnb_switcher_zone_unselect_all (MnbSwitcherZone *zone)
{
  MnbSwitcherZoneClass *klass = MNB_SWITCHER_ZONE_GET_CLASS (zone);

  if (klass->unselect_all)
    klass->unselect_all (zone);
}

MnbSwitcher *
mnb_switcher_zone_get_switcher (MnbSwitcherZone *zone)
{
  MnbSwitcherZonePrivate *priv  = zone->priv;

  return priv->switcher;
}

gboolean
mnb_switcher_zone_is_active (MnbSwitcherZone *zone)
{
  MnbSwitcherZonePrivate *priv = zone->priv;

  return priv->active;
}

void
mnb_switcher_zone_set_active (MnbSwitcherZone *zone, gboolean active)
{
  MnbSwitcherZonePrivate *priv = zone->priv;

  if ((active && priv->active) || (!active && !priv->active))
    return;

  priv->active = active;

  mnb_switcher_zone_reset_state (zone);

  g_object_notify (G_OBJECT (zone), "active");
}

void
mnb_switcher_zone_set_index (MnbSwitcherZone *zone, gint index)
{
  MnbSwitcherZonePrivate *priv = zone->priv;

  g_assert (index >= 0);

  if (priv->index == index)
    return;

  priv->index = index;

  if (!priv->constructed)
    return;

  /*
   * If not using custom text, update.
   */
  if (!priv->text_set && priv->text)
    {
      gchar *s = g_strdup_printf ("%d", index + 1);

      mx_label_set_text (MX_LABEL (priv->text), s);
      g_free (s);
    }

  g_object_notify (G_OBJECT (zone), "index");
}

/*
 * Returns the content area of this table, i.e., where the items go; this can
 * be used by any subclasses to manipulate their contents.
 */
MxTable *
mnb_switcher_zone_get_content_area (MnbSwitcherZone *zone)
{
  MnbSwitcherZonePrivate *priv = zone->priv;

  return MX_TABLE (priv->table);
}

gboolean
mnb_switcher_zone_is_pageable (MnbSwitcherZone *zone)
{
  MnbSwitcherZonePrivate *priv = zone->priv;

  return priv->pageable;
}

void
mnb_switcher_zone_set_pageable (MnbSwitcherZone *zone, gboolean whether)
{
  MnbSwitcherZonePrivate *priv = zone->priv;

  if ((priv->pageable && whether) || (!priv->pageable && !whether))
    return;

  priv->pageable = whether;

  g_object_notify (G_OBJECT (zone), "pageable");
}

gboolean
mnb_switcher_zone_has_items (MnbSwitcherZone *zone)
{
  MnbSwitcherZonePrivate *priv = zone->priv;

  return priv->has_items;
}

void
mnb_switcher_zone_set_has_items (MnbSwitcherZone *zone, gboolean whether)
{
  MnbSwitcherZonePrivate *priv = zone->priv;

  if ((priv->has_items && whether) || (!priv->has_items && !whether))
    return;

  priv->has_items = whether;

  g_object_notify (G_OBJECT (zone), "has-items");
}

/*
 * Finds the active item in this zone
 */
MnbSwitcherItem *
mnb_switcher_zone_get_active_item (MnbSwitcherZone *zone)
{
  MnbSwitcherZonePrivate *priv = zone->priv;
  GList                  *l, *o;
  ClutterContainer       *table;
  MnbSwitcherItem        *item = NULL;

  if (!priv->has_items || !priv->active)
    return NULL;

  table = CLUTTER_CONTAINER (priv->table);

  o = l = clutter_container_get_children (table);
  while (l)
    {
      MnbSwitcherItem *i = l->data;

      if (MNB_IS_SWITCHER_ITEM (i) && mnb_switcher_item_is_active (i))
        {
          item = i;
          break;
        }

      l = l->next;
    }

  g_list_free (o);

  return item;
}

gboolean
mnb_switcher_zone_activate (MnbSwitcherZone *zone)
{
  MnbSwitcherZoneClass   *klass  = MNB_SWITCHER_ZONE_GET_CLASS (zone);

  if (!mnb_switcher_zone_is_pageable (zone))
    {
      g_warning (G_STRLOC " only pageable zones can be activated");
      return FALSE;
    }

  /*
   * If the zone provides a class method, we call it first; if not,
   * we fall back on the default behaviour, which is to set the state to
   * selected.
   */
  if (klass->activate)
    return klass->activate (zone);

  g_warning ("Object of type %s is pageable, but does not implement activate()",
             G_OBJECT_TYPE_NAME (zone));

  return FALSE;
}

