// -*- mode: js; js-indent-level: 4; indent-tabs-mode: nil -*-

const Clutter = imports.gi.Clutter;
const Gdk = imports.gi.Gdk;
const Gio = imports.gi.Gio;
const GLib = imports.gi.GLib;
const Lang = imports.lang;
const Mainloop = imports.mainloop;
const Meta = imports.gi.Meta;
const Shell = imports.gi.Shell;
const St = imports.gi.St;

const Layout = imports.ui.layout;
const Panel = imports.ui.panel;

const DEFAULT_BACKGROUND_COLOR = new Clutter.Color();
DEFAULT_BACKGROUND_COLOR.from_pixel(0x2266bbff);

let _errorLogStack = [];

let modalCount = 0;
let modalActorFocusStack = [];
let layoutManager = null;
let panel = null;
let uiGroup = null;

let _defaultCssStylesheet = null;
let _cssStylesheet = null;

/**
 * _log:
 * @category: string message type ('info', 'error')
 * @msg: A message string
 * ...: Any further arguments are converted into JSON notation,
 *      and appended to the log message, separated by spaces.
 *
 * Log a message into the LookingGlass error
 * stream.  This is primarily intended for use by the
 * extension system as well as debugging.
 */
function _log(category, msg) {
    let text = msg;
    if (arguments.length > 2) {
        text += ': ';
        for (let i = 2; i < arguments.length; i++) {
            text += JSON.stringify(arguments[i]);
            if (i < arguments.length - 1)
                text += ' ';
        }
    }
    _errorLogStack.push({timestamp: new Date().getTime(),
                         category: category,
                         message: text });
}

function _logError(msg) {
    return _log('error', msg);
}

function _logDebug(msg) {
    return _log('debug', msg);
}

// Used by the error display in lookingGlass.js
function _getAndClearErrorStack() {
    let errors = _errorLogStack;
    _errorLogStack = [];
    return errors;
}

function logStackTrace(msg) {
    try {
        throw new Error();
    } catch (e) {
        // e.stack must have at least two lines, with the first being
        // logStackTrace() (which we strip off), and the second being
        // our caller.
        let trace = e.stack.substr(e.stack.indexOf('\n') + 1);
        log(msg ? (msg + '\n' + trace) : trace);
    }
}

/**
 * Utils functions
 */

function isWindowActorDisplayedOnWorkspace(win, workspaceIndex) {
    return win.get_workspace() == workspaceIndex ||
        (win.get_meta_window() && win.get_meta_window().is_on_all_workspaces());
}

function getWindowActorsForWorkspace(workspaceIndex) {
    return global.get_window_actors().filter(function (win) {
        return isWindowActorDisplayedOnWorkspace(win, workspaceIndex);
    });
}

/**
 * getThemeStylesheet:
 *
 * Get the theme CSS file that the shell will load
 *
 * Returns: A file path that contains the theme CSS,
 *          null if using the default
 */
function getThemeStylesheet()
{
    return _cssStylesheet;
}

/**
 * setThemeStylesheet:
 * @cssStylesheet: A file path that contains the theme CSS,
 *                  set it to null to use the default
 *
 * Set the theme CSS file that the shell will load
 */
function setThemeStylesheet(cssStylesheet)
{
    _cssStylesheet = cssStylesheet;
}

/**
 * loadTheme:
 *
 * Reloads the theme CSS file
 */
function loadTheme() {
    let themeContext = St.ThemeContext.get_for_stage (global.stage);
    let previousTheme = themeContext.get_theme();

    let cssStylesheet = _defaultCssStylesheet;
    if (_cssStylesheet != null)
        cssStylesheet = _cssStylesheet;

    let theme = new St.Theme ({ application_stylesheet: cssStylesheet });

    if (previousTheme) {
        let customStylesheets = previousTheme.get_custom_stylesheets();

        for (let i = 0; i < customStylesheets.length; i++)
            theme.load_stylesheet(customStylesheets[i]);
    }

    themeContext.set_theme (theme);
}

function _findModal(actor) {
    for (let i = 0; i < modalActorFocusStack.length; i++) {
        if (modalActorFocusStack[i].actor == actor)
            return i;
    }
    return -1;
}

/**
 * pushModal:
 * @actor: #ClutterActor which will be given keyboard focus
 * @timestamp: optional timestamp
 *
 * Ensure we are in a mode where all keyboard and mouse input goes to
 * the stage, and focus @actor. Multiple calls to this function act in
 * a stacking fashion; the effect will be undone when an equal number
 * of popModal() invocations have been made.
 *
 * Next, record the current Clutter keyboard focus on a stack. If the
 * modal stack returns to this actor, reset the focus to the actor
 * which was focused at the time pushModal() was invoked.
 *
 * @timestamp is optionally used to associate the call with a specific user
 * initiated event.  If not provided then the value of
 * global.get_current_time() is assumed.
 *
 * @options: optional Meta.ModalOptions flags to indicate that the
 *           pointer is alrady grabbed
 *
 * Returns: true iff we successfully acquired a grab or already had one
 */
function pushModal(actor, timestamp, options) {
    if (timestamp == undefined)
        timestamp = global.get_current_time();

    if (modalCount == 0) {
        if (!global.begin_modal(timestamp, options ? options : 0)) {
            log('pushModal: invocation of begin_modal failed');
            return false;
        }
    }

    global.set_stage_input_mode(Shell.StageInputMode.FULLSCREEN);

    modalCount += 1;
    let actorDestroyId = actor.connect('destroy', function() {
        let index = _findModal(actor);
        if (index >= 0)
            modalActorFocusStack.splice(index, 1);
    });
    let curFocus = global.stage.get_key_focus();
    let curFocusDestroyId;
    if (curFocus != null) {
        curFocusDestroyId = curFocus.connect('destroy', function() {
            let index = _findModal(actor);
            if (index >= 0)
                modalActorFocusStack[index].actor = null;
        });
    }
    modalActorFocusStack.push({ actor: actor,
                                focus: curFocus,
                                destroyId: actorDestroyId,
                                focusDestroyId: curFocusDestroyId });

    global.stage.set_key_focus(actor);
    return true;
}

/**
 * popModal:
 * @actor: #ClutterActor passed to original invocation of pushModal().
 * @timestamp: optional timestamp
 *
 * Reverse the effect of pushModal().  If this invocation is undoing
 * the topmost invocation, then the focus will be restored to the
 * previous focus at the time when pushModal() was invoked.
 *
 * @timestamp is optionally used to associate the call with a specific user
 * initiated event.  If not provided then the value of
 * global.get_current_time() is assumed.
 */
function popModal(actor, timestamp) {
    if (timestamp == undefined)
        timestamp = global.get_current_time();

    let focusIndex = _findModal(actor);
    if (focusIndex < 0) {
        global.stage.set_key_focus(null);
        global.end_modal(timestamp);
        global.set_stage_input_mode(Shell.StageInputMode.NORMAL);

        throw new Error('incorrect pop');
    }

    modalCount -= 1;

    let record = modalActorFocusStack[focusIndex];
    record.actor.disconnect(record.destroyId);

    if (focusIndex == modalActorFocusStack.length - 1) {
        if (record.focus)
            record.focus.disconnect(record.focusDestroyId);
        global.stage.set_key_focus(record.focus);
    } else {
        let t = modalActorFocusStack[modalActorFocusStack.length - 1];
        if (t.focus)
            t.focus.disconnect(t.focusDestroyId);
        // Remove from the middle, shift the focus chain up
        for (let i = modalActorFocusStack.length - 1; i > focusIndex; i--) {
            modalActorFocusStack[i].focus = modalActorFocusStack[i - 1].focus;
            modalActorFocusStack[i].focusDestroyId = modalActorFocusStack[i - 1].focusDestroyId;
        }
    }
    modalActorFocusStack.splice(focusIndex, 1);

    if (modalCount > 0)
        return;

    global.end_modal(timestamp);
    global.set_stage_input_mode(Shell.StageInputMode.NORMAL);
}

/**
 * Plugin startup function
 */
function start() {
    // Monkey patch utility functions into the global proxy;
    // This is easier and faster than indirecting down into global
    // if we want to call back up into JS.
    global.logError = _logError;
    global.log = _logDebug;

    // The stage is always covered so Clutter doesn't need to clear it; however
    // the color is used as the default contents for the Mutter root background
    // actor so set it anyways.
    global.stage.color = DEFAULT_BACKGROUND_COLOR;
    global.stage.no_clear_hint = true;

    _defaultCssStylesheet = global.datadir + '/theme/dawati-shell.css';
    loadTheme();

    log("creating basic layout");
    // Set up stage hierarchy to group all UI actors under one container.
    uiGroup = new Shell.GenericContainer({ name: 'uiGroup' });
    uiGroup.connect('allocate',
                    function (actor, box, flags) {
                        let children = uiGroup.get_children();
                        for (let i = 0; i < children.length; i++)
                            children[i].allocate_preferred_size(flags);
                    });
    uiGroup.connect('get-preferred-width',
                    function(actor, forHeight, alloc) {
                        let width = global.stage.width;
                        [alloc.min_size, alloc.natural_size] = [width, width];
                    });
    uiGroup.connect('get-preferred-height',
                    function(actor, forWidth, alloc) {
                        let height = global.stage.height;
                        [alloc.min_size, alloc.natural_size] = [height, height];
                    });
    global.window_group.reparent(uiGroup);
    global.overlay_group.reparent(uiGroup);
    global.stage.add_actor(uiGroup);

    layoutManager = new Layout.LayoutManager();
    panel = new Panel.Panel();

    panel.startStatusArea();

    layoutManager.init();

    Meta.keybindings_set_custom_handler('panel-main-menu', function () {
        panel.toggleToolbar();
    });

    global.display.connect('overlay-key',
                           Lang.bind(this, function() {
                               panel.toggleToolbar();
                           }));
}
