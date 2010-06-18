
#ifndef _MPL_PANEL_COMMON
#define _MPL_PANEL_COMMON

/**
 * MPL_PANEL_DBUS_NAME_PREFIX:
 *
 * Common prefix used by Panel dbus service names. For example, service for
 * panel 'foo' will have the name MPL_PANEL_DBUS_NAME_PREFIX + 'foo'.
 */
#define MPL_PANEL_DBUS_NAME_PREFIX "com.meego.UX.Shell.Panels."

/**
 * MPL_PANEL_DBUS_PATH_PREFIX:
 *
 * Common prefix used by Panel dbus object paths. For example, the dbus object
 * for panel 'foo' will be found at MPL_PANEL_DBUS_PATH_PREFIX + 'foo'.
 */
#define MPL_PANEL_DBUS_PATH_PREFIX "/com/meego/UX/Shell/Panels/"

/**
 * MPL_PANEL_DBUS_INTERFACE:
 *
 * Dbus interface for Panel functionality.
 */
#define MPL_PANEL_DBUS_INTERFACE   "com.meego.UX.Shell.Panel"

/**
 * MPL_TOOLBAR_DBUS_PATH:
 *
 * Path for the Toolbar dbus object.
 */
#define MPL_TOOLBAR_DBUS_PATH      "/com/meego/UX/Shell/Toolbar"

/**
 * MPL_TOOLBAR_DBUS_NAME:
 *
 * Name of the dbus Toolbar service.
 */
#define MPL_TOOLBAR_DBUS_NAME      "com.meego.UX.Shell.Toolbar"

/**
 * MPL_TOOLBAR_DBUS_INTERFACE:
 *
 * Dbus interface for Toolbar functionality.
 */
#define MPL_TOOLBAR_DBUS_INTERFACE "com.meego.UX.Shell.Toolbar"

/**
 * MPL_PANEL_MYZONE:
 *
 * Canonical name of the myzone panel.
 */
#define MPL_PANEL_MYZONE       "myzone"

/**
 * MPL_PANEL_STATUS:
 *
 * Canonical name of the status panel.
 */
#define MPL_PANEL_STATUS       "status"

/**
 * MPL_PANEL_ZONES:
 *
 * Canonical name of the zones panel.
 */
#define MPL_PANEL_ZONES        "zones"

/**
 * MPL_PANEL_INTERNET:
 *
 * Canonical name of the internet panel.
 */
#define MPL_PANEL_INTERNET     "internet"

/**
 * MPL_PANEL_MEDIA:
 *
 * Canonical name of the media panel.
 */
#define MPL_PANEL_MEDIA        "media"

/**
 * MPL_PANEL_APPLICATIONS:
 *
 * Canonical name of the applications panel.
 */
#define MPL_PANEL_APPLICATIONS "applications"

/**
 * MPL_PANEL_PEOPLE:
 *
 * Canonical name of the people panel.
 */
#define MPL_PANEL_PEOPLE       "people"

/**
 * MPL_PANEL_PASTEBOARD:
 *
 * Canonical name of the pasteboard panel.
 */
#define MPL_PANEL_PASTEBOARD   "pasteboard"

/**
 * MPL_PANEL_NETWORK:
 *
 * Canonical name of the network panel.
 */
#define MPL_PANEL_NETWORK      "network"

/**
 * MPL_PANEL_BLUETOOTH:
 *
 * Canonical name of the bluetooth panel.
 */
#define MPL_PANEL_BLUETOOTH    "bluetooth"

/**
 * MPL_PANEL_POWER:
 *
 * Canonical name of the power icon.
 */
#define MPL_PANEL_POWER        "power"

/**
 * MPL_PANEL_TEST:
 *
 * Canonical name for a test panel.
 */
#define MPL_PANEL_TEST         "test"

/**
 * MnbButtonState:
 * @MNB_BUTTON_NORMAL: normal (visible, interactive) button state
 * @MNB_BUTTON_HIDDEN: button should not be visible to user
 * @MNB_BUTTON_INSENSITIVE: button should be insensitive to user actions
 *
 * State of the Toolbar button; values are or-able.
 */
typedef enum
{
  MNB_BUTTON_NORMAL      = 0,
  MNB_BUTTON_HIDDEN      = 0x1,
  MNB_BUTTON_INSENSITIVE = 0x2
} MnbButtonState;

#endif
