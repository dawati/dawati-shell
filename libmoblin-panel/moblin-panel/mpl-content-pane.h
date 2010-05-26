
/*
 * Copyright © 2010 Intel Corp.
 *
 * Authors: Rob Staudinger <robert.staudinger@intel.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU Lesser General Public License,
 * version 2.1, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
 * more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef MPL_CONTENT_PANE_H
#define MPL_CONTENT_PANE_H

#include <glib-object.h>
#include <mx/mx.h>

G_BEGIN_DECLS

#define MPL_TYPE_CONTENT_PANE mpl_content_pane_get_type()

#define MPL_CONTENT_PANE(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), MPL_TYPE_CONTENT_PANE, MplContentPane))

#define MPL_CONTENT_PANE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), MPL_TYPE_CONTENT_PANE, MplContentPaneClass))

#define MPL_IS_CONTENT_PANE(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MPL_TYPE_CONTENT_PANE))

#define MPL_IS_CONTENT_PANE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), MPL_TYPE_CONTENT_PANE))

#define MPL_CONTENT_PANE_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), MPL_TYPE_CONTENT_PANE, MplContentPaneClass))

typedef struct _MplContentPane      MplContentPane;
typedef struct _MplContentPaneClass MplContentPaneClass;

/**
 * MplContentPane:
 *
 * A styled widget implementing a content pane with a header.
 */
struct _MplContentPane
{
  /*<private>*/
  MxBoxLayout parent;
};

/**
 * MplContentPaneClass:
 *
 * Class struct for #MplContentPane
 */
struct _MplContentPaneClass
{
  /*<private>*/
  MxBoxLayoutClass parent;
};

GType
mpl_content_pane_get_type (void);

ClutterActor *
mpl_content_pane_new (char const *title);

char const *
mpl_content_pane_get_title (MplContentPane *self);

void
mpl_content_pane_set_title (MplContentPane  *self,
                            char const      *title);

ClutterActor *
mpl_content_pane_get_header_actor (MplContentPane  *self);

void
mpl_content_pane_set_header_actor (MplContentPane *self,
                                   ClutterActor   *actor);

ClutterActor *
mpl_content_pane_get_child (MplContentPane *self);

void
mpl_content_pane_set_child (MplContentPane  *self,
                            ClutterActor    *child);

G_END_DECLS

#endif /* MPL_CONTENT_PANE_H */

