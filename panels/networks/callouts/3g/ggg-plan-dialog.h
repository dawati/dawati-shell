#ifndef __GGG_PLAN_DIALOG_H__
#define __GGG_PLAN_DIALOG_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define GGG_TYPE_PLAN_DIALOG (ggg_plan_dialog_get_type())
#define GGG_PLAN_DIALOG(obj)                                         \
   (G_TYPE_CHECK_INSTANCE_CAST ((obj),                                  \
                                GGG_TYPE_PLAN_DIALOG,                \
                                GggPlanDialog))
#define GGG_PLAN_DIALOG_CLASS(klass)                                 \
   (G_TYPE_CHECK_CLASS_CAST ((klass),                                   \
                             GGG_TYPE_PLAN_DIALOG,                   \
                             GggPlanDialogClass))
#define IS_GGG_PLAN_DIALOG(obj)                                      \
   (G_TYPE_CHECK_INSTANCE_TYPE ((obj),                                  \
                                GGG_TYPE_PLAN_DIALOG))
#define IS_GGG_PLAN_DIALOG_CLASS(klass)                              \
   (G_TYPE_CHECK_CLASS_TYPE ((klass),                                   \
                             GGG_TYPE_PLAN_DIALOG))
#define GGG_PLAN_DIALOG_GET_CLASS(obj)                               \
   (G_TYPE_INSTANCE_GET_CLASS ((obj),                                   \
                               GGG_TYPE_PLAN_DIALOG,                 \
                               GggPlanDialogClass))

typedef struct _GggPlanDialogPrivate GggPlanDialogPrivate;
typedef struct _GggPlanDialog      GggPlanDialog;
typedef struct _GggPlanDialogClass GggPlanDialogClass;

struct _GggPlanDialog {
  GtkDialog parent;
  GggPlanDialogPrivate *priv;
};

struct _GggPlanDialogClass {
  GtkDialogClass parent_class;
};

GType ggg_plan_dialog_get_type (void) G_GNUC_CONST;

RestXmlNode *ggg_plan_dialog_get_selected (GggPlanDialog *dialog);

G_END_DECLS

#endif /* __GGG_PLAN_DIALOG_H__ */
