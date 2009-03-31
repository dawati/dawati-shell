#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "mnb-clipboard-store.h"
#include "marshal.h"

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
  COLUMN_ITEM_IS_SELECTION,

  N_COLUMNS
};

enum
{
  ITEM_ADDED,

  LAST_SIGNAL
};

G_DEFINE_TYPE (MnbClipboardStore, mnb_clipboard_store, CLUTTER_TYPE_LIST_MODEL);

struct _ClipboardItem
{
  MnbClipboardItemType type;

  MnbClipboardStore *store;

  gint64 mtime;

  guint is_selection : 1;
};

static gulong store_signals[LAST_SIGNAL] = { 0, };

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
                         COLUMN_ITEM_IS_SELECTION, item->is_selection,
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

  g_signal_emit (item->store, store_signals[ITEM_ADDED], 0,
                 item->type,
                 0,
                 item->is_selection);

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
  tmp->store = g_object_ref (store);
  tmp->mtime = now.tv_sec;
  tmp->is_selection = FALSE;

  /* step 1: we ask what the clipboard is holding */
  gtk_clipboard_request_targets (clipboard,
                                 on_clipboard_request_targets,
                                 tmp);
}

static void
mnb_clipboard_store_class_init (MnbClipboardStoreClass *klass)
{
  g_type_class_add_private (klass, sizeof (MnbClipboardStorePrivate));

  store_signals[ITEM_ADDED] =
    g_signal_new (g_intern_static_string ("item-added"),
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (MnbClipboardStoreClass, item_added),
                  NULL, NULL,
                  moblin_netbook_marshal_VOID__ENUM,
                  G_TYPE_NONE, 1,
                  MNB_TYPE_CLIPBOARD_ITEM_TYPE);
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
