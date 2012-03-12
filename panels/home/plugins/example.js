/*
 * Copyright (c) 2011 Intel Corp.
 *
 * Author: Danielle Madeley <danielle.madeley@collabora.co.uk>
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

imports.gi.versions.Mx = '2.0';

const Mx = imports.gi.Mx;

function app_plugin () {}

app_plugin.prototype = {
  get_widget: function() {
    let label = new Mx.Label({ text: "Example Plugin" });

    return label;
  },

  get_configuration: function() {
    let label = new Mx.Label({ text: "Config" });

    return label;
  },
}

var extensions = {
  'DawatiHomePluginsApp': app_plugin,
};
