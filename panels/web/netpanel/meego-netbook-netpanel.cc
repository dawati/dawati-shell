/* meego-netbook-netpanel.c */
/*
 * Copyright (c) 2009 Intel Corp.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU Lesser General Public License,
 * version 2.1, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <dbus/dbus-glib.h>
#include <glib/gi18n.h>
#include <sys/file.h>
#include <glib/gstdio.h>
#include <unistd.h>
#include <string>
#include <stdlib.h>
#include <sqlite3.h>
#include <json-glib/json-glib.h>

extern "C" {
#include <meego-panel/mpl-entry.h>
#include <meego-panel/mpl-utils.h>
#include <meego-panel/mpl-panel-client.h>
#include "meego-netbook-netpanel.h"
#include "mnb-netpanel-bar.h"
#include "mnb-netpanel-scrollview.h"
#include "mwb-utils.h"
}

/* Number of favorites columns to display */
#define NR_FAVORITE_MAX 16
#define NR_FAVORITE 9

/* FIXME: Replace with stylable spacing */
#define COL_SPACING 0
#define ROW_SPACING 0
#define CELL_WIDTH  210
#define CELL_HEIGHT 114
#define FAVI_SIZE   16
#define DISPLAY_TABS_MAX 4

#define START_PAGE "meego://start/"
#define NEWTAB_URL "chrome://newtab/"

#define FAVORITE_SQL  "SELECT url, title FROM urls " \
                      "ORDER BY visit_count DESC LIMIT 9"
#define TAB_SQL       "SELECT tab_id, url, title FROM current_tabs " \
                      "LIMIT 256"

#define CMD_SELECT_TAB 1
#define CMD_NEW_TAB    2

static gboolean
meego_netbook_netpanel_open_tab (MeegoNetbookNetpanel *self, const gint type, void *data);

static void
meego_netbook_netpanel_restore_tab (MeegoNetbookNetpanel *self, gchar* tab_url);

G_DEFINE_TYPE (MeegoNetbookNetpanel, meego_netbook_netpanel, MX_TYPE_WIDGET)

#define NETPANEL_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MEEGO_TYPE_NETBOOK_NETPANEL, MeegoNetbookNetpanelPrivate))

struct _MeegoNetbookNetpanelPrivate
{
  GList          *calls;

  MxWidget     *entry_table;
  MxWidget     *entry;

  MxWidget     *tabs_label;
  MxWidget     *tabs_view;
  MxWidget     *favs_label;
  MxWidget     *favs_view;

  guint           n_tabs;
  guint           n_favs;
  guint           display_tab;
  guint           display_fav;

  MxWidget    **tabs;
  MxWidget    **tab_titles;

  gchar         **fav_urls;
  gchar         **fav_titles;

  GList          *session_urls;

  MplPanelClient *panel_client;

  /*  SQLite connection */
  gchar          *places_db;
  sqlite3        *dbcon;

  gchar          *search_url;
};


static void
meego_netbook_netpanel_dispose (GObject *object)
{
  MeegoNetbookNetpanel *self = MEEGO_NETBOOK_NETPANEL (object);
  MeegoNetbookNetpanelPrivate *priv = self->priv;
  guint i;

  if (priv->panel_client)
    {
      g_object_unref (priv->panel_client);
      priv->panel_client = NULL;
    }

  if (priv->fav_urls)
    {
      for (i = 0; i < priv->n_favs; i++)
        g_free (priv->fav_urls[i]);
      g_free (priv->fav_urls);
      priv->fav_urls = NULL;
    }

  if (priv->fav_titles)
    {
      for (i = 0; i < priv->n_favs; i++)
        g_free (priv->fav_titles[i]);
      g_free (priv->fav_titles);
      priv->fav_titles = NULL;
    }

  if (priv->entry_table)
    {
      clutter_actor_unparent (CLUTTER_ACTOR (priv->entry_table));
      priv->entry_table = NULL;
    }

  if (priv->tabs_label)
    {
      clutter_actor_unparent (CLUTTER_ACTOR (priv->tabs_label));
      priv->tabs_label = NULL;
    }

  if (priv->tabs_view)
    {
      clutter_actor_unparent (CLUTTER_ACTOR (priv->tabs_view));
      priv->tabs_view = NULL;
    }

  if (priv->favs_label)
    {
      clutter_actor_unparent (CLUTTER_ACTOR (priv->favs_label));
      priv->favs_label = NULL;
    }

  if (priv->favs_view)
    {
      clutter_actor_unparent (CLUTTER_ACTOR (priv->favs_view));
      priv->favs_view = NULL;
    }

  if (priv->places_db)
    {
      g_free (priv->places_db);
      priv->places_db = NULL;
    }

  if (priv->search_url)
    {
      g_free (priv->search_url);
      priv->search_url = NULL;
    }

  while (priv->session_urls)
    {
      g_free (priv->session_urls->data);
      priv->session_urls = g_list_delete_link (priv->session_urls,
                                               priv->session_urls);
    }

  G_OBJECT_CLASS (meego_netbook_netpanel_parent_class)->dispose (object);
}

static void
meego_netbook_netpanel_finalize (GObject *object)
{
  G_OBJECT_CLASS (meego_netbook_netpanel_parent_class)->finalize (object);
}

static void
meego_netbook_netpanel_allocate (ClutterActor           *actor,
                                  const ClutterActorBox  *box,
                                  ClutterAllocationFlags  flags)
{
  ClutterActorBox child_box;
  MxPadding padding;
  gfloat width, height;
  gfloat min_heights[5], natural_heights[5], final_heights[5];
  gfloat natural_height;
  guint i;
  MeegoNetbookNetpanelPrivate *priv = MEEGO_NETBOOK_NETPANEL (actor)->priv;

  CLUTTER_ACTOR_CLASS (meego_netbook_netpanel_parent_class)->
    allocate (actor, box, flags);

  mx_widget_get_padding (MX_WIDGET (actor), &padding);
  padding.left   = MWB_PIXBOUND (padding.left);
  padding.top    = MWB_PIXBOUND (padding.top);
  padding.right  = MWB_PIXBOUND (padding.right);
  padding.bottom = MWB_PIXBOUND (padding.bottom);

  width = box->x2 - box->x1 - padding.left - padding.right;

  min_heights[1] = natural_heights[1] = 0.0;
  min_heights[2] = natural_heights[2] = 0.0;

  /* Find out desired child heights */
  clutter_actor_get_preferred_height (CLUTTER_ACTOR (priv->entry_table), width,
                                      min_heights + 0, natural_heights + 0);

  if (priv->tabs_label)
    clutter_actor_get_preferred_height (CLUTTER_ACTOR (priv->tabs_label), width,
                                        min_heights + 1, natural_heights + 1);

  if (priv->tabs_view)
    clutter_actor_get_preferred_height (CLUTTER_ACTOR (priv->tabs_view), width,
                                        min_heights + 2, natural_heights + 2);

  if (priv->favs_label)
    clutter_actor_get_preferred_height (CLUTTER_ACTOR (priv->favs_label), width,
                                        min_heights + 3, natural_heights + 3);

  if (priv->favs_view)
    clutter_actor_get_preferred_height (CLUTTER_ACTOR (priv->favs_view), width,
                                        min_heights + 4, natural_heights + 4);

  height = box->y2 - box->y1 - padding.top - padding.bottom;

  /* Leave extra space between tabs view and favs label for readability */
  height -= ROW_SPACING;

  for (i = 1; i <= 5; i++)
    {
      if (natural_heights[i])
        height -= ROW_SPACING;
    }

  natural_height = 0.0;
  for (i = 0; i < 5; i++)
    natural_height += natural_heights[i];
  if (height >= natural_height)
    {
      /* Just allocate the natural heights */
      for (i = 0; i < 5; i++)
        final_heights[i] = natural_heights[i];
    }
  else
    {
      /* Allocate the minimum heights */
      for (i = 0; i < 5; i++)
        {
          final_heights[i] = min_heights[i];
          height -= min_heights[i];
        }

      if (height > 0.0)
        {
          for (i = 0; i < 5; i++)
            {
              /* Allocate extra space up to natural height */
              gfloat diff = natural_heights[i] - min_heights[i];
              if (height < diff)
                diff = height;
              final_heights[i] += diff;
              height -= diff;
            }
        }
    }

  child_box.x1 = padding.left;
  child_box.x2 = child_box.x1 + width;

  child_box.y1 = padding.top;
  child_box.y2 = child_box.y1 + final_heights[0];
  clutter_actor_allocate (CLUTTER_ACTOR (priv->entry_table),
                          &child_box, flags);

  child_box.y1 = child_box.y2 + ROW_SPACING;
  if (priv->tabs_label)
    {
      child_box.y2 = child_box.y1 + final_heights[1];
      clutter_actor_allocate (CLUTTER_ACTOR (priv->tabs_label),
                              &child_box, flags);
      child_box.y1 = child_box.y2 + ROW_SPACING;
    }

  if (priv->tabs_view)
    {
      child_box.y2 = child_box.y1 + final_heights[2];
      clutter_actor_allocate (CLUTTER_ACTOR (priv->tabs_view),
                              &child_box, flags);
      child_box.y1 = child_box.y2 + ROW_SPACING;
    }

  if (priv->favs_label)
    {
      /* Add extra space left for readability */
      child_box.y1 += ROW_SPACING;

      child_box.y2 = child_box.y1 + final_heights[3];
      clutter_actor_allocate (CLUTTER_ACTOR (priv->favs_label),
                              &child_box, flags);
      child_box.y1 = child_box.y2 + ROW_SPACING;
    }

  if (priv->favs_view)
    {
      child_box.y2 = child_box.y1 + final_heights[4];
      clutter_actor_allocate (CLUTTER_ACTOR (priv->favs_view),
                              &child_box, flags);
    }
}

static void
expand_to_child_width (ClutterActor *actor,
                       gfloat        for_height,
                       gfloat       *min_width_p,
                       gfloat       *natural_width_p)
{
  gfloat min_width, natural_width;

  if (!actor)
    return;

  clutter_actor_get_preferred_width (actor, for_height, &min_width,
                                      &natural_width);
  if (min_width_p && *min_width_p < min_width)
    *min_width_p = min_width;
  if (natural_width_p && *natural_width_p < natural_width)
    *natural_width_p = natural_width;
}

static void
meego_netbook_netpanel_get_preferred_width (ClutterActor *self,
                                             gfloat        for_height,
                                             gfloat        *min_width_p,
                                             gfloat        *natural_width_p)
{
  MxPadding padding;
  gfloat min_width = 0.0, natural_width = 0.0;
  MeegoNetbookNetpanelPrivate *priv = MEEGO_NETBOOK_NETPANEL (self)->priv;

  CLUTTER_ACTOR_CLASS (meego_netbook_netpanel_parent_class)->
    get_preferred_width (self, for_height, min_width_p, natural_width_p);

  if (for_height != -1.0)
    {
      mx_widget_get_padding (MX_WIDGET (self), &padding);
      for_height -= padding.top + padding.bottom;
    }

  expand_to_child_width (CLUTTER_ACTOR (priv->entry_table), for_height,
                         &min_width, &natural_width);
  if (priv->tabs_label)
    expand_to_child_width (CLUTTER_ACTOR (priv->tabs_label), for_height,
                           &min_width, &natural_width);
  if (priv->tabs_view)
    expand_to_child_width (CLUTTER_ACTOR (priv->tabs_view), for_height,
                           &min_width, &natural_width);
  if (priv->favs_label)
    expand_to_child_width (CLUTTER_ACTOR (priv->favs_label), for_height,
                           &min_width, &natural_width);
  if (priv->favs_view)
    expand_to_child_width (CLUTTER_ACTOR (priv->favs_view), for_height,
                           &min_width, &natural_width);

  if (min_width_p)
    *min_width_p += min_width;
  if (natural_width_p)
    *natural_width_p += natural_width;
}

static void
add_child_height (ClutterActor *actor,
                  gfloat        for_width,
                  gfloat       *min_height_p,
                  gfloat       *natural_height_p,
                  gfloat        spacing)
{
  gfloat min_height, natural_height;

  if (!actor)
    return;

  clutter_actor_get_preferred_height (actor, for_width, &min_height,
                                      &natural_height);
  if (min_height_p)
    *min_height_p += min_height + spacing;
  if (natural_height_p)
    *natural_height_p += natural_height + spacing;
}

static void
meego_netbook_netpanel_get_preferred_height (ClutterActor *self,
                                              gfloat        for_width,
                                              gfloat       *min_height_p,
                                              gfloat       *natural_height_p)
{
  MxPadding padding;
  MeegoNetbookNetpanelPrivate *priv = MEEGO_NETBOOK_NETPANEL (self)->priv;

  CLUTTER_ACTOR_CLASS (meego_netbook_netpanel_parent_class)->
    get_preferred_height (self, for_width, min_height_p, natural_height_p);

  mx_widget_get_padding (MX_WIDGET (self), &padding);
  for_width -= padding.left + padding.right;

  add_child_height (CLUTTER_ACTOR (priv->entry_table), for_width,
                    min_height_p, natural_height_p, 0.0);
  add_child_height (CLUTTER_ACTOR (priv->tabs_label), for_width,
                    min_height_p, natural_height_p, ROW_SPACING);
  add_child_height (CLUTTER_ACTOR (priv->tabs_view), for_width,
                    min_height_p, natural_height_p, ROW_SPACING);
  add_child_height (CLUTTER_ACTOR (priv->favs_label), for_width,
                    min_height_p, natural_height_p, ROW_SPACING);
  add_child_height (CLUTTER_ACTOR (priv->favs_view), for_width,
                    min_height_p, natural_height_p, ROW_SPACING);
}

static void
meego_netbook_netpanel_paint (ClutterActor *actor)
{
  MeegoNetbookNetpanelPrivate *priv = MEEGO_NETBOOK_NETPANEL (actor)->priv;

  /* Chain up to get the background */
  CLUTTER_ACTOR_CLASS (meego_netbook_netpanel_parent_class)->paint (actor);

  if (priv->tabs_label)
    clutter_actor_paint (CLUTTER_ACTOR (priv->tabs_label));
  if (priv->tabs_view)
    clutter_actor_paint (CLUTTER_ACTOR (priv->tabs_view));
  if (priv->favs_label)
    clutter_actor_paint (CLUTTER_ACTOR (priv->favs_label));
  if (priv->favs_view)
    clutter_actor_paint (CLUTTER_ACTOR (priv->favs_view));

  /* Paint the entry last so automagic dropdown will be on top */
  clutter_actor_paint (CLUTTER_ACTOR (priv->entry_table));
}

static void
meego_netbook_netpanel_pick (ClutterActor *actor, const ClutterColor *color)
{
  meego_netbook_netpanel_paint (actor);
}

static void
meego_netbook_netpanel_map (ClutterActor *actor)
{
  MeegoNetbookNetpanelPrivate *priv = MEEGO_NETBOOK_NETPANEL (actor)->priv;

  CLUTTER_ACTOR_CLASS (meego_netbook_netpanel_parent_class)->map (actor);

  clutter_actor_map (CLUTTER_ACTOR (priv->entry_table));
  if (priv->tabs_label)
    clutter_actor_map (CLUTTER_ACTOR (priv->tabs_label));
  if (priv->tabs_view)
    clutter_actor_map (CLUTTER_ACTOR (priv->tabs_view));
  if (priv->favs_label)
    clutter_actor_map (CLUTTER_ACTOR (priv->favs_label));
  if (priv->favs_view)
    clutter_actor_map (CLUTTER_ACTOR (priv->favs_view));
}

static void
meego_netbook_netpanel_unmap (ClutterActor *actor)
{
  MeegoNetbookNetpanelPrivate *priv = MEEGO_NETBOOK_NETPANEL (actor)->priv;

  CLUTTER_ACTOR_CLASS (meego_netbook_netpanel_parent_class)->unmap (actor);

  clutter_actor_unmap (CLUTTER_ACTOR (priv->entry_table));
  if (priv->tabs_label)
    clutter_actor_unmap (CLUTTER_ACTOR (priv->tabs_label));
  if (priv->tabs_view)
    clutter_actor_unmap (CLUTTER_ACTOR (priv->tabs_view));
  if (priv->favs_label)
    clutter_actor_unmap (CLUTTER_ACTOR (priv->favs_label));
  if (priv->favs_view)
    clutter_actor_unmap (CLUTTER_ACTOR (priv->favs_view));
}

void
meego_netbook_netpanel_button_press (MeegoNetbookNetpanel *netpanel)
{
  if (!netpanel)
    return;

  MeegoNetbookNetpanelPrivate *priv = MEEGO_NETBOOK_NETPANEL (netpanel)->priv;
  if (priv->entry)
    mnb_netpanel_bar_button_press_cb(NULL, NULL,
                                     MNB_NETPANEL_BAR (priv->entry));
}

/*
 * TODO -- we might need dbus API for launching things to retain control over
 * the application workspace; investigate further.
 */

static void
meego_netbook_netpanel_launch_url (MeegoNetbookNetpanel *netpanel,
                                    const gchar         *url,
                                    gboolean            bool_exec)
{
  MeegoNetbookNetpanelPrivate *priv = MEEGO_NETBOOK_NETPANEL (netpanel)->priv;

  gchar *exec, *ptr, *remaining, *esc_url=NULL;
  gchar *prefix = g_strdup ("");

  if(!bool_exec)
    {
      if (strlen(url)>0 && !strstr (url, "."))
        {
          gchar *temp = g_strdup(priv->search_url);
          gchar *s1 = g_strstr_len(temp, -1, "{searchTerms}");
          if(s1)
            {
              gchar *s2 = &priv->search_url[(size_t)(s1-temp)+strlen("{searchTerms}")];
              *s1='\0';
              esc_url = g_strdup_printf("%s%s%s", temp, url, s2);
              if(strchr (esc_url, ' '))
                {
                  g_strdelimit(esc_url, " ", '+');
                }
            }
          g_free(temp);
        }

      if(!esc_url)
        {
          esc_url = g_strescape (url, NULL);
        }
      if(!g_str_has_prefix(esc_url, "http://") && !g_str_has_prefix(esc_url, "https://"))
        {
          gchar *tmp_url = g_strdup_printf("%s%s", "http://", esc_url);
          g_free(esc_url);
          esc_url = tmp_url;
        }
      if(meego_netbook_netpanel_open_tab(netpanel, CMD_NEW_TAB, (void*)esc_url))
        {
          g_free(esc_url);
          return;
        }
    }
  else
    {
      esc_url = g_strescape (url, NULL);
    }

  /* Change any % to %% to work around g_app_info_launch */
  remaining = esc_url;
  while ((ptr = strchr (remaining, '%')))
    {
      gchar *tmp = prefix;
      *ptr = '\0';
      prefix = g_strdup_printf ("%s%s%%%%", tmp, remaining);
      g_free (tmp);
      *ptr = '%';
      remaining = ptr + 1;
    }

  exec = g_strdup_printf ("gvfs-open \"%s%s\"",
                          prefix, remaining);

//   printf("exec %s\n", exec);

  g_free (prefix);

  if (priv->panel_client)
    {
      if (!mpl_panel_client_launch_application (priv->panel_client, exec))
        g_warning (G_STRLOC ": Error launching browser for url '%s'", esc_url);
      else
        mpl_panel_client_hide (priv->panel_client);
    }

  g_free (exec);
  g_free (esc_url);
}

static void
new_tab_clicked_cb (MxWidget *button, MeegoNetbookNetpanel *self)
{
  // -1 means open New Tab
  // FIXME: avoid hardcode here
  int id = -1;
  if (!meego_netbook_netpanel_open_tab (self, CMD_SELECT_TAB, &id))
    {
      meego_netbook_netpanel_restore_tab (self, NEWTAB_URL);
    }
}

static void
fav_button_clicked_cb (MxWidget *button, MeegoNetbookNetpanel *self)
{
  MeegoNetbookNetpanelPrivate *priv = self->priv;
  guint fav = GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (button), "fav"));

  meego_netbook_netpanel_launch_url (self, priv->fav_urls[fav], FALSE);
}

static void
pipe_sendv (GIOChannel *channel, va_list args)
{
  GType type;

  /* FIXME: Add error handling */
  /*g_debug ("Sending command: %d", command_id);*/

  if (!channel)
    {
      g_warning ("Trying to send with NULL channel");
      return;
    }

  while ((type = va_arg (args, GType)) != G_TYPE_INVALID)
    {
      gint int_val;
      glong long_val;
      gint64 int64_val;
      gdouble double_val;
      const gchar *buffer = NULL;
      gssize size = 0;

      switch (type)
        {
        case G_TYPE_UINT :
        case G_TYPE_INT :
        case G_TYPE_BOOLEAN :
          int_val = va_arg (args, gint);
          size = sizeof (gint);
          buffer = (const gchar *)(&int_val);
          break;

        case G_TYPE_ULONG :
        case G_TYPE_LONG :
          long_val = va_arg (args, glong);
          size = sizeof (glong);
          buffer = (const gchar *)(&long_val);
          break;

        case G_TYPE_UINT64 :
        case G_TYPE_INT64 :
          int64_val = va_arg (args, gint64);
          size = sizeof (gint64);
          buffer = (const gchar *)(&int64_val);
          break;

        case G_TYPE_DOUBLE :
          double_val = va_arg (args, gdouble);
          size = sizeof (gdouble);
          buffer = (const gchar *)(&double_val);
          break;

        case G_TYPE_STRING :
          buffer = (gchar*)va_arg (args, gpointer);
          if (buffer)
            size = strlen (buffer) + 1;
          else
            size = 0;
          g_io_channel_write_chars (channel,
                                    (gchar *)(&size),
                                    sizeof (size),
                                    NULL,
                                    NULL);
          break;

        case G_TYPE_NONE:
          size = va_arg (args, gsize);
          buffer = (const gchar *)va_arg (args, gpointer);
          break;

        default:
          g_warning ("Trying to send unknown type");
        }

      if (!buffer)
        {
          g_warning ("No data to send");
          continue;
        }

      if (size)
        g_io_channel_write_chars (channel,
                                  buffer,
                                  size,
                                  NULL,
                                  NULL);
    }

  g_io_channel_flush (channel, NULL);
}

static void
pipe_send (GIOChannel *channel, ...)
{
  va_list args;

  va_start (args, channel);

  pipe_sendv (channel, args);

  va_end (args);
}

static gboolean
meego_netbook_netpanel_open_tab (MeegoNetbookNetpanel *self, const gint type, void *data)
{
  MeegoNetbookNetpanelPrivate *priv = MEEGO_NETBOOK_NETPANEL (self)->priv;

  gchar *plugin_pipe = g_strdup_printf ("%s/chrome-meego-plugin.fifo",
                                        g_get_tmp_dir ());

  if (g_file_test (plugin_pipe, G_FILE_TEST_EXISTS))
    {
      GError *error = NULL;
      gint fd = open(plugin_pipe, O_WRONLY | O_NONBLOCK);
      // pipe file might be not closed properly
      // it will cause fd is -1. fallback to launch chrome.
      if (fd == -1)
        {
          g_free (plugin_pipe);
          return FALSE;
        }
      GIOChannel * output = g_io_channel_unix_new(fd);
      g_io_channel_set_encoding (output, NULL, NULL);
      g_io_channel_set_buffered (output, FALSE);
      g_io_channel_set_close_on_unref (output, TRUE);

      switch(type)
        {
          case CMD_SELECT_TAB:
            pipe_send (output, G_TYPE_UINT, CMD_SELECT_TAB, G_TYPE_INT, *((gint*)data), G_TYPE_INVALID);
            break;
          case CMD_NEW_TAB:
            pipe_send (output, G_TYPE_UINT, CMD_NEW_TAB, G_TYPE_STRING, (gchar*)data, G_TYPE_INVALID);
            break;
          default:
            g_io_channel_unref (output);
            g_free(plugin_pipe);
            return FALSE;
        }

      if (g_io_channel_shutdown (output, FALSE, &error) ==
          G_IO_STATUS_ERROR)
        {
          g_warning ("Error closing IO channel: %s", error->message);
          g_error_free (error);
        }

      g_io_channel_unref (output);
      output = NULL;
      mpl_panel_client_hide (priv->panel_client);
      g_free(plugin_pipe);
      return TRUE;
    }
  else
    {
      g_free(plugin_pipe);
      return FALSE;
    }
}

/*
static gboolean
meego_netbook_netpanel_select_tab (MeegoNetbookNetpanel *self, gint tab_id)
{
  MeegoNetbookNetpanelPrivate *priv = MEEGO_NETBOOK_NETPANEL (self)->priv;

  gchar *plugin_pipe = g_strdup_printf ("%s/chrome-meego-plugin.fifo",
                                        g_get_tmp_dir ());

  if (g_file_test (plugin_pipe, G_FILE_TEST_EXISTS))
    {
      GError *error = NULL;
      gint fd = open(plugin_pipe, O_WRONLY | O_NONBLOCK);
      // pipe file might be not closed properly
      // it will cause fd is -1. fallback to launch chrome.
      if (fd == -1)
        {
          g_free (plugin_pipe);
          return FALSE;
        }
      GIOChannel * output = g_io_channel_unix_new(fd);
      g_io_channel_set_encoding (output, NULL, NULL);
      g_io_channel_set_buffered (output, FALSE);
      g_io_channel_set_close_on_unref (output, TRUE);

      pipe_send (output, G_TYPE_UINT, CMD_SELECT_TAB, G_TYPE_INT, tab_id, G_TYPE_INVALID);

      if (g_io_channel_shutdown (output, FALSE, &error) ==
          G_IO_STATUS_ERROR)
        {
          g_warning ("Error closing IO channel: %s", error->message);
          g_error_free (error);
        }

      g_io_channel_unref (output);
      output = NULL;
      mpl_panel_client_hide (priv->panel_client);
      g_free(plugin_pipe);
      return TRUE;
    }
  else
    {
      g_free(plugin_pipe);
      return FALSE;
    }
}
*/

static void
meego_netbook_netpanel_restore_tab (MeegoNetbookNetpanel *self, gchar* tab_url)
{
  MeegoNetbookNetpanelPrivate *priv = MEEGO_NETBOOK_NETPANEL (self)->priv;

  gchar *plugin_cmd = g_strdup_printf ("%s/chrome-meego-extension.cmd",
                                        g_get_tmp_dir ());

  // Put the tab id to a startup command file
  // Chrome extension will executes the commands in this file
  // and deletes it.
  // FIXME: need define the protocol of startup command
  if (!g_file_test (plugin_cmd, G_FILE_TEST_EXISTS))
    {
      g_file_set_contents (plugin_cmd, (gchar *)tab_url, strlen(tab_url), NULL);
    }
  // Launch the Chrome to restore tabs and execute startup commands
  meego_netbook_netpanel_launch_url(self, "", TRUE);
  g_free (plugin_cmd);
}

static void
session_tab_button_clicked_cb (MxWidget *button, MeegoNetbookNetpanel *self)
{
  MeegoNetbookNetpanelPrivate *priv = MEEGO_NETBOOK_NETPANEL (self)->priv;

  gchar *tab_url = (gchar *)g_object_get_data (G_OBJECT (button), "url");
  guint tab_id = (guint)g_object_get_data (G_OBJECT (button), "tab_id");

  if (!meego_netbook_netpanel_open_tab (self, CMD_SELECT_TAB, (void*)&tab_id))
    {
      meego_netbook_netpanel_restore_tab (self, tab_url);
    }
}

static void
create_tabs_view (MeegoNetbookNetpanel *self)
{
  MeegoNetbookNetpanelPrivate *priv = self->priv;

  if (priv->tabs_view)
    clutter_actor_unparent (CLUTTER_ACTOR (priv->tabs_view));

  priv->tabs_view = mnb_netpanel_scrollview_new ();
  clutter_actor_set_parent (CLUTTER_ACTOR (priv->tabs_view),
                            CLUTTER_ACTOR (self));

  /* TODO: set column and row spacing properties */
  clutter_actor_set_name (CLUTTER_ACTOR (priv->tabs_view),
                          "netpanel-subtable");
  clutter_actor_show (CLUTTER_ACTOR (priv->tabs_view));
}

static void
create_favs_view (MeegoNetbookNetpanel *self)
{
  MeegoNetbookNetpanelPrivate *priv = self->priv;

  if (priv->favs_view)
    clutter_actor_unparent (CLUTTER_ACTOR (priv->favs_view));

  /* Construct favorites table */
  priv->favs_view = mnb_netpanel_scrollview_new ();
  clutter_actor_set_parent (CLUTTER_ACTOR (priv->favs_view),
                            CLUTTER_ACTOR (self));

  /* TODO: set column and row spacing properties */
  clutter_actor_set_name (CLUTTER_ACTOR (priv->favs_view),
                          "netpanel-subtable");
  clutter_actor_show (CLUTTER_ACTOR (priv->favs_view));
}

static void
create_favs_placeholder (MeegoNetbookNetpanel *self)
{
  MeegoNetbookNetpanelPrivate *priv = self->priv;
  MxWidget *label, *bin;

  if (priv->favs_view)
    clutter_actor_unparent (CLUTTER_ACTOR (priv->favs_view));

  priv->favs_view = bin = MX_WIDGET(mx_frame_new ());
  clutter_actor_set_name (CLUTTER_ACTOR (bin), "netpanel-placeholder-bin");

  label = MX_WIDGET(mx_label_new_with_text (_("As you visit web pages, your favorites will "
                                              "appear here and on the New tab page in the "
                                              "browser.")));
  clutter_actor_set_name (CLUTTER_ACTOR (label), "netpanel-placeholder-label");
  mx_bin_set_child (MX_BIN (bin), CLUTTER_ACTOR (label));
  mx_bin_set_alignment (MX_BIN (bin), MX_ALIGN_START, MX_ALIGN_MIDDLE);

  clutter_actor_set_parent (CLUTTER_ACTOR (bin), CLUTTER_ACTOR (self));
}

typedef struct _TextureData{
  ClutterActor *tex;
  ClutterActor *favi;
  gchar *url;
  gchar *ff;
}TextureData;

#define NETPANEL_DIR ".config/internet-panel"

static gchar *
get_favicon_filename(MeegoNetbookNetpanel *self, const char *url)
{
  MeegoNetbookNetpanelPrivate *priv = self->priv;
  gchar *result = NULL;

  gchar *id_stmt = g_strdup_printf("select favicon_id from urls where url='%s'", url);
  sqlite3_stmt *f1_stmt = NULL;
  int rc = sqlite3_prepare_v2 (priv->dbcon,
                           id_stmt,
                           -1,
                           &f1_stmt, NULL);
  int f_id = 0;
  if (!rc && sqlite3_step (f1_stmt) == SQLITE_ROW)
    {
      f_id = sqlite3_column_int(f1_stmt, 0);
    }
  sqlite3_finalize(f1_stmt);

  gchar *fv_stmt = g_strdup_printf("select url from favicons where id=%d", f_id);
  if( f_id!= 0 )
    {
      sqlite3_stmt *f2_stmt = NULL;
      rc = sqlite3_prepare_v2 (priv->dbcon,
                               fv_stmt,
                               -1,
                               &f2_stmt, NULL);
      if (!rc && sqlite3_step (f2_stmt) == SQLITE_ROW)
        {
          gchar *csum = g_compute_checksum_for_string (G_CHECKSUM_MD5,
                                (gchar*)sqlite3_column_text(f2_stmt, 0), -1);
          gchar *thumbnail_filename = g_strconcat (csum, ".ico", NULL);
          result = g_build_filename (g_get_home_dir (),
                                     NETPANEL_DIR,
                                     "favicons",
                                     thumbnail_filename,
                                     NULL);
          g_free(csum);
          g_free(thumbnail_filename);
        }
      sqlite3_finalize(f2_stmt);
    }

  g_free(id_stmt);
  g_free(fv_stmt);

  return result;
}

static gboolean
add_texture_to_scrollview(void*data)
{

  ClutterActor *tex = ((TextureData*)data)->tex;
  ClutterActor *favi = ((TextureData*)data)->favi;
  gchar *url = ((TextureData*)data)->url;
  gchar *ff = ((TextureData*)data)->ff;

  gchar *csum = g_compute_checksum_for_string (G_CHECKSUM_MD5, url, -1);
  gchar *thumbnail_filename = g_strconcat (csum, ".png", NULL);
  gchar *path = g_build_filename (g_get_home_dir (),
                                  NETPANEL_DIR,
                                  "thumbnails",
                                  thumbnail_filename,
                                  NULL);
  g_free(csum);
  g_free(thumbnail_filename);
  GError *error = NULL;

  if (path)
    {
      mx_image_set_from_file_at_size (MX_IMAGE (tex), path,
                                      CELL_WIDTH, CELL_HEIGHT,
                                      &error);
      g_free (path);
      if (error)
        {
          g_warning ("[netpanel] unable to open thumbnail: %s\n",
                     error->message);
          g_error_free (error);
        }
    }
  if (!path || error)
    {
      error = NULL;
      mx_image_set_from_file_at_size (MX_IMAGE (tex),
                                      THEMEDIR "/fallback-page.png",
                                      CELL_WIDTH, CELL_HEIGHT,
                                      &error);
      if (error)
        {
          g_warning ("[netpanel] unable to open fallback thumbnail: %s\n",
                     error->message);
          g_error_free (error);
        }
    }

  error = NULL;
  if(ff)
    {
      mx_image_set_from_file_at_size (MX_IMAGE (favi), ff,
                                      FAVI_SIZE, FAVI_SIZE,
                                      &error);
      if (error)
        {
          g_warning ("[netpanel] unable to open thumbnail: %s\n",
                     error->message);
          g_error_free (error);
        }
    }

  g_free(url);
  g_free(ff);
  g_free(data);
  return FALSE;
}

static MxWidget *
add_thumbnail_to_scrollview (MnbNetpanelScrollview *scrollview,
                             const gchar *url, const gchar *title,
                             const gchar *favicon_filename, const int priority)
{
  GError *error = NULL;
  ClutterActor *vbox, *hbox;
  ClutterActor *button, *tex;
  ClutterActor *label, *favi_tex;
  gchar *path;
  gboolean new_tab = FALSE;

  if (!title && !strcmp (url, START_PAGE))
    {
      title = _("New tab");
      new_tab = TRUE;
    }

#if 0
  if (!new_tab && !g_file_test (path, G_FILE_TEST_EXISTS))
    return NULL;
#endif

  vbox = mx_box_layout_new ();
  mx_box_layout_set_orientation (MX_BOX_LAYOUT (vbox),
                                 MX_ORIENTATION_VERTICAL);

  button = mx_button_new ();
  clutter_actor_set_name (button, "weblink");
  mx_stylable_set_style_class (MX_STYLABLE (button), "weblink");
  clutter_container_add_actor (CLUTTER_CONTAINER (vbox), button);

  tex = mx_image_new ();
  clutter_actor_set_size (tex, CELL_WIDTH, CELL_HEIGHT);
  clutter_container_add_actor (CLUTTER_CONTAINER (button), tex);

  hbox = mx_box_layout_new ();
  mx_box_layout_set_orientation (MX_BOX_LAYOUT (hbox),
                                 MX_ORIENTATION_HORIZONTAL);
  clutter_container_add_actor (CLUTTER_CONTAINER (vbox), hbox);

  favi_tex = mx_image_new ();
  clutter_actor_set_name (favi_tex, "favicon");
  // clutter_actor_set_size (favi_tex, FAVI_SIZE, FAVI_SIZE);
  clutter_container_add_actor (CLUTTER_CONTAINER (hbox), favi_tex);

  if (!title)
    title = url;
  label = mx_label_new_with_text (title);
  clutter_actor_set_name (label, "title");
  clutter_container_add_actor (CLUTTER_CONTAINER (hbox), label);

  mnb_netpanel_scrollview_add_item (scrollview, 0, vbox);

  TextureData *tex_data = (TextureData*) g_malloc0 (sizeof (TextureData));
  tex_data->tex = tex;
  tex_data->favi = favi_tex;
  tex_data->url = g_strdup (url);
  tex_data->ff = g_strdup (favicon_filename);
  clutter_threads_add_idle_full (priority, add_texture_to_scrollview,
                                 (void*) tex_data, NULL);

  return MX_WIDGET (button);
}

static void
favs_exception (void *context, int errno)
{
  MeegoNetbookNetpanel *self = (MeegoNetbookNetpanel*)context;
  MeegoNetbookNetpanelPrivate *priv = self->priv;

  create_favs_placeholder(self);
}

static void
favs_received (void *context, const char* url, const char *title, const int priority)
{
  MeegoNetbookNetpanel *self = (MeegoNetbookNetpanel*)context;
  MeegoNetbookNetpanelPrivate *priv = self->priv;

  MxWidget *button;
  MnbNetpanelScrollview *scrollview;

  scrollview = MNB_NETPANEL_SCROLLVIEW (priv->favs_view);

  // it is possible that when this callback is called
  // the scrollview is destroied by hide
  if (!scrollview)
    return;

  gchar *favicon_filename = get_favicon_filename(self, url);
  button = add_thumbnail_to_scrollview (scrollview, url, title, favicon_filename,  priority);
  g_free(favicon_filename);

  if (button)
    {
      priv->fav_urls[priv->n_favs] = g_strdup (url);
      priv->fav_titles[priv->n_favs] = g_strdup (title);

      g_object_set_data (G_OBJECT (button), "fav", GUINT_TO_POINTER (priv->n_favs));
      g_signal_connect (button, "clicked",
                        G_CALLBACK (fav_button_clicked_cb), self);
      priv->n_favs++;
    }
}

static void tabs_exception(void* context, int errno)
{
  MeegoNetbookNetpanel* self = (MeegoNetbookNetpanel*)context;
  MeegoNetbookNetpanelPrivate *priv = self->priv;
  MnbNetpanelScrollview *scrollview = MNB_NETPANEL_SCROLLVIEW (priv->tabs_view);

  ClutterActor *vbox, *hbox;
  ClutterActor *button, *tex;
  ClutterActor *label, *favi_tex;
  GError *error = NULL;

  vbox = mx_box_layout_new ();
  mx_box_layout_set_orientation (MX_BOX_LAYOUT (vbox),
                                 MX_ORIENTATION_VERTICAL);

  tex = mx_image_new ();
  mx_image_set_from_file_at_size (MX_IMAGE (tex),
                                  THEMEDIR "/newtab-thumbnail.png",
                                  CELL_WIDTH, CELL_HEIGHT,
                                  &error);
  if (error)
    {
      g_warning ("[netpanel] unable to open new tab thumbnail: %s\n",
                 error->message);
      g_error_free (error);
    }

  button = mx_button_new ();
  mx_stylable_set_style_class (MX_STYLABLE (button), "weblink");

  clutter_container_add_actor (CLUTTER_CONTAINER (button), tex);
  g_signal_connect (button, "clicked",
                    G_CALLBACK (new_tab_clicked_cb), self);
  clutter_container_add_actor (CLUTTER_CONTAINER (vbox), button);

  hbox = mx_box_layout_new ();
  mx_box_layout_set_orientation (MX_BOX_LAYOUT (hbox),
                                 MX_ORIENTATION_HORIZONTAL);
  clutter_container_add_actor (CLUTTER_CONTAINER (vbox), hbox);

  favi_tex = mx_image_new ();
  clutter_actor_set_name (favi_tex, "favicon");
  // clutter_actor_set_size (favi_tex, FAVI_SIZE, FAVI_SIZE);
  clutter_container_add_actor (CLUTTER_CONTAINER (hbox), favi_tex);

  label = mx_label_new_with_text (_("New tab"));
  clutter_actor_set_name (label, "title");
  clutter_container_add_actor (CLUTTER_CONTAINER (hbox), label);

  mnb_netpanel_scrollview_add_item (scrollview, 0, vbox);
}

static void tabs_received(void* context, int tab_id,
                                     const char* url,
                                     const char* title,
                                     const int priority)
{
    MeegoNetbookNetpanel* self = (MeegoNetbookNetpanel*)context;
    MeegoNetbookNetpanelPrivate *priv = self->priv;
    MnbNetpanelScrollview *scrollview = MNB_NETPANEL_SCROLLVIEW (priv->tabs_view);

    if (!scrollview)
      return;

    if (url)
      {
        MxWidget *button;

        if (!strcmp (url, "NULL") || (url[0] == '\0'))
          {
            url = g_strdup (START_PAGE);
          }

        gchar *prev_url = (gchar *)malloc(strlen(url) + 3 + 8);
        //sprintf(prev_url, "%s#%d,%d", url, tab_id, navigation_index);
        sprintf(prev_url, "%s", url);

        gchar *favicon_filename = get_favicon_filename(self, url);
        button = add_thumbnail_to_scrollview (scrollview, url, title, favicon_filename,  priority);
        g_free(favicon_filename);
        //free(prev_url);

        if (button)
        {
            g_object_set_data (G_OBJECT (button), "url", (char*)prev_url);
            g_object_set_data (G_OBJECT (button), "tab_id", (void*)tab_id);
            g_signal_connect (button, "clicked",
                              G_CALLBACK (session_tab_button_clicked_cb), self);
            priv->session_urls = g_list_prepend (priv->session_urls, (char*)prev_url);
        }
    }
}

static void
create_tabs(MeegoNetbookNetpanel *self)
{
  MeegoNetbookNetpanelPrivate *priv = self->priv;
  /* Create tabs table */
  create_tabs_view (self);

  if (!priv->favs_view)
    create_favs_placeholder (self);


  sqlite3_stmt *tab_stmt;
  int rc = sqlite3_prepare_v2 (priv->dbcon,
                           TAB_SQL,
                           -1,
                           &tab_stmt, NULL);
  if (rc)
    g_warning ("[netpanel] sqlite3_prepare_v2():tab_stmt %s",
               sqlite3_errmsg(priv->dbcon));

  int priority = G_PRIORITY_DEFAULT_IDLE - NR_FAVORITE - DISPLAY_TABS_MAX;
  if (priv->dbcon && tab_stmt && sqlite3_step (tab_stmt) == SQLITE_ROW)
    {
      do
        {
          priority = ++priority>(G_PRIORITY_DEFAULT_IDLE-NR_FAVORITE)?G_PRIORITY_DEFAULT_IDLE:priority;
          tabs_received (self,
                         sqlite3_column_int(tab_stmt, 0),
                         (gchar*)sqlite3_column_text(tab_stmt, 1),
                         (gchar*)sqlite3_column_text(tab_stmt, 2),
                         priority);
        }
      while (sqlite3_step (tab_stmt) == SQLITE_ROW);
    }
  else
    {
      tabs_exception (self, 0);
    }

  sqlite3_finalize(tab_stmt);
}

static void
create_history (MeegoNetbookNetpanel *self)
{
  MeegoNetbookNetpanelPrivate *priv = self->priv;
  gint rc, i;

  if (!priv->tabs_view)
    create_tabs_view (self);

  create_favs_view (self);

  if (priv->fav_urls)
    {
      for (i = 0; i < priv->n_favs; i++)
        g_free (priv->fav_urls[i]);
      g_free (priv->fav_urls);
      priv->fav_urls = NULL;
    }
  if (priv->fav_titles)
    {
      for (i = 0; i < priv->n_favs; i++)
        g_free (priv->fav_titles[i]);
      g_free (priv->fav_titles);
      priv->fav_titles = NULL;
    }

  priv->fav_urls = (gchar**)g_malloc0 (NR_FAVORITE_MAX * sizeof (gchar*));
  priv->fav_titles = (gchar**)g_malloc0 (NR_FAVORITE_MAX * sizeof (gchar*));
  priv->n_favs = 0;

  sqlite3_stmt *fav_stmt;
  rc = sqlite3_prepare_v2 (priv->dbcon,
                           FAVORITE_SQL,
                           -1,
                           &fav_stmt, NULL);
  if (rc)
    g_warning ("[netpanel] sqlite3_prepare_v2():fav_stmt %s",
               sqlite3_errmsg(priv->dbcon));

  int priority = G_PRIORITY_DEFAULT_IDLE - NR_FAVORITE + 1;
  if (priv->dbcon && fav_stmt && sqlite3_step (fav_stmt) == SQLITE_ROW)
    {
      do
        {
          if (priv->n_favs < NR_FAVORITE)
            {
              /*XXX: Duplicated computing */
              gchar* url = (gchar*)sqlite3_column_text(fav_stmt, 0);
              gchar *csum = g_compute_checksum_for_string (G_CHECKSUM_MD5, url, -1);
              gchar *thumbnail_filename = g_strconcat (csum, ".png", NULL);
              gchar *path = g_build_filename (g_get_home_dir (),
                                              NETPANEL_DIR,
                                              "thumbnails",
                                              thumbnail_filename,
                                              NULL);

              if(g_file_test(path, G_FILE_TEST_EXISTS))
                favs_received (self,
                               (gchar*) sqlite3_column_text (fav_stmt, 0),  // url
                               (gchar*) sqlite3_column_text (fav_stmt, 1),  // title
                               priority++);
              g_free(csum);
              g_free(thumbnail_filename);
              g_free(path);
            }
        }
      while (sqlite3_step (fav_stmt) == SQLITE_ROW);
    }
  else
    {
      create_favs_placeholder (self);
    }

  sqlite3_finalize(fav_stmt);
}

static void
request_live_previews (MeegoNetbookNetpanel *self)
{
  MeegoNetbookNetpanelPrivate *priv = self->priv;

  priv->display_tab = 0;
  priv->display_fav = 0;

  create_tabs(self);
  create_history (self);
}

static void
meego_netbook_netpanel_unload (ClutterActor *actor)
{
  MeegoNetbookNetpanel *netpanel = MEEGO_NETBOOK_NETPANEL (actor);
  MeegoNetbookNetpanelPrivate *priv = netpanel->priv;

  clutter_main_quit();
}

static void
meego_netbook_netpanel_set_search_provider(MeegoNetbookNetpanel *self)
{
  MeegoNetbookNetpanelPrivate *priv = self->priv;

  JsonNode *provider, *sname;
  gchar *search_name = NULL;
  GError *error = NULL;
  JsonParser *parser = json_parser_new ();

  gchar *filename = g_build_filename(g_get_home_dir(),
                                     ".config",
                                     "chromium",
                                     "Default",
                                     "Preferences",
                                      NULL);

  json_parser_load_from_file (parser, filename, &error);

  if (!error)
    {
      provider = json_object_get_member(
                      json_node_get_object(json_parser_get_root (parser)),
                      "default_search_provider");
      sname = json_object_get_member(json_node_get_object(provider),"name");
      search_name = g_strdup(json_node_get_string(sname));
      if(g_strcmp0(search_name, "Google")!=0)
        {
          priv->search_url = g_strdup(json_node_get_string(json_object_get_member(
                                      json_node_get_object(provider),"search_url")));
        }
      g_free(search_name);
    }

  if(error || !priv->search_url)
    {
      priv->search_url = g_strdup("http://www.google.com/search?q={searchTerms}");
      if(error)
          g_error_free (error);
    }
  g_object_unref (parser);
  g_free(filename);
}

static void
meego_netbook_netpanel_show (ClutterActor *actor)
{
  MeegoNetbookNetpanel *netpanel = MEEGO_NETBOOK_NETPANEL (actor);
  MeegoNetbookNetpanelPrivate *priv = netpanel->priv;

  if (!priv->places_db)
    priv->places_db = mwb_utils_places_db_get_filename ();

  mwb_utils_places_db_connect(priv->places_db, &priv->dbcon);

//   mwb_utils_db_stmt_prepare(priv->dbcon, &priv->fav_stmt,
//           &priv->tab_stmt, &priv->thumbnail_stmt);

  mnb_netpanel_bar_set_dbcon (G_OBJECT (priv->entry), priv->dbcon);

  meego_netbook_netpanel_focus (netpanel);

  request_live_previews (netpanel);

  meego_netbook_netpanel_set_search_provider(netpanel);

  CLUTTER_ACTOR_CLASS (meego_netbook_netpanel_parent_class)->show (actor);
}

static void
meego_netbook_netpanel_hide (ClutterActor *actor)
{
  MeegoNetbookNetpanel *netpanel = MEEGO_NETBOOK_NETPANEL (actor);
  MeegoNetbookNetpanelPrivate *priv = netpanel->priv;
  guint i;

  meego_netbook_netpanel_clear (netpanel);

  if (priv->tabs)
    {
      for (i = 0; i < priv->n_tabs; i++)
        if (priv->tabs[i])
          g_object_unref (priv->tabs[i]);
      g_free (priv->tabs);
      priv->tabs = NULL;
    }

  if (priv->tab_titles)
    {
      for (i = 0; i < priv->n_tabs; i++)
        g_object_unref (priv->tab_titles[i]);
      g_free (priv->tab_titles);
      priv->tab_titles = NULL;
    }

  priv->n_tabs = 0;

  if (priv->fav_urls)
    {
      for (i = 0; i < priv->n_favs; i++)
        g_free (priv->fav_urls[i]);
    }

  if (priv->fav_titles)
    {
      for (i = 0; i < priv->n_favs; i++)
        g_free (priv->fav_titles[i]);
    }

  priv->n_favs = 0;

  if (priv->search_url)
    {
      g_free (priv->search_url);
      priv->search_url = NULL;
    }

  /* Destroy tab table */
  if (priv->tabs_view)
    {
      clutter_actor_unparent (CLUTTER_ACTOR (priv->tabs_view));
      priv->tabs_view = NULL;
    }

  if (priv->favs_view)
    {
      clutter_actor_unparent (CLUTTER_ACTOR (priv->favs_view));
      priv->favs_view = NULL;
    }

  while (priv->session_urls)
    {
      g_free (priv->session_urls->data);
      priv->session_urls = g_list_delete_link (priv->session_urls,
                                               priv->session_urls);
    }

  mnb_netpanel_bar_clear_dbcon (G_OBJECT (priv->entry));

  mwb_utils_places_db_close (priv->dbcon);

  CLUTTER_ACTOR_CLASS (meego_netbook_netpanel_parent_class)->hide (actor);
}

static void
meego_netbook_netpanel_class_init (MeegoNetbookNetpanelClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  ClutterActorClass *actor_class = CLUTTER_ACTOR_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MeegoNetbookNetpanelPrivate));

  object_class->dispose = meego_netbook_netpanel_dispose;
  object_class->finalize = meego_netbook_netpanel_finalize;

  actor_class->allocate = meego_netbook_netpanel_allocate;
  actor_class->get_preferred_width =
    meego_netbook_netpanel_get_preferred_width;
  actor_class->get_preferred_height =
    meego_netbook_netpanel_get_preferred_height;
  actor_class->paint = meego_netbook_netpanel_paint;
  actor_class->pick = meego_netbook_netpanel_pick;
  actor_class->map = meego_netbook_netpanel_map;
  actor_class->unmap = meego_netbook_netpanel_unmap;
  actor_class->show = meego_netbook_netpanel_show;
  actor_class->hide = meego_netbook_netpanel_hide;
}

static void
netpanel_bar_go_cb (MnbNetpanelBar        *netpanel_bar,
                    const gchar           *url,
                    MeegoNetbookNetpanel *self)
{
  if (!url)
    return;

  while (*url == ' ')
    url++;

  if (*url == '\0')
    {
      mpl_entry_set_text (MPL_ENTRY (netpanel_bar), "");
      return;
    }

  meego_netbook_netpanel_launch_url (self, url, FALSE);
}

static void
netpanel_bar_button_clicked_cb (MnbNetpanelBar        *netpanel_bar,
                                MeegoNetbookNetpanel *self)
{
  netpanel_bar_go_cb (netpanel_bar,
                      mpl_entry_get_text (MPL_ENTRY (netpanel_bar)),
                      self);
}

static void
meego_netbook_netpanel_init (MeegoNetbookNetpanel *self)
{
  DBusGConnection *connection;
  MxWidget *table, *label;

  GError *error = NULL;
  MeegoNetbookNetpanelPrivate *priv = self->priv = NETPANEL_PRIVATE (self);

  /* Construct entry table */
  priv->entry_table = table = MX_WIDGET (mx_table_new ());
  mx_table_set_column_spacing (MX_TABLE (table), COL_SPACING);
  mx_table_set_row_spacing (MX_TABLE (table), ROW_SPACING);

  clutter_actor_set_name (CLUTTER_ACTOR (table), "netpanel-entrytable");
  clutter_actor_set_parent (CLUTTER_ACTOR (table), CLUTTER_ACTOR (self));

  /* Construct entry table widgets */

  label = MX_WIDGET(mx_label_new_with_text (_("Your pages")));
  clutter_actor_set_name (CLUTTER_ACTOR (label), "netpanel-label");
  mx_table_add_actor_with_properties (MX_TABLE (table),
                                        CLUTTER_ACTOR (label),
                                        0, 0,
                                        "x-expand", TRUE,
                                        "y-expand", FALSE,
                                        "x-fill", FALSE,
                                        "y-fill", FALSE,
                                        "x-align", MX_ALIGN_START,
                                        "y-align", MX_ALIGN_MIDDLE,
                                        NULL);

  priv->entry = mnb_netpanel_bar_new (_("Search"));

  clutter_actor_set_name (CLUTTER_ACTOR (priv->entry), "netpanel-entry");
  clutter_actor_set_width (CLUTTER_ACTOR (priv->entry), 500);
  mx_table_add_actor_with_properties (MX_TABLE (table),
                                        CLUTTER_ACTOR (priv->entry),
                                        0, 1,
                                        "x-expand", TRUE,
                                        "y-expand", FALSE,
                                        "x-fill", FALSE,
                                        "y-fill", FALSE,
                                        "x-align", MX_ALIGN_END,
                                        "y-align", MX_ALIGN_MIDDLE,
                                        NULL);

  g_signal_connect (priv->entry, "go",
                    G_CALLBACK (netpanel_bar_go_cb), self);
  g_signal_connect (priv->entry, "button-clicked",
                    G_CALLBACK (netpanel_bar_button_clicked_cb), self);

  /* Construct title for tab preview section */
  priv->tabs_label = label = MX_WIDGET (mx_label_new_with_text (_("Tabs")));
  clutter_actor_set_name (CLUTTER_ACTOR (label), "section");
  clutter_actor_set_parent (CLUTTER_ACTOR (label), CLUTTER_ACTOR (self));

  /* Construct title for favorite pages section */
  priv->favs_label = label = MX_WIDGET (mx_label_new_with_text (_("Favorite pages")));
  clutter_actor_set_name (CLUTTER_ACTOR (label), "section");
  clutter_actor_set_parent (CLUTTER_ACTOR (label), CLUTTER_ACTOR (self));

  priv->places_db = mwb_utils_places_db_get_filename();
  if (!priv->places_db)
    {
      g_warning ("[netpanel]: no places database found");
    }
  priv->dbcon = NULL;
//   priv->fav_stmt = NULL;
//   priv->tab_stmt = NULL;
//   priv->thumbnail_stmt = NULL;
}


MxWidget*
meego_netbook_netpanel_new (void)
{
  return (MxWidget*)g_object_new (MEEGO_TYPE_NETBOOK_NETPANEL,
                                  "visible", FALSE,
                                  NULL);
}

void
meego_netbook_netpanel_focus (MeegoNetbookNetpanel *netpanel)
{
  MeegoNetbookNetpanelPrivate *priv = netpanel->priv;
  mnb_netpanel_bar_focus (MNB_NETPANEL_BAR (priv->entry));
}

void
meego_netbook_netpanel_clear (MeegoNetbookNetpanel *netpanel)
{
  MeegoNetbookNetpanelPrivate *priv = netpanel->priv;
  mpl_entry_set_text (MPL_ENTRY (priv->entry), "");
}

void
meego_netbook_netpanel_set_panel_client (MeegoNetbookNetpanel *netpanel,
                                          MplPanelClient *panel_client)
{
  MeegoNetbookNetpanelPrivate *priv = MEEGO_NETBOOK_NETPANEL (netpanel)->priv;

  priv->panel_client = (MplPanelClient*)g_object_ref (panel_client);

  g_signal_connect_swapped (panel_client, "show-begin",
                            (GCallback)meego_netbook_netpanel_show, netpanel);

  g_signal_connect_swapped (panel_client, "hide-end",
                            (GCallback)meego_netbook_netpanel_hide, netpanel);

  g_signal_connect_swapped (panel_client, "unload",
                            (GCallback)meego_netbook_netpanel_unload, netpanel);
}
