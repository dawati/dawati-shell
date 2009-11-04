
/*
 * Copyright (c) 2009 Intel Corp.
 *
 * Author: Robert Staudinger <robertx.staudinger@intel.com>
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

#include "mnb-entry.h"

static void _stylable_iface_init (NbtkStylableIface *iface);

G_DEFINE_TYPE_WITH_CODE (MnbEntry, mnb_entry, MPL_TYPE_ENTRY,
                         G_IMPLEMENT_INTERFACE (NBTK_TYPE_STYLABLE, _stylable_iface_init))

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MNB_TYPE_ENTRY, MnbEntryPrivate))

typedef struct {
    int dummy;
} MnbEntryPrivate;

/*
 * NbtkStylable, needed to get parent type styling applied.
 */

static const gchar *
_stylable_get_style_type (NbtkStylable *stylable)
{
  return "MplEntry";
}

static void
_stylable_iface_init (NbtkStylableIface *iface)
{
  static gboolean _is_initialized = FALSE;

  if (!_is_initialized)
  {
    iface->get_style_type = _stylable_get_style_type;
    _is_initialized = TRUE;
  }
}

/*
 * MnbEntry
 */

static void
mnb_entry_get_property (GObject *object, guint property_id,
                              GValue *value, GParamSpec *pspec)
{
  switch (property_id) {
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
mnb_entry_set_property (GObject *object, guint property_id,
                              const GValue *value, GParamSpec *pspec)
{
  switch (property_id) {
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
mnb_entry_class_init (MnbEntryClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MnbEntryPrivate));

  object_class->get_property = mnb_entry_get_property;
  object_class->set_property = mnb_entry_set_property;
}

static void
mnb_entry_init (MnbEntry *self)
{
}

ClutterActor *
mnb_entry_new (const char *label)
{
  return g_object_new (MNB_TYPE_ENTRY,
                       "label", label,
                       NULL);
}


