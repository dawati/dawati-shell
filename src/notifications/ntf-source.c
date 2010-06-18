/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */

/*
 * Copyright (c) 2010 Intel Corp.
 *
 * Authors: Tomas Frydrych <tf@linux.intel.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "../meego-netbook.h"

#include "ntf-source.h"

static GHashTable *sources = NULL;

static void ntf_source_dispose (GObject *object);
static void ntf_source_finalize (GObject *object);

G_DEFINE_TYPE (NtfSource, ntf_source, G_TYPE_OBJECT);

#define NTF_SOURCE_GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), NTF_TYPE_SOURCE, NtfSourcePrivate))

struct _NtfSourcePrivate
{
  gchar        *id;
  MetaWindow   *window;
  ClutterActor *icon;

  gulong window_unmanaged_id;

  guint disposed : 1;
};

enum {
  CLOSED,

  N_SIGNALS,
};

enum {
  PROP_0,

  PROP_ID,
  PROP_WINDOW,
};

static guint signals[N_SIGNALS] = {0};

static void
ntf_source_get_property (GObject    *object,
                         guint       property_id,
                         GValue     *value,
                         GParamSpec *pspec)
{
  NtfSource        *self = NTF_SOURCE (object);
  NtfSourcePrivate *priv = self->priv;

  switch (property_id)
    {
    case PROP_ID:
      g_value_set_string (value, priv->id);
      break;
    case PROP_WINDOW:
      g_value_set_object (value, priv->window);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
ntf_source_window_unamanged_cb (MetaWindow *window, NtfSource *self)
{
  NtfSourcePrivate *priv = self->priv;

  priv->window_unmanaged_id = 0;
  priv->window = NULL;

  g_object_ref (self);

  g_signal_emit (self, signals[CLOSED], 0);

  g_object_unref (self);
}

static void
ntf_source_set_property (GObject      *object,
                         guint         property_id,
                         const GValue *value,
                         GParamSpec   *pspec)
{
  NtfSource        *self = NTF_SOURCE (object);
  NtfSourcePrivate *priv = self->priv;

  switch (property_id)
    {
    case PROP_ID:
      g_assert (!priv->id);
      priv->id = g_value_dup_string (value);
      break;
    case PROP_WINDOW:
      g_assert (!priv->window);
      priv->window = g_value_get_object (value);

      if (priv->window)
        {
          priv->window_unmanaged_id =
            g_signal_connect (priv->window, "unmanaged",
                              G_CALLBACK (ntf_source_window_unamanged_cb),
                              self);
        }
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
ntf_source_constructed (GObject *object)
{
  NtfSource        *self = NTF_SOURCE (object);
  NtfSourcePrivate *priv = self->priv;

  if (G_OBJECT_CLASS (ntf_source_parent_class)->constructed)
    G_OBJECT_CLASS (ntf_source_parent_class)->constructed (object);

  g_assert (priv->id);
}

static void
ntf_source_closed (NtfSource *self)
{
  NtfSourcePrivate *priv = self->priv;

  g_assert (priv->id);

  /*
   * Remove this source from the sources database; doing so will release the
   * default reference held by the database.
   */
  if (!g_hash_table_remove (sources, priv->id))
    g_warning (G_STRLOC ": Source %s was not found in database", priv->id);
}

static void
ntf_source_icon_changed_cb (MetaWindow *window,
                            GParamSpec *pspec,
                            NtfSource  *src)
{
  NtfSourcePrivate *priv   = src->priv;
  GdkPixbuf        *pixbuf = NULL;

  g_object_get (priv->window, "icon", &pixbuf, NULL);

  if (!pixbuf && priv->icon)
    {
      clutter_actor_destroy (priv->icon);
      priv->icon = NULL;
    }
  else if (pixbuf && priv->icon)
    {
      clutter_texture_set_from_rgb_data (CLUTTER_TEXTURE (priv->icon),
                                         gdk_pixbuf_get_pixels (pixbuf),
                                         gdk_pixbuf_get_has_alpha (pixbuf),
                                         gdk_pixbuf_get_width (pixbuf),
                                         gdk_pixbuf_get_height (pixbuf),
                                         gdk_pixbuf_get_rowstride (pixbuf),
                                         gdk_pixbuf_get_has_alpha (pixbuf) ? 4 : 3,
                                         0, NULL);
    }
}

static ClutterActor *
ntf_source_get_icon_real (NtfSource *self)
{
  NtfSourcePrivate *priv   = self->priv;
  GdkPixbuf        *pixbuf = NULL;

  if (!priv->window)
    return NULL;

  g_object_get (priv->window, "icon", &pixbuf, NULL);

  if (pixbuf)
    {
      ClutterActor *icon = clutter_texture_new ();

      clutter_texture_set_from_rgb_data (CLUTTER_TEXTURE (icon),
                                         gdk_pixbuf_get_pixels (pixbuf),
                                         gdk_pixbuf_get_has_alpha (pixbuf),
                                         gdk_pixbuf_get_width (pixbuf),
                                         gdk_pixbuf_get_height (pixbuf),
                                         gdk_pixbuf_get_rowstride (pixbuf),
                                         gdk_pixbuf_get_has_alpha (pixbuf) ? 4 : 3,
                                         0, NULL);

      g_signal_connect (priv->window, "notify::icon",
                        G_CALLBACK (ntf_source_icon_changed_cb),
                        self);

      return icon;
    }

  return NULL;
}

static void
ntf_source_class_init (NtfSourceClass *klass)
{
  GObjectClass *object_class = (GObjectClass *)klass;

  g_type_class_add_private (klass, sizeof (NtfSourcePrivate));

  object_class->dispose      = ntf_source_dispose;
  object_class->finalize     = ntf_source_finalize;
  object_class->get_property = ntf_source_get_property;
  object_class->set_property = ntf_source_set_property;
  object_class->constructed  = ntf_source_constructed;

  klass->closed              = ntf_source_closed;
  klass->get_icon            = ntf_source_get_icon_real;

  g_object_class_install_property (object_class,
                                   PROP_ID,
                                   g_param_spec_string ("id",
                                                    "id",
                                                    "Unique id of the source",
                                                    NULL,
                                                    G_PARAM_READWRITE |
                                                    G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class,
                                   PROP_WINDOW,
                                   g_param_spec_object ("window",
                                                    "MetaWindow",
                                                    "MetaWindow associated with this source",
                                                    META_TYPE_WINDOW,
                                                    G_PARAM_READWRITE |
                                                    G_PARAM_CONSTRUCT_ONLY));

  signals[CLOSED]
    = g_signal_new ("closed",
                    G_OBJECT_CLASS_TYPE (klass),
                    G_SIGNAL_RUN_LAST,
                    G_STRUCT_OFFSET (NtfSourceClass, closed),
                    NULL, NULL,
                    g_cclosure_marshal_VOID__VOID,
                    G_TYPE_NONE, 0);

  sources = g_hash_table_new_full (g_str_hash,
                                   g_str_equal,
                                   NULL, /* keys are owned by the sources */
                                   g_object_unref);
}

static void
ntf_source_init (NtfSource *self)
{
  self->priv = NTF_SOURCE_GET_PRIVATE (self);
}

static void
ntf_source_dispose (GObject *object)
{
  NtfSource        *self = NTF_SOURCE (object);
  NtfSourcePrivate *priv = self->priv;

  if (priv->disposed)
    return;

  priv->disposed = TRUE;

  if (priv->window_unmanaged_id)
    {
      g_assert (priv->window);

      g_signal_handler_disconnect (priv->window, priv->window_unmanaged_id);
      priv->window = NULL;
      priv->window_unmanaged_id = 0;
    }

  G_OBJECT_CLASS (ntf_source_parent_class)->dispose (object);
}

static void
ntf_source_finalize (GObject *object)
{
  NtfSource        *self = (NtfSource *)object;
  NtfSourcePrivate *priv = self->priv;

  g_free (priv->id);

  G_OBJECT_CLASS (ntf_source_parent_class)->finalize (object);
}

/**
 * ntf_source_new_for_window:
 * @window: #MetaWindow associated with this source
 *
 * Create a new source; the source will be automatically destroyed if the
 * associated window is destroyed.
 */
NtfSource *
ntf_source_new_for_window (MetaWindow *window)
{
  const gchar *machine;
  gint         pid;
  gchar       *id;
  NtfSource   *src;

  g_return_val_if_fail (window, NULL);

  machine = meta_window_get_client_machine (window);
  pid     = meta_window_get_pid (window);

  /*
   * Refuse to create sources for badly behaved clients for which pid is not
   * set.
   */
  g_return_val_if_fail (pid, NULL);

  id = g_strdup_printf ("application-%d@%s", pid, machine ? machine : "local");

  src = g_object_new (NTF_TYPE_SOURCE,
                      "window", window,
                      "id", id,
                      NULL);

  g_free (id);

  return src;
}

/**
 * ntf_source_new_for_pid:
 * @machine: client machine of the source process,
 * @pid: pid of the source process
 *
 * Create a new source; this function attempts to locate a suitable window
 * to associate with this process.
 */
NtfSource *
ntf_source_new_for_pid (const gchar *machine, gint pid)
{
  MutterPlugin *plugin = meego_netbook_get_plugin_singleton ();
  MetaScreen   *screen = mutter_plugin_get_screen (plugin);
  gchar        *id;
  NtfSource    *src;
  MetaWindow   *window = NULL;
  GList        *l;

  g_return_val_if_fail (pid, NULL);


  for (l = mutter_get_windows (screen); l; l = l->next)
    {
      MutterWindow *m = l->data;
      MetaWindow   *w = mutter_window_get_meta_window (m);
      const gchar  *c = meta_window_get_client_machine (w);
      gint          p = meta_window_get_pid (w);

      if (p != pid)
        continue;

      if ((!c && machine) || (c && !machine))
        continue;

      if (c && strcmp (c, machine))
        continue;

      window = w;
      break;
    }

  id = g_strdup_printf ("application-%d@%s", pid, machine ? machine : "local");

  src = g_object_new (NTF_TYPE_SOURCE,
                      "window", window,
                      "id", id,
                      NULL);

  g_free (id);

  return src;
}

/**
 * ntf_source_get_id:
 * @src: #NtfSource,
 *
 * Returns id associated with the given source.
 *
 * Return value: source id.
 */
const gchar *
ntf_source_get_id (NtfSource *src)
{
  g_return_val_if_fail (NTF_IS_SOURCE (src), NULL);

  return src->priv->id;
}

/**
 * ntf_source_get_icon:
 * @src: #NtfSource
 *
 * Returns icon associated with this source; the #ClutterActor is owned by the
 * source, and the caller must clone it.
 *
 * For the default source implementation this is the icon of the window
 * associated with the source.
 *
 * Return value: icon or %NULL.
 */
ClutterActor *
ntf_source_get_icon (NtfSource *src)
{
  NtfSourcePrivate *priv;
  NtfSourceClass   *klass;
  ClutterActor     *icon;

  g_return_val_if_fail (src, NULL);

  priv = src->priv;

  if (priv->icon)
    return priv->icon;

  klass = NTF_SOURCE_GET_CLASS (src);

  if (klass && (icon = klass->get_icon (src)))
    {
      priv->icon = icon;

      clutter_container_add_actor (CLUTTER_CONTAINER (
                                   clutter_stage_get_default ()),
                                   icon);
      clutter_actor_hide (icon);
    }

  return priv->icon;
}

/*
 * The API for manipulating the sources database. The process of creating a
 * notification, should be as follows:
 *
 *  a) Construct the id for the required source,
 *  b) Check for existence of the source in the databased with
 *     ntf_sources_find_for_id(),
 *  c) If found, use it to construct a notification,
 *  d) If not found, create a new sources and add it to the database using
 *     ntf_sources_add().
 */

/**
 * ntf_sources_find_for_id:
 * @id: id of the source to lookup
 *
 * Looks up source with given id in the global source table. The function does
 * not increase reference count of the returned source, this is up to the
 * caller to do, if reference is required.
 *
 * Return value: #NtfSource for given id, or NULL, if such source does not
 * exist.
 */
NtfSource *
ntf_sources_find_for_id (const gchar *id)
{
  g_return_val_if_fail (id, NULL);

  if (!sources)
    return NULL;

  return g_hash_table_lookup (sources, id);
}

/**
 * ntf_sources_add:
 * @src: #NtfSource
 *
 * Adds the supplied source into the global source database; the caller must
 * check that source for the given id does not exist already. The database takes
 * over the ownership of the src object (i.e., assumes a reference).
 */
void
ntf_sources_add (NtfSource *src)
{
  const gchar *id;

  g_return_if_fail (NTF_IS_SOURCE (src));

  id = ntf_source_get_id (src);

  /* aggressively enforce basic assumptions */
  g_assert (id && !g_hash_table_lookup (sources, id));

  g_hash_table_insert (sources, (gpointer)id, src);
}

MetaWindow *
ntf_source_get_window (NtfSource *src)
{
  NtfSourcePrivate *priv;

  g_return_val_if_fail (src, NULL);

  priv = src->priv;

  return priv->window;
}
