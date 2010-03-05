#include "moblin-netbook-constraints.h"
#include "moblin-netbook-mutter-hints.h"

#include "constraints.h"

#include <stdlib.h>
#include <math.h>

#define NOT_TOO_SMALL_BORDER 0     /* space around the resized window */
#define NOT_TOO_SMALL_TRIGGER 0.60 /* percentage of screen size below which
                                    * the NOT_TOO_SMALL constraint triggers
                                    */

static gboolean
not_too_small (MutterPlugin       *plugin,
               MetaWindow         *window,
               ConstraintInfo     *info,
               ConstraintPriority  priority,
               gboolean            check_only)
{
  gboolean already_satisfied = TRUE;
  gboolean resize = FALSE;
  gint screen_width, screen_height, width, height;
  gint x = 0, y = 0, old_x = 0, old_y;
  MetaRectangle *start_rect;
  MetaRectangle min_size, max_size;

  if (priority > PRIORITY_MAXIMUM || /* None of our businesss */
      info->is_user_action ||        /* Don't mess users about */
      meta_window_is_user_placed (window)   ||
      meta_window_get_window_type (window) != META_WINDOW_NORMAL)
    return TRUE;

  old_x = info->current.x - info->fgeom->left_width;
  old_y = info->current.y - info->fgeom->top_height;

  width  = info->current.width  +
    info->fgeom->left_width + info->fgeom->right_width;
  height = info->current.height +
    info->fgeom->top_height + info->fgeom->bottom_height;

  screen_width  = info->work_area_monitor.width;
  screen_height = info->work_area_monitor.height;

  if (width == screen_width && height == screen_height)
    return TRUE;

  if ((((gfloat)width / (gfloat) screen_width) > NOT_TOO_SMALL_TRIGGER))
    {
      already_satisfied = FALSE;
      resize = TRUE;
    }

  if (!resize)
    {
      /*
       * If we are not resizing the application, attempt to center it.
       */
      x = (screen_width  - width) / 2;
      y = (screen_height > height) ? (screen_height - height) / 2 : 0;

      if (x != old_x || y != old_y)
        already_satisfied = FALSE;
    }

  if (check_only || already_satisfied)
    return already_satisfied;

  if (resize)
    {
      width  = screen_width - 2 * NOT_TOO_SMALL_BORDER;
      height = screen_height - 2 * NOT_TOO_SMALL_BORDER;

      /* We respect both the max and min size hints */
      /* We should include the frame here */
      meta_constraints_get_size_limits (window, info->fgeom, TRUE,
                                        &min_size, &max_size);

      if (width > max_size.width)
          width = max_size.width;
      else if (width < min_size.width)
          width = min_size.width;

      if (height > max_size.height)
          height = max_size.height;
      else if (height < min_size.height)
          height = min_size.height;

      x = (screen_width  - width) / 2;
      y = (screen_height > height) ? (screen_height - height) / 2 : 0;
    }

  if (info->action_type == ACTION_MOVE_AND_RESIZE)
    start_rect = &info->current;
  else
    start_rect = &info->orig;

  start_rect->x      = x;
  start_rect->y      = y;
  start_rect->width  = width;
  start_rect->height = height;

  meta_constraints_unextend_by_frame (start_rect, info->fgeom);

  return TRUE;
}

gboolean
moblin_netbook_constrain_window (MutterPlugin       *plugin,
                                 MetaWindow         *window,
                                 ConstraintInfo     *info,
                                 ConstraintPriority  priority,
                                 gboolean            check_only)
{
  MoblinNetbookPluginPrivate *priv = MOBLIN_NETBOOK_PLUGIN (plugin)->priv;
  gboolean satisfied = TRUE;

  if (priv->netbook_mode)
    {
      satisfied &= not_too_small (plugin, window, info, priority, check_only);
    }

  return satisfied;
}
