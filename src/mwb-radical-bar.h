#ifndef _MWB_RADICAL_BAR_H
#define _MWB_RADICAL_BAR_H

#include <glib-object.h>
#include <clutter/clutter.h>
#include <nbtk/nbtk.h>

G_BEGIN_DECLS

#define MWB_TYPE_RADICAL_BAR mwb_radical_bar_get_type()

#define MWB_RADICAL_BAR(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
  MWB_TYPE_RADICAL_BAR, MwbRadicalBar))

#define MWB_RADICAL_BAR_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), \
  MWB_TYPE_RADICAL_BAR, MwbRadicalBarClass))

#define MWB_IS_RADICAL_BAR(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
  MWB_TYPE_RADICAL_BAR))

#define MWB_IS_RADICAL_BAR_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), \
  MWB_TYPE_RADICAL_BAR))

#define MWB_RADICAL_BAR_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
  MWB_TYPE_RADICAL_BAR, MwbRadicalBarClass))

typedef struct _MwbRadicalBarPrivate MwbRadicalBarPrivate;

typedef struct {
  NbtkWidget parent;
  
  MwbRadicalBarPrivate *priv;
} MwbRadicalBar;

typedef struct {
  NbtkWidgetClass parent_class;
  
  void (* go)     (MwbRadicalBar *radical_bar, const gchar *url);
  void (* reload) (MwbRadicalBar *radical_bar);
  void (* stop)   (MwbRadicalBar *radical_bar);
} MwbRadicalBarClass;

GType mwb_radical_bar_get_type (void);

NbtkWidget *mwb_radical_bar_new (void);

void mwb_radical_bar_focus         (MwbRadicalBar *radical_bar);

void mwb_radical_bar_set_text      (MwbRadicalBar *radical_bar, const gchar  *text);
void mwb_radical_bar_set_icon      (MwbRadicalBar *radical_bar, ClutterActor *icon);
void mwb_radical_bar_set_loading   (MwbRadicalBar *radical_bar, gboolean loading);
void mwb_radical_bar_set_progress  (MwbRadicalBar *radical_bar, gdouble progress);

const gchar  *mwb_radical_bar_get_text      (MwbRadicalBar *radical_bar);
ClutterActor *mwb_radical_bar_get_icon      (MwbRadicalBar *radical_bar);
gboolean      mwb_radical_bar_get_loading   (MwbRadicalBar *radical_bar);
gdouble       mwb_radical_bar_get_progress  (MwbRadicalBar *radical_bar);

G_END_DECLS

#endif /* _MWB_RADICAL_BAR_H */

