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
#include <stdlib.h>
#include <rest/rest-xml-parser.h>
#include "ggg-mobile-info.h"

static gpointer
mobile_info_init (gpointer user_data)
{
  char *contents;
  gsize length;
  RestXmlParser *parser;
  RestXmlNode *root;

  if (!g_file_get_contents (MOBILE_DATA, &contents, &length, NULL)) {
    g_printerr ("Cannot open mobile broadband provider information\n");
    return NULL;
  }

  parser = rest_xml_parser_new ();
  root = rest_xml_parser_parse_from_data (parser, contents, length);
  g_free (contents);
  g_object_unref (parser);

  return root;
}

RestXmlNode *
ggg_mobile_info_get_root (void)
{
  static GOnce my_once = G_ONCE_INIT;
  g_once (&my_once, mobile_info_init, NULL);
  return my_once.retval;
}

RestXmlNode *
ggg_mobile_info_get_provider_for_ids (const char *mcc, const char *mnc)
{
  RestXmlNode *root, *c_node, *p_node, *n_node;
  const char *this_mcc, *this_mnc;

  root = ggg_mobile_info_get_root ();

  /* Iterate over every country */
  for (c_node = rest_xml_node_find (root, "country");
       c_node;
       c_node = c_node->next) {

    /* Iterate over every provider */
    for (p_node = rest_xml_node_find (c_node, "provider");
         p_node;
         p_node = p_node->next) {

      /* Iterate over every network-id */
      for (n_node = rest_xml_node_find (p_node, "network-id");
           n_node;
           n_node = n_node->next) {

        this_mcc = rest_xml_node_get_attr (n_node, "mcc");
        this_mnc = rest_xml_node_get_attr (n_node, "mnc");

        if (g_strcmp0 (this_mcc, mcc) == 0 &&
            g_strcmp0 (this_mnc, mnc) == 0) {
          return p_node;
        }
      }
    }
  }

  return NULL;
}
