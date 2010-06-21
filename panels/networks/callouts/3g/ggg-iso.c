/*
 * Carrick - a connection panel for the MeeGo Netbook
 * Copyright (C) 2009 Intel Corporation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License version
 * 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 *
 * Written by - Ross Burton <ross@linux.intel.com>
 */

#include <config.h>
#include <rest/rest-xml-parser.h>
#include "ggg-iso.h"

static GHashTable *iso3166 = NULL;

static guint
ascii_hash (gconstpointer key)
{
  char *s;
  guint hash;

  s = g_ascii_strup (key, -1);
  hash = g_str_hash (s);
  g_free (s);

  return hash;
}

static gboolean
ascii_equal (gconstpointer a, gconstpointer b)
{
  char *s_a, *s_b;
  gboolean equal;

  s_a = g_ascii_strup (a, -1);
  s_b = g_ascii_strup (b, -1);

  equal = g_str_equal (s_a, s_b);

  g_free (s_a);
  g_free (s_b);

  return equal;
}

static void
init_iso3166 (void)
{
  GError *error = NULL;
  char *contents;
  gsize length;
  RestXmlParser *parser;
  RestXmlNode *root, *node;

  iso3166 = g_hash_table_new_full (ascii_hash, ascii_equal, g_free, g_free);

  if (!g_file_get_contents (ISOCODES_PREFIX "/share/xml/iso-codes/iso_3166.xml", &contents, &length, &error)) {
    g_printerr ("Cannot open iso-codes: %s\n", error->message);
    g_error_free (error);
    return;
  }

  parser = rest_xml_parser_new ();
  root = rest_xml_parser_parse_from_data (parser, contents, length);
  g_free (contents);
  g_object_unref (parser);

  for (node = rest_xml_node_find (root, "iso_3166_entry"); node; node = node->next) {
    g_hash_table_insert (iso3166,
                         g_strdup (rest_xml_node_get_attr (node, "alpha_2_code")),
                         g_strdup (rest_xml_node_get_attr (node, "name")));
  }
}

const char *
ggg_iso_country_name_for_code (const char *code)
{
  if (iso3166 == NULL)
    init_iso3166 ();

  return g_hash_table_lookup (iso3166, code);
}
