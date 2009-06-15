/*
 * Anerley - people feeds and widgets
 * Copyright (C) 2009, Intel Corporation.
 *
 * Authors: Rob Bradford <rob@linux.intel.com>
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
 *
 */

#include "anerley-econtact-item.h"

G_DEFINE_TYPE (AnerleyEContactItem, anerley_econtact_item, ANERLEY_TYPE_ITEM)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), ANERLEY_TYPE_ECONTACT_ITEM, AnerleyEContactItemPrivate))

typedef struct _AnerleyEContactItemPrivate AnerleyEContactItemPrivate;

struct _AnerleyEContactItemPrivate {
  EContact *contact;
  gchar *sortable_name;
  gchar *display_name;
};

enum
{
  PROP_0,
  PROP_CONTACT
};

static void
anerley_econtact_item_get_property (GObject *object, guint property_id,
                              GValue *value, GParamSpec *pspec)
{
  AnerleyEContactItemPrivate *priv = GET_PRIVATE (object);

  switch (property_id) {
    case PROP_CONTACT:
      g_value_set_object (value, priv->contact);
      break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
anerley_econtact_item_set_contact (AnerleyEContactItem *item,
                                   EContact            *contact)
{
  AnerleyEContactItemPrivate *priv = GET_PRIVATE (item);
  gboolean contact_changed = FALSE;

  if (priv->contact)
  {
    g_object_unref (priv->contact);
    priv->contact = NULL;
    contact_changed = TRUE;
  }

  if (priv->sortable_name)
  {
    g_free (priv->sortable_name);
    priv->sortable_name = NULL;
  }

  if (priv->display_name)
  {
    g_free (priv->display_name);
    priv->display_name = NULL;
  }

  priv->contact = g_object_ref (contact);

  if (contact_changed)
  {
    anerley_item_emit_display_name_changed ((AnerleyItem *)item);
    anerley_item_emit_avatar_path_changed ((AnerleyItem *)item);
  }
}


static void
anerley_econtact_item_set_property (GObject *object, guint property_id,
                              const GValue *value, GParamSpec *pspec)
{
  switch (property_id) {
    case PROP_CONTACT:
      anerley_econtact_item_set_contact ((AnerleyEContactItem *)object,
                                         g_value_get_object (value));
      break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
anerley_econtact_item_dispose (GObject *object)
{
  AnerleyEContactItemPrivate *priv = GET_PRIVATE (object);

  if (priv->contact)
  {
    g_object_unref (priv->contact);
    priv->contact = NULL;
  }

  G_OBJECT_CLASS (anerley_econtact_item_parent_class)->dispose (object);
}

static void
anerley_econtact_item_finalize (GObject *object)
{
  AnerleyEContactItemPrivate *priv = GET_PRIVATE (object);

  g_free (priv->sortable_name);

  G_OBJECT_CLASS (anerley_econtact_item_parent_class)->finalize (object);
}

const gchar *
anerley_econtact_item_get_display_name (AnerleyItem *item)
{
  AnerleyEContactItemPrivate *priv = GET_PRIVATE (item);
  gchar *full_name;

  if (!priv->display_name)
  {
    full_name = g_strconcat (e_contact_get_const (priv->contact,
                                                  E_CONTACT_GIVEN_NAME),
                             " ",
                             e_contact_get_const (priv->contact,
                                                  E_CONTACT_FAMILY_NAME),
                             NULL);
    if (full_name)
      priv->display_name = full_name;
    else
      priv->display_name = g_strdup ("");
  }

  return priv->display_name;
}

const gchar *
anerley_econtact_item_get_sortable_name (AnerleyItem *item)
{
  AnerleyEContactItemPrivate *priv = GET_PRIVATE (item);

  if (!priv->sortable_name)
  {
    priv->sortable_name = g_utf8_casefold (
                            anerley_econtact_item_get_display_name (item),
                            -1);
  }

  return priv->sortable_name;
}

const gchar *
anerley_econtact_item_get_avatar_path (AnerleyItem *item)
{
  return NULL;
}

const gchar *
anerley_econtact_item_get_presence_status (AnerleyItem *item)
{
  return NULL;
}

const gchar *
anerley_econtact_item_get_presence_message (AnerleyItem *item)
{
  return NULL;
}

const gchar *
anerley_econtact_item_get_first_name (AnerleyItem *item)
{
  AnerleyEContactItemPrivate *priv = GET_PRIVATE (item);
  const gchar *first_name;

  first_name = e_contact_get_const (priv->contact,
                                    E_CONTACT_GIVEN_NAME);
  return first_name;
}

const gchar *
anerley_econtact_item_get_last_name (AnerleyItem *item)
{
  AnerleyEContactItemPrivate *priv = GET_PRIVATE (item);

  return e_contact_get_const (priv->contact,
                              E_CONTACT_FAMILY_NAME);
}

void
anerley_econtact_item_activate (AnerleyItem *item)
{
  AnerleyEContactItemPrivate *priv = GET_PRIVATE (item);
  const gchar *email;
  gchar *url;
  GError *error = NULL;

  email = e_contact_get_const (priv->contact,
                               E_CONTACT_EMAIL_1);

  if (email)
  {
    url = g_strdup_printf ("mailto:%s",
                           email);
    if (!g_app_info_launch_default_for_uri (url,
                                            NULL,
                                            &error))
    {
      g_warning (G_STRLOC ": Error activating item: %s",
                 error->message);
      g_clear_error (&error);
    }

    g_free (url);
  }
}

static void
anerley_econtact_item_class_init (AnerleyEContactItemClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  AnerleyItemClass *item_class = ANERLEY_ITEM_CLASS (klass);
  GParamSpec *pspec;

  g_type_class_add_private (klass, sizeof (AnerleyEContactItemPrivate));

  object_class->get_property = anerley_econtact_item_get_property;
  object_class->set_property = anerley_econtact_item_set_property;
  object_class->dispose = anerley_econtact_item_dispose;
  object_class->finalize = anerley_econtact_item_finalize;

  item_class->get_display_name = anerley_econtact_item_get_display_name;
  item_class->get_sortable_name = anerley_econtact_item_get_sortable_name;
  item_class->get_avatar_path = anerley_econtact_item_get_avatar_path;
  item_class->get_presence_status = anerley_econtact_item_get_presence_status;
  item_class->get_presence_message = anerley_econtact_item_get_presence_message;
  item_class->get_first_name = anerley_econtact_item_get_first_name;
  item_class->get_last_name = anerley_econtact_item_get_last_name;
  item_class->activate = anerley_econtact_item_activate;

  pspec = g_param_spec_object ("contact",
                               "The contact",
                               "Underlying contact whose details we represent",
                               E_TYPE_CONTACT,
                               G_PARAM_READWRITE | G_PARAM_CONSTRUCT);
  g_object_class_install_property (object_class, PROP_CONTACT, pspec);

}

static void
anerley_econtact_item_init (AnerleyEContactItem *self)
{
}

AnerleyEContactItem *
anerley_econtact_item_new (EContact *contact)
{
  return g_object_new (ANERLEY_TYPE_ECONTACT_ITEM,
                       "contact",
                       contact,
                       NULL);
}

const gchar *
anerley_econtact_item_get_uid (AnerleyEContactItem *item)
{
  AnerleyEContactItemPrivate *priv = GET_PRIVATE (item);

  return (const gchar *)e_contact_get_const (priv->contact,
                                             E_CONTACT_UID);
}
