/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */

/*
 * Copyright (C) 2010 Intel Corporation.
 *
 * Author: Srinivasa Ragavan <srini@linux.intel.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _MNP_SHELL
#define _MNP_SHELL

#include <glib-object.h>
#include <mx/mx.h>
#include <meego-panel/mpl-panel-clutter.h>
#include <meego-panel/mpl-panel-common.h>

G_BEGIN_DECLS

#define MNP_TYPE_SHELL mnp_shell_get_type()

#define MNP_SHELL(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), MNP_TYPE_SHELL, MnpShell))

#define MNP_SHELL_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), MNP_TYPE_SHELL, MnpShellClass))

#define MNP_IS_SHELL(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MNP_TYPE_SHELL))

#define MNP_IS_SHELL_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), MNP_TYPE_SHELL))

#define MNP_SHELL_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), MNP_TYPE_SHELL, MnpShellClass))

typedef struct _MnpShellPrivate MnpShellPrivate;

typedef struct {
  MxBoxLayout parent;

  MnpShellPrivate *priv;
} MnpShell;

typedef struct {
  MxBoxLayoutClass parent_class;
} MnpShellClass;

GType mnp_shell_get_type (void);

ClutterActor *mnp_shell_new (void);
void mnp_shell_set_panel_client (MnpShell *shell,
                                        MplPanelClient *panel_client);

G_END_DECLS

#endif /* _MNP_SHELL */
