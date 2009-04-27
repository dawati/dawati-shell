/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */

/*
 * Based on Gnome Shell.
 */
#ifndef __SHELL_TRAY_MANAGER_H__
#define __SHELL_TRAY_MANAGER_H__

#include <clutter/clutter.h>
#include <clutter/x11/clutter-x11.h>
#include <nbtk/nbtk.h>

G_BEGIN_DECLS

#define SHELL_TYPE_TRAY_MANAGER			(shell_tray_manager_get_type ())
#define SHELL_TRAY_MANAGER(obj)			(G_TYPE_CHECK_INSTANCE_CAST ((obj), SHELL_TYPE_TRAY_MANAGER, ShellTrayManager))
#define SHELL_TRAY_MANAGER_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST ((klass), SHELL_TYPE_TRAY_MANAGER, ShellTrayManagerClass))
#define SHELL_IS_TRAY_MANAGER(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), SHELL_TYPE_TRAY_MANAGER))
#define SHELL_IS_TRAY_MANAGER_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), SHELL_TYPE_TRAY_MANAGER))
#define SHELL_TRAY_MANAGER_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), SHELL_TYPE_TRAY_MANAGER, ShellTrayManagerClass))

typedef struct _ShellTrayManager        ShellTrayManager;
typedef struct _ShellTrayManagerPrivate ShellTrayManagerPrivate;
typedef struct _ShellTrayManagerClass   ShellTrayManagerClass;

struct _ShellTrayManager
{
  GObject parent_instance;

  ShellTrayManagerPrivate *priv;
};

struct _ShellTrayManagerClass
{
  GObjectClass parent_class;

  void (* tray_icon_added)   (ShellTrayManager *manager,
			      ClutterActor     *icon);
  void (* tray_icon_removed) (ShellTrayManager *manager,
			      ClutterActor     *icon);

};

GType             shell_tray_manager_get_type     (void);

ShellTrayManager *shell_tray_manager_new          (void);
void              shell_tray_manager_manage_stage (ShellTrayManager *manager,
                                                   ClutterStage     *stage);
gboolean          shell_tray_manager_is_config_window (ShellTrayManager *manager,
                                                       Window xwindow);
gboolean          shell_tray_manager_config_windows_showing (ShellTrayManager *manager);

void              shell_tray_manager_hide_config_window (ShellTrayManager *manager,
                                                         Window xwindow);

void              shell_tray_manager_close_config_window (ShellTrayManager *manager,
                                                          Window            xwindow);

void              shell_tray_manager_close_all_config_windows (ShellTrayManager *manager);

void              shell_tray_manager_close_all_other_config_windows (ShellTrayManager *manager,
                                                                     Window            xwindow);


G_END_DECLS

#endif /* __SHELL_TRAY_MANAGER_H__ */
