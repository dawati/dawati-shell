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

const Lang = imports.lang;

imports.gi.versions.Mx = '2.0';

const Clutter = imports.gi.Clutter;
const Mx = imports.gi.Mx;
const Gio = imports.gi.Gio;

const Plugins = imports.gi.DawatiHomePlugins;

const Example = new Lang.Class({
  Name: 'Example',
  Implements: [ Plugins.App ],

  init: function() {
    print("Init");

    this._settings = new Gio.Settings({
        schema: 'org.dawati.shell.home.plugin.example',
        path: this.settings_path,
      });

    /* Init Widget actor */
    this.widget = new Mx.Stack();

    let bg = new Clutter.Texture({
        width: 1,
        height: 1,
    });
    this._settings.bind('picture', bg, 'filename', 1);

    let label = new Mx.Label({text: "Example Plugin"});

    this.widget.add_actor(bg);
    this.widget.add_actor(label);


    /* Init Configuration actor */
    this.configuration = new Mx.Table();
    let label = new Mx.Label({ text: "Picture" });
    let entry = new Mx.Entry();
    this._settings.bind ("picture", entry, "text", 1);

    this.configuration.insert_actor(label, 0, 0);
    this.configuration.insert_actor(entry, 0, 1);
  },

  get_widget: function() {
    return this.widget;
  },

  get_configuration: function() {
    return this.configuration;
  },

  deinit: function() {
  	this.widget.unref();
	this.configuration.unref();
  },
});

var extensions = {
  'DawatiHomePluginsApp': Example,
};
