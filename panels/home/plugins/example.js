/*
 * Copyright (c) 2012 Intel Corp.
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

const Clutter = imports.gi.Clutter;
const Mx = imports.gi.Mx;
const Gio = imports.gi.Gio;

function app_plugin () {}

app_plugin.prototype = {
  init: function() {
    print("Init");
    print(this.settings_path);

    this._settings = new Gio.Settings({
        schema: 'org.dawati.shell.home.plugin.example',
        path: this.settings_path,
      });
  },

  get_widget: function() {
    let stack = new Mx.Stack();
    let bg = new Clutter.Texture({
        // FIXME: hack to limit the size of the widget
        width: 1,
        height: 1,
      });
    let label = new Mx.Label({ text: "Example Plugin" });

    this._settings.bind('picture', bg, 'filename', 0);

    stack.add_actor (bg);
    stack.add_actor (label);

    return stack;
  },

  get_configuration: function() {
    let table = new Mx.Table();

    let label = new Mx.Label({ text: "Picture" });
    let entry = new Mx.Entry();

    this._settings.bind('picture', entry, 'text', 0);

    table.add_actor(label, 0, 0);
    table.add_actor(entry, 0, 1);

    print(this.settings_path);

    return table;
  },
}

var extensions = {
  'DawatiHomePluginsApp': app_plugin,
};
