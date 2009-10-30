
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

#include "mnb-expander.h"

static void _stylable_iface_init (NbtkStylableIface *iface);

G_DEFINE_TYPE_WITH_CODE (MnbExpander, mnb_expander, NBTK_TYPE_EXPANDER,
                         G_IMPLEMENT_INTERFACE (NBTK_TYPE_STYLABLE, _stylable_iface_init))

/*
 * NbtkStylable, needed to get parent type styling applied.
 */

static const gchar *
_stylable_get_style_type (NbtkStylable *stylable)
{
  return "NbtkExpander";
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
 * MnbExpander
 */

static void
mnb_expander_class_init (MnbExpanderClass *klass)
{
}

static void
mnb_expander_init (MnbExpander *self)
{
}

MnbExpander *
mnb_expander_new (void)
{
  return g_object_new (MNB_TYPE_EXPANDER, NULL);
}

