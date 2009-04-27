#include "anerley-item.h"

G_DEFINE_TYPE (AnerleyItem, anerley_item, G_TYPE_OBJECT)

enum
{
  DISPLAY_NAME_CHANGED,
  AVATAR_PATH_CHANGED,
  PRESENCE_CHANGED,
  LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

static void
anerley_item_class_init (AnerleyItemClass *klass)
{
  signals[DISPLAY_NAME_CHANGED] =
    g_signal_new ("display-name-changed",
                  ANERLEY_TYPE_ITEM,
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (AnerleyItemClass, display_name_changed),
                  NULL,
                  NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE,
                  0);

  signals[AVATAR_PATH_CHANGED] =
    g_signal_new ("avatar-path-changed",
                  ANERLEY_TYPE_ITEM,
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (AnerleyItemClass, avatar_path_changed),
                  NULL,
                  NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE,
                  0);
  signals[PRESENCE_CHANGED] =
    g_signal_new ("presence_changed",
                  ANERLEY_TYPE_ITEM,
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (AnerleyItemClass, presence_changed),
                  NULL,
                  NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE,
                  0);
}

static void
anerley_item_init (AnerleyItem *self)
{
}

const gchar *
anerley_item_get_display_name (AnerleyItem *item)
{
  return ANERLEY_ITEM_GET_CLASS (item)->get_display_name (item);
}

const gchar *
anerley_item_get_avatar_path (AnerleyItem *item)
{
  return ANERLEY_ITEM_GET_CLASS (item)->get_avatar_path (item);
}

const gchar *
anerley_item_get_presence_status (AnerleyItem *item)
{
  return ANERLEY_ITEM_GET_CLASS (item)->get_presence_status (item);
}

const gchar *
anerley_item_get_presence_message (AnerleyItem *item)
{
  return ANERLEY_ITEM_GET_CLASS (item)->get_presence_message (item);
}

void
anerley_item_emit_display_name_changed (AnerleyItem *item)
{
  g_signal_emit (item, signals[DISPLAY_NAME_CHANGED], 0);
}

void
anerley_item_emit_avatar_path_changed (AnerleyItem *item)
{
  g_signal_emit (item, signals[AVATAR_PATH_CHANGED], 0);
}

void
anerley_item_emit_presence_changed (AnerleyItem *item)
{
  g_signal_emit (item, signals[PRESENCE_CHANGED], 0);
}

void
anerley_item_activate (AnerleyItem *item)
{
  return ANERLEY_ITEM_GET_CLASS (item)->activate (item);
}
