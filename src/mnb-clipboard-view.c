#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "mnb-clipboard-view.h"
#include "mnb-clipboard-store.h"

#define MNB_CLIPBOARD_VIEW_GET_PRIVATE(obj)     (G_TYPE_INSTANCE_GET_PRIVATE ((obj), MNB_TYPE_CLIPBOARD_VIEW, MnbClipboardViewPrivate))

struct _MnbClipboardViewPrivate
{
  MnbClipboardStore *store;
};

enum
{
  PROP_0,

  PROP_STORE
};

G_DEFINE_TYPE (MnbClipboardView, mnb_clipboard_view, NBTK_TYPE_GRID);

static void
on_store_item_added (MnbClipboardStore    *store,
                     MnbClipboardItemType  item_type,
                     MnbClipboardView     *view)
{
  ClutterActor *row = NULL;
  ClutterUnit view_width = 0;

  switch (item_type)
    {
    case MNB_CLIPBOARD_ITEM_TEXT:
      {
        gchar *text;

        text = mnb_clipboard_store_get_last_text (store);
        row = CLUTTER_ACTOR (nbtk_label_new (text));

        g_free (text);
      }
      break;

    case MNB_CLIPBOARD_ITEM_URIS:
      break;

    case MNB_CLIPBOARD_ITEM_IMAGE:
      break;

    case MNB_CLIPBOARD_ITEM_INVALID:
      g_assert_not_reached ();
      break;
    }

  if (row == NULL)
    return;

  /* FIXME - hack hack hack; we need NbtkBox */
  view_width = clutter_actor_get_widthu (CLUTTER_ACTOR (view));
  clutter_actor_set_widthu (row, view_width - 1);

  clutter_container_add_actor (CLUTTER_CONTAINER (view), row);
}

static void
mnb_clipboard_view_finalize (GObject *gobject)
{
  MnbClipboardViewPrivate *priv = MNB_CLIPBOARD_VIEW (gobject)->priv;

  g_object_unref (priv->store);

  G_OBJECT_CLASS (mnb_clipboard_view_parent_class)->finalize (gobject);
}

static void
mnb_clipboard_view_set_property (GObject      *gobject,
                                 guint         prop_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
  MnbClipboardViewPrivate *priv = MNB_CLIPBOARD_VIEW (gobject)->priv;

  switch (prop_id)
    {
    case PROP_STORE:
      if (priv->store != NULL)
        g_object_unref (priv->store);

      if (g_value_get_object (value) != NULL)
        priv->store = g_value_dup_object (value);
      else
        priv->store = mnb_clipboard_store_new ();

      g_signal_connect (priv->store, "item-added",
                        G_CALLBACK (on_store_item_added),
                        gobject);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
      break;
    }
}

static void
mnb_clipboard_view_get_property (GObject    *gobject,
                                 guint       prop_id,
                                 GValue     *value,
                                 GParamSpec *pspec)
{
  MnbClipboardViewPrivate *priv = MNB_CLIPBOARD_VIEW (gobject)->priv;

  switch (prop_id)
    {
    case PROP_STORE:
      g_value_set_object (value, priv->store);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
      break;
    }
}

static void
mnb_clipboard_view_class_init (MnbClipboardViewClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GParamSpec *pspec;

  g_type_class_add_private (klass, sizeof (MnbClipboardViewPrivate));

  gobject_class->set_property = mnb_clipboard_view_set_property;
  gobject_class->get_property = mnb_clipboard_view_get_property;
  gobject_class->finalize = mnb_clipboard_view_finalize;

  pspec = g_param_spec_object ("store",
                               "Store",
                               "The MnbClipboardStore",
                               MNB_TYPE_CLIPBOARD_STORE,
                               G_PARAM_READWRITE |
                               G_PARAM_CONSTRUCT);
  g_object_class_install_property (gobject_class, PROP_STORE, pspec);
}

static void
mnb_clipboard_view_init (MnbClipboardView *view)
{
  view->priv = MNB_CLIPBOARD_VIEW_GET_PRIVATE (view);
}

NbtkWidget *
mnb_clipboard_view_new (MnbClipboardStore *store)
{
  g_return_val_if_fail (store == NULL || MNB_IS_CLIPBOARD_STORE (store), NULL);

  return g_object_new (MNB_TYPE_CLIPBOARD_VIEW,
                       "store", store,
                       NULL);
}
