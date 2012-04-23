// -*- mode: js; js-indent-level: 4; indent-tabs-mode: nil -*-

const Cairo = imports.cairo;
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
// const CtrlAltTab = imports.ui.ctrlAltTab;
const DND = imports.ui.dnd;
const Layout = imports.ui.layout;
const PopupMenu = imports.ui.popupMenu;
const PanelMenu = imports.ui.panelMenu;
const DateMenu = imports.ui.dateMenu;
const Toolbar = imports.ui.toolbar;
const Main = imports.ui.main;
const Tweener = imports.ui.tweener;

const PANEL_ICON_SIZE = 24;

const TOOLBAR_TRIGGER_THRESHOLD = 1;
const TOOLBAR_TRIGGER_ADJUSTMENT = 2;
const TOOLBAR_TRIGGER_TIMEOUT = 500;

const BUTTON_DND_ACTIVATION_TIMEOUT = 250;

const ANIMATED_ICON_UPDATE_TIMEOUT = 100;
const SPINNER_ANIMATION_TIME = 0.2;

const STANDARD_STATUS_AREA_ORDER = ['a11y', 'bluetooth', 'keyboard', 'volume', 'battery'];
const STANDARD_STATUS_AREA_SHELL_IMPLEMENTATION = {
    'a11y': imports.ui.status.accessibility.ATIndicator,
    'volume': imports.ui.status.volume.Indicator,
    'battery': imports.ui.status.power.Indicator,
    'keyboard': imports.ui.status.keyboard.XKBIndicator
};

STANDARD_STATUS_AREA_SHELL_IMPLEMENTATION['bluetooth'] = imports.ui.status.bluetooth.Indicator;

// try {
//     STANDARD_STATUS_AREA_SHELL_IMPLEMENTATION['network'] = imports.ui.status.network.NMApplet;
// } catch(e) {
//     log('NMApplet is not supported. It is possible that your NetworkManager version is too old');
// }

const GDM_STATUS_AREA_ORDER = ['a11y', 'display', 'keyboard', 'volume', 'battery'//, 'powerMenu'
                              ];
const GDM_STATUS_AREA_SHELL_IMPLEMENTATION = {
    'a11y': imports.ui.status.accessibility.ATIndicator,
    'volume': imports.ui.status.volume.Indicator,
    'battery': imports.ui.status.power.Indicator,
    'keyboard': imports.ui.status.keyboard.XKBIndicator//,
    //'powerMenu': imports.gdm.powerMenu.PowerMenuButton
};

// To make sure the panel corners blend nicely with the panel,
// we draw background and borders the same way, e.g. drawing
// them as filled shapes from the outside inwards instead of
// using cairo stroke(). So in order to give the border the
// appearance of being drawn on top of the background, we need
// to blend border and background color together.
// For that purpose we use the following helper methods, taken
// from st-theme-node-drawing.c
function _norm(x) {
    return Math.round(x / 255);
}

function _over(srcColor, dstColor) {
    let src = _premultiply(srcColor);
    let dst = _premultiply(dstColor);
    let result = new Clutter.Color();

    result.alpha = src.alpha + _norm((255 - src.alpha) * dst.alpha);
    result.red = src.red + _norm((255 - src.alpha) * dst.red);
    result.green = src.green + _norm((255 - src.alpha) * dst.green);
    result.blue = src.blue + _norm((255 - src.alpha) * dst.blue);

    return _unpremultiply(result);
}

function _premultiply(color) {
    return new Clutter.Color({ red: _norm(color.red * color.alpha),
                               green: _norm(color.green * color.alpha),
                               blue: _norm(color.blue * color.alpha),
                               alpha: color.alpha });
};

function _unpremultiply(color) {
    if (color.alpha == 0)
        return new Clutter.Color();

    let red = Math.min((color.red * 255 + 127) / color.alpha, 255);
    let green = Math.min((color.green * 255 + 127) / color.alpha, 255);
    let blue = Math.min((color.blue * 255 + 127) / color.alpha, 255);
    return new Clutter.Color({ red: red, green: green,
                               blue: blue, alpha: color.alpha });
};

const AnimatedIcon = new Lang.Class({
    Name: 'AnimatedIcon',

    _init: function(name, size) {
        this.actor = new St.Bin({ visible: false });
        this.actor.connect('destroy', Lang.bind(this, this._onDestroy));
        this.actor.connect('notify::visible', Lang.bind(this, this._onVisibleNotify));

        this._timeoutId = 0;
        this._frame = 0;
        this._animations = St.TextureCache.get_default().load_sliced_image (global.datadir + '/theme/' + name, size, size);
        this.actor.set_child(this._animations);
    },

    _disconnectTimeout: function() {
        if (this._timeoutId > 0) {
            Mainloop.source_remove(this._timeoutId);
            this._timeoutId = 0;
        }
    },

    _onVisibleNotify: function() {
        if (this.actor.visible)
            this._timeoutId = Mainloop.timeout_add(ANIMATED_ICON_UPDATE_TIMEOUT, Lang.bind(this, this._update));
        else
            this._disconnectTimeout();
    },

    _showFrame: function(frame) {
        let oldFrameActor = this._animations.get_child_at_index(this._frame);
        if (oldFrameActor)
            oldFrameActor.hide();

        this._frame = (frame % this._animations.get_n_children());

        let newFrameActor = this._animations.get_child_at_index(this._frame);
        if (newFrameActor)
            newFrameActor.show();
    },

    _update: function() {
        this._showFrame(this._frame + 1);
        return true;
    },

    _onDestroy: function() {
        this._disconnectTimeout();
    }
});

const PanelCorner = new Lang.Class({
    Name: 'PanelCorner',

    _init: function(box, side) {
        this._side = side;

        this._box = box;
        this._box.connect('style-changed', Lang.bind(this, this._boxStyleChanged));

        this.actor = new St.DrawingArea({ style_class: 'panel-corner' });
        this.actor.connect('style-changed', Lang.bind(this, this._styleChanged));
        this.actor.connect('repaint', Lang.bind(this, this._repaint));
    },

    _findRightmostButton: function(container) {
        if (!container.get_children)
            return null;

        let children = container.get_children();

        if (!children || children.length == 0)
            return null;

        // Start at the back and work backward
        let index = children.length - 1;
        while (!children[index].visible && index >= 0)
            index--;

        if (index < 0)
            return null;

        if (!(children[index].has_style_class_name('panel-menu')) &&
            !(children[index].has_style_class_name('panel-button')))
            return this._findRightmostButton(children[index]);

        return children[index];
    },

    _findLeftmostButton: function(container) {
        if (!container.get_children)
            return null;

        let children = container.get_children();

        if (!children || children.length == 0)
            return null;

        // Start at the front and work forward
        let index = 0;
        while (!children[index].visible && index < children.length)
            index++;

        if (index == children.length)
            return null;

        if (!(children[index].has_style_class_name('panel-menu')) &&
            !(children[index].has_style_class_name('panel-button')))
            return this._findLeftmostButton(children[index]);

        return children[index];
    },

    _boxStyleChanged: function() {
        let side = this._side;

        let rtlAwareContainer = this._box instanceof St.BoxLayout;
        if (rtlAwareContainer &&
            this._box.get_text_direction() == Clutter.TextDirection.RTL) {
            if (this._side == St.Side.LEFT)
                side = St.Side.RIGHT;
            else if (this._side == St.Side.RIGHT)
                side = St.Side.LEFT;
        }

        let button;
        if (side == St.Side.LEFT)
            button = this._findLeftmostButton(this._box);
        else if (side == St.Side.RIGHT)
            button = this._findRightmostButton(this._box);

        if (button) {
            if (this._button && this._buttonStyleChangedSignalId) {
                this._button.disconnect(this._buttonStyleChangedSignalId);
                this._button.style = null;
            }

            this._button = button;

            button.connect('destroy', Lang.bind(this,
                function() {
                    if (this._button == button) {
                        this._button = null;
                        this._buttonStyleChangedSignalId = 0;
                    }
                }));

            // Synchronize the locate button's pseudo classes with this corner
            this._buttonStyleChangedSignalId = button.connect('style-changed', Lang.bind(this,
                function(actor) {
                    let pseudoClass = button.get_style_pseudo_class();
                    this.actor.set_style_pseudo_class(pseudoClass);
                }));

            // The corner doesn't support theme transitions, so override
            // the .panel-button default
            button.style = 'transition-duration: 0';
        }
    },

    _repaint: function() {
        let node = this.actor.get_theme_node();

        let cornerRadius = node.get_length("-panel-corner-radius");
        let borderWidth = node.get_length('-panel-corner-border-width');

        let backgroundColor = node.get_color('-panel-corner-background-color');
        let borderColor = node.get_color('-panel-corner-border-color');

        let cr = this.actor.get_context();
        cr.setOperator(Cairo.Operator.SOURCE);

        cr.moveTo(0, 0);
        if (this._side == St.Side.LEFT)
            cr.arc(cornerRadius,
                   borderWidth + cornerRadius,
                   cornerRadius, Math.PI, 3 * Math.PI / 2);
        else
            cr.arc(0,
                   borderWidth + cornerRadius,
                   cornerRadius, 3 * Math.PI / 2, 2 * Math.PI);
        cr.lineTo(cornerRadius, 0);
        cr.closePath();

        let savedPath = cr.copyPath();

        let xOffsetDirection = this._side == St.Side.LEFT ? -1 : 1;
        let over = _over(borderColor, backgroundColor);
        Clutter.cairo_set_source_color(cr, over);
        cr.fill();

        let offset = borderWidth;
        Clutter.cairo_set_source_color(cr, backgroundColor);

        cr.save();
        cr.translate(xOffsetDirection * offset, - offset);
        cr.appendPath(savedPath);
        cr.fill();
        cr.restore();
    },

    _styleChanged: function() {
        let node = this.actor.get_theme_node();

        let cornerRadius = node.get_length("-panel-corner-radius");
        let borderWidth = node.get_length('-panel-corner-border-width');

        this.actor.set_size(cornerRadius, borderWidth + cornerRadius);
        this.actor.set_anchor_point(0, borderWidth);
    }
});

const Panel = new Lang.Class({
    Name: 'Panel',

    _init : function() {
        this.actor = new Shell.GenericContainer({ name: 'panel',
                                                  reactive: true });
        this.actor._delegate = this;

        this._statusArea = {};

        this._menus = new PopupMenu.PopupMenuManager(this);

        this._leftBox = new St.BoxLayout({ name: 'panelLeft' });
        this.actor.add_actor(this._leftBox);
        this._centerBox = new St.BoxLayout({ name: 'panelCenter' });
        this.actor.add_actor(this._centerBox);
        this._rightBox = new St.BoxLayout({ name: 'panelRight' });
        this.actor.add_actor(this._rightBox);

        this.actor.connect('get-preferred-width', Lang.bind(this, this._getPreferredWidth));
        this.actor.connect('get-preferred-height', Lang.bind(this, this._getPreferredHeight));
        this.actor.connect('allocate', Lang.bind(this, this._allocate));
        this.actor.connect('button-press-event', Lang.bind(this, this._onButtonPress));

        /* left */
        if (global.session_type == Shell.SessionType.USER)
            this._dateMenu = new DateMenu.DateMenuButton({ showEvents: true });
        else
            this._dateMenu = new DateMenu.DateMenuButton({ showEvents: false });
        this._leftBox.add(this._dateMenu.actor, { y_fill: true });
        this._menus.addMenu(this._dateMenu.menu);

        /* center (listen to events) */
        this._isToolbarVisible = false;
        this._isToolbarInShowTransition = false;
        this._toolbarTriggerTimeoutId = 0;
        this._toolbarTriggerThreshold = TOOLBAR_TRIGGER_THRESHOLD;
        this._centerBox.set_reactive(true);
        this._centerBox.connect('event', Lang.bind(this, this._onCenterEvent));

        /* right */
        if (global.session_type == Shell.SessionType.GDM) {
            this._status_area_order = GDM_STATUS_AREA_ORDER;
            this._status_area_shell_implementation = GDM_STATUS_AREA_SHELL_IMPLEMENTATION;
        } else {
            this._status_area_order = STANDARD_STATUS_AREA_ORDER;
            this._status_area_shell_implementation = STANDARD_STATUS_AREA_SHELL_IMPLEMENTATION;
        }


        Main.layoutManager.panelBox.add(this.actor);
        // Main.ctrlAltTabManager.addGroup(this.actor, _("Top Bar"), 'start-here',
        //                                 { sortGroup: CtrlAltTab.SortGroup.TOP });

        /* Add the toolbar only once the panel has been added to the
         * stage. */
        this._toolbar = new Toolbar.Toolbar();
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

        let [leftMinWidth, leftNaturalWidth] = this._leftBox.get_preferred_width(-1);
        let [centerMinWidth, centerNaturalWidth] = this._centerBox.get_preferred_width(-1);
        let [rightMinWidth, rightNaturalWidth] = this._rightBox.get_preferred_width(-1);

        let sideWidth, centerWidth;
        centerWidth = centerNaturalWidth;
        sideWidth = (allocWidth - centerWidth) / 2;

        let childBox = new Clutter.ActorBox();

        childBox.y1 = 0;
        childBox.y2 = allocHeight;

        if (this.actor.get_text_direction() == Clutter.TextDirection.RTL) {
            childBox.x1 = allocWidth - Math.min(Math.floor(sideWidth),
                                                leftNaturalWidth);
            childBox.x2 = allocWidth;
        } else {
            childBox.x1 = 0;
            childBox.x2 = Math.min(Math.floor(sideWidth),
                                   leftNaturalWidth);
        }
        this._leftBox.allocate(childBox, flags);

        childBox.x1 = childBox.x2;
        childBox.x2 = allocWidth - childBox.x2 - Math.min(Math.floor(sideWidth),
                                                          rightNaturalWidth);
        this._centerBox.allocate(childBox, flags);

        if (this.actor.get_text_direction() == Clutter.TextDirection.RTL) {
            childBox.x1 = 0;
            childBox.x2 = Math.min(Math.floor(sideWidth),
                                   rightNaturalWidth);
        } else {
            childBox.x1 = allocWidth - Math.min(Math.floor(sideWidth),
                                                rightNaturalWidth);
            childBox.x2 = allocWidth;
        }
        this._rightBox.allocate(childBox, flags);
    },

    _onButtonPress: function(actor, event) {
        if (event.get_source() != actor)
            return false;

        let button = event.get_button();
        if (button != 1)
            return false;

        let focusWindow = global.display.focus_window;
        if (!focusWindow)
            return false;

        let dragWindow = focusWindow.is_attached_dialog() ? focusWindow.get_transient_for()
                                                          : focusWindow;
        if (!dragWindow)
            return false;

        let rect = dragWindow.get_outer_rect();
        let [stageX, stageY] = event.get_coords();

        let allowDrag = dragWindow.maximized_vertically &&
                        stageX > rect.x && stageX < rect.x + rect.width;

        if (!allowDrag)
            return false;

        global.display.begin_grab_op(global.screen,
                                     dragWindow,
                                     Meta.GrabOp.MOVING,
                                     false, /* pointer grab */
                                     true, /* frame action */
                                     button,
                                     event.get_state(),
                                     event.get_time(),
                                     stageX, stageY);

        return true;
    },

    _triggerToolbar: function() {
        this._toolbar.show();
        this._toolbarTriggerTimeoutId = 0;

        return false;
    },

    _pointerInZone: function(y) {
        return ((y >= 0) && (y <= this._toolbarTriggerThreshold));
    },

    _onCenterEvent: function(actor, event) {
        let ev_type = event.type();

        if (!(ev_type == Clutter.EventType.ENTER ||
              ev_type == Clutter.EventType.LEAVE ||
              ev_type == Clutter.EventType.MOTION))
            return false;

        let [x, y] = event.get_coords();
        let show_toolbar = ((ev_type == Clutter.EventType.ENTER) &&
                            this._pointerInZone(y));
        show_toolbar |= ((ev_type == Clutter.EventType.LEAVE) &&
                         this._pointerInZone(y));
        show_toolbar |= ((ev_type == Clutter.EventType.MOTION) &&
                         this._pointerInZone(y));

        if (show_toolbar) {
            if (!this._isToolbarVisible &&
                !this._isToolbarInShowTransition) {

                if (this._toolbarTriggerTimeoutId == 0) {
                    /*
                     * Increase sensitivity -- increasing size of the
                     * trigger zone while the timeout reduces the
                     * effect of a shaking hand.
                     */
                    this._toolbarTriggerThreshold = TOOLBAR_TRIGGER_ADJUSTMENT;
                    this._toolbarTriggerTimeoutId =
                        Mainloop.timeout_add(TOOLBAR_TRIGGER_TIMEOUT,
                                             Lang.bind(this, this._triggerToolbar));
                }
            }

            this._inTriggerZone = true;
        } else {
            if (this._toolbarTriggerTimeoutId != 0) {
                Mainloop.source_remove(this._toolbarTriggerTimeoutId);
                this._toolbarTriggerTimeoutId = 0;
            } else if (this.isToolbarVisible) {
                this._toolbarTriggerThreshold = TOOLBAR_TRIGGER_THRESHOLD;
                this._toolbar.hide();
            }

            this._inTriggerZone = false;
        }

        return true;
    },

    startStatusArea: function() {
        for (let i = 0; i < this._status_area_order.length; i++) {
            let role = this._status_area_order[i];
            let constructor = this._status_area_shell_implementation[role];
            if (!constructor) {
                // This icon is not implemented (this is a bug)
                continue;
            }

            let indicator = new constructor();
            this.addToStatusArea(role, indicator, i);
        }
    },

    _insertStatusItem: function(actor, position) {
        let children = this._rightBox.get_children();
        let i;
        for (i = children.length - 1; i >= 0; i--) {
            let rolePosition = children[i]._rolePosition;
            if (position > rolePosition) {
                this._rightBox.insert_child_at_index(actor, i + 1);
                break;
            }
        }
        if (i == -1) {
            // If we didn't find a position, we must be first
            this._rightBox.insert_child_at_index(actor, 0);
        }
        actor._rolePosition = position;
    },

    _onTrayIconAdded: function(o, icon, role) {
        if (this._status_area_shell_implementation[role]) {
            // This icon is legacy, and replaced by a Shell version
            // Hide it
            return;
        }

        icon.height = PANEL_ICON_SIZE;
        let buttonBox = new PanelMenu.ButtonBox();
        let box = buttonBox.actor;
        box.add_actor(icon);

        this._insertStatusItem(box, this._status_area_order.indexOf(role));
    },

    _onTrayIconRemoved: function(o, icon) {
        let box = icon.get_parent();
        if (box && box._delegate instanceof PanelMenu.ButtonBox)
            box.destroy();
    },

    /* Public */

    addToStatusArea: function(role, indicator, position) {
        if (this._statusArea[role])
            throw new Error('Extension point conflict: there is already a status indicator for role ' + role);

        if (!(indicator instanceof PanelMenu.Button))
            throw new TypeError('Status indicator must be an instance of PanelMenu.Button');

        if (!position)
            position = 0;
        this._insertStatusItem(indicator.actor, position);
        this._menus.addMenu(indicator.menu);

        this._statusArea[role] = indicator;
        let destroyId = indicator.connect('destroy', Lang.bind(this, function(emitter) {
            this._statusArea[role] = null;
            emitter.disconnect(destroyId);
        }));

        return indicator;
    },

    toggleToolbar: function() {
        if (this._toolbarTriggerTimeoutId != 0) {
            Mainloop.source_remove(this._toolbarTriggerTimeoutId);
            this._toolbarTriggerTimeoutId = 0;
        }

        if (this._toolbar.visible)
            this._toolbar.hide();
        else
            this._toolbar.show();
    }
});
