#ifndef __GGG_MANUAL_DIALOG_H__
#define __GGG_MANUAL_DIALOG_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define GGG_TYPE_MANUAL_DIALOG (ggg_manual_dialog_get_type())
#define GGG_MANUAL_DIALOG(obj)                                          \
   (G_TYPE_CHECK_INSTANCE_CAST ((obj),                                  \
                                GGG_TYPE_MANUAL_DIALOG,                 \
                                GggManualDialog))
#define GGG_MANUAL_DIALOG_CLASS(klass)                                  \
   (G_TYPE_CHECK_CLASS_CAST ((klass),                                   \
                             GGG_TYPE_MANUAL_DIALOG,                    \
                             GggManualDialogClass))
#define IS_GGG_MANUAL_DIALOG(obj)                                       \
   (G_TYPE_CHECK_INSTANCE_TYPE ((obj),                                  \
                                GGG_TYPE_MANUAL_DIALOG))
#define IS_GGG_MANUAL_DIALOG_CLASS(klass)                               \
   (G_TYPE_CHECK_CLASS_TYPE ((klass),                                   \
                             GGG_TYPE_MANUAL_DIALOG))
#define GGG_MANUAL_DIALOG_GET_CLASS(obj)                                \
   (G_TYPE_INSTANCE_GET_CLASS ((obj),                                   \
                               GGG_TYPE_MANUAL_DIALOG,                  \
                               GggManualDialogClass))

typedef struct _GggManualDialogPrivate GggManualDialogPrivate;
typedef struct _GggManualDialog      GggManualDialog;
typedef struct _GggManualDialogClass GggManualDialogClass;

struct _GggManualDialog {
  GtkDialog parent;
  GggManualDialogPrivate *priv;
};

struct _GggManualDialogClass {
  GtkDialogClass parent_class;
};

GType ggg_manual_dialog_get_type (void) G_GNUC_CONST;

G_END_DECLS

#endif /* __GGG_MANUAL_DIALOG_H__ */
