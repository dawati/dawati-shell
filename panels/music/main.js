const Gtk = imports.gi.Gtk;
const GLib = imports.gi.GLib;
const GObject = imports.gi.GObject;
const Gio = imports.gi.Gio;
const Lang = imports.lang;
const Gettext = imports.gettext;
const Clutter = imports.gi.Clutter;
const GtkClutter = imports.gi.GtkClutter;
const Pango = imports.gi.Pango;
const Mx = imports.gi.Mx;
const DawatiPanel = imports.gi.DawatiPanel;
const Tracker = imports.gi.Tracker;
const DBus = imports.dbus;
const Mainloop = imports.mainloop;
const Path = imports.path;

Gettext.textdomain("dawati-shell");
Gettext.bindtextdomain("dawati-shell", Path.LOCALE_DIR);

const _ = Gettext.gettext;


//
// Result set: Model a tracker query
//

function ResultSet(request) {
    this._init(request);
}

ResultSet.prototype = {
    _init: function(request) {
        this.store = DawatiPanel.mpl_create_audio_store();
        this.request = request;

        this._init_tracker();
    },

    _init_tracker: function() {
        log("Starting request : " + this.request);
        this.trk_connection = Tracker.SparqlConnection.get(null);
        this.trk_connection.query_async(this.request,
                                        null,
                                        Lang.bind(this, this._tracker_results));
    },

    _tracker_process_item: function(cursor, result) {
        if (cursor.next_finish(result)) {
            // Batching results
            if (this.data_array == null || this.data_array.length == 0)
            {
                this.data_array = new Array();
                this.data_array[this.data_array.length] = ["",
                                                           cursor.get_string(0, null)[0],
                                                           cursor.get_string(1, null)[0],
                                                           cursor.get_string(2, null)[0],
                                                           cursor.get_string(3, null)[0]];
            } else {
                if (this.data_array.length < 50) {
                    this.data_array.push(["",
                                          cursor.get_string(0, null)[0],
                                          cursor.get_string(1, null)[0],
                                          cursor.get_string(2, null)[0],
                                          cursor.get_string(3, null)[0]]);
                }
                else
                {
                    while (this.data_array.length > 1) {
                        let ldata = this.data_array.shift();
                        let iter = this.store.append();
                        DawatiPanel.mpl_audio_store_set(this.store, iter,
                                                        ldata[0],
                                                        ldata[1],
                                                        ldata[2],
                                                        ldata[3],
                                                        ldata[4]);
                    }
                    this.data_array = new Array();
                }
            }
            cursor.next_async(null,
                              Lang.bind(this, this._tracker_process_item));
        } else {
            if (this.data_array != null) {
                while (this.data_array.length >= 1) {
                    let ldata = this.data_array.shift();
                    let iter = this.store.append();
                    DawatiPanel.mpl_audio_store_set(this.store, iter,
                                                    ldata[0],
                                                    ldata[1],
                                                    ldata[2],
                                                    ldata[3],
                                                    ldata[4]);
                }
            }
            this.data_array = new Array();
            log("loading done.");
        }
    },

    _tracker_results: function(connection, result) {
        let cursor = connection.query_finish(result);

        cursor.next_async(null,
                          Lang.bind(this, this._tracker_process_item));
    }
};

//
// Audio Library
//

function AudioLibrary(panel) {
    this._init(panel);
}

AudioLibrary.prototype = {
    _init: function(panel) {
        this._panel = panel;
        this._player = new MprisProxy();

        this.controls_actor =
            new Mx.BoxLayout({ orientation: Mx.Orientation.HORIZONTAL });

        this.combo = new Mx.ComboBox();
        this.combo.append_text(_("Recently Added"));
        this.combo.append_text(_("Favorites"));
        this.combo.append_text(_("Rediscover"));
        this.combo.connect('notify::index', Lang.bind(this, this._switched_model));
        this.controls_actor.add_actor(this.combo, 0);
        this.controls_actor.child_set_x_align(this.combo, Mx.Align.START);
        this.controls_actor.child_set_x_fill(this.combo, false);
        this.controls_actor.child_set_expand(this.combo, false);

        let button = new Mx.Button({ label: _("Open Audio Player") });
        this.controls_actor.add_actor(button, 1);
        //this.controls_actor.child_set_(button, true);
        this.controls_actor.child_set_x_align(button, Mx.Align.END);
        this.controls_actor.child_set_x_fill(button, false);
        this.controls_actor.child_set_y_fill(button, true);

        this._player_info = Gio.DesktopAppInfo.new("rhythmbox.desktop");
        button.connect('clicked', Lang.bind(this, this._open_player));


        let scroll = new Gtk.ScrolledWindow({ hexpand: true,
                                              vexpand: true,
                                              shadow_type: Gtk.ShadowType.IN });
        scroll.show_all();

        this.listview_actor = new GtkClutter.Actor({ contents: scroll });

        this.results = new Array();
        this.results.push(new ResultSet("select" +
                                        " nie:url(?u)" +
                                        " nie:title(?u)" +
                                        " nmm:artistName(nmm:performer(?u)) " +
                                        " nmm:albumTitle(nmm:musicAlbum(?u))" +
                                        " where { ?u a nmm:MusicPiece }" +
                                        " ORDER BY DESC(nie:contentAccessed(?u))" +
                                        " LIMIT 10"));
        this.results.push(new ResultSet("select" +
                                        " nie:url(?u)" +
                                        " nie:title(?u)" +
                                        " nmm:artistName(nmm:performer(?u)) " +
                                        " nmm:albumTitle(nmm:musicAlbum(?u))" +
                                        " where { ?u a nmm:MusicPiece }" +
                                        " ORDER BY ASC(nie:usageCounter(?u))" +
                                        " LIMIT 50"));
        this.results.push(new ResultSet("select" +
                                        " nie:url(?u)" +
                                        " nie:title(?u)" +
                                        " nmm:artistName(nmm:performer(?u)) " +
                                        " nmm:albumTitle(nmm:musicAlbum(?u))" +
                                        " where { ?u a nmm:MusicPiece }" +
                                        " ORDER BY ASC(tracker:added(?u))" +
                                        " LIMIT 50"));

        // GtkIconView setup
        // let icon = new Gtk.CellRendererPixbuf();
        // icon.set_property("xalign", 0.5);
        // icon.set_property("yalign", 0.5);
        // this.iconview.pack_start(icon, false);
        // this.iconview.add_attribute(icon, "pixbuf", 1);

        let area = new Gtk.CellAreaBox();

        this.iconview = new Gtk.IconView({ cell_area: area });
        //this.iconview.set_model(this.store);
        scroll.add(this.iconview);

        let text = new Gtk.CellRendererText();
        //text.set_property("alignment", Pango.ALIGN_LEFT);
        area.pack_start(text, false, true, true);
        area.attribute_connect(text,  "text", 2);

        text = new Gtk.CellRendererText();
        //text.set_property("alignment", Pango.ALIGN_LEFT);
        area.pack_start(text, false, true, true);
        area.attribute_connect(text,  "text", 3);

        text = new Gtk.CellRendererText();
        //text.set_property("alignment", Pango.ALIGN_LEFT);
        area.pack_start(text, false, true, true);
        area.attribute_connect(text,  "text", 4);

        // Setup the combo on the first entry
        this.combo.set_property("index", 0);


        //
        this.controls_actor.show_all();
        this.listview_actor.show_all();
    },


    _open_player: function() {
        //this._player_info.launch([], null);
        this._player.RaiseRemote();
        this._panel.hide();
    },

    _switched_model: function() {
        this.iconview.set_model(this.results[this.combo.get_index()].store);
    }
};

//
// MprisPlayerProxy: proxy to a Mpris compatible dbus service
//

const freedesktopPropertiesIface = {
    signals: [{ name: 'PropertiesChanged', inSignature: 'sa{sv}a{sv}' }]
};

function FreeDesktopPropertiesProxy() {
    this._init();
}

FreeDesktopPropertiesProxy.prototype = {
    _init: function() {
        DBus.session.proxifyObject(this,
                                   'org.gnome.Rhythmbox3',
                                   '/org/mpris/MediaPlayer2');
        DBus.session.watch_name('org.gnome.Rhythmbox3',
                                true, // do (not) launch a name-owner if none exists
                                null,
                                null);
    }
}
DBus.proxifyPrototype(FreeDesktopPropertiesProxy.prototype, freedesktopPropertiesIface);

const mprisPlayerIface = {
    name: 'org.mpris.MediaPlayer2.Player',
    methods: [{ name: 'Next', inSignature: '' },
              { name: 'Previous', inSignature: '' },
              { name: 'OpenUri', inSignature: 's' },
              { name: 'PlayPause', inSignature: '' },
              { name: 'Play', inSignature: '' },
              { name: 'Pause', inSignature: '' },
              { name: 'Seek', inSignature: 'x' }],
    signals: [{ name: 'Seeked',
                inSignature: 'x' }],
    properties: [{ name: 'Metadata',
                   signature: '{sv}',
                   access: 'read' }]
};

function MprisPlayerProxy(scope, callback) {
    this._init(scope, callback);
}

MprisPlayerProxy.prototype = {
    _init: function(scope, callback) {
        this._scope = scope;
        this._callback = callback;

        DBus.session.proxifyObject(this,
                                   'org.gnome.Rhythmbox3',
                                   '/org/mpris/MediaPlayer2');
        DBus.session.watch_name('org.gnome.Rhythmbox3',
                                true, // do not launch a name-owner if none exists
                                Lang.bind(this, this._on_appeared),
                                Lang.bind(this, this._on_vanished));
        this.remotePlayerActive = false;

        this.title = _("None");
        this.artist = _("None");
        this.album = _("None");
        this.position = 0;
        this.duration = 1000 * 1000; // 1s
        // TODO: placeholder
        this.cover = "file:///home/djdeath/Pictures/avatars/plop.png";
        this.playing = false;

        this._notifier = new FreeDesktopPropertiesProxy();
        this._notifier.connect('PropertiesChanged',
                               Lang.bind(this, this._updated_properties));
        this.connect('Seeked',
                     Lang.bind(this, this._updated_position));
    },

    _on_appeared: function(owner) {
        this.remotePlayerActive = true;
    },

    _on_vanished: function(owner) {
        this.remotePlayerActive = false;
    },

    _update_metadatas: function(dict) {
        if (dict['xesam:title'])
            this.title = "" + dict['xesam:title'];
        if (dict['xesam:artist'])
            this.artist = "" + dict['xesam:artist'];
        if (dict['xesam:album'])
            this.album = "" + dict['xesam:album'];
        if (dict['mpris:length'])
            this.duration = dict['mpris:length'];
        if (dict['mpris:artUrl'])
            this.cover = dict['mpris:artUrl'];
        else
            this.cover = "file:///home/djdeath/Pictures/avatars/plop.png";
    },

    _update_status: function(str) {
        if (str == 'Playing')
            this.playing = true;
        else
            this.playing = false;
    },

    update_infos: function(scope, callback) {
        this.GetRemote('Metadata', Lang.bind(this, function(dict) {
            this._update_metadatas(dict);
        }));
        this.GetRemote('PlaybackStatus', Lang.bind(this, function(str) {
            this._update_status(str);
        }));
        this.GetRemote('Position', Lang.bind(this, function(position) {
            this.position = position;

            if (this._callback != null)
                this._callback.call(this._scope);
        }));
    },

    _updated_position: function(wut, position) {
        this.position = position;

        if (this._callback != null)
            this._callback.call(this._scope);
    },

    _updated_properties: function(wut, str, dict, dicti) {
        let need_ui_update = false;
        log("external update from " + str);

        if (dict['Metadata'] != undefined) {
            this._update_metadatas(dict['Metadata']);
            need_ui_update = true;
        }
        if (dict['PlaybackStatus'] != undefined) {
            this._update_status(dict['PlaybackStatus']);
            need_ui_update = true;
        }

        if (need_ui_update) {
            this.GetRemote('Position', Lang.bind(this, function(position) {
                this.position = position;

                // log (this.title + " / " +
                //      this.artist +  " / " +
                //      this.album + " / " +
                //      this.cover + " / " +
                //      this.duration);
                // log("playing = " + this.playing);
                // log("new pos = " + this.position);

                if (this._callback != null)
                    this._callback.call(this._scope);
            }));
        }
    }
};
DBus.proxifyPrototype(MprisPlayerProxy.prototype, mprisPlayerIface);

const mprisIface = {
    signals: [{ name: 'Quit' },
              { name: 'Raise' }]
};

function MprisProxy() {
    this._init();
}

MprisProxy.prototype = {
    _init: function() {
        DBus.session.proxifyObject(this,
                                   'org.gnome.Rhythmbox3',
                                   '/org/mpris/MediaPlayer2');
        DBus.session.watch_name('org.gnome.Rhythmbox3',
                                true, // do (not) launch a name-owner if none exists
                                null,
                                null);
    }
}
DBus.proxifyPrototype(MprisProxy.prototype, MprisIface);


//
// NowPlaying
//

function NowPlaying() {
    this._init();
}

NowPlaying.prototype = {
    _init: function() {
        this._player = new MprisPlayerProxy(this, this._updated_player);

        this.controls_actor =
            new Mx.BoxLayout({ name: 'playing-widgets',
                               orientation: Mx.Orientation.VERTICAL });

        let hbox = new Mx.BoxLayout({ orientation: Mx.Orientation.HORIZONTAL });
        this.controls_actor.add_actor(hbox, 0);

        // Infos
        let hbox1 = new Mx.BoxLayout({ orientation: Mx.Orientation.HORIZONTAL });
        hbox.add_actor(hbox1, 0);

        this._playing_item = new DawatiPanel.MplAudioTile({ name: 'playing-item' });
        this._playing_item.set_size(180, 90);
        hbox1.add_actor(this._playing_item, 0);

        // Controls
        let table = new Mx.Table({ name : 'playing-controls' });
        hbox.add_actor(table, 1);
        hbox.child_set_x_align(table, Mx.Align.MIDDLE);
        hbox.child_set_y_align(table, Mx.Align.MIDDLE);
        hbox.child_set_y_fill(table, true);
        hbox.child_set_expand(table, true);

        let button = new Mx.Button({ icon_name: "player_rew",
                                     icon_size: 22 });
        table.add_actor(button, 0, 0);
        table.child_set_x_expand(button, false);
        table.child_set_y_expand(button, false);
        table.child_set_x_fill(button, false);
        table.child_set_y_fill(button, false);
        button.connect('clicked',
                       Lang.bind(this, function() {
                           this._player.PreviousRemote();
                       }));

        this._play_pause_button = new Mx.Button({ icon_name: "player_play",
                                                  icon_size: 22 });
        table.add_actor(this._play_pause_button, 0, 1);
        table.child_set_x_expand(this._play_pause_button, false);
        table.child_set_y_expand(this._play_pause_button, false);
        table.child_set_x_fill(this._play_pause_button, false);
        table.child_set_y_fill(this._play_pause_button, false);
        this._play_pause_button.connect('clicked',
                                        Lang.bind(this, function() {
                                            this._player.PlayPauseRemote();
                                        }));

        let button = new Mx.Button({ icon_name: "player_fwd",
                                     icon_size: 22 });
        table.add_actor(button, 0, 2);
        table.child_set_x_expand(button, false);
        table.child_set_y_expand(button, false);
        table.child_set_x_fill(button, false);
        table.child_set_y_fill(button, false);
        button.connect('clicked',
                       Lang.bind(this, function() {
                           this._player.NextRemote();
                       }));

        // Slider & Co
        let hbox = new Mx.BoxLayout({ orientation: Mx.Orientation.HORIZONTAL });
        this.controls_actor.add_actor(hbox, 1);

        this._elapsed = new Mx.Label({ name: 'playing-elapsed' });
        hbox.add_actor(this._elapsed, 0);

        this._slider = new Mx.Slider({ name: 'playing-slider' });
        hbox.add_actor(this._slider, 1);
        hbox.child_set_expand(this._slider, true);
        this._in_slider_update = false;
        this._slider.connect('slide-start',
                             Lang.bind(this, this._user_slide_start));
        this._slider.connect('slide-stop',
                             Lang.bind(this, this._user_slide_stop));
        this._slider.connect('notify::value',
                             Lang.bind(this, this._user_update_slider));

        this._remaining = new Mx.Label({ name: 'playing-elapsed' });
        hbox.add_actor(this._remaining, 2);

        // Playlist
        this.listview_actor = new Mx.ScrollView();
        this.listview_actor.set_scroll_policy(Mx.ScrollPolicy.VERTICAL);

        this.list = new Mx.BoxLayout({ orientation: Mx.Orientation.VERTICAL });
        this.listview_actor.add_actor(this.list);

        //
        this.controls_actor.show_all();
        this.listview_actor.show_all();

        //
        this._player.update_infos();
    },

    _updated_player: function() {
        this._update_item();
        this._update_playpause_button();
        this._current_position = this._player.position;
        this._update_slider(true);
    },

    _update_playpause_button: function() {
        if (this._player.playing)
            this._play_pause_button.set_property('icon-name',
                                                 'player_pause');
        else
            this._play_pause_button.set_property('icon-name',
                                                 'player_play');
    },

    _update_item: function() {
        if (this._player.cover != undefined) {
            let filename = GLib.filename_from_uri(this._player.cover, '');
            this._playing_item.set_property('cover-art', filename);
        }
        this._playing_item.set_property('artist-name', this._player.artist);
        this._playing_item.set_property('song-title', this._player.title);
        this._playing_item.set_property('album-title', this._player.album);
    },

    //
    _set_slider_text: function(label, prefix, minutes, seconds) {
        if (seconds < 10)
            label.set_text(prefix + minutes + ":0" + seconds);
        else
            label.set_text(prefix + minutes + ":" + seconds);
    },

    _update_slider: function(need_resched) {
        // TODO: Ok, there is a bug in rhythmbox here
        if (this._current_position > this._player.duration) {
            return;
        }

        this._in_slider_update = true;

        this._slider.set_value(this._current_position / this._player.duration);

        let seconds = this._current_position / (1000 * 1000);
        let minutes = Math.floor(seconds / 60);
        seconds = Math.floor(seconds - minutes * 60);
        this._set_slider_text(this._elapsed, "", minutes, seconds);

        seconds = (this._player.duration - this._current_position) / (1000 * 1000);
        minutes = Math.floor(seconds / 60);
        seconds = Math.floor(seconds - minutes * 60);
        this._set_slider_text(this._remaining, "-", minutes, seconds);

        if (need_resched) {
            // Reset auto-update
            if (this._timeout_handle > 0) {
                Mainloop.source_remove(this._timeout_handle);
                this._timeout_handle = 0;
            }
            if (this._player.playing) {
                this._timeout_handle =
                    Mainloop.timeout_add(1000,
                                         Lang.bind(this,
                                                   this._auto_inc_slider));
            }
        }

        this._in_slider_update = false;
    },

    _auto_inc_slider: function() {
        this._current_position += 1000 * 1000;
        if (!this._in_sliding)
            this._update_slider(false);
        return true;
    },

    _user_slide_start: function() {
        this._in_sliding = true;
    },

    _user_slide_stop: function() {
        this._player.SeekRemote((this._player.duration * this._slider.get_value ()) - this._current_position);
        this._in_sliding = false;
    },

    _user_update_slider: function() {
        if (!this._in_slider_update && !this._in_sliding) {
            // Mpris seek is a bit... sick (huhuhu).
            this._player.SeekRemote((this._player.duration * this._slider.get_value ()) - this._current_position);
        }
    }
};

//
// Main
//

GtkClutter.init(null, null);

Gio.DesktopAppInfo.set_desktop_env('GNOME');

let style = Mx.Style.get_default();
style.load_from_file(Path.get_panel_css_path('music'));

let panel = new DawatiPanel.MplPanelGtk({ name: 'music',
                                          tooltip: 'Music' });
panel.set_size_request(1024, 580);

let mwindow = panel.get_window();

let embed = new GtkClutter.Embed();
embed.set_hexpand(true);
embed.set_vexpand(true);
mwindow.add(embed);
embed.show();

let stage = embed.get_stage();

let vbox = new Mx.BoxLayout({ orientation: Mx.Orientation.VERTICAL });
vbox.add_constraint (new Clutter.BindConstraint({ coordinate: Clutter.BindCoordinate.SIZE,
                                                  source: stage }));
stage.add_actor(vbox);

let label = new Mx.Label({ name: 'title',
                           text: _("Music") });
vbox.add_actor(label, 0);

let table = new Mx.Table({ name: 'panel-grid' });
vbox.add_actor(table, 1);
vbox.child_set_expand(table, true);


let label = new Mx.Label({ name: 'section-title',
                           text: _("Library") });
table.add_actor(label, 0, 0);
table.child_set_x_fill(label, false);
table.child_set_x_expand(label, false);
table.child_set_y_fill(label, false);
table.child_set_y_expand(label, false);
table.child_set_x_align(label, Mx.Align.START);

let label = new Mx.Label({ name: 'section-title',
                           text: _("Now Playing") });
table.add_actor(label, 0, 1);
table.child_set_x_fill(label, false);
table.child_set_x_expand(label, false);
table.child_set_y_fill(label, false);
table.child_set_y_expand(label, false);
table.child_set_x_align(label, Mx.Align.START);

let lib = new AudioLibrary(panel);
table.add_actor(lib.controls_actor, 1, 0);
table.child_set_x_fill(lib.controls_actor, true);
table.child_set_x_expand(lib.controls_actor, true);
table.child_set_y_fill(lib.controls_actor, false);
table.child_set_y_expand(lib.controls_actor, false);

table.add_actor(lib.listview_actor, 2, 0);

let player = new NowPlaying();
table.add_actor(player.controls_actor, 1, 1);
table.child_set_x_fill(player.controls_actor, false);
table.child_set_x_expand(player.controls_actor, false);
table.child_set_y_fill(player.controls_actor, false);
table.child_set_y_expand(player.controls_actor, false);

table.add_actor(player.listview_actor, 2, 1);
table.child_set_x_expand(player.listview_actor, false);

Gtk.main();
