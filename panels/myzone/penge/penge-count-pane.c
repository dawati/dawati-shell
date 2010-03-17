/*
 * moblin-app-installer -- Moblin garage client application.
 * Copyright (c) 2009, Intel Corporation.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Author: Nicolas Dufresne <nicolas.dufresne@collabora.co.uk>
 */

#include "penge-count-pane.h"

G_DEFINE_TYPE (PengeCountPane, penge_count_pane, MX_TYPE_BUTTON)

#define GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj),\
                          PENGE_TYPE_COUNT_PANE,\
                          PengeCountPanePrivate))

enum
{
  PROP_0,
  PROP_MESSAGE,
  PROP_ACCOUNT,
  PROP_COUNT,
  PROP_COMPACT
};

struct _PengeCountPanePrivate
{
  ClutterActor *table;
  ClutterActor *count_label;
  ClutterActor *message_label;
  ClutterActor *account_label;
  guint count;
  gboolean compact;
};

static ClutterColor grey_color = { 0xa7, 0xa7, 0xa7, 0xff };

static GObject *
penge_count_pane_constructor (GType                  type,
                              guint                  n_construct_params,
                              GObjectConstructParam *construct_params)
{
  GObject *object;

  object = G_OBJECT_CLASS (penge_count_pane_parent_class)->constructor (type,
                                                       n_construct_params,
                                                       construct_params);

  g_object_set (object, "width", 280.0, NULL);

  return object;
}

static void
penge_count_pane_set_message (PengeCountPane *self,
                              const gchar    *message)
{
  PengeCountPanePrivate *priv = self->priv;
  mx_label_set_text (MX_LABEL (priv->message_label), message);
}

static void
penge_count_pane_set_account (PengeCountPane *self,
                              const gchar    *account)
{
  PengeCountPanePrivate *priv = self->priv;
  mx_label_set_text (MX_LABEL (priv->account_label), account);
}

static void
penge_count_pane_set_count (PengeCountPane *self,
                            guint           count)
{
  gchar *count_str;
  PengeCountPanePrivate *priv = self->priv;

  priv->count = count;

  /* min-width does not seem to work, so we workaround adding padding spaced
   * around the number for 1 digit version */
  if (count < 10)
    count_str = g_strdup_printf (" %u ", count);
  else
    count_str = g_strdup_printf ("%u", count);

  mx_label_set_text (MX_LABEL (priv->count_label), count_str);
  g_free (count_str);

  if (count == 0)
    mx_stylable_set_style_class (MX_STYLABLE (priv->count_label),
                                 "PengeZeroCountLabel");
  else
    mx_stylable_set_style_class (MX_STYLABLE (priv->count_label),
                                 "PengeCountLabel");
}

static void
penge_count_pane_set_compact (PengeCountPane *self,
                              gboolean        compact)
{
  PengeCountPanePrivate *priv = self->priv;

  if (priv->compact == compact)
    return;

  priv->compact = compact;
  if (compact)
  {
    gchar *tooltip_str;
    clutter_actor_set_width (CLUTTER_ACTOR (self), -1);
    
    g_object_ref (priv->count_label);
    clutter_container_remove_actor (CLUTTER_CONTAINER (priv->table),
                                    priv->count_label);
    clutter_actor_unparent (priv->count_label);
    mx_bin_set_child (MX_BIN (self), priv->count_label);

    tooltip_str = g_strdup_printf ("%s\n%s",
                                   mx_label_get_text (MX_LABEL (priv->message_label)),
                                   mx_label_get_text (MX_LABEL (priv->account_label)));
    mx_widget_set_tooltip_text (MX_WIDGET (self), tooltip_str);
    g_free (tooltip_str);
                                
  }
  else
  {
    clutter_actor_set_width (CLUTTER_ACTOR (self), 280);
    clutter_actor_reparent (priv->count_label, priv->table);
    clutter_container_child_set (CLUTTER_CONTAINER (priv->table),
                                 priv->count_label,
                                 "col", 0,
                                 "row", 0,
                                 "row-span", 2,
                                 "x-expand", FALSE,
                                 NULL);
    mx_bin_set_child (MX_BIN (self), priv->table);

    mx_widget_set_tooltip_text (MX_WIDGET (self), NULL);
  }
}

static void
penge_count_pane_get_property (GObject    *gobject,
                               guint       prop_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
  PengeCountPanePrivate *priv = PENGE_COUNT_PANE (gobject)->priv;

  switch (prop_id)
  {
    case PROP_MESSAGE:
      g_value_set_string (value,
                          mx_label_get_text (MX_LABEL (priv->message_label)));
      break;

    case PROP_ACCOUNT:
      g_value_set_string (value,
                          mx_label_get_text (MX_LABEL (priv->account_label)));
      break;

    case PROP_COUNT:
      g_value_set_uint (value, priv->count);
      break;

    case PROP_COMPACT:
      g_value_set_boolean (value, priv->compact);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
      break;
  }
}

static void
penge_count_pane_set_property (GObject      *gobject,
                               guint         prop_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
  switch (prop_id)
    {
    case PROP_MESSAGE:
      penge_count_pane_set_message (PENGE_COUNT_PANE (gobject),
                                    g_value_get_string (value));
      break;

    case PROP_ACCOUNT:
      penge_count_pane_set_account (PENGE_COUNT_PANE (gobject),
                                    g_value_get_string (value));
      break;

    case PROP_COUNT:
      penge_count_pane_set_count (PENGE_COUNT_PANE (gobject),
                                  g_value_get_uint (value));
      break;

    case PROP_COMPACT:
      penge_count_pane_set_compact (PENGE_COUNT_PANE (gobject),
                                    g_value_get_boolean (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
      break;
    }
}

static void
penge_count_pane_dispose (GObject *object)
{
  PengeCountPanePrivate *priv = PENGE_COUNT_PANE (object)->priv;

  if (priv->table)
  {
    g_object_unref (priv->table);
    priv->table = NULL;
  }
  
  /* The following are borrowed ref from master class */
  priv->count_label = NULL;
  priv->message_label = NULL;
  priv->account_label = NULL;

  G_OBJECT_CLASS (penge_count_pane_parent_class)->dispose (object);
}

static void
penge_count_pane_class_init (PengeCountPaneClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->constructor = penge_count_pane_constructor;
  gobject_class->dispose = penge_count_pane_dispose;
  gobject_class->get_property = penge_count_pane_get_property;
  gobject_class->set_property = penge_count_pane_set_property;

  g_type_class_add_private (gobject_class, sizeof (PengeCountPanePrivate));

  g_object_class_install_property
          (gobject_class,
           PROP_MESSAGE,
           g_param_spec_string ("message",
                                "Message",
                                "The message the follow the count (e.g 4 "
                                  "unread messages)",
                                "Message(s)",
                                G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE));

  g_object_class_install_property
          (gobject_class,
           PROP_ACCOUNT,
           g_param_spec_string ("account",
                                "Account",
                                "The name of the account",
                                NULL,
                                G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE));

  g_object_class_install_property
          (gobject_class,
           PROP_COUNT,
           g_param_spec_uint ("count",
                              "Count",
                              "The count",
                              0, G_MAXUINT, 0,
                              G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE));

  g_object_class_install_property
          (gobject_class,
           PROP_COMPACT,
           g_param_spec_boolean ("compact",
                                 "Compact",
                                 "When true, make the UI more compact",
                                 FALSE,
                                 G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE));

}

static void
penge_count_pane_init (PengeCountPane *self)
{
  ClutterActor *tmp_text;
  PengeCountPanePrivate *priv = GET_PRIVATE (self);

  self->priv = priv;
  priv->table = mx_table_new ();
  g_object_ref (priv->table);

  priv->count_label = mx_label_new ("");

  tmp_text = mx_label_get_clutter_text (MX_LABEL (priv->count_label));
  clutter_text_set_ellipsize (CLUTTER_TEXT (tmp_text), PANGO_ELLIPSIZE_NONE);
  clutter_text_set_single_line_mode (CLUTTER_TEXT (tmp_text), TRUE);
  clutter_text_set_color (CLUTTER_TEXT (tmp_text), &grey_color);

  g_object_set (G_OBJECT (priv->count_label),
                "x-align", MX_ALIGN_MIDDLE,
                "y-align", MX_ALIGN_MIDDLE,
                NULL);

  penge_count_pane_set_count (self, 0);

  clutter_actor_show (priv->count_label);
  mx_table_add_actor_with_properties (MX_TABLE (priv->table),
                                      priv->count_label,
                                      0, 0,
                                      "row-span", 2,
                                      "x-expand", FALSE,
                                      NULL);

  priv->message_label = mx_label_new ("Message(s)");

  tmp_text = mx_label_get_clutter_text (MX_LABEL (priv->message_label));
  clutter_text_set_ellipsize (CLUTTER_TEXT (tmp_text), PANGO_ELLIPSIZE_END);
  clutter_text_set_single_line_mode (CLUTTER_TEXT (tmp_text), TRUE);

  mx_stylable_set_style_class (MX_STYLABLE (priv->message_label),
                              "PengeCountMessageLabel");

  clutter_actor_show (priv->message_label);
  mx_table_add_actor_with_properties (MX_TABLE (priv->table),
                                      priv->message_label,
                                      0, 1,
                                      "x-expand", TRUE,
                                      NULL);

  priv->account_label = mx_label_new ("The Account");

  tmp_text = mx_label_get_clutter_text (MX_LABEL (priv->account_label));
  clutter_text_set_ellipsize (CLUTTER_TEXT (tmp_text), PANGO_ELLIPSIZE_END);
  clutter_text_set_single_line_mode (CLUTTER_TEXT (tmp_text), TRUE);
  clutter_text_set_color (CLUTTER_TEXT (tmp_text), &grey_color);

  mx_stylable_set_style_class (MX_STYLABLE (priv->account_label),
                              "PengeCountAccountLabel");

  clutter_actor_show (priv->account_label);
  mx_table_add_actor_with_properties (MX_TABLE (priv->table),
                                      priv->account_label,
                                      1, 1,
                                      "x-expand", TRUE,
                                      NULL);
  mx_table_set_col_spacing (MX_TABLE (priv->table), 12);
  clutter_actor_show (priv->table);

  mx_bin_set_child (MX_BIN (self), priv->table);
  mx_bin_set_fill (MX_BIN (self), TRUE, FALSE);
}


MxWidget *penge_count_pane_new (void)
{
  return g_object_new (PENGE_TYPE_COUNT_PANE, NULL);
}

