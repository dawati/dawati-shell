/* moblin-netbook-netpanel.c */
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

extern "C" {
#include <moblin-panel/mpl-entry.h>
#include <moblin-panel/mpl-utils.h>
#include <moblin-panel/mpl-panel-client.h>
#include "moblin-netbook-netpanel.h"
#include "mnb-netpanel-bar.h"
#include "mnb-netpanel-scrollview.h"
#include "mwb-utils.h"
}
#include "chrome-profile-provider.h"
#include "base/message_loop.h"


/* Number of favorites columns to display */
#define NR_FAVORITE_MAX 16
#define NR_FAVORITE 9

/* FIXME: Replace with stylable spacing */
#define COL_SPACING 6
#define ROW_SPACING 6
#define CELL_WIDTH  223
#define CELL_HEIGHT 111

#define START_PAGE "moblin://start/"

G_DEFINE_TYPE (MoblinNetbookNetpanel, moblin_netbook_netpanel, MX_TYPE_WIDGET)

#define NETPANEL_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MOBLIN_TYPE_NETBOOK_NETPANEL, MoblinNetbookNetpanelPrivate))

struct _MoblinNetbookNetpanelPrivate
{
  DBusGProxy     *proxy;
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
};

static void
cancel_dbus_calls (MoblinNetbookNetpanel *self)
{
  MoblinNetbookNetpanelPrivate *priv = self->priv;

  /* Cancel any dbus calls */
  while (priv->calls)
    {
      DBusGProxyCall *call = (DBusGProxyCall*)priv->calls->data;
      dbus_g_proxy_cancel_call (priv->proxy, call);
      priv->calls = g_list_delete_link (priv->calls, priv->calls);
    }
}

static void
moblin_netbook_netpanel_dispose (GObject *object)
{
  MoblinNetbookNetpanel *self = MOBLIN_NETBOOK_NETPANEL (object);
  MoblinNetbookNetpanelPrivate *priv = self->priv;
  guint i;

  if (priv->panel_client)
    {
      g_object_unref (priv->panel_client);
      priv->panel_client = NULL;
    }

  if (priv->proxy)
    {
      cancel_dbus_calls (self);
      g_object_unref (priv->proxy);
      priv->proxy = NULL;
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

  while (priv->session_urls)
    {
      g_free (priv->session_urls->data);
      priv->session_urls = g_list_delete_link (priv->session_urls,
                                               priv->session_urls);
    }

  G_OBJECT_CLASS (moblin_netbook_netpanel_parent_class)->dispose (object);
}

static void
moblin_netbook_netpanel_finalize (GObject *object)
{
  G_OBJECT_CLASS (moblin_netbook_netpanel_parent_class)->finalize (object);
}

static void
moblin_netbook_netpanel_allocate (ClutterActor           *actor,
                                  const ClutterActorBox  *box,
                                  ClutterAllocationFlags  flags)
{
  ClutterActorBox child_box;
  MxPadding padding;
  gfloat width, height;
  gfloat min_heights[5], natural_heights[5], final_heights[5];
  gfloat natural_height;
  guint i;
  MoblinNetbookNetpanelPrivate *priv = MOBLIN_NETBOOK_NETPANEL (actor)->priv;

  CLUTTER_ACTOR_CLASS (moblin_netbook_netpanel_parent_class)->
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
moblin_netbook_netpanel_get_preferred_width (ClutterActor *self,
                                             gfloat        for_height,
                                             gfloat        *min_width_p,
                                             gfloat        *natural_width_p)
{
  MxPadding padding;
  gfloat min_width = 0.0, natural_width = 0.0;
  MoblinNetbookNetpanelPrivate *priv = MOBLIN_NETBOOK_NETPANEL (self)->priv;

  CLUTTER_ACTOR_CLASS (moblin_netbook_netpanel_parent_class)->
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
moblin_netbook_netpanel_get_preferred_height (ClutterActor *self,
                                              gfloat        for_width,
                                              gfloat       *min_height_p,
                                              gfloat       *natural_height_p)
{
  MxPadding padding;
  MoblinNetbookNetpanelPrivate *priv = MOBLIN_NETBOOK_NETPANEL (self)->priv;

  CLUTTER_ACTOR_CLASS (moblin_netbook_netpanel_parent_class)->
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
moblin_netbook_netpanel_paint (ClutterActor *actor)
{
  MoblinNetbookNetpanelPrivate *priv = MOBLIN_NETBOOK_NETPANEL (actor)->priv;

  /* Chain up to get the background */
  CLUTTER_ACTOR_CLASS (moblin_netbook_netpanel_parent_class)->paint (actor);

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
moblin_netbook_netpanel_pick (ClutterActor *actor, const ClutterColor *color)
{
  moblin_netbook_netpanel_paint (actor);
}

static void
moblin_netbook_netpanel_map (ClutterActor *actor)
{
  MoblinNetbookNetpanelPrivate *priv = MOBLIN_NETBOOK_NETPANEL (actor)->priv;

  CLUTTER_ACTOR_CLASS (moblin_netbook_netpanel_parent_class)->map (actor);

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
moblin_netbook_netpanel_unmap (ClutterActor *actor)
{
  MoblinNetbookNetpanelPrivate *priv = MOBLIN_NETBOOK_NETPANEL (actor)->priv;

  CLUTTER_ACTOR_CLASS (moblin_netbook_netpanel_parent_class)->unmap (actor);

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
moblin_netbook_netpanel_button_press (MoblinNetbookNetpanel *netpanel)
{
  if (!netpanel)
    return;

  MoblinNetbookNetpanelPrivate *priv = MOBLIN_NETBOOK_NETPANEL (netpanel)->priv;

  if (priv->entry)
    mnb_netpanel_bar_button_press_cb(NULL, NULL,
                                     MNB_NETPANEL_BAR (priv->entry));
}

/*
 * TODO -- we might need dbus API for launching things to retain control over
 * the application workspace; investigate further.
 */
extern gboolean google_chrome;

static void
moblin_netbook_netpanel_launch_url (MoblinNetbookNetpanel *netpanel,
                                    const gchar           *url,
                                    gboolean               user_initiated)
{
  MoblinNetbookNetpanelPrivate *priv = MOBLIN_NETBOOK_NETPANEL (netpanel)->priv;
  gchar *exec, *esc_url, *ptr, *remaining;
  gchar *prefix = g_strdup ("");

  /* FIXME: Should not be hard-coded? */
  esc_url = g_strescape (url, NULL);

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

  if (google_chrome)
     exec = g_strdup_printf ("%s %s \"%s%s\"", 
                            "google-chrome", 
                            "",
                            prefix, remaining);
  else
    exec = g_strdup_printf ("%s %s \"%s%s\"", 
                            "chromium-browser", 
                            "",
                            prefix, remaining);
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
new_tab_clicked_cb (MxWidget *button, MoblinNetbookNetpanel *self)
{
  /* FIXME: remove hardcoded start path */
  moblin_netbook_netpanel_launch_url (self, START_PAGE, FALSE);
}

static void
fav_button_clicked_cb (MxWidget *button, MoblinNetbookNetpanel *self)
{
  MoblinNetbookNetpanelPrivate *priv = self->priv;
  guint fav = GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (button), "fav"));

  moblin_netbook_netpanel_launch_url (self, priv->fav_urls[fav], TRUE);
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

void
pipe_send (GIOChannel *channel, ...)
{
  va_list args;

  va_start (args, channel);

  pipe_sendv (channel, args);

  va_end (args);
}

static void
session_tab_button_clicked_cb (MxWidget *button, MoblinNetbookNetpanel *self)
{
  MoblinNetbookNetpanelPrivate *priv = MOBLIN_NETBOOK_NETPANEL (self)->priv;

  gchar *url = (gchar *)g_object_get_data (G_OBJECT (button), "url");
  guint tab_id = (guint)g_object_get_data (G_OBJECT (button), "tab_id");

  //moblin_netbook_netpanel_launch_url (self, url, TRUE);
  gchar *plugin_pipe = g_strdup_printf ("%s/chrome-moblin-plugin.fifo",
                                         g_get_tmp_dir ());

  if (g_file_test (plugin_pipe, G_FILE_TEST_EXISTS))
    {
      GError *error = NULL;
      gint fd = open(plugin_pipe, O_WRONLY | O_NONBLOCK);
      GIOChannel * output = g_io_channel_unix_new(fd);
      g_io_channel_set_encoding (output, NULL, NULL);
      g_io_channel_set_buffered (output, FALSE);
      g_io_channel_set_close_on_unref (output, TRUE);

      pipe_send (output, G_TYPE_UINT, tab_id, G_TYPE_INVALID);
      
      if (g_io_channel_shutdown (output, FALSE, &error) ==
          G_IO_STATUS_ERROR)
        {
          g_warning ("Error closing IO channel: %s", error->message);
          g_error_free (error);
        }

      g_io_channel_unref (output);
      output = NULL;
      mpl_panel_client_hide (priv->panel_client);
    }
  else {
    moblin_netbook_netpanel_launch_url (self, url, TRUE);
  }

  g_free(plugin_pipe);
}

static void
create_tabs_view (MoblinNetbookNetpanel *self)
{
  MoblinNetbookNetpanelPrivate *priv = self->priv;

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
create_favs_view (MoblinNetbookNetpanel *self)
{
  MoblinNetbookNetpanelPrivate *priv = self->priv;

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
create_favs_placeholder (MoblinNetbookNetpanel *self)
{
  MoblinNetbookNetpanelPrivate *priv = self->priv;
  MxWidget *label, *bin;

  if (priv->favs_view)
    clutter_actor_unparent (CLUTTER_ACTOR (priv->favs_view));

  priv->favs_view = bin = MX_WIDGET(mx_frame_new ());
  clutter_actor_set_name (CLUTTER_ACTOR (bin), "netpanel-placeholder-bin");

  label = MX_WIDGET(mx_label_new (_("As you visit web pages, your favorites will "
                                    "appear here and on the New tab page in the "
                                    "browser.")));
  clutter_actor_set_name (CLUTTER_ACTOR (label), "netpanel-placeholder-label");
  mx_bin_set_child (MX_BIN (bin), CLUTTER_ACTOR (label));
  mx_bin_set_alignment (MX_BIN (bin), MX_ALIGN_START, MX_ALIGN_MIDDLE);

  clutter_actor_set_parent (CLUTTER_ACTOR (bin), CLUTTER_ACTOR (self));
}

static MxWidget *
add_thumbnail_to_scrollview (MnbNetpanelScrollview *scrollview,
                             const gchar *url, const gchar *title)
{
  ClutterActor *tex;
  GError *error = NULL;
  MxWidget *button, *label;
  gchar *path;
  gboolean new_tab = FALSE;

  if (!title && !strcmp (url, START_PAGE))
    {
      title = _("New tab");
      new_tab = TRUE;
    }

  path = mpl_utils_get_thumbnail_path (url);

#if 0
  if (!new_tab && !g_file_test (path, G_FILE_TEST_EXISTS))
    return NULL;
#endif

  button = MX_WIDGET(mx_button_new ());
  clutter_actor_set_name (CLUTTER_ACTOR (button), "weblink");
  mx_stylable_set_style_class ((MxStylable*)button, "weblink");

  if (!title)
    title = url;

  label = MX_WIDGET(mx_label_new (title));
  clutter_actor_set_width (CLUTTER_ACTOR (label), CELL_WIDTH);

  tex = clutter_texture_new ();

  if (path)
    {
      clutter_texture_set_from_file (CLUTTER_TEXTURE (tex), path, &error);
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
      clutter_texture_set_from_file (CLUTTER_TEXTURE (tex),
                                     THEMEDIR "/fallback-page.png", &error);
      if (error)
        {
          g_warning ("[netpanel] unable to open fallback thumbnail: %s\n",
                     error->message);
          g_error_free (error);
        }
    }

  clutter_actor_set_size (CLUTTER_ACTOR (tex), CELL_WIDTH, CELL_HEIGHT);
  clutter_container_add_actor (CLUTTER_CONTAINER (button), tex);

  mnb_netpanel_scrollview_add_item (scrollview, 0, CLUTTER_ACTOR (button),
                                    CLUTTER_ACTOR (label));

  return button;
}

static void
favs_received (void *context, const char* url, const char *title)
{
  MoblinNetbookNetpanel *self = (MoblinNetbookNetpanel*)context;
  MoblinNetbookNetpanelPrivate *priv = self->priv;

  MxWidget *button;
  MnbNetpanelScrollview *scrollview;

  scrollview = MNB_NETPANEL_SCROLLVIEW (priv->favs_view);
  button = add_thumbnail_to_scrollview (scrollview, url, title);

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

static void tabs_received(void* context, int tab_id,
                                     int navigation_index,
                                     const char* url,
                                     const char* title) 
{
    MoblinNetbookNetpanel* self = (MoblinNetbookNetpanel*)context;
    MoblinNetbookNetpanelPrivate *priv = self->priv;
    MnbNetpanelScrollview *scrollview = MNB_NETPANEL_SCROLLVIEW (priv->tabs_view);

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
        
        button = add_thumbnail_to_scrollview (scrollview, url, title);
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

static gboolean
load_session (MoblinNetbookNetpanel *self, MnbNetpanelScrollview *scrollview)
{
  char *session_file;
  int fd;
  gboolean opened_something = FALSE;
  GKeyFile *keys;
  char **groups;
  int i;
  MoblinNetbookNetpanelPrivate *priv = self->priv;

  session_file = g_build_filename (g_get_home_dir (), 
                                   ".mozilla/moblin-web-browser",
                                   "session", NULL);

  if (!g_file_test (session_file, G_FILE_TEST_EXISTS))
    {
      g_free (session_file);
      return FALSE;
    }

  fd = g_open (session_file, O_RDONLY);
  if (fd == -1)
    {
      g_warning ("Failed to open session file %s", session_file);
      g_free (session_file);
      return FALSE;
    }

  keys = g_key_file_new ();

  if (!g_key_file_load_from_file (keys, session_file, (GKeyFileFlags)0, NULL))
    {
      g_warning ("Failed to load keys from session file %s", session_file);
      g_free (session_file);
      g_key_file_free (keys);
      return FALSE;
    }

  groups = g_key_file_get_groups (keys, NULL);
  for (i = 0; groups[i]; i++)
    {
      gchar *url = g_key_file_get_string (keys, groups[i], "url", NULL);
      gchar *title = g_key_file_get_string (keys, groups[i], "title", NULL);

      if (url)
        {
          MxWidget *button;

          if (!strcmp (url, "NULL") || (url[0] == '\0'))
            {
              g_free (url);
              url = g_strdup (START_PAGE);
            }

          gchar *prev_url = g_strdup(url);
          gchar *delim = g_strrstr(prev_url, "###");
          if (delim)
              *delim = '\0';
          button = add_thumbnail_to_scrollview (scrollview, prev_url, title);
          g_free(prev_url);

          if (button)
            {
              g_object_set_data (G_OBJECT (button), "url", url);
              g_signal_connect (button, "clicked",
                                G_CALLBACK (session_tab_button_clicked_cb), self);
              priv->session_urls = g_list_prepend (priv->session_urls, url);
              opened_something = TRUE;
            }
        }

      g_free (title);
    }
  g_strfreev (groups);

  g_key_file_free (keys);
  g_free (session_file);

  close (fd);

  return opened_something;
}

static void
notify_get_ntabs (DBusGProxy     *proxy,
                  DBusGProxyCall *call_id,
                  void           *user_data)
{
  MoblinNetbookNetpanel *self = MOBLIN_NETBOOK_NETPANEL (user_data);
  MoblinNetbookNetpanelPrivate *priv = self->priv;
  MxWidget *label;

  /* Create tabs table */
  create_tabs_view (self);

  if (!priv->favs_view)
    create_favs_placeholder (self);

  priv->calls = g_list_remove (priv->calls, call_id);

  int tabs_count = 0;
  /* Call chrome sessions reader */
    {
      ChromeProfileProvider chrome_profile_provider;
      chrome_profile_provider.Initialize(PROVIDER_SESSION);

      chrome_profile_provider.GetSessions(self, tabs_received);
      tabs_count = chrome_profile_provider.ResultCount();
    }

  /* Add a new tab thumbnail */
  if (tabs_count == 0)
    {
      MxWidget *button;
      ClutterActor *tex;
      GError *error = NULL;

      tex = clutter_texture_new_from_file (THEMEDIR "/newtab-thumbnail.png",
                                           &error);
      if (error)
        {
          g_warning ("[netpanel] unable to open new tab thumbnail: %s\n",
                     error->message);
          g_error_free (error);
        }

      button = MX_WIDGET(mx_button_new ());
      mx_stylable_set_style_class ((MxStylable*)button, "weblink");

      clutter_container_add_actor (CLUTTER_CONTAINER (button),
                                   CLUTTER_ACTOR (tex));
      g_signal_connect (button, "clicked",
                        G_CALLBACK (new_tab_clicked_cb), self);

      label = MX_WIDGET(mx_label_new (_("New tab")));

      mnb_netpanel_scrollview_add_item (MNB_NETPANEL_SCROLLVIEW (priv->tabs_view),
                                        0,
                                        CLUTTER_ACTOR (button),
                                        CLUTTER_ACTOR (label));
    }
}

static void
create_history (MoblinNetbookNetpanel *self)
{
  MoblinNetbookNetpanelPrivate *priv = self->priv;
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

  /* Call chrome history service */
    {
      ChromeProfileProvider chrome_profile_provider;
      MessageLoopForUI main_message_loop;
      chrome_profile_provider.Initialize(PROVIDER_FAVORITE);

      chrome_profile_provider.GetFavoritePages(self, favs_received);
    }

  if (priv->n_favs == 0)
      create_favs_placeholder (self);
}

static void
request_live_previews (MoblinNetbookNetpanel *self)
{
  MoblinNetbookNetpanelPrivate *priv = self->priv;

  priv->display_tab = 0;
  priv->display_fav = 0;

  if (!priv->proxy)
    return;

  /* Get the number of tabs */
  priv->calls = g_list_prepend (priv->calls,
    dbus_g_proxy_begin_call (priv->proxy, "GetNTabs", notify_get_ntabs,
                             g_object_ref (self), g_object_unref,
                             G_TYPE_INVALID));
  create_history (self);
}

static void
moblin_netbook_netpanel_show (ClutterActor *actor)
{
  MoblinNetbookNetpanel *netpanel = MOBLIN_NETBOOK_NETPANEL (actor);
  MoblinNetbookNetpanelPrivate *priv = netpanel->priv;

  moblin_netbook_netpanel_focus (netpanel);

  request_live_previews (netpanel);

  CLUTTER_ACTOR_CLASS (moblin_netbook_netpanel_parent_class)->show (actor);
}

static void
moblin_netbook_netpanel_hide (ClutterActor *actor)
{
  MoblinNetbookNetpanel *netpanel = MOBLIN_NETBOOK_NETPANEL (actor);
  MoblinNetbookNetpanelPrivate *priv = netpanel->priv;
  guint i;

  moblin_netbook_netpanel_clear (netpanel);

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

  /* Destroy tab table */
  cancel_dbus_calls (netpanel);
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

  CLUTTER_ACTOR_CLASS (moblin_netbook_netpanel_parent_class)->hide (actor);
}

static void
moblin_netbook_netpanel_class_init (MoblinNetbookNetpanelClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  ClutterActorClass *actor_class = CLUTTER_ACTOR_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MoblinNetbookNetpanelPrivate));

  object_class->dispose = moblin_netbook_netpanel_dispose;
  object_class->finalize = moblin_netbook_netpanel_finalize;

  actor_class->allocate = moblin_netbook_netpanel_allocate;
  actor_class->get_preferred_width =
    moblin_netbook_netpanel_get_preferred_width;
  actor_class->get_preferred_height =
    moblin_netbook_netpanel_get_preferred_height;
  actor_class->paint = moblin_netbook_netpanel_paint;
  actor_class->pick = moblin_netbook_netpanel_pick;
  actor_class->map = moblin_netbook_netpanel_map;
  actor_class->unmap = moblin_netbook_netpanel_unmap;
  actor_class->show = moblin_netbook_netpanel_show;
  actor_class->hide = moblin_netbook_netpanel_hide;
}

static void
netpanel_bar_go_cb (MnbNetpanelBar        *netpanel_bar,
                    const gchar           *url,
                    MoblinNetbookNetpanel *self)
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

  moblin_netbook_netpanel_launch_url (self, url, TRUE);
}

static void
netpanel_bar_button_clicked_cb (MnbNetpanelBar        *netpanel_bar,
                                MoblinNetbookNetpanel *self)
{
  netpanel_bar_go_cb (netpanel_bar,
                      mpl_entry_get_text (MPL_ENTRY (netpanel_bar)),
                      self);
}

static void
moblin_netbook_netpanel_init (MoblinNetbookNetpanel *self)
{
  DBusGConnection *connection;
  MxWidget *table, *label;

  GError *error = NULL;
  MoblinNetbookNetpanelPrivate *priv = self->priv = NETPANEL_PRIVATE (self);

  /* Construct entry table */
  priv->entry_table = table = MX_WIDGET (mx_table_new ());
  mx_table_set_col_spacing (MX_TABLE (table), COL_SPACING);
  mx_table_set_row_spacing (MX_TABLE (table), ROW_SPACING);

  clutter_actor_set_name (CLUTTER_ACTOR (table), "netpanel-entrytable");
  clutter_actor_set_parent (CLUTTER_ACTOR (table), CLUTTER_ACTOR (self));

  /* Construct entry table widgets */

  label = MX_WIDGET(mx_label_new (_("Internet")));
  clutter_actor_set_name (CLUTTER_ACTOR (label), "netpanel-label");
  mx_table_add_actor_with_properties (MX_TABLE (table),
                                        CLUTTER_ACTOR (label),
                                        0, 0,
                                        "x-expand", FALSE,
                                        "y-expand", FALSE,
                                        "x-fill", FALSE,
                                        "y-fill", FALSE,
                                        "x-align", 0.0,
                                        "y-align", 0.5,
                                        NULL);

  priv->entry = mnb_netpanel_bar_new (_("Go"));

  clutter_actor_set_name (CLUTTER_ACTOR (priv->entry), "netpanel-entry");
  clutter_actor_set_width (CLUTTER_ACTOR (priv->entry), 600);
  mx_table_add_actor_with_properties (MX_TABLE (table),
                                        CLUTTER_ACTOR (priv->entry),
                                        0, 1,
                                        "x-expand", FALSE,
                                        "y-expand", FALSE,
                                        "x-fill", FALSE,
                                        "y-fill", FALSE,
                                        "x-align", 0.0,
                                        "y-align", 0.5,
                                        NULL);

  g_signal_connect (priv->entry, "go",
                    G_CALLBACK (netpanel_bar_go_cb), self);
  g_signal_connect (priv->entry, "button-clicked",
                    G_CALLBACK (netpanel_bar_button_clicked_cb), self);

  /* Construct title for tab preview section */
  priv->tabs_label = label = MX_WIDGET (mx_label_new (_("Tabs")));
  clutter_actor_set_name (CLUTTER_ACTOR (label), "section");
  clutter_actor_set_parent (CLUTTER_ACTOR (label), CLUTTER_ACTOR (self));

  /* Construct title for favorite pages section */
  priv->favs_label = label = MX_WIDGET (mx_label_new (_("Favorite pages")));
  clutter_actor_set_name (CLUTTER_ACTOR (label), "section");
  clutter_actor_set_parent (CLUTTER_ACTOR (label), CLUTTER_ACTOR (self));

  /* Connect to DBus */
  connection = dbus_g_bus_get (DBUS_BUS_SESSION, &error);
  if (!connection)
    {
      g_warning ("Failed to connect to session bus: %s", error->message);
      g_error_free (error);
    }
  else
    {
      priv->proxy = dbus_g_proxy_new_for_name (connection,
                                               "org.moblin.MoblinWebBrowser",
                                               "/org/moblin/MoblinWebBrowser",
                                               "org.moblin.MoblinWebBrowser");
    }
}

MxWidget*
moblin_netbook_netpanel_new (void)
{
  return (MxWidget*)g_object_new (MOBLIN_TYPE_NETBOOK_NETPANEL,
                                  "visible", FALSE,
                                  NULL);
}

void
moblin_netbook_netpanel_focus (MoblinNetbookNetpanel *netpanel)
{
  MoblinNetbookNetpanelPrivate *priv = netpanel->priv;
  mnb_netpanel_bar_focus (MNB_NETPANEL_BAR (priv->entry));
}

void
moblin_netbook_netpanel_clear (MoblinNetbookNetpanel *netpanel)
{
  MoblinNetbookNetpanelPrivate *priv = netpanel->priv;
  mpl_entry_set_text (MPL_ENTRY (priv->entry), "");
}

void
moblin_netbook_netpanel_set_panel_client (MoblinNetbookNetpanel *netpanel,
                                          MplPanelClient *panel_client)
{
  MoblinNetbookNetpanelPrivate *priv = MOBLIN_NETBOOK_NETPANEL (netpanel)->priv;

  priv->panel_client = (MplPanelClient*)g_object_ref (panel_client);

  g_signal_connect_swapped (panel_client, "show-begin",
                            (GCallback)moblin_netbook_netpanel_show, netpanel);

  g_signal_connect_swapped (panel_client, "hide-end",
                            (GCallback)moblin_netbook_netpanel_hide, netpanel);
}
