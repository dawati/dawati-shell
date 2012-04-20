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
#include "mnb-home-plugins-engine.h"
#include "mnb-home-widget.h"
#include "utils.h"

#include "dawati-home-plugins-app.h"

#define GSETTINGS_PLUGIN_SCHEMA       "org.dawati.shell.home.plugin"
#define GSETTINGS_PLUGIN_PATH_PREFIX  "/org/dawati/shell/home/plugin/"

G_DEFINE_TYPE (MnbHomeWidget, mnb_home_widget, MX_TYPE_FRAME);

enum /* properties */
{
  PROP_0,
  PROP_ROW,
  PROP_COLUMN,
  PROP_SETTINGS_PATH,
  PROP_MODULE,
  PROP_EDIT_MODE,
  PROP_LAST
};

struct _MnbHomeWidgetPrivate
{
  GSettings *settings;
  MnbHomePluginsEngine *engine;
  DawatiHomePluginsApp *app;

  gint row;
  gint column;
  gchar *settings_path;
  gboolean edit_mode;
  gchar *module;
};

static void home_widget_set_module (MnbHomeWidget *self, const gchar* module);


static void
mnb_home_widget_get_property (GObject *self,
    guint prop_id,
    GValue *value,
    GParamSpec *pspec)
{
  MnbHomeWidgetPrivate *priv = MNB_HOME_WIDGET (self)->priv;

  switch (prop_id)
    {
      case PROP_ROW:
        g_value_set_int (value, priv->row);
        break;

      case PROP_COLUMN:
        g_value_set_int (value, priv->column);
        break;

      case PROP_SETTINGS_PATH:
        g_value_set_string (value, priv->settings_path);
        break;

      case PROP_MODULE:
        g_value_set_string (value, priv->module);
        break;

      case PROP_EDIT_MODE:
        g_value_set_boolean (value, priv->edit_mode);
        break;

      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (self, prop_id, pspec);
        break;
    }
}

static void
mnb_home_widget_set_property (GObject *self,
    guint prop_id,
    const GValue *value,
    GParamSpec *pspec)
{
  MnbHomeWidgetPrivate *priv = MNB_HOME_WIDGET (self)->priv;

  switch (prop_id)
    {
      case PROP_ROW:
        priv->row = g_value_get_int (value);
        break;

      case PROP_COLUMN:
        priv->column = g_value_get_int (value);
        break;

      case PROP_SETTINGS_PATH:
        priv->settings_path = g_strdup (g_value_get_string (value));
        break;

      case PROP_MODULE:
        home_widget_set_module (MNB_HOME_WIDGET (self),
            g_value_get_string (value));
        break;

      case PROP_EDIT_MODE:
        mnb_home_widget_set_edit_mode (MNB_HOME_WIDGET (self),
            g_value_get_boolean (value));
        break;

      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (self, prop_id, pspec);
        break;
    }
}

static void
home_widget_set_module (MnbHomeWidget *self,
    const gchar *module)
{
  if (0 == g_strcmp0 (module, self->priv->module))
    return;

  DEBUG ("%s -> %s (%s)", self->priv->module, module,
      self->priv->settings_path);

  g_free (self->priv->module);
  self->priv->module = g_strdup (module);

  if (self->priv->app != NULL)
    dawati_home_plugins_app_deinit (self->priv->app);

  g_clear_object (&self->priv->app);

  if (STR_EMPTY (self->priv->module))
    {
      DEBUG ("no module, going into edit mode (%s)",
          self->priv->settings_path);
      mnb_home_widget_set_edit_mode (self, TRUE);
    }
  else
    {
      DEBUG ("module = '%s' (%s)", self->priv->module,
          self->priv->settings_path);

      if (self->priv->engine == NULL)
        self->priv->engine = mnb_home_plugins_engine_dup ();

      self->priv->app =
        mnb_home_plugins_engine_create_app (self->priv->engine,
            self->priv->module, self->priv->settings_path);
      if (self->priv->app != NULL)
        dawati_home_plugins_app_init (self->priv->app);
    }

  /* reload the widget */
  mnb_home_widget_set_edit_mode (self, self->priv->edit_mode);

  g_object_notify (G_OBJECT (self), "module");
}

static void
mnb_home_widget_constructed (GObject *self)
{
  MnbHomeWidgetPrivate *priv = MNB_HOME_WIDGET (self)->priv;

  priv->settings = g_settings_new_with_path (GSETTINGS_PLUGIN_SCHEMA,
      priv->settings_path);

  g_settings_bind (priv->settings, "module", self, "module",
      G_SETTINGS_BIND_DEFAULT);

  /* update from the property to GSettings (in order to store it) */
  priv->column = g_settings_get_int (priv->settings, "column");
  g_settings_bind (priv->settings, "column", self, "column", G_SETTINGS_BIND_SET);

  /* update from the property to GSettings (in order to store it) */
  priv->row = g_settings_get_int (priv->settings, "row");
  g_settings_bind (priv->settings, "row", self, "row", G_SETTINGS_BIND_SET);

  /* force the widget to go into not-edit-mode */
  priv->edit_mode = TRUE;
  mnb_home_widget_set_edit_mode (MNB_HOME_WIDGET (self), FALSE);
}

static void
mnb_home_widget_dispose (GObject *self)
{
  MnbHomeWidgetPrivate *priv = MNB_HOME_WIDGET (self)->priv;

  g_clear_object (&priv->settings);
  g_clear_object (&priv->engine);
  g_clear_object (&priv->app);

  G_OBJECT_CLASS (mnb_home_widget_parent_class)->dispose (self);
}

static void
mnb_home_widget_finalize (GObject *self)
{
  MnbHomeWidgetPrivate *priv = MNB_HOME_WIDGET (self)->priv;

  g_free (priv->module);
  g_free (priv->settings_path);

  G_OBJECT_CLASS (mnb_home_widget_parent_class)->finalize (self);
}

static void
mnb_home_widget_class_init (MnbHomeWidgetClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->get_property = mnb_home_widget_get_property;
  gobject_class->set_property = mnb_home_widget_set_property;
  gobject_class->constructed = mnb_home_widget_constructed;
  gobject_class->dispose = mnb_home_widget_dispose;
  gobject_class->finalize = mnb_home_widget_finalize;

  g_type_class_add_private (gobject_class, sizeof (MnbHomeWidgetPrivate));

  g_object_class_install_property (gobject_class, PROP_ROW,
      g_param_spec_int ("row",
        "Row",
        "",
        -1, G_MAXINT, -1,
        G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_COLUMN,
      g_param_spec_int ("column",
        "Column",
        "",
        -1, G_MAXINT, -1,
        G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_SETTINGS_PATH,
      g_param_spec_string ("settings-path",
        "GSetting path",
        "The GSettings path for the widget",
        NULL,
        G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_MODULE,
      g_param_spec_string ("module",
        "plugin module",
        "The name of the plugin module for the widget",
        NULL,
        G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_EDIT_MODE,
      g_param_spec_boolean ("edit-mode",
        "Edit Mode",
        "%TRUE if we are editing the settings for this widget",
        FALSE,
        G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
}

static void
mnb_home_widget_init (MnbHomeWidget *self)
{
  self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, MNB_TYPE_HOME_WIDGET,
      MnbHomeWidgetPrivate);

  g_object_set (self,
      "x-fill", TRUE,
      "y-fill", TRUE,
      NULL);
}

ClutterActor *
mnb_home_widget_new (const gchar* path)
{
  return g_object_new (MNB_TYPE_HOME_WIDGET,
      "settings-path", path,
      NULL);
}

static void
home_widget_add_module_response (ClutterActor *dialog,
    const gchar *module,
    MnbHomeWidget *self)
{
  if (!STR_EMPTY (module))
    {
      DEBUG ("Setting module '%s', chosen from dialog", module);
      home_widget_set_module (self, module);
    }

  g_object_unref (dialog);
  /* FIXME: why do I need this */
  clutter_actor_queue_redraw (clutter_actor_get_stage (CLUTTER_ACTOR (self)));
}

static void
home_widget_add_module (MxButton *button,
    MnbHomeWidget *self)
{
  ClutterActor *dialog;

  dialog = mnb_home_new_widget_dialog_new (
      clutter_actor_get_stage (CLUTTER_ACTOR (button)));

  g_signal_connect (dialog, "response",
      G_CALLBACK (home_widget_add_module_response), self);

  clutter_actor_show (dialog);
}

static void
home_widget_remove_module (MxButton *button,
    MnbHomeWidget *self)
{
  dconf_recursive_unset (self->priv->settings_path, NULL);
}

void
mnb_home_widget_set_edit_mode (MnbHomeWidget *self,
    gboolean edit_mode)
{
  //if (edit_mode == self->priv->edit_mode)
  //  return;

  DEBUG ("%d -> %d for widget %s", self->priv->edit_mode,
      edit_mode, self->priv->settings_path);
  self->priv->edit_mode = edit_mode;
  //g_object_notify (G_OBJECT (self), "edit-mode");

  /* FIXME: should hold refs to the actors rather than destroy/recreate them? */
  mx_bin_set_child (MX_BIN (self), NULL);

  /* FIXME: animate this */
  if (edit_mode)
    {
      ClutterActor *table;

      table = mx_table_new ();
      mx_bin_set_child (MX_BIN (self), table);

      if (!STR_EMPTY (self->priv->module))
        {
          ClutterActor *config, *remove;

          remove = mx_button_new_with_label ("x");
          mx_table_insert_actor_with_properties (MX_TABLE (table), remove, 0, 1,
                                                 "x-expand", FALSE,
                                                 "y-expand", FALSE,
                                                 "x-fill", FALSE,
                                                 "y-fill", FALSE,
                                                 NULL);

          g_signal_connect (remove, "clicked",
              G_CALLBACK (home_widget_remove_module), self);

          if (self->priv->app != NULL)
            config = dawati_home_plugins_app_get_configuration (
                self->priv->app);
          else
            config = mx_label_new_with_text (_("Plugin missing"));

          if (CLUTTER_IS_ACTOR (config))
            mx_table_insert_actor_with_properties (MX_TABLE (table), config, 1, 0,
                                                   "x-expand", TRUE,
                                                   "y-expand", TRUE,
                                                   "x-fill", TRUE,
                                                   "y-fill", TRUE,
                                                   NULL);
        }
      else /* STR_EMPTY (self->priv->module) */
        {
          ClutterActor *button;

          button = mx_button_new_with_label ("+");
          mx_table_insert_actor (MX_TABLE (table), button, 0, 0);

          g_signal_connect (button, "clicked",
              G_CALLBACK (home_widget_add_module), self);
        }

      clutter_actor_show_all (table);
    }
  else /* !edit_mode */
    {
      ClutterActor *widget = NULL;

      if (self->priv->app != NULL)
        {
          widget = dawati_home_plugins_app_get_widget (self->priv->app);

          if (!CLUTTER_IS_ACTOR (widget))
            /* FIXME: make this better */
            {
            widget = mx_label_new_with_text (_("Broken plugin"));
            }
        }
      else if (!STR_EMPTY (self->priv->module))
        {
          widget = mx_label_new_with_text (_("Plugin missing"));
        }

      if (widget != NULL)
        mx_bin_set_child (MX_BIN (self), widget);
    }
}
