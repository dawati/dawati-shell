#include "penge-grid-view.h"

#include "penge-calendar-pane.h"
#include "penge-recent-files-pane.h"
#include "penge-people-pane.h"

G_DEFINE_TYPE (PengeGridView, penge_grid_view, NBTK_TYPE_TABLE)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), PENGE_TYPE_GRID_VIEW, PengeGridViewPrivate))

typedef struct _PengeGridViewPrivate PengeGridViewPrivate;

struct _PengeGridViewPrivate {
  ClutterActor *calendar_pane;
  ClutterActor *recent_files_pane;
  ClutterActor *people_pane;
};

enum
{
  ACTIVATED_SIGNAL,
  LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

static void
penge_grid_view_get_property (GObject *object, guint property_id,
                              GValue *value, GParamSpec *pspec)
{
  switch (property_id) {
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
penge_grid_view_set_property (GObject *object, guint property_id,
                              const GValue *value, GParamSpec *pspec)
{
  switch (property_id) {
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
penge_grid_view_dispose (GObject *object)
{
  G_OBJECT_CLASS (penge_grid_view_parent_class)->dispose (object);
}

static void
penge_grid_view_finalize (GObject *object)
{
  G_OBJECT_CLASS (penge_grid_view_parent_class)->finalize (object);
}

static void
penge_grid_view_class_init (PengeGridViewClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (PengeGridViewPrivate));

  object_class->get_property = penge_grid_view_get_property;
  object_class->set_property = penge_grid_view_set_property;
  object_class->dispose = penge_grid_view_dispose;
  object_class->finalize = penge_grid_view_finalize;

  signals[ACTIVATED_SIGNAL] =
    g_signal_new ("activated",
                  PENGE_TYPE_GRID_VIEW,
                  G_SIGNAL_RUN_FIRST,
                  0,
                  NULL,
                  NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE,
                  0);
}

static void
penge_grid_view_init (PengeGridView *self)
{
  PengeGridViewPrivate *priv = GET_PRIVATE (self);

  priv->calendar_pane = g_object_new (PENGE_TYPE_CALENDAR_PANE,
                                      NULL);

  nbtk_table_add_actor (NBTK_TABLE (self),
                        priv->calendar_pane,
                        0,
                        0);

  priv->recent_files_pane = g_object_new (PENGE_TYPE_RECENT_FILES_PANE, 
                                          NULL);

  nbtk_table_add_actor (NBTK_TABLE (self),
                        priv->recent_files_pane,
                        0,
                        1);

  priv->people_pane = g_object_new (PENGE_TYPE_PEOPLE_PANE,
                                    NULL);

  nbtk_table_add_actor (NBTK_TABLE (self),
                        priv->people_pane,
                        0,
                        2);


  nbtk_table_set_row_spacing (NBTK_TABLE (self), 8);
  nbtk_table_set_col_spacing (NBTK_TABLE (self), 8);
}

