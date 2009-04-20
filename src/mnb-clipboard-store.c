#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "mnb-clipboard-store.h"
#include "marshal.h"

#include <gtk/gtk.h>
#include <string.h>

#define MNB_CLIPBOARD_STORE_GET_PRIVATE(obj)    (G_TYPE_INSTANCE_GET_PRIVATE ((obj), MNB_TYPE_CLIPBOARD_STORE, MnbClipboardStorePrivate))

typedef struct _ClipboardItem  ClipboardItem;

struct _MnbClipboardStorePrivate
{
  /* XXX owned by GTK+ - DO NOT UNREF */
  GtkClipboard *clipboard;
  GtkClipboard *primary;

  /* expiration delta */
  gint64 max_time;

  /* serial */
  gint64 last_serial;

  gulong expire_id;

  gchar *selection;
};

enum
{
  COLUMN_ITEM_TYPE = 0,
  COLUMN_ITEM_TEXT,
  COLUMN_ITEM_URIS,
  COLUMN_ITEM_IMAGE,
  COLUMN_ITEM_MTIME,
  COLUMN_ITEM_IS_SELECTION,
  COLUMN_ITEM_SERIAL,

  N_COLUMNS
};

enum
{
  ITEM_ADDED,
  ITEM_REMOVED,
  SELECTION_CHANGED,

  LAST_SIGNAL
};

G_DEFINE_TYPE (MnbClipboardStore, mnb_clipboard_store, CLUTTER_TYPE_LIST_MODEL);

struct _ClipboardItem
{
  MnbClipboardItemType type;

  MnbClipboardStore *store;

  gint64 mtime;
  gint64 serial;

  guint is_selection : 1;
};

static gulong store_signals[LAST_SIGNAL] = { 0, };

static gboolean
expire_clipboard_items (gpointer data)
{
  MnbClipboardStore *store = data;
  ClutterModelIter *iter;
  GArray *expire_list = g_array_new (FALSE, FALSE, sizeof (guint));
  GTimeVal now;
  gint i;

  g_get_current_time (&now);

  iter = clutter_model_get_last_iter (CLUTTER_MODEL (store));
  do
    {
      guint row = clutter_model_iter_get_row (iter);
      gint64 item_mtime = 0;

      clutter_model_iter_get (iter, COLUMN_ITEM_MTIME, &item_mtime, -1);

      /* we should not remove while iterating */
      if ((now.tv_sec - item_mtime) > store->priv->max_time)
        g_array_append_val (expire_list, row);

      iter = clutter_model_iter_prev (iter);
    }
  while (!clutter_model_iter_is_first (iter));

  g_object_unref (iter);

  for (i = 0; i < expire_list->len; i++)
    {
      clutter_model_remove (CLUTTER_MODEL (store),
                            g_array_index (expire_list, guint, i));
    }

  g_array_free (expire_list, TRUE);

  store->priv->expire_id = 0;

  return FALSE;
}

static void
on_clipboard_request_text (GtkClipboard *clipboard,
                           const gchar  *text,
                           gpointer      data)
{
  MnbClipboardStorePrivate *priv;
  ClipboardItem *item = data;

  if (text == NULL || *text == '\0')
    return;

  priv = item->store->priv;

  if (item->is_selection)
    {
      g_free (priv->selection);
      priv->selection = g_strdup (text);

      g_signal_emit (item->store, store_signals[SELECTION_CHANGED], 0, text);

      g_object_unref (item->store);
      g_slice_free (ClipboardItem, item);

      return;
    }

  clutter_model_prepend (CLUTTER_MODEL (item->store),
                         COLUMN_ITEM_TYPE, item->type,
                         COLUMN_ITEM_SERIAL, item->serial,
                         COLUMN_ITEM_MTIME, item->mtime,
                         COLUMN_ITEM_TEXT, text,
                         COLUMN_ITEM_IS_SELECTION, item->is_selection,
                         -1);

  {
    gchar *str = g_strndup (text, 32);
    gboolean ellipsized = FALSE;

    if (strcmp (str, text) != 0)
      ellipsized = TRUE;

    g_debug ("%s: Added '%s%s' (mtime: %lld, serial: %lld)",
             G_STRLOC,
             str,
             ellipsized ? "..." : "",
             item->mtime,
             item->serial);

    g_free (str);
  }

  g_signal_emit (item->store, store_signals[ITEM_ADDED], 0,
                 item->type,
                 0,
                 item->is_selection);

  /* if an expiration has already been schedule, coalesce it */
  if (priv->expire_id == 0)
    priv->expire_id = g_idle_add_full (G_PRIORITY_LOW,
                                       expire_clipboard_items,
                                       g_object_ref (item->store),
                                       (GDestroyNotify) g_object_unref);

  g_object_unref (item->store);
  g_slice_free (ClipboardItem, item);
}

#if GTK_CHECK_VERSION(2, 14, 0)
static void
on_clipboard_request_uris (GtkClipboard  *clipboard,
                           gchar        **uris,
                           gpointer       data)
{
  ClipboardItem *item = data;

  clutter_model_prepend (CLUTTER_MODEL (item->store),
                         COLUMN_ITEM_TYPE, item->type,
                         COLUMN_ITEM_SERIAL, item->serial,
                         COLUMN_ITEM_MTIME, item->mtime,
                         COLUMN_ITEM_URIS, uris,
                         COLUMN_ITEM_IS_SELECTION, item->is_selection,
                         -1);

  g_signal_emit (item->store, store_signals[ITEM_ADDED], 0, item->type);

  g_object_unref (item->store);
  g_slice_free (ClipboardItem, item);
}
#endif /* GTK_CHECK_VERSION */

static void
on_clipboard_request_targets (GtkClipboard *clipboard,
                              GdkAtom      *atoms,
                              gint          n_atoms,
                              gpointer      data)
{
  ClipboardItem *tmp = data;
  gboolean free_item = TRUE;
  gint i;

  if (atoms == NULL)
    goto out;

  /* step 2: we get a copy of what the clipboard is holding */
  for (i = 0; i < n_atoms; i++)
    {
      if (atoms[i] == gdk_atom_intern_static_string ("UTF8_STRING"))
        {
          tmp->type = MNB_CLIPBOARD_ITEM_TEXT;
          break;
        }
      else if (atoms[i] == gdk_atom_intern_static_string ("text/uri-list"))
        {
          tmp->type = MNB_CLIPBOARD_ITEM_URIS;
          break;
        }
      else
        continue;
    }

  if (tmp->type == MNB_CLIPBOARD_ITEM_INVALID)
    goto out;

  switch (tmp->type)
    {
    case MNB_CLIPBOARD_ITEM_TEXT:
      gtk_clipboard_request_text (clipboard,
                                  on_clipboard_request_text,
                                  tmp);
      free_item = FALSE;
      break;

    case MNB_CLIPBOARD_ITEM_URIS:
#if GTK_CHECK_VERSION(2, 14, 0)
      gtk_clipboard_request_uris (clipboard,
                                  on_clipboard_request_uris,
                                  tmp);
      free_item = FALSE;
#endif
      break;

    case MNB_CLIPBOARD_ITEM_IMAGE:
      break;

    case MNB_CLIPBOARD_ITEM_INVALID:
      g_assert_not_reached ();
      break;
    }

out:

  if (free_item)
    {
      g_object_unref (tmp->store);
      g_slice_free (ClipboardItem, tmp);
    }
}

static void
on_clipboard_owner_change (GtkClipboard      *clipboard,
                           GdkEvent          *event,
                           MnbClipboardStore *store)
{
  ClipboardItem *tmp;
  GTimeVal now;

  g_get_current_time (&now);

  tmp = g_slice_new (ClipboardItem);

  tmp->type = MNB_CLIPBOARD_ITEM_INVALID;
  tmp->serial = store->priv->last_serial;
  tmp->store = g_object_ref (store);
  tmp->mtime = now.tv_sec;
  tmp->is_selection = (clipboard == store->priv->primary) ? TRUE : FALSE;

  store->priv->last_serial += 1;

  /* step 1: we ask what the clipboard is holding */
  gtk_clipboard_request_targets (clipboard,
                                 on_clipboard_request_targets,
                                 tmp);
}

static void
mnb_clipboard_store_row_removed (ClutterModel     *model,
                                 ClutterModelIter *iter)
{
  gint64 serial = 0;

  clutter_model_iter_get (iter, COLUMN_ITEM_SERIAL, &serial, -1);

  CLUTTER_MODEL_CLASS (mnb_clipboard_store_parent_class)->row_removed (model, iter);

  /* now the row does not exist anymore and we can emit the signal */

  g_signal_emit (model, store_signals[ITEM_REMOVED], 0, serial);
}

static void
mnb_clipboard_store_class_init (MnbClipboardStoreClass *klass)
{
  ClutterModelClass *model_class = CLUTTER_MODEL_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MnbClipboardStorePrivate));

  model_class->row_removed = mnb_clipboard_store_row_removed;

  store_signals[ITEM_ADDED] =
    g_signal_new (g_intern_static_string ("item-added"),
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (MnbClipboardStoreClass, item_added),
                  NULL, NULL,
                  moblin_netbook_marshal_VOID__ENUM,
                  G_TYPE_NONE, 1,
                  MNB_TYPE_CLIPBOARD_ITEM_TYPE);

  store_signals[ITEM_REMOVED] =
    g_signal_new (g_intern_static_string ("item-removed"),
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (MnbClipboardStoreClass, item_removed),
                  NULL, NULL,
                  moblin_netbook_marshal_VOID__INT64,
                  G_TYPE_NONE, 1,
                  G_TYPE_INT64);

  store_signals[SELECTION_CHANGED] =
    g_signal_new (g_intern_static_string ("selection-changed"),
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (MnbClipboardStoreClass, selection_changed),
                  NULL, NULL,
                  moblin_netbook_marshal_VOID__STRING,
                  G_TYPE_NONE, 1,
                  G_TYPE_STRING);
}

static void
mnb_clipboard_store_init (MnbClipboardStore *self)
{
  MnbClipboardStorePrivate *priv;
  ClutterModel *model = CLUTTER_MODEL (self);
  GType column_types[] = {
    G_TYPE_INT,         /* COLUMN_ITEM_TYPE */
    G_TYPE_STRING,      /* COLUMN_ITEM_TEXT */
    G_TYPE_STRV,        /* COLUMN_ITEM_URIS */
    G_TYPE_POINTER,     /* COLUMN_ITEM_IMAGE */
    G_TYPE_INT64,       /* COLUMN_ITEM_MTIME */
    G_TYPE_BOOLEAN,     /* COLUMN_ITEM_IS_SELECTION */
    G_TYPE_INT64,       /* COLUMN_ITEM_SERIAL */
  };

  self->priv = priv = MNB_CLIPBOARD_STORE_GET_PRIVATE (self);

  clutter_model_set_types (model, N_COLUMNS, column_types);

  priv->clipboard = gtk_clipboard_get (GDK_SELECTION_CLIPBOARD);
  g_signal_connect (self->priv->clipboard,
                    "owner-change", G_CALLBACK (on_clipboard_owner_change),
                    self);

  priv->primary = gtk_clipboard_get (GDK_SELECTION_PRIMARY);
  g_signal_connect (self->priv->primary,
                    "owner-change", G_CALLBACK (on_clipboard_owner_change),
                    self);

  /* XXX - keep an item around for two hours; this should be
   * hooked into GConf
   */
  priv->max_time = (60 * 60 * 2);

  priv->last_serial = 1;
}

MnbClipboardStore *
mnb_clipboard_store_new (void)
{
  return g_object_new (MNB_TYPE_CLIPBOARD_STORE, NULL);
}

gchar *
mnb_clipboard_store_get_last_text (MnbClipboardStore *store,
                                   gint64            *mtime,
                                   gint64            *serial)
{
  ClutterModelIter *iter;
  MnbClipboardItemType item_type = MNB_CLIPBOARD_ITEM_INVALID;
  gchar *text = NULL;
  gint64 timestamp = 0;
  gint64 id = 0;

  g_return_val_if_fail (MNB_IS_CLIPBOARD_STORE (store), NULL);

  iter = clutter_model_get_first_iter (CLUTTER_MODEL (store));
  clutter_model_iter_get (iter,
                          COLUMN_ITEM_TYPE, &item_type,
                          COLUMN_ITEM_TEXT, &text,
                          COLUMN_ITEM_MTIME, &timestamp,
                          COLUMN_ITEM_SERIAL, &id,
                          -1);

  g_object_unref (iter);

  if (item_type != MNB_CLIPBOARD_ITEM_TEXT)
    {
      GEnumClass *enum_class;
      GEnumValue *enum_value;

      enum_class = g_type_class_peek (MNB_TYPE_CLIPBOARD_ITEM_TYPE);
      if (G_LIKELY (enum_class != NULL))
        {
          const gchar *nick;

          enum_value = g_enum_get_value (enum_class, item_type);
          if (G_LIKELY (enum_value != NULL))
            nick = enum_value->value_nick;
          else
            nick = "<unknown>";

          g_warning ("Requested text, but the last column has type '%s'",
                     nick);
        }
      else
        g_warning ("Requested text, but the last column has type <unknown>");

      g_free (text);
      text = NULL;

      timestamp = 0;
      id = 0;
    }

  if (mtime)
    *mtime = timestamp;

  if (serial)
    *serial = id;

  return text;
}

void
mnb_clipboard_store_remove (MnbClipboardStore *store,
                            gint64             serial)
{
  ClutterModelIter *iter;
  gint row_id = -1;

  g_return_if_fail (MNB_IS_CLIPBOARD_STORE (store));
  g_return_if_fail (serial > 0);

  iter = clutter_model_get_first_iter (CLUTTER_MODEL (store));
  while (!clutter_model_iter_is_last (iter))
    {
      gint64 serial_iter = 0;

      clutter_model_iter_get (iter, COLUMN_ITEM_SERIAL, &serial_iter, -1);
      if (serial_iter == serial)
        {
          row_id = clutter_model_iter_get_row (iter);
          break;
        }

      iter = clutter_model_iter_next (iter);
    }

  g_object_unref (iter);

  if (row_id == -1)
    return;

  clutter_model_remove (CLUTTER_MODEL (store), row_id);
}

void
mnb_clipboard_store_save_selection (MnbClipboardStore *store)
{
  MnbClipboardStorePrivate *priv;
  GTimeVal now;
  gint64 serial;

  g_return_if_fail (MNB_IS_CLIPBOARD_STORE (store));

  priv = store->priv;

  g_get_current_time (&now);

  serial = priv->last_serial;
  priv->last_serial += 1;

  clutter_model_prepend (CLUTTER_MODEL (store),
                         COLUMN_ITEM_TYPE, MNB_CLIPBOARD_ITEM_TEXT,
                         COLUMN_ITEM_SERIAL, serial,
                         COLUMN_ITEM_TEXT, priv->selection,
                         -1);

  g_free (priv->selection);
  priv->selection = NULL;

  g_signal_emit (store, store_signals[SELECTION_CHANGED], 0, NULL);
}

GType
mnb_clipboard_item_type_get_type (void)
{
  static GType our_type = 0;

  if (G_UNLIKELY (our_type == 0))
    {
      static const GEnumValue values[] = {
        { MNB_CLIPBOARD_ITEM_INVALID, "MNB_CLIPBOARD_ITEM_INVALID", "invalid" },
        { MNB_CLIPBOARD_ITEM_TEXT, "MNB_CLIPBOARD_ITEM_TEXT", "text" },
        { MNB_CLIPBOARD_ITEM_URIS, "MNB_CLIPBOARD_ITEM_URIS", "uris" },
        { MNB_CLIPBOARD_ITEM_IMAGE, "MNB_CLIPBOARD_ITEM_IMAGE", "image" },
        { 0, NULL, NULL }
      };

      our_type = g_enum_register_static (g_intern_static_string ("MnbClipboardItemType"),
                                         values);
    }

  return our_type;
}
