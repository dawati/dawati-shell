/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */

/*
 * Moblin Netbook
 * Copyright © 2009, Intel Corporation.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU Lesser General Public License,
 * version 2.1, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA
 *
 */

#include "mnb-switcher.h"
#include "moblin-netbook-ui.h"
#include "moblin-netbook.h"
#include <nbtk/nbtk-tooltip.h>

#define CHILD_DATA_KEY "MNB_SWITCHER_CHILD_DATA"
#define HOVER_TIMEOUT  800

static GQuark child_data_quark = 0;

/*
 * MnbSwitcherApp
 *
 * A NbtkWidget subclass represening a single thumb in the switcher.
 */
#define MNB_TYPE_SWITCHER_APP                 (mnb_switcher_app_get_type ())
#define MNB_SWITCHER_APP(obj)                 (G_TYPE_CHECK_INSTANCE_CAST ((obj), MNB_TYPE_SWITCHER_APP, MnbSwitcherApp))
#define MNB_IS_SWITCHER_APP(obj)              (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MNB_TYPE_SWITCHER_APP))
#define MNB_SWITCHER_APP_CLASS(klass)         (G_TYPE_CHECK_CLASS_CAST ((klass), MNB_TYPE_SWITCHER_APP, MnbSwitcherAppClass))
#define MNB_IS_SWITCHER_APP_CLASS(klass)      (G_TYPE_CHECK_CLASS_TYPE ((klass), MNB_TYPE_SWITCHER_APP))
#define MNB_SWITCHER_APP_GET_CLASS(obj)       (G_TYPE_INSTANCE_GET_CLASS ((obj), MNB_TYPE_SWITCHER_APP, MnbSwitcherAppClass))

typedef struct _MnbSwitcherApp               MnbSwitcherApp;
typedef struct _MnbSwitcherAppClass          MnbSwitcherAppClass;

struct _MnbSwitcherApp
{
  /*< private >*/
  NbtkWidget parent_instance;
};

struct _MnbSwitcherAppClass
{
  /*< private >*/
  NbtkWidgetClass parent_class;
};

GType mnb_switcher_app_get_type (void);

G_DEFINE_TYPE (MnbSwitcherApp, mnb_switcher_app, NBTK_TYPE_WIDGET)

static void
mnb_switcher_app_class_init (MnbSwitcherAppClass *klass)
{
}

static void
mnb_switcher_app_init (MnbSwitcherApp *self)
{
}

G_DEFINE_TYPE (MnbSwitcher, mnb_switcher, MNB_TYPE_DROP_DOWN)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MNB_TYPE_SWITCHER, MnbSwitcherPrivate))

struct _MnbSwitcherPrivate {
  MutterPlugin *plugin;
  NbtkWidget   *table;
  NbtkWidget   *new_workspace;
  NbtkWidget   *new_label;
  GList        *last_workspaces;

  ClutterActor *last_focused;

  gboolean      dnd_in_progress : 1;
};

struct input_data
{
  gint          index;
  MutterPlugin *plugin;
};

/*
 * Calback for clicks on a workspace in the switcher (switches to the
 * appropriate ws).
 */
static gboolean
workspace_input_cb (ClutterActor *clone, ClutterEvent *event, gpointer data)
{
  struct input_data *input_data = data;
  gint               indx       = input_data->index;
  MutterPlugin      *plugin     = input_data->plugin;
  MetaScreen        *screen     = mutter_plugin_get_screen (plugin);
  MetaWorkspace     *workspace;
  MoblinNetbookPluginPrivate *priv = MOBLIN_NETBOOK_PLUGIN (plugin)->priv;

  if (MNB_SWITCHER (priv->switcher)->priv->dnd_in_progress)
    return FALSE;

  workspace = meta_screen_get_workspace_by_index (screen, indx);

  if (!workspace)
    {
      g_warning ("No workspace specified, %s:%d\n", __FILE__, __LINE__);
      return FALSE;
    }

  clutter_actor_hide (priv->switcher);
  hide_panel (plugin);
  meta_workspace_activate (workspace, event->any.time);

  return FALSE;
}

struct child_data
{
  ClutterActor *self;
  MnbSwitcher  *switcher;
  MutterWindow *mw;
  guint         hover_timeout_id;
  ClutterActor *tooltip;
  guint         focus_id;
  guint         raised_id;
};

static struct child_data *
get_child_data (ClutterActor *child)
{
  struct child_data * child_data;


  child_data = g_object_get_qdata (G_OBJECT (child), child_data_quark);

  return child_data;
}

static struct child_data *
make_child_data (MnbSwitcher  *switcher,
		 MutterWindow *mw,
		 ClutterActor *actor,
		 const gchar  *text)
{
  struct child_data *child_data = g_new0 (struct child_data, 1);

  child_data->self = actor;
  child_data->switcher = switcher;
  child_data->mw = mw;
  child_data->tooltip = g_object_new (NBTK_TYPE_TOOLTIP,
                                      "widget", actor,
                                      "label", text,
                                      NULL);

  return child_data;
}

static void
free_child_data (struct child_data *child_data)
{
  MetaWindow *meta_win;

  g_object_set_qdata (G_OBJECT (child_data->self),
		      child_data_quark, NULL);

  meta_win = mutter_window_get_meta_window (child_data->mw);

  if (child_data->hover_timeout_id)
    g_source_remove (child_data->hover_timeout_id);

  if (child_data->focus_id)
    g_signal_handler_disconnect (meta_win, child_data->focus_id);

  if (child_data->raised_id)
    g_signal_handler_disconnect (meta_win, child_data->raised_id);

  /*
   * Do not destroy the tooltip, this is happens automatically.
   */

  g_free (child_data);
}

static gboolean
workspace_switcher_clone_input_cb (ClutterActor *clone,
                                   ClutterEvent *event,
                                   gpointer      data)
{
  struct child_data          *child_data = data;
  MutterWindow               *mw = child_data->mw;
  MetaWindow                 *window;
  MetaWorkspace              *workspace;
  MetaWorkspace              *active_workspace;
  MetaScreen                 *screen;
  MnbSwitcher                *switcher = child_data->switcher;
  MutterPlugin               *plugin = switcher->priv->plugin;
  MoblinNetbookPluginPrivate *priv = MOBLIN_NETBOOK_PLUGIN (plugin)->priv;

  if (MNB_SWITCHER (priv->switcher)->priv->dnd_in_progress)
    return FALSE;

  window           = mutter_window_get_meta_window (mw);
  screen           = meta_window_get_screen (window);
  workspace        = meta_window_get_workspace (window);
  active_workspace = meta_screen_get_active_workspace (screen);

  clutter_actor_hide (CLUTTER_ACTOR (switcher));
  hide_panel (plugin);

  if (!active_workspace || (active_workspace == workspace))
    {
      meta_window_activate_with_workspace (window, event->any.time, workspace);
    }
  else
    {
      meta_workspace_activate_with_focus (workspace, window, event->any.time);
    }

  return FALSE;
}

static void
dnd_begin_cb (NbtkWidget   *table,
	      ClutterActor *dragged,
	      ClutterActor *icon,
	      gint          x,
	      gint          y,
	      gpointer      data)
{
  MnbSwitcherPrivate *priv = MNB_SWITCHER (data)->priv;
  struct child_data  *child_data = get_child_data (dragged);

  priv->dnd_in_progress = TRUE;

  if (child_data->hover_timeout_id)
    {
      g_source_remove (child_data->hover_timeout_id);
      child_data->hover_timeout_id = 0;
    }

  if (CLUTTER_ACTOR_IS_VISIBLE (child_data->tooltip))
    nbtk_tooltip_hide (NBTK_TOOLTIP (child_data->tooltip));

  clutter_actor_set_rotation (icon, CLUTTER_Y_AXIS, 60.0, 0, 0, 0);
  clutter_actor_set_opacity (dragged, 0x4f);
}

static void
dnd_end_cb (NbtkWidget   *table,
	    ClutterActor *dragged,
	    ClutterActor *icon,
	    gint          x,
	    gint          y,
	    gpointer      data)
{
  MnbSwitcherPrivate *priv = MNB_SWITCHER (data)->priv;

  priv->dnd_in_progress = FALSE;

  clutter_actor_set_rotation (icon, CLUTTER_Y_AXIS, 0.0, 0, 0, 0);
  clutter_actor_set_opacity (dragged, 0xff);
}

static void
dnd_dropped_cb (NbtkWidget   *table,
		ClutterActor *dragged,
		ClutterActor *icon,
		gint          x,
		gint          y,
		gpointer      data)
{
  ClutterChildMeta *meta;
  ClutterActor     *parent;
  ClutterActor     *table_actor = CLUTTER_ACTOR (table);
  MutterWindow     *mcw;
  MetaWindow       *mw;
  gint              col;

  if (!(mcw = get_child_data (dragged)->mw) ||
      !(mw = mutter_window_get_meta_window (mcw)))
    {
      g_warning ("No MutterWindow associated with this item.");
      return;
    }

  parent = clutter_actor_get_parent (table_actor);

  g_assert (NBTK_IS_TABLE (parent));

  meta = clutter_container_get_child_meta (CLUTTER_CONTAINER (parent),
					   table_actor);

  g_object_get (meta, "column", &col, NULL);

  /*
   * TODO -- perhaps we should expose the timestamp from the pointer event,
   * or event the entire Clutter event.
   */
  meta_window_change_workspace_by_index (mw, col, TRUE, CurrentTime);
}

static NbtkTable *
mnb_switcher_append_workspace (MnbSwitcher *switcher);

static void
dnd_new_dropped_cb (NbtkWidget   *table,
                    ClutterActor *dragged,
                    ClutterActor *icon,
                    gint          x,
                    gint          y,
                    gpointer      data)
{
  MnbSwitcher      *switcher = MNB_SWITCHER (data);
  ClutterChildMeta *meta, *d_meta;
  ClutterActor     *parent;
  ClutterActor     *table_actor = CLUTTER_ACTOR (table);
  MutterWindow     *mcw;
  MetaWindow       *mw;
  gint              col;
  NbtkTable        *new_ws;
  gboolean          keep_ratio = FALSE;

  if (!(mcw = get_child_data (dragged)->mw) ||
      !(mw = mutter_window_get_meta_window (mcw)))
    {
      g_warning ("No MutterWindow associated with this item.");
      return;
    }

  parent = clutter_actor_get_parent (table_actor);

  g_assert (NBTK_IS_TABLE (parent));

  meta = clutter_container_get_child_meta (CLUTTER_CONTAINER (parent),
					   table_actor);
  d_meta = clutter_container_get_child_meta (CLUTTER_CONTAINER (table),
                                             dragged);

  g_object_get (meta, "column", &col, NULL);
  g_object_get (d_meta, "keep-aspect-ratio", &keep_ratio, NULL);

  new_ws = mnb_switcher_append_workspace (switcher);

  g_object_ref (dragged);
  clutter_container_remove_actor (CLUTTER_CONTAINER (table), dragged);
  nbtk_table_add_actor (new_ws, dragged, 1, 0);

  clutter_container_child_set (CLUTTER_CONTAINER (new_ws), dragged,
			       "keep-aspect-ratio", keep_ratio, NULL);

  g_object_unref (dragged);

  /*
   * TODO -- perhaps we should expose the timestamp from the pointer event,
   * or event the entire Clutter event.
   */
  meta_window_change_workspace_by_index (mw, col, TRUE, CurrentTime);
}

static gboolean
clone_hover_timeout_cb (gpointer data)
{
  struct child_data  *child_data = data;
  MnbSwitcherPrivate *priv       = child_data->switcher->priv;

  if (!priv->dnd_in_progress)
    nbtk_tooltip_show (NBTK_TOOLTIP (child_data->tooltip));

  child_data->hover_timeout_id = 0;

  return FALSE;
}

static gboolean
clone_enter_event_cb (ClutterActor *actor,
		      ClutterCrossingEvent *event,
		      gpointer data)
{
  struct child_data  *child_data = data;
  MnbSwitcherPrivate *priv       = child_data->switcher->priv;

  if (!priv->dnd_in_progress)
    child_data->hover_timeout_id = g_timeout_add (HOVER_TIMEOUT,
						  clone_hover_timeout_cb,
						  data);

  return FALSE;
}

static gboolean
clone_leave_event_cb (ClutterActor *actor,
		      ClutterCrossingEvent *event,
		      gpointer data)
{
  struct child_data *child_data = data;

  if (child_data->hover_timeout_id)
    {
      g_source_remove (child_data->hover_timeout_id);
      child_data->hover_timeout_id = 0;
    }

  if (CLUTTER_ACTOR_IS_VISIBLE (child_data->tooltip))
    nbtk_tooltip_hide (NBTK_TOOLTIP (child_data->tooltip));

  return FALSE;
}

static NbtkWidget *
make_workspace_content (MnbSwitcher *switcher, gboolean active, gint col)
{
  MnbSwitcherPrivate *priv = switcher->priv;
  NbtkWidget         *table = priv->table;
  NbtkWidget         *new_ws;
  struct input_data  *input_data = g_new (struct input_data, 1);
  NbtkPadding         padding = MNB_PADDING (6, 6, 6, 6);

  input_data = g_new (struct input_data, 1);
  input_data->index = col;
  input_data->plugin = priv->plugin;

  new_ws = nbtk_table_new ();
  nbtk_table_set_row_spacing (NBTK_TABLE (new_ws), 6);
  nbtk_table_set_col_spacing (NBTK_TABLE (new_ws), 6);
  nbtk_widget_set_padding (new_ws, &padding);
  nbtk_widget_set_style_class_name (NBTK_WIDGET (new_ws),
                                    "switcher-workspace");

  if (active)
    clutter_actor_set_name (CLUTTER_ACTOR (new_ws),
                            "switcher-workspace-active");

  nbtk_widget_set_dnd_threshold (new_ws, 5);

  g_signal_connect (new_ws, "dnd-begin",
                    G_CALLBACK (dnd_begin_cb), switcher);

  g_signal_connect (new_ws, "dnd-end",
                    G_CALLBACK (dnd_end_cb), switcher);

  g_signal_connect (new_ws, "dnd-dropped",
                    G_CALLBACK (dnd_dropped_cb), switcher);

  nbtk_table_add_widget (NBTK_TABLE (table), new_ws, 1, col);

  /* switch workspace when the workspace is selected */
  g_signal_connect_data (new_ws, "button-press-event",
                         G_CALLBACK (workspace_input_cb), input_data,
                         (GClosureNotify)g_free, 0);

  return new_ws;
}

static NbtkWidget *
make_workspace_label (MnbSwitcher *switcher, gboolean active, gint col)
{
  MnbSwitcherPrivate *priv = switcher->priv;
  NbtkWidget         *table = priv->table;
  NbtkWidget         *ws_label;
  gchar              *s;
  struct input_data  *input_data = g_new (struct input_data, 1);

  input_data->index = col;
  input_data->plugin = priv->plugin;

  s = g_strdup_printf ("%d", col + 1);

  ws_label = nbtk_label_new (s);

  if (active)
    clutter_actor_set_name (CLUTTER_ACTOR (ws_label), "workspace-title-active");

  nbtk_widget_set_style_class_name (ws_label, "workspace-title");

  clutter_actor_set_reactive (CLUTTER_ACTOR (ws_label), TRUE);

  g_signal_connect_data (ws_label, "button-press-event",
                         G_CALLBACK (workspace_input_cb), input_data,
                         (GClosureNotify) g_free, 0);

  nbtk_table_add_widget (NBTK_TABLE (table), ws_label, 0, col);
  clutter_container_child_set (CLUTTER_CONTAINER (table),
                               CLUTTER_ACTOR (ws_label),
                               "y-expand", FALSE, NULL);

  return ws_label;
}

struct ws_remove_data
{
  MnbSwitcher *switcher;
  gint         col;
  GList       *remove;
};

static void
table_foreach_remove_ws (ClutterActor *child, gpointer data)
{
  struct ws_remove_data *remove_data = data;
  MnbSwitcher           *switcher    = remove_data->switcher;
  NbtkWidget            *table       = switcher->priv->table;
  ClutterChildMeta      *meta;
  gint                   row, col;

  meta = clutter_container_get_child_meta (CLUTTER_CONTAINER (table), child);

  g_assert (meta);
  g_object_get (meta, "row", &row, "column", &col, NULL);

  /*
   * Children below the column we are removing are unaffected.
   */
  if (col < remove_data->col)
    return;

  /*
   * We cannot remove the actors in the foreach function, as that potentially
   * affects a list in which the container holds the data (e.g., NbtkTable).
   * We schedule it for removal, and then remove all once we are finished with
   * the foreach.
   */
  if (col == remove_data->col)
    {
      remove_data->remove = g_list_prepend (remove_data->remove, child);
    }
  else
    {
      /*
       * For some reason changing the colum clears the y-expand property :-(
       * Need to preserve it on the first row.
       */
      if (!row)
        clutter_container_child_set (CLUTTER_CONTAINER (table), child,
                                     "column", col - 1,
                                     "y-expand", FALSE, NULL);
      else
        clutter_container_child_set (CLUTTER_CONTAINER (table), child,
                                     "column", col - 1, NULL);
    }
}

static void
screen_n_workspaces_notify (MetaScreen *screen,
                            GParamSpec *pspec,
                            gpointer    data)
{
  MnbSwitcher *switcher = MNB_SWITCHER (data);
  gint   n_c_workspaces = meta_screen_get_n_workspaces (screen);
  GList *c_workspaces = meta_screen_get_workspaces (screen);
  GList *o_workspaces = switcher->priv->last_workspaces;
  gint   n_o_workspaces = g_list_length (o_workspaces);
  gboolean *map;
  gint i;
  GList *k;
  struct ws_remove_data remove_data;

  if (n_o_workspaces < n_c_workspaces)
    {
      g_warning ("Adding workspaces into running switcher is currently not "
                 "supported.");

      g_list_free (switcher->priv->last_workspaces);
      switcher->priv->last_workspaces = g_list_copy (c_workspaces);
      return;
    }

  remove_data.switcher = switcher;
  remove_data.remove = NULL;

  map = g_slice_alloc0 (sizeof (gboolean) * n_o_workspaces);

  k = c_workspaces;

  while (k)
    {
      MetaWorkspace *w = k->data;
      GList         *l = o_workspaces;

      i = 0;

      while (l)
        {
          MetaWorkspace *w2 = l->data;

          if (w == w2)
            {
              map[i] = TRUE;
              break;
            }

          ++i;
          l = l->next;
        }

      k = k->next;
    }

  for (i = 0; i < n_o_workspaces; ++i)
    {
      if (!map[i])
        {
          GList *l;
          ClutterContainer *table = CLUTTER_CONTAINER (switcher->priv->table);

          remove_data.col = i;
          clutter_container_foreach (table,
                                     (ClutterCallback) table_foreach_remove_ws,
                                     &remove_data);

          l = remove_data.remove;
          while (l)
            {
              ClutterActor *child = l->data;

              clutter_container_remove_actor (table, child);

              l = l->next;
            }

          g_list_free (remove_data.remove);
        }
    }

  g_list_free (switcher->priv->last_workspaces);
  switcher->priv->last_workspaces = g_list_copy (c_workspaces);
}

static void
dnd_new_enter_cb (NbtkWidget   *table,
                  ClutterActor *dragged,
                  ClutterActor *icon,
                  gint          x,
                  gint          y,
                  gpointer      data)
{
  MnbSwitcherPrivate *priv = MNB_SWITCHER (data)->priv;

  clutter_actor_set_name (CLUTTER_ACTOR (priv->new_workspace),
                          "switcher-workspace-new-active");
  clutter_actor_set_name (CLUTTER_ACTOR (priv->new_label),
                          "switcher-workspace-new-active");
}

static void
dnd_new_leave_cb (NbtkWidget   *table,
                  ClutterActor *dragged,
                  ClutterActor *icon,
                  gint          x,
                  gint          y,
                  gpointer      data)
{
  MnbSwitcherPrivate *priv = MNB_SWITCHER (data)->priv;

  clutter_actor_set_name (CLUTTER_ACTOR (priv->new_workspace), "");
  clutter_actor_set_name (CLUTTER_ACTOR (priv->new_label), "");
}

static void
meta_window_focus_cb (MetaWindow *mw, gpointer data)
{
  struct child_data  *child_data = data;
  MnbSwitcher        *switcher = child_data->switcher;
  MnbSwitcherPrivate *priv = switcher->priv;

  if (priv->last_focused == child_data->self)
    return;

  if (priv->last_focused)
    clutter_actor_set_name (priv->last_focused, "");

  clutter_actor_set_name (child_data->self, "switcher-application-active");
  priv->last_focused = child_data->self;
}

static void
mnb_switcher_show (ClutterActor *self)
{
  MnbSwitcherPrivate *priv = MNB_SWITCHER (self)->priv;
  MetaScreen   *screen = mutter_plugin_get_screen (priv->plugin);
  gint          ws_count, active_ws, ws_max_windows = 0;
  gint         *n_windows;
  gint          i, screen_width;
  NbtkWidget   *table;
  GList        *window_list, *l;
  NbtkWidget  **spaces;
  NbtkPadding   padding = MNB_PADDING (6, 6, 6, 6);
  GList        *workspaces = meta_screen_get_workspaces (screen);

  priv->last_workspaces = g_list_copy (workspaces);

  /* create the contents */

  table = nbtk_table_new ();
  priv->table = table;
  nbtk_table_set_row_spacing (NBTK_TABLE (table), 4);
  nbtk_table_set_col_spacing (NBTK_TABLE (table), 7);
  nbtk_widget_set_padding (table, &padding);

  clutter_actor_set_name (CLUTTER_ACTOR (table), "switcher-table");

  ws_count = meta_screen_get_n_workspaces (screen);
  active_ws = meta_screen_get_active_workspace_index (screen);

  mutter_plugin_query_screen_size (priv->plugin, &screen_width, &i);

  /* loop through all the workspaces, adding a label for each */
  for (i = 0; i < ws_count; i++)
    {
      gboolean active = FALSE;

      if (i == active_ws)
        active = TRUE;

      make_workspace_label (MNB_SWITCHER (self), active, i);
    }

  /* iterate through the windows, adding them to the correct workspace */

  n_windows = g_new0 (gint, ws_count);
  spaces = g_new0 (NbtkWidget*, ws_count);
  window_list = mutter_plugin_get_windows (priv->plugin);
  for (l = window_list; l; l = g_list_next (l))
    {
      MutterWindow       *mw = l->data;
      ClutterActor       *texture, *c_tx, *clone;
      gint                ws_indx;
      MetaCompWindowType  type;
      gint                w, h;
      struct child_data  *child_data;
      MetaWindow         *meta_win = mutter_window_get_meta_window (mw);
      gchar              *title;

      ws_indx = mutter_window_get_workspace (mw);
      type = mutter_window_get_window_type (mw);
      /*
       * Only show regular windows that are not sticky (getting stacking order
       * right for sticky windows would be really hard, and since they appear
       * on each workspace, they do not help in identifying which workspace
       * it is).
       */
      if (ws_indx < 0                             ||
          mutter_window_is_override_redirect (mw) ||
          type != META_COMP_WINDOW_NORMAL)
        {
          continue;
        }

      /* create the table for this workspace if we don't already have one */
      if (!spaces[ws_indx])
        {
          gboolean active = FALSE;

          if (ws_indx == active_ws)
            active = TRUE;

          spaces[ws_indx] =
            make_workspace_content (MNB_SWITCHER (self), active, ws_indx);
        }

      texture = mutter_window_get_texture (mw);
      c_tx    = clutter_clone_texture_new (CLUTTER_TEXTURE (texture));
      clone   = g_object_new (MNB_TYPE_SWITCHER_APP, NULL);
      nbtk_widget_set_style_class_name (NBTK_WIDGET (clone),
                                        "switcher-application");

      if (meta_window_has_focus (meta_win))
          clutter_actor_set_name (clone, "switcher-application-active");

      priv->last_focused = clone;

      clutter_container_add_actor (CLUTTER_CONTAINER (clone), c_tx);

      clutter_actor_set_reactive (clone, TRUE);

      g_object_weak_ref (G_OBJECT (mw), switcher_origin_weak_notify, clone);
      g_object_weak_ref (G_OBJECT (clone), switcher_clone_weak_notify, mw);

      g_object_get (meta_win, "title", &title, NULL);
      child_data = make_child_data (MNB_SWITCHER (self), mw, clone, title);
      g_free (title);

      g_object_set_qdata (G_OBJECT (clone), child_data_quark, child_data);

      g_signal_connect_data (clone, "enter-event",
			     G_CALLBACK (clone_enter_event_cb), child_data,
			     (GClosureNotify)free_child_data, 0);
      g_signal_connect (clone, "leave-event",
                        G_CALLBACK (clone_leave_event_cb), child_data);

      child_data->focus_id =
        g_signal_connect (meta_win, "focus",
                          G_CALLBACK (meta_window_focus_cb), child_data);
      child_data->raised_id =
        g_signal_connect (meta_win, "raised",
                          G_CALLBACK (meta_window_focus_cb), child_data);

      n_windows[ws_indx]++;
      nbtk_table_add_actor (NBTK_TABLE (spaces[ws_indx]), clone,
                            n_windows[ws_indx], 0);
      clutter_container_child_set (CLUTTER_CONTAINER (spaces[ws_indx]), clone,
                                   "keep-aspect-ratio", TRUE, NULL);

      clutter_actor_get_size (clone, &h, &w);
      clutter_actor_set_size (clone, h/(gdouble)w * 80.0, 80);

      g_signal_connect (clone, "button-release-event",
                        G_CALLBACK (workspace_switcher_clone_input_cb),
			child_data);

      ws_max_windows = MAX (ws_max_windows, n_windows[ws_indx]);
    }

  /* add an "empty" message for empty workspaces */
  for (i = 0; i < ws_count; i++)
    {
      if (!spaces[i])
        {
          NbtkWidget *label;

          label = nbtk_label_new ("No applications on this space");

          nbtk_table_add_widget (NBTK_TABLE (table), label, 1, i);
        }
    }

  /*
   * Now create the new workspace column.
   */
  {
    NbtkWidget *new_ws = nbtk_table_new ();
    NbtkWidget *label;
    ClutterChildMeta *child;

    label = nbtk_label_new ("");
    nbtk_table_add_widget (NBTK_TABLE (table), label, 0, ws_count);
    nbtk_widget_set_style_class_name (label, "workspace-title-new");
    clutter_container_child_set (CLUTTER_CONTAINER (table),
                                 CLUTTER_ACTOR (label),
                                 "y-expand", FALSE, NULL);

    nbtk_table_set_row_spacing (NBTK_TABLE (new_ws), 6);
    nbtk_table_set_col_spacing (NBTK_TABLE (new_ws), 6);
    nbtk_widget_set_padding (new_ws, &padding);
    nbtk_widget_set_style_class_name (NBTK_WIDGET (new_ws),
                                      "switcher-workspace-new");

    nbtk_widget_set_dnd_threshold (new_ws, 5);

    g_signal_connect (new_ws, "dnd-begin",
                      G_CALLBACK (dnd_begin_cb), self);

    g_signal_connect (new_ws, "dnd-end",
                      G_CALLBACK (dnd_end_cb), self);

    g_signal_connect (new_ws, "dnd-dropped",
                      G_CALLBACK (dnd_new_dropped_cb), self);

    g_signal_connect (new_ws, "dnd-enter",
                      G_CALLBACK (dnd_new_enter_cb), self);

    g_signal_connect (new_ws, "dnd-leave",
                      G_CALLBACK (dnd_new_leave_cb), self);

    priv->new_workspace = new_ws;
    priv->new_label = label;

    nbtk_table_add_widget (NBTK_TABLE (table), new_ws, 1, ws_count);
  }

  g_free (spaces);
  g_free (n_windows);

  mnb_drop_down_set_child (MNB_DROP_DOWN (self),
                           CLUTTER_ACTOR (table));

  g_signal_connect (screen, "notify::n-workspaces",
                    G_CALLBACK (screen_n_workspaces_notify), self);

  CLUTTER_ACTOR_CLASS (mnb_switcher_parent_class)->show (self);
}

static NbtkTable *
mnb_switcher_append_workspace (MnbSwitcher *switcher)
{
  MnbSwitcherPrivate *priv = switcher->priv;
  NbtkWidget         *table = priv->table;
  NbtkWidget         *last_ws = priv->new_workspace;
  NbtkWidget         *last_label = priv->new_label;
  NbtkTable          *new_ws;
  gint                row, col;
  ClutterChildMeta   *meta;

  meta = clutter_container_get_child_meta (CLUTTER_CONTAINER (table),
                                           CLUTTER_ACTOR (last_ws));

  g_object_get (meta, "column", &col, NULL);

  clutter_container_child_set (CLUTTER_CONTAINER (table),
                               CLUTTER_ACTOR (last_ws),
                               "column", col + 1, NULL);

  clutter_container_child_set (CLUTTER_CONTAINER (table),
                               CLUTTER_ACTOR (last_label),
                               "column", col + 1,
                               "y-expand", FALSE, NULL);

  /*
   * Insert new workspace label and content pane where the new workspace
   * area was.
   */
  make_workspace_label   (switcher, FALSE, col);
  new_ws = NBTK_TABLE (make_workspace_content (switcher, FALSE, col));

  return new_ws;
}

static void
mnb_switcher_hide (ClutterActor *self)
{
  MnbSwitcherPrivate *priv;

  g_return_if_fail (MNB_IS_SWITCHER (self));

  priv = MNB_SWITCHER (self)->priv;

  mnb_drop_down_set_child (MNB_DROP_DOWN (self),
                           NULL);
  CLUTTER_ACTOR_CLASS (mnb_switcher_parent_class)->hide (self);
}

static void
mnb_switcher_finalize (GObject *object)
{
  MnbSwitcher *switcher = MNB_SWITCHER (object);

  g_list_free (switcher->priv->last_workspaces);

  G_OBJECT_CLASS (mnb_switcher_parent_class)->finalize (object);
}

static void
mnb_switcher_class_init (MnbSwitcherClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  ClutterActorClass *actor_class = CLUTTER_ACTOR_CLASS (klass);

  object_class->finalize = mnb_switcher_finalize;

  actor_class->show = mnb_switcher_show;
  actor_class->hide = mnb_switcher_hide;

  g_type_class_add_private (klass, sizeof (MnbSwitcherPrivate));

  child_data_quark = g_quark_from_static_string (CHILD_DATA_KEY);
}

static void
mnb_switcher_init (MnbSwitcher *self)
{
  self->priv = GET_PRIVATE (self);
}

NbtkWidget*
mnb_switcher_new (MutterPlugin *plugin)
{
  MnbSwitcher *switcher;

  g_return_val_if_fail (MUTTER_PLUGIN (plugin), NULL);

  switcher = g_object_new (MNB_TYPE_SWITCHER, NULL);
  switcher->priv->plugin = plugin;

  return NBTK_WIDGET (switcher);
}


