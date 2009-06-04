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

#include "anerley-ebook-feed.h"
#include <libebook/e-book.h>
#include <anerley/anerley-feed.h>
#include <anerley/anerley-econtact-item.h>

static void feed_interface_init (gpointer g_iface, gpointer iface_data);
G_DEFINE_TYPE_WITH_CODE (AnerleyEBookFeed,
                         anerley_ebook_feed,
                         G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (ANERLEY_TYPE_FEED,
                                                feed_interface_init));

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), ANERLEY_TYPE_EBOOK_FEED, AnerleyEBookFeedPrivate))

typedef struct _AnerleyEBookFeedPrivate AnerleyEBookFeedPrivate;

struct _AnerleyEBookFeedPrivate {
  EBook *book;
  EBookView *view;
  GHashTable *uids_to_items;
};

enum
{
  PROP_0,
  PROP_BOOK
};

static void
anerley_ebook_feed_set_book (AnerleyEBookFeed *feed,
                             EBook            *book);

static void
anerley_ebook_feed_get_property (GObject *object, guint property_id,
                              GValue *value, GParamSpec *pspec)
{
  AnerleyEBookFeedPrivate *priv = GET_PRIVATE (object);

  switch (property_id) {
    case PROP_BOOK:
      g_value_set_object (value, priv->book);
      break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
anerley_ebook_feed_set_property (GObject *object, guint property_id,
                              const GValue *value, GParamSpec *pspec)
{
  switch (property_id) {
    case PROP_BOOK:
      anerley_ebook_feed_set_book ((AnerleyEBookFeed *)object,
                                   (EBook *)g_value_get_object (value));
      break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
anerley_ebook_feed_dispose (GObject *object)
{
  AnerleyEBookFeedPrivate *priv = GET_PRIVATE (object);

  if (priv->book)
  {
    g_object_unref (priv->book);
    priv->book = NULL;
  }

  if (priv->view)
  {
    g_object_unref (priv->view);
    priv->view = NULL;
  }

  if (priv->uids_to_items)
  {
    g_hash_table_unref (priv->uids_to_items);
    priv->uids_to_items = NULL;
  }

  G_OBJECT_CLASS (anerley_ebook_feed_parent_class)->dispose (object);
}

static void
anerley_ebook_feed_finalize (GObject *object)
{
  G_OBJECT_CLASS (anerley_ebook_feed_parent_class)->finalize (object);
}

static void
anerley_ebook_feed_class_init (AnerleyEBookFeedClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GParamSpec *pspec;

  g_type_class_add_private (klass, sizeof (AnerleyEBookFeedPrivate));

  object_class->get_property = anerley_ebook_feed_get_property;
  object_class->set_property = anerley_ebook_feed_set_property;
  object_class->dispose = anerley_ebook_feed_dispose;
  object_class->finalize = anerley_ebook_feed_finalize;

  pspec = g_param_spec_object ("book",
                               "The book to read",
                               "The book to pull the contacts from",
                               E_TYPE_BOOK,
                               G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
  g_object_class_install_property (object_class, PROP_BOOK, pspec);
}

static void
anerley_ebook_feed_init (AnerleyEBookFeed *self)
{
  AnerleyEBookFeedPrivate *priv = GET_PRIVATE (self);

  priv->uids_to_items = g_hash_table_new_full (g_str_hash,
                                               g_str_equal,
                                               g_free,
                                               g_object_unref);
}

AnerleyEBookFeed *
anerley_ebook_feed_new (EBook *book)
{
  return g_object_new (ANERLEY_TYPE_EBOOK_FEED,
                       "book",
                       book,
                       NULL);
}

static void
_e_book_view_contacts_added_cb (EBookView *view,
                                GList     *contacts,
                                gpointer   userdata)
{
  AnerleyEBookFeed *feed = (AnerleyEBookFeed *)userdata;
  AnerleyEBookFeedPrivate *priv = GET_PRIVATE (userdata);
  AnerleyEContactItem *item;
  EContact *contact;
  GList *l;
  const gchar *uid;
  GList *items_added = NULL;

  for (l = contacts; l; l = l->next)
  {
    contact = (EContact *)l->data;

    item = anerley_econtact_item_new (contact);

    uid = e_contact_get_const (contact,
                               E_CONTACT_UID);
    /* captures reference */
    g_hash_table_insert (priv->uids_to_items,
                         g_strdup (uid),
                         item);
    items_added = g_list_append (items_added,
                                 item);
  }

  if (items_added)
  {
    g_signal_emit_by_name (feed,
                           "items-added",
                           items_added);
    g_list_free (items_added);
  }
}

static void
_e_book_view_contacts_removed_cb (EBookView *view,
                                  GList     *uids,
                                  gpointer   userdata)
{
  AnerleyEBookFeed *feed = (AnerleyEBookFeed *)userdata;
  AnerleyEBookFeedPrivate *priv = GET_PRIVATE (userdata);
  AnerleyEContactItem *item;
  GList *l;
  GList *items_removed = NULL;
  const gchar *uid;

  for (l = uids; l; l = l->next)
  {
    uid = (gchar *)l->data;

    item = g_hash_table_lookup (priv->uids_to_items, uid);

    /* reference item here since when we remove it we lose original */
    items_removed = g_list_append (items_removed, g_object_ref (item));
    g_hash_table_remove (priv->uids_to_items, uid);
  }

  if (items_removed)
  {
    g_signal_emit_by_name (feed,
                           "items-removed",
                           items_removed);
    g_list_foreach (items_removed, (GFunc)g_object_unref, NULL);
    g_list_free (items_removed);
  }
}

static void
_e_book_view_contacts_changed_cb (EBookView *view,
                                  GList     *contacts,
                                  gpointer   userdata)
{
  AnerleyEBookFeedPrivate *priv = GET_PRIVATE (userdata);
  GList *l;
  EContact *contact;
  AnerleyEContactItem *item;
  const gchar *uid;

  for (l = contacts; l; l = l->next)
  {
    contact = (EContact *)l->data;
    uid = e_contact_get_const (contact, E_CONTACT_UID);

    item = g_hash_table_lookup (priv->uids_to_items,
                                uid);

    g_object_set (item,
                  "contact",
                  contact,
                  NULL);
  }
}

static void
_e_book_async_get_view_cb (EBook       *book,
                           EBookStatus  status,
                           EBookView   *view,
                           gpointer     userdata)
{
  AnerleyEBookFeed *feed = (AnerleyEBookFeed *)userdata;
  AnerleyEBookFeedPrivate *priv = GET_PRIVATE (feed);

  if (!view)
    return;

  priv->view = g_object_ref (view);

  g_signal_connect (view,
                    "contacts-added",
                    (GCallback)_e_book_view_contacts_added_cb,
                    feed);
  g_signal_connect (view,
                    "contacts-removed",
                    (GCallback)_e_book_view_contacts_removed_cb,
                    feed);
  g_signal_connect (view,
                    "contacts-changed",
                    (GCallback)_e_book_view_contacts_changed_cb,
                    feed);

  e_book_view_start (view);
}

static void
anerley_ebook_feed_set_book (AnerleyEBookFeed *feed,
                             EBook            *book)
{
  AnerleyEBookFeedPrivate *priv = GET_PRIVATE (feed);
  GError *error = NULL;
  EBookQuery *query;

  priv->book = g_object_ref (book);

  if (!e_book_open (priv->book,
                    FALSE,
                    &error))
  {
    g_warning (G_STRLOC ": Error opening addressbook: %s",
               error->message);
    g_clear_error (&error);

    return;
  }

  query = e_book_query_any_field_contains ("");
  e_book_async_get_book_view (priv->book,
                              query,
                              NULL,
                              0,
                              _e_book_async_get_view_cb,
                              feed);
  e_book_query_unref (query);
}

static void
feed_interface_init (gpointer g_iface,
                     gpointer iface_data)
{
  /* Nothing to do here..? */
}
