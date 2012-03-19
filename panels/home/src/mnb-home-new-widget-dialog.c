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

#include <glib/gi18n.h>

#include "mnb-home-new-widget-dialog.h"
#include "mnb-home-widget-preview.h"
#include "mnb-home-plugins-engine.h"
#include "utils.h"

static void mnb_home_new_widget_dialog_item_factory_iface_init (
    MxItemFactoryIface *iface);

typedef struct {
    GObject parent;

    MnbHomeNewWidgetDialog *dialog;
} MnbHomeNewWidgetDialogItemFactory;

typedef struct {
    GObjectClass parent_class;
} MnbHomeNewWidgetDialogItemFactoryClass;

G_DEFINE_TYPE (MnbHomeNewWidgetDialog, mnb_home_new_widget_dialog,
    MX_TYPE_DIALOG)
G_DEFINE_TYPE_WITH_CODE (MnbHomeNewWidgetDialogItemFactory,
    mnb_home_new_widget_dialog_item_factory,
    G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE (MX_TYPE_ITEM_FACTORY,
      mnb_home_new_widget_dialog_item_factory_iface_init)
    )

#define MNB_TYPE_HOME_NEW_WIDGET_DIALOG_ITEM_FACTORY \
  (mnb_home_new_widget_dialog_item_factory_get_type ())

enum /* item factory signals */
{
  RESPONSE,
  LAST_SIGNAL
};

static guint _signals[LAST_SIGNAL] = { 0, };

enum /* model columns */
{
  ITEMS_MODULE,
  ITEMS_NAME,
  ITEMS_ICON,
  ITEMS_N_COLUMNS
};

struct _MnbHomeNewWidgetDialogPrivate
{
  ClutterModel *items;
};

static void
home_new_widget_dialog_cancel (MxAction *action,
    MnbHomeNewWidgetDialog *self)
{
  g_signal_emit (self, _signals[RESPONSE], 0, NULL);
}

static void
mnb_home_new_widget_dialog_dispose (GObject *self)
{
  MnbHomeNewWidgetDialogPrivate *priv = MNB_HOME_NEW_WIDGET_DIALOG (self)->priv;

  g_clear_object (&priv->items);

  G_OBJECT_CLASS (mnb_home_new_widget_dialog_parent_class)->dispose (self);
}

static void
mnb_home_new_widget_dialog_class_init (MnbHomeNewWidgetDialogClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->dispose = mnb_home_new_widget_dialog_dispose;

  g_type_class_add_private (gobject_class,
      sizeof (MnbHomeNewWidgetDialogPrivate));

  _signals[RESPONSE] = g_signal_new ("response",
      G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST,
      0, NULL, NULL,
      g_cclosure_marshal_generic,
      G_TYPE_NONE,
      1, G_TYPE_STRING);
}

static void
mnb_home_new_widget_dialog_init (MnbHomeNewWidgetDialog *self)
{
  ClutterActor *table, *itemview;
  MnbHomeNewWidgetDialogItemFactory *factory;
  MnbHomePluginsEngine *engine;
  const GList *l;

  self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self,
      MNB_TYPE_HOME_NEW_WIDGET_DIALOG, MnbHomeNewWidgetDialogPrivate);

  /* set up model and view */
  self->priv->items = clutter_list_model_new (ITEMS_N_COLUMNS,
      G_TYPE_STRING, "Module", /* ITEMS_MODULE */
      G_TYPE_STRING, "Name", /* ITEMS_NAME */
      G_TYPE_STRING, "Icon" /* ITEMS_ICON */);

  table = mx_table_new ();
  mx_bin_set_child (MX_BIN (self), table);

  factory = g_object_new (MNB_TYPE_HOME_NEW_WIDGET_DIALOG_ITEM_FACTORY, NULL);
  factory->dialog = self;

  itemview = mx_item_view_new ();
  mx_table_add_actor (MX_TABLE (table), itemview, 0, 1);
  mx_item_view_set_model (MX_ITEM_VIEW (itemview), self->priv->items);
  mx_item_view_set_factory (MX_ITEM_VIEW (itemview), MX_ITEM_FACTORY (factory));

  mx_item_view_add_attribute (MX_ITEM_VIEW (itemview), "module", ITEMS_MODULE);
  mx_item_view_add_attribute (MX_ITEM_VIEW (itemview), "label", ITEMS_NAME);
  mx_item_view_add_attribute (MX_ITEM_VIEW (itemview), "icon", ITEMS_ICON);

  clutter_actor_show_all (table);

  /* find plugins */
  /* FIXME: should we monitor for new plugins on the fly? */
  engine = mnb_home_plugins_engine_dup ();

  for (l = peas_engine_get_plugin_list (PEAS_ENGINE (engine));
       l != NULL;
       l = g_list_next (l))
    {
      PeasPluginInfo *info = l->data;
      char *icon;

      if (!peas_plugin_info_is_available (info, NULL))
        continue;

      icon = g_build_filename (
          peas_plugin_info_get_module_dir (info),
          peas_plugin_info_get_icon_name (info),
          NULL);

      clutter_model_append (self->priv->items,
          ITEMS_MODULE, peas_plugin_info_get_module_name (info),
          ITEMS_NAME, peas_plugin_info_get_name (info),
          ITEMS_ICON, icon,
          -1);

      g_free (icon);
    }

  /* add actions */
  mx_dialog_add_action (MX_DIALOG (self),
      mx_action_new_full ("cancel", _("Cancel"),
        G_CALLBACK (home_new_widget_dialog_cancel),
        self));

  g_object_unref (engine);
  g_object_unref (factory);
}

ClutterActor *
mnb_home_new_widget_dialog_new (ClutterActor *parent)
{
  ClutterActor *dialog = g_object_new (MNB_TYPE_HOME_NEW_WIDGET_DIALOG,
      NULL);

  if (parent != NULL)
    mx_dialog_set_transient_parent (MX_DIALOG (dialog), parent);

  return dialog;
}

static gboolean
home_new_widget_dialog_item_factory_clicked (ClutterActor *actor,
    ClutterButtonEvent *event,
    MnbHomeNewWidgetDialogItemFactory *self)
{
  g_signal_emit (self->dialog, _signals[RESPONSE], 0,
      mnb_home_widget_preview_get_module (MNB_HOME_WIDGET_PREVIEW (actor)));

  return TRUE;
}

static ClutterActor *
mnb_home_new_widget_dialog_item_factory_create (MxItemFactory *self)
{
  ClutterActor *actor = g_object_new (MNB_TYPE_HOME_WIDGET_PREVIEW,
      NULL);

  g_signal_connect (actor, "button-release-event",
      G_CALLBACK (home_new_widget_dialog_item_factory_clicked), self);

  return actor;
}

static void
mnb_home_new_widget_dialog_item_factory_class_init (
    MnbHomeNewWidgetDialogItemFactoryClass *klass)
{
}

static void
mnb_home_new_widget_dialog_item_factory_init (
    MnbHomeNewWidgetDialogItemFactory *self)
{
}

static void
mnb_home_new_widget_dialog_item_factory_iface_init (
    MxItemFactoryIface *iface)
{
  iface->create = mnb_home_new_widget_dialog_item_factory_create;
}
