const Clutter = imports.gi.Clutter;
const Gio = imports.gi.Gio;
const GLib = imports.gi.GLib;
const Gtk = imports.gi.Gtk;
const Lang = imports.lang;
const Mainloop = imports.mainloop;
const Meta = imports.gi.Meta;
const Pango = imports.gi.Pango;
const Shell = imports.gi.Shell;
const St = imports.gi.St;
const Signals = imports.signals;
const Atk = imports.gi.Atk;

const Config = imports.misc.config;
const DND = imports.ui.dnd;
const Layout = imports.ui.layout;
const PopupMenu = imports.ui.popupMenu;
const PanelMenu = imports.ui.panelMenu;
const DateMenu = imports.ui.dateMenu;
const Main = imports.ui.main;
const Tweener = imports.ui.tweener;

const TOOLBAR_ANIMATION_TIME = 0.25;
const TOOLBAR_HIDE_TIMEOUT = 1500;


/**
 * The Toolbar widget
 */

const Toolbar = new Lang.Class({
    Name: 'Toolbar',

    _init : function() {
        this.actor = new Shell.GenericContainer({ name: 'toolbar',
                                                  reactive: true });
        this.actor._delegate = this;

        this._buttonBox = new St.BoxLayout({ name: 'toolbarButtonBox' });
        this.actor.add_actor(this._buttonBox);

        this.actor.connect('get-preferred-width',
                           Lang.bind(this, this._getPreferredWidth));
        this.actor.connect('get-preferred-height',
                           Lang.bind(this, this._getPreferredHeight));
        this.actor.connect('allocate', Lang.bind(this, this._allocate));
        this.actor.connect('enter-event', Lang.bind(this, this._enterEvent));
        this.actor.connect('leave-event', Lang.bind(this, this._leaveEvent));

        Main.uiGroup.add_actor(this.actor);

        /* Position toolbar */
        this.actor.lower(Main.layoutManager.panelBox);
        let [toolbarMinHeight, toolbarNaturalHeight] =
            this.actor.get_preferred_height(-1);
        this.actor.y = -toolbarNaturalHeight;

        /* Grab settings */
        this._settings = new Gio.Settings({ schema: 'org.dawati.shell.toolbar' });
        this._settings.connect('changed', Lang.bind(this, this._settingsChanged));

        this._hideTimeoutId = 0;
        this._panelList = new Array();
        this._buttonsHash = {};
        this._fixupPanels(this._settings.get_strv('order'));
    },

    _getPreferredWidth: function(actor, forHeight, alloc) {
        alloc.min_size = -1;
        alloc.natural_size = Main.layoutManager.primaryMonitor.width;
    },

    _getPreferredHeight: function(actor, forWidth, alloc) {
        // We don't need to implement this; it's forced by the CSS
        alloc.min_size = -1;
        alloc.natural_size = -1;
    },

    _allocate: function(actor, box, flags) {
        let allocWidth = box.x2 - box.x1;
        let allocHeight = box.y2 - box.y1;

        let [buttonBoxMinWidth, buttonBoxNaturalWidth] =
            this._buttonBox.get_preferred_width(-1);

        let childBox = new Clutter.ActorBox();

        childBox.y1 = 0;
        childBox.y2 = allocHeight;

        childBox.x1 = Math.max(0,
                               (allocWidth / 2) - (buttonBoxNaturalWidth / 2));
        childBox.x2 = Math.min(allocWidth - childBox.x1,
                               childBox.x1 + buttonBoxNaturalWidth);
        this._buttonBox.allocate(childBox, flags);
    },

    _hideTimeout: function() {
        this.hide();
        return false;
    },

    _enterEvent: function(actor, event) {
        if (this._hideTimeoutId != 0)
            Mainloop.source_remove(this._hideTimeoutId);
    },

    _leaveEvent: function(actor, event) {
        if (this._hideTimeoutId != 0)
            Mainloop.source_remove(this._hideTimeoutId);
        this._hideTimeoutId = Mainloop.timeout_add(TOOLBAR_HIDE_TIMEOUT,
                                                   Lang.bind(this,
                                                             this._hideTimeout));
    },

    _settingsChanged: function(settings, key) {
        this._fixupPanels(this._settings.get_strv('order'));
    },

    _buttonClicked: function(button) {
        let name = button.name;

        if (this._lastClicked &&
            (this._lastClicked != this._buttonsHash[name]))
            this._lastClicked.actor.set_checked(false)

        this._lastClicked = this._buttonsHash[name];
    },

    _insertButton: function(name) {
        log("insert " + name);
        let button = new ToolbarButton(name);
        this._buttonsHash[name] = button;
        this._buttonBox.add_actor(button.actor);
        button.signalId =
            button.actor.connect('clicked',
                                 Lang.bind(this, this._buttonClicked));
    },

    _removeButton: function(name) {
        log("remove " + name);
        let button = this._buttonsHash[name];
        button.actor.disconnect(button.signalId);
        this._buttonBox.remove_actor(button.actor);
        button.unload();
        delete this._buttonsHash[name];
    },

    _fixupPanels: function(newList) {
        /* First create a hash table of already runnings panels as
         * well as panels in the new list. The keys of the hash table
         * are the panels' names, the values associated tell you
         * whether each of them must be
         * added(1)/removed(2)/left-as-is(3). */
        let panelHash = {};

        for (let i = 0; i < newList.length; i++) {
            panelHash[newList[i]] = 1;
        }

        if (this._buttonsHash) {
            for (let n in this._buttonsHash) {
                if (panelHash[n])
                    panelHash[n] += 2;
                else
                    panelHash[n] = 2;
            }
        }

        /* Go through the hash table and do what's needed. */
        for (let n in panelHash) {
            switch (panelHash[n]) {
            case 1:
                this._insertButton(n);
                break;

            case 2:
                this._removeButton(n);
                break;

            default:
                break;
            }
        }

        /* Refresh internal copy of panels' names list. */
        this._panelList = new Array();
        for (let i = 0; i < newList.length; i++)
            this._panelList.push(newList[i]);

        /* Finally reorder the icons. */
        for (let i = 0; i < this._panelList.length; i++) {
            let button = this._buttonsHash[this._panelList[i]].actor;
            this._buttonBox.set_child_above_sibling(button, null);
        }
    },

    /* Public */

    show: function() {
        let [toolbarMinHeight, toolbarNaturalHeight] =
            this.actor.get_preferred_height(-1);
        this.actor.y = (Main.panel.actor.height - toolbarNaturalHeight);
        Tweener.addTween(this.actor,
                         { y: Main.panel.actor.height,
                           transition: 'linear',
                           time: TOOLBAR_ANIMATION_TIME,
                         });
    },

    hide: function() {
        let [toolbarMinHeight, toolbarNaturalHeight] =
            this.actor.get_preferred_height(-1);
        Tweener.addTween(this.actor,
                         { y:  (Main.panel.actor.height - toolbarNaturalHeight),
                           transition: 'linear',
                           time: TOOLBAR_ANIMATION_TIME,
                         });
    }
});


/**
 * Toolbar button
 */

const PanelIface = <interface name="com.dawati.UX.Shell.Panel">
<method name="InitPanel">
    <arg name="x" type="i" direction="in"/>
    <arg name="y" type="i" direction="in"/>
    <arg name="max_window_width" type="i" direction="in"/>
    <arg name="max_window_height" type="i" direction="in"/>
    <arg name="panel_name" type="s" direction="out"/>
    <arg name="xid" type="u" direction="out"/>
    <arg name="tooltip" type="s" direction="out"/>
    <arg name="stylesheet" type="s" direction="out"/>
    <arg name="button_style" type="s" direction="out"/>
    <arg name="window_width" type="i" direction="out"/>
    <arg name="window_height" type="i" direction="out"/>
</method>

<method name="Unload"/>

<method name="SetMaximumSize">
    <arg name="max_window_width" type="i" direction="in"/>
    <arg name="max_window_height" type="i" direction="in"/>
</method>

<method name="Show"/>
<method name="ShowBegin"/>
<method name="ShowEnd"/>

<method name="Hide"/>
<method name="HideBegin"/>
<method name="HideEnd"/>

<method name="Ping"/>

<signal name="RequestButtonStyle">
    <arg name="style_id" type="s"/>
</signal>

<signal name="RequestTooltip">
    <arg name="tooltip" type="s"/>
</signal>

<signal name="RequestButtonState">
    <arg name="state" type="i"/>
</signal>

<signal name="SizeChanged">
    <arg name="requested_window_width" type="i"/>
    <arg name="requested_window_height" type="i"/>
</signal>

<signal name="RequestFocus"/>

<signal name="RequestModality">
    <arg name="state" type="b"/>
</signal>

<signal name="Ready"/>

</interface>;

const PanelProxy = Gio.DBusProxy.makeProxyWrapper(PanelIface);

const ToolbarButton = new Lang.Class({
    Name: 'ToolbarButton',

    _init : function(name) {
        this.name = name;
        this._keyfile = new GLib.KeyFile();
        this._keyfile.load_from_file(Config.PANELS_DIR + "/" + name + ".desktop",
                                     GLib.KeyFileFlags.NONE);

        this.actor = new St.Button({ name: name });
        let hbox = new St.BoxLayout();
        this._image = new St.Bin();
        hbox.add_child(this._image);
        let label = new St.Label({ text: this._keyfile.get_string(GLib.KEY_FILE_DESKTOP_GROUP,
                                                                   'Name') });
        hbox.add_child(label);
        hbox.child_set(label, { y_align: St.Align.MIDDLE,
                                y_fill: false});
        this.actor.set_child(hbox);

        this.actor.set_toggle_mode(true);
        this.actor.connect('clicked', Lang.bind(this, this._clicked));

        try {
            this.actor.set_style_pseudo_class(this._keyfile.get_string(GLib.KEY_FILE_DESKTOP_GROUP,
                                                                       'X-Dawati-Panel-Button-Style'));
        } catch (e) {
        }

        this._windowId = 0;

        this._init_dbus();
    },

    _init_dbus: function() {
        let serviceName = this._keyfile.get_string(GLib.KEY_FILE_DESKTOP_GROUP,
                                                   'X-Dawati-Service');
        let objectPath = '/' + serviceName.replace(/\./g, '/');

        log(serviceName);
        log(objectPath);
        this._running = false;
        this._needShow = false;
        this._dbusProxy =
            new PanelProxy(Gio.DBus.session, serviceName, objectPath);

// ({ g_connection: Gio.DBus.session,
//                                 g_interface_name: 'com.dawati.UX.Shell.Panel',
//                                 g_interface_info: PanelInfo,
//                                 g_name: serviceName,
//                                 g_object_path: objectPath,
//                                 g_flags: (Gio.DBusProxyFlags.DO_NOT_AUTO_START |
//                                           Gio.DBusProxyFlags.DO_NOT_LOAD_PROPERTIES) });

        this._dbusProxy.connect('notify::g-name-owner', Lang.bind(this, function() {
            if (this._dbusProxy.g_name_owner)
                this._onNameAppeared();
            else
                this._onNameVanished();
        }));
        this._dbusProxy.connectSignal('Ready', Lang.bind(this, this._onReady));
        this._dbusProxy.connectSignal('RequestButtonStyle',
                                      Lang.bind(this, this._onButtonStyle));

    },

    _onInitialized: function([panelName, windowId,
                              tooltipText,
                              stylesheet, buttonStyle,
                              width, height]) {
        log("Initialized: " + panelName + " windowId=" + windowId);
        this._windowId = windowId;
        this._running = true;
        if (this._needShow)
            this.show();
    },

    _onNameAppeared: function(owner) {
        let monitor = Main.layoutManager.primaryMonitor;
        this._dbusProxy.InitPanelRemote(0, 0,
                                        monitor.width,
                                        Math.round(monitor.height - Main.panel.actor.height),
                                        Lang.bind(this, this._onInitialized),
                                        Gio.DBusCallFlags.NONE);
    },

    _onNameVanished: function(oldOwner) {
        this._running = false;
    },

    _onReady: function() {
        log("Ready " + this.name);
        this._running = true;
    },

    _onButtonStyle: function(styleName) {
        this.actor.set_style_pseudo_class(styleName);
    },

    _clicked: function() {
        if (this.actor.get_checked())
            this.show();
        else
            this.hide();
    },

    _pingReply: function() {
        let monitor = Main.layoutManager.primaryMonitor;
        this._dbusProxy.InitPanelRemote(0, 0,
                                        monitor.width,
                                        Math.round(monitor.height - Main.panel.actor.height),
                                        Lang.bind(this, this._onInitialized),
                                        Gio.DBusCallFlags.NONE);
    },

    /* Public */

    show: function() {
        if (!this._running) {
            this._needShow = true;
            this._dbusProxy.PingRemote(Lang.bind(this, this._pingReply),
                                       Gio.DBusCallFlags.NONE);
        } else {
            this._dbusProxy.ShowRemote();
        }
    },

    hide: function() {
        if (this._running)
            this._dbusProxy.HideRemote();
    },

    unload: function() {
        if (this._running)
            this._dbusProxy.UnloadRemote();
    }
});
