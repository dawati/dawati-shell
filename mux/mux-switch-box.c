#include "mux-switch-box.h"

#include "mux-light-switch.h"

#include <gtk/gtk.h>

G_DEFINE_TYPE (MuxSwitchBox, mux_switch_box, GTK_TYPE_HBOX)

#define MUX_SWITCH_BOX_GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MUX_TYPE_SWITCH_BOX, MuxSwitchBoxPrivate))

enum
{
        SWITCH_TOGGLED,
        LAST_SIGNAL
};

enum
{
        ARG_LABEL = 1
};

enum
{
        PROP_0,
        PROP_NAME,
				PROP_SWITCH_ACTIVE
};

static GtkHBoxClass *parent_class = NULL;

static guint mux_switch_box_signals[LAST_SIGNAL] = { 0 };

struct _MuxSwitchBoxPrivate {
        GtkWidget *lightswitch;
        GtkWidget *switch_box;
        GtkWidget *switch_label;
};

static void
_switch_flipped_cb (GtkWidget *lightswitch, gboolean state, gpointer data)
{
        MuxSwitchBox *box = (MuxSwitchBox *) data;
        g_signal_emit (box, mux_switch_box_signals[SWITCH_TOGGLED],
          0, state);
}

static void
mux_switch_box_get_property (GObject *object, guint property_id,
                              GValue *value, GParamSpec *pspec)
{
	MuxSwitchBoxPrivate *priv;

	g_return_if_fail (object != NULL);
	g_return_if_fail (MUX_IS_SWITCH_BOX (object));

	priv = MUX_SWITCH_BOX_GET_PRIVATE (object);

  switch (property_id) {
  case PROP_NAME:
		g_value_set_string (value,
		  gtk_label_get_text (GTK_LABEL (priv->switch_label)));
		break;
	case PROP_SWITCH_ACTIVE:
		g_value_set_boolean (value,
		  mux_light_switch_get_active ((MuxLightSwitch *) priv->lightswitch));
		break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
mux_switch_box_set_property (GObject *object, guint property_id,
                              const GValue *value, GParamSpec *pspec)
{
	MuxSwitchBoxPrivate *priv;

	g_return_if_fail (object != NULL);
	g_return_if_fail (MUX_IS_SWITCH_BOX (object));

	priv = MUX_SWITCH_BOX_GET_PRIVATE (object);

  switch (property_id) {
  case PROP_NAME:
		gtk_label_set_text (GTK_LABEL (priv->switch_label),
												g_value_get_string (value));
    break;
	case PROP_SWITCH_ACTIVE:
		mux_light_switch_set_active ((MuxLightSwitch *) priv->lightswitch,
																 g_value_get_boolean (value));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
mux_switch_box_class_init (MuxSwitchBoxClass *klass)
{
        GObjectClass *object_class;
        GtkWidgetClass *widget_class;
        GParamSpec *pspec;

        parent_class = g_type_class_peek_parent (klass);

        object_class = G_OBJECT_CLASS (klass);
        widget_class = GTK_WIDGET_CLASS (klass);

        object_class->set_property = mux_switch_box_set_property;
        object_class->get_property = mux_switch_box_get_property;

        mux_switch_box_signals[SWITCH_TOGGLED] = g_signal_new (
                "switch-toggled",
                MUX_TYPE_SWITCH_BOX,
                G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
                G_STRUCT_OFFSET (MuxSwitchBoxClass, switch_toggled),
                NULL, NULL,
                gtk_marshal_VOID__BOOLEAN,
                G_TYPE_NONE, 1,
                G_TYPE_BOOLEAN);

        g_type_class_add_private (klass, sizeof (MuxSwitchBoxPrivate));

        object_class->get_property = mux_switch_box_get_property;
        object_class->set_property = mux_switch_box_set_property;

        pspec = g_param_spec_string ("name",
                                     "name",
                                     "name of label",
                                     NULL,
                                     G_PARAM_READWRITE | G_PARAM_CONSTRUCT);
        g_object_class_install_property (object_class,
                                         PROP_NAME,
                                         pspec);

        pspec = g_param_spec_boolean ("switch-active",
                                      "switch-active",
                                      "whether the switch is on",
                                      FALSE,
                                      G_PARAM_READWRITE);
        g_object_class_install_property (object_class,
                                         PROP_SWITCH_ACTIVE,
                                         pspec);
}

static void
mux_switch_box_init (MuxSwitchBox *self)
{
        MuxSwitchBoxPrivate *priv;

        priv = MUX_SWITCH_BOX_GET_PRIVATE (self);
        self->priv = priv;

        gtk_box_set_spacing (GTK_BOX (self), 6);

        priv->switch_box = gtk_hbox_new (TRUE, 0);
        gtk_box_pack_start (GTK_BOX (self), priv->switch_box,
                            FALSE, FALSE, 0);
        priv->switch_label = gtk_label_new ("Name");
        gtk_box_pack_start (GTK_BOX (priv->switch_box), priv->switch_label,
                            FALSE, FALSE, 0);
        gtk_widget_show (priv->switch_label);
        priv->lightswitch = mux_light_switch_new ();
        gtk_box_pack_start (GTK_BOX (priv->switch_box), priv->lightswitch,
                            FALSE, FALSE, 0);
        gtk_widget_show (priv->lightswitch);

        g_signal_connect (priv->lightswitch, "switch-flipped",
                          G_CALLBACK (_switch_flipped_cb), self);
}

GtkWidget *
mux_switch_box_new (const gchar *text)
{
        return g_object_new (MUX_TYPE_SWITCH_BOX, "name", text, NULL);
}

void
mux_switch_box_set_active (MuxSwitchBox *switch_box,
			   gboolean active)
{
	MuxSwitchBoxPrivate *priv = MUX_SWITCH_BOX_GET_PRIVATE (switch_box);

	mux_light_switch_set_active (priv->lightswitch, active);
}
