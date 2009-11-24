/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */

/* mnb-panel.c */
/*
 * Copyright (c) 2009 Intel Corp.
 *
 * Author: Tomas Frydrych <tf@linux.intel.com>
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 */

#include "mnb-panel.h"
#include "mnb-toolbar.h"

#define MNB_PANEL_WARN_NOT_IMPLEMENTED(panel,vfunc)               \
  G_STMT_START {                                                  \
          g_warning ("Panel of type '%s' does not implement "     \
                     "the required MnbPanel::%s virtual "         \
                     "function.",                                 \
                     G_OBJECT_TYPE_NAME ((panel)),                \
                     (vfunc));                                    \
  } G_STMT_END

#define I_(str)  (g_intern_static_string ((str)))

enum
{
  SHOW_BEGIN,
  SHOW_COMPLETED,
  HIDE_BEGIN,
  HIDE_COMPLETED,

  REQUEST_BUTTON_STYLE,
  REQUEST_TOOLTIP,

  LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0, };

static void
mnb_panel_base_init (gpointer g_iface)
{
  static gboolean initialised = FALSE;

  if (!initialised)
    {
      GType iface_type = G_TYPE_FROM_INTERFACE (g_iface);

      initialised = TRUE;

      signals[SHOW_BEGIN] =
        g_signal_new ("show-begin",
                      iface_type,
                      G_SIGNAL_RUN_LAST,
                      G_STRUCT_OFFSET (MnbPanelIface, show_begin),
                      NULL, NULL,
                      g_cclosure_marshal_VOID__VOID,
                      G_TYPE_NONE, 0);

      signals[SHOW_COMPLETED] =
        g_signal_new ("show-completed",
                      iface_type,
                      G_SIGNAL_RUN_LAST,
                      G_STRUCT_OFFSET (MnbPanelIface, show_completed),
                      NULL, NULL,
                      g_cclosure_marshal_VOID__VOID,
                      G_TYPE_NONE, 0);

      signals[HIDE_BEGIN] =
        g_signal_new ("hide-begin",
                      iface_type,
                      G_SIGNAL_RUN_LAST,
                      G_STRUCT_OFFSET (MnbPanelIface, hide_begin),
                      NULL, NULL,
                      g_cclosure_marshal_VOID__VOID,
                      G_TYPE_NONE, 0);

      signals[HIDE_COMPLETED] =
        g_signal_new ("hide-completed",
                      iface_type,
                      G_SIGNAL_RUN_LAST,
                      G_STRUCT_OFFSET (MnbPanelIface, hide_completed),
                      NULL, NULL,
                      g_cclosure_marshal_VOID__VOID,
                      G_TYPE_NONE, 0);

      signals[REQUEST_BUTTON_STYLE] =
        g_signal_new ("request-button-style",
                      iface_type,
                      G_SIGNAL_RUN_LAST,
                      G_STRUCT_OFFSET (MnbPanelIface, request_button_style),
                      NULL, NULL,
                      g_cclosure_marshal_VOID__STRING,
                      G_TYPE_NONE, 1,
                      G_TYPE_STRING);

      signals[REQUEST_TOOLTIP] =
        g_signal_new ("request-tooltip",
                      iface_type,
                      G_SIGNAL_RUN_LAST,
                      G_STRUCT_OFFSET (MnbPanelIface, request_tooltip),
                      NULL, NULL,
                      g_cclosure_marshal_VOID__STRING,
                      G_TYPE_NONE, 1,
                      G_TYPE_STRING);
    }
}

GType
mnb_panel_get_type (void)
{
  static GType panel_type = 0;

  if (G_UNLIKELY (!panel_type))
    {
      const GTypeInfo panel_info =
      {
        sizeof (MnbPanelIface),
        mnb_panel_base_init,
        NULL, /* iface_base_finalize */
      };

      panel_type = g_type_register_static (G_TYPE_INTERFACE,
                                           I_("MnbPanel"),
                                           &panel_info, 0);

      g_type_interface_add_prerequisite (panel_type, G_TYPE_OBJECT);
    }

  return panel_type;
}

void
mnb_panel_show (MnbPanel *panel)
{
  MnbPanelIface *iface;

  g_return_if_fail (MNB_IS_PANEL (panel));

  iface = MNB_PANEL_GET_IFACE (panel);

  if (!iface->show)
    {
      MNB_PANEL_WARN_NOT_IMPLEMENTED (panel, "show");
      return;
    }

  iface->show (panel);
}

void
mnb_panel_hide (MnbPanel *panel)
{
  MnbPanelIface *iface;

  g_return_if_fail (MNB_IS_PANEL (panel));

  iface = MNB_PANEL_GET_IFACE (panel);

  if (!iface->hide)
    {
      MNB_PANEL_WARN_NOT_IMPLEMENTED (panel, "hide");
      return;
    }

  iface->hide (panel);
}

const gchar *
mnb_panel_get_name (MnbPanel *panel)
{
  MnbPanelIface *iface;

  g_return_val_if_fail (MNB_IS_PANEL (panel), NULL);

  iface = MNB_PANEL_GET_IFACE (panel);

  if (!iface->get_name)
    {
      MNB_PANEL_WARN_NOT_IMPLEMENTED (panel, "get_name");
      return NULL;
    }

  return iface->get_name (panel);
}

const gchar *
mnb_panel_get_tooltip (MnbPanel *panel)
{
  MnbPanelIface *iface;

  g_return_val_if_fail (MNB_IS_PANEL (panel), NULL);

  iface = MNB_PANEL_GET_IFACE (panel);

  if (!iface->get_tooltip)
    {
      MNB_PANEL_WARN_NOT_IMPLEMENTED (panel, "get_tooltip");
      return NULL;
    }

  return iface->get_tooltip (panel);
}

const gchar *
mnb_panel_get_button_style (MnbPanel *panel)
{
  MnbPanelIface *iface;

  g_return_val_if_fail (MNB_IS_PANEL (panel), NULL);

  iface = MNB_PANEL_GET_IFACE (panel);

  if (!iface->get_button_style)
    {
      MNB_PANEL_WARN_NOT_IMPLEMENTED (panel, "get_button_style");
      return NULL;
    }

  return iface->get_button_style (panel);
}

const gchar *
mnb_panel_get_stylesheet (MnbPanel *panel)
{
  MnbPanelIface *iface;

  g_return_val_if_fail (MNB_IS_PANEL (panel), NULL);

  iface = MNB_PANEL_GET_IFACE (panel);

  if (!iface->get_stylesheet)
    return NULL;

  return iface->get_stylesheet (panel);
}

void
mnb_panel_set_size (MnbPanel *panel, guint width, guint height)
{
  MnbPanelIface *iface;

  g_return_if_fail (MNB_IS_PANEL (panel));

  iface = MNB_PANEL_GET_IFACE (panel);

  if (!iface->set_size)
    {
      MNB_PANEL_WARN_NOT_IMPLEMENTED (panel, "set_size");
      return;
    }

  iface->set_size (panel, width, height);
}

void
mnb_panel_get_size (MnbPanel *panel, guint *width, guint *height)
{
  MnbPanelIface *iface;

  g_return_if_fail (MNB_IS_PANEL (panel));

  iface = MNB_PANEL_GET_IFACE (panel);

  if (!iface->get_size)
    {
      MNB_PANEL_WARN_NOT_IMPLEMENTED (panel, "get_size");
      return;
    }

  iface->get_size (panel, width, height);
}

void
mnb_panel_set_position (MnbPanel *panel, gint x, gint y)
{
  MnbPanelIface *iface;

  g_return_if_fail (MNB_IS_PANEL (panel));

  iface = MNB_PANEL_GET_IFACE (panel);

  if (!iface->set_position)
    {
      MNB_PANEL_WARN_NOT_IMPLEMENTED (panel, "set_position");
      return;
    }

  iface->set_position (panel, x, y);
}

void
mnb_panel_get_position (MnbPanel *panel, gint *x, gint *y)
{
  MnbPanelIface *iface;

  g_return_if_fail (MNB_IS_PANEL (panel));

  iface = MNB_PANEL_GET_IFACE (panel);

  if (!iface->get_position)
    {
      MNB_PANEL_WARN_NOT_IMPLEMENTED (panel, "get_position");
      return;
    }

  iface->get_position (panel, x, y);
}

gboolean
mnb_panel_is_mapped (MnbPanel *panel)
{
  MnbPanelIface *iface;

  g_return_val_if_fail (MNB_IS_PANEL (panel), FALSE);

  iface = MNB_PANEL_GET_IFACE (panel);

  if (!iface->is_mapped)
    {
      MNB_PANEL_WARN_NOT_IMPLEMENTED (panel, "is_mapped");
      return FALSE;
    }

  return iface->is_mapped (panel);
}

void
mnb_panel_set_button (MnbPanel *panel, NbtkButton *button)
{
  MnbPanelIface *iface;

  g_return_if_fail (MNB_IS_PANEL (panel));

  iface = MNB_PANEL_GET_IFACE (panel);

  if (!iface->set_button)
    {
      MNB_PANEL_WARN_NOT_IMPLEMENTED (panel, "set_button");
      return;
    }

  iface->set_button (panel, button);
}

static void
mnb_panel_hide_with_toolbar_hide_completed_cb (MnbPanel    *panel,
                                               MnbToolbar  *toolbar)
{
  g_signal_handlers_disconnect_by_func (panel,
                                 mnb_panel_hide_with_toolbar_hide_completed_cb,
                                 toolbar);

  mnb_toolbar_hide (toolbar);
}

void
mnb_panel_hide_with_toolbar (MnbPanel *panel)
{
  MutterPlugin *plugin  = moblin_netbook_get_plugin_singleton ();
  ClutterActor *toolbar = moblin_netbook_get_toolbar (plugin);

  if (!mnb_panel_is_mapped (panel))
    {
      if (CLUTTER_ACTOR_IS_MAPPED (toolbar))
        mnb_toolbar_hide (MNB_TOOLBAR (toolbar));
    }
  else
    {
      g_signal_connect (panel, "hide-completed",
                     G_CALLBACK (mnb_panel_hide_with_toolbar_hide_completed_cb),
                     toolbar);

      mnb_panel_hide (panel);
    }
}

void
mnb_panel_ensure_size (MnbPanel *panel)
{
  MetaRectangle  r;
  MetaScreen    *screen;
  MetaWorkspace *workspace;
  MutterPlugin  *plugin = moblin_netbook_get_plugin_singleton ();

  screen    = mutter_plugin_get_screen (plugin);
  workspace = meta_screen_get_active_workspace (screen);

  if (workspace)
    {
      gint  x, y;
      guint w, h;
      guint max_height, max_width;

      meta_workspace_get_work_area_all_monitors (workspace, &r);

      mnb_panel_get_position (panel, &x, &y);
      mnb_panel_get_size (panel, &w, &h);

      /*
       * Maximum height of the panel is the available working height plus
       * the height of the panel shadow (we allow the shadow to stretch out
       * of the available area).
       */
      /* FIXME -- devise a way of doing the shadow */
      max_height = r.height - TOOLBAR_HEIGHT - 4 - 30;
      max_width  = r.width - TOOLBAR_X_PADDING * 2;

      if (max_height != h || r.width != w)
        {
          mnb_panel_set_size (panel, max_width, max_height);
        }
    }
}
