#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "mnb-clipboard-store.h"

#include <gtk/gtk.h>

#define MNB_CLIPBOARD_STORE_GET_PRIVATE(obj)    (G_TYPE_INSTANCE_GET_PRIVATE ((obj), MNB_TYPE_CLIPBOARD_STORE, MnbClipboardStorePrivate))

typedef struct _ClipboardItem  ClipboardItem;

struct _MnbClipboardStorePrivate
{
  /* XXX owned by GTK+ - DO NOT UNREF */
  GtkClipboard *clipboard;
};

enum
{
  COLUMN_ITEM_TYPE,
  COLUMN_ITEM_TEXT,
  COLUMN_ITEM_URIS,
  COLUMN_ITEM_IMAGE,
  COLUMN_ITEM_MTIME,

  N_COLUMNS
};

G_DEFINE_TYPE (MnbClipboardStore, mnb_clipboard_store, CLUTTER_TYPE_LIST_MODEL);

struct _ClipboardItem
{
  MnbClipboardItemType type;

  MnbClipboardStore *store;

  gint64 mtime;
};

static void
on_clipboard_request_text (GtkClipboard *clipboard,
                           const gchar  *text,
                           gpointer      data)
{
  ClipboardItem *item = data;

  if (text == NULL || *text == '\0')
    return;

  clutter_model_prepend (CLUTTER_MODEL (item->store),
                         COLUMN_ITEM_TYPE, item->type,
                         COLUMN_ITEM_MTIME, item->mtime,
                         COLUMN_ITEM_TEXT, text,
                         -1);

  {
    gchar *str = g_strndup (text, 32);
    gboolean ellipsized = FALSE;

    if (strcmp (str, text) != 0)
      ellipsized = TRUE;

    g_debug ("%s: Added '%s%s' (mtime: %lld)",
             G_STRLOC,
             str,
             ellipsized ? "..." : "",
             item->mtime);

    g_free (str);
  }

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
                         COLUMN_ITEM_MTIME, item->mtime,
                         COLUMN_ITEM_URIS, uris,
                         -1);

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

  tmp = g_slice_new (ClipboardItem);

  tmp->type = MNB_CLIPBOARD_ITEM_INVALID;
  tmp->store = g_object_ref (store);
  tmp->mtime = gdk_event_get_time (event);

  /* step 1: we ask what the clipboard is holding */
  gtk_clipboard_request_targets (clipboard,
                                 on_clipboard_request_targets,
                                 tmp);
}

static void
mnb_clipboard_store_class_init (MnbClipboardStoreClass *klass)
{
  g_type_class_add_private (klass, sizeof (MnbClipboardStorePrivate));
}

static void
mnb_clipboard_store_init (MnbClipboardStore *self)
{
  ClutterModel *model = CLUTTER_MODEL (self);
  GType column_types[] = {
    G_TYPE_INT,         /* COLUMN_ITEM_TYPE */
    G_TYPE_STRING,      /* COLUMN_ITEM_TEXT */
    G_TYPE_STRV,        /* COLUMN_ITEM_URIS */
    G_TYPE_POINTER,     /* COLUMN_ITEM_IMAGE */
    G_TYPE_INT64,       /* COLUMN_ITEM_MTIME */
  };

  self->priv = MNB_CLIPBOARD_STORE_GET_PRIVATE (self);

  clutter_model_set_types (model, N_COLUMNS, column_types);

  self->priv->clipboard = gtk_clipboard_get (GDK_SELECTION_CLIPBOARD);
  g_signal_connect (self->priv->clipboard,
                    "owner-change", G_CALLBACK (on_clipboard_owner_change),
                    self);
}

MnbClipboardStore *
mnb_clipboard_store_new (void)
{
  return g_object_new (MNB_TYPE_CLIPBOARD_STORE, NULL);
}
