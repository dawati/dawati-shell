#include "mux-light-switch.h"

#include <glib/gi18n.h>

G_DEFINE_TYPE (MuxLightSwitch, mux_light_switch, GTK_TYPE_DRAWING_AREA)

#define MUX_LIGHT_SWITCH_GET_PRIVATE(o)                                 \
        (G_TYPE_INSTANCE_GET_PRIVATE ((o), MUX_TYPE_LIGHT_SWITCH, MuxLightSwitchPrivate))

#define SWITCH_WIDTH 40

static gboolean mux_light_switch_expose (GtkWidget *lightswitch,
                                         GdkEventExpose *event);
static gboolean mux_light_switch_button_press (GtkWidget *lightswitch,
                                               GdkEventButton *event);
static gboolean mux_light_switch_button_release (GtkWidget    *lightswitch,
                                                 GdkEventButton *event);
static gboolean mux_light_switch_motion_notify (GtkWidget *lightswitch,
                                                GdkEventMotion *event);
static void mux_light_switch_size_request (GtkWidget *lightswitch,
                                           GtkRequisition *req);

enum
{
        SWITCH_FLIPPED,
        LAST_SIGNAL
};

static guint mux_light_switch_signals[LAST_SIGNAL] = { 0 };

static gint switch_width, switch_height, trough_width;

typedef struct _MuxLightSwitchPrivate MuxLightSwitchPrivate;

struct _MuxLightSwitchPrivate {
        gboolean active; /* boolean state of switch */
        gboolean dragging; /* true if dragging switch */
        gint x; /* the x position of the switch */
};

static void
mux_light_switch_class_init (MuxLightSwitchClass *klass)
{
        GObjectClass *object_class;
        GtkWidgetClass *widget_class;

        object_class = G_OBJECT_CLASS (klass);
        widget_class = GTK_WIDGET_CLASS (klass);

        widget_class->expose_event = mux_light_switch_expose;
        widget_class->button_press_event = mux_light_switch_button_press;
        widget_class->button_release_event = mux_light_switch_button_release;
        widget_class->motion_notify_event = mux_light_switch_motion_notify;
        widget_class->size_request = mux_light_switch_size_request;

        /* MuxLightSwitch signals */
        mux_light_switch_signals[SWITCH_FLIPPED] = g_signal_new (
                "switch-flipped",
                G_OBJECT_CLASS_TYPE (object_class),
                G_SIGNAL_RUN_FIRST,
                G_STRUCT_OFFSET (MuxLightSwitchClass, switch_flipped),
                NULL, NULL,
                gtk_marshal_VOID__BOOLEAN,
                G_TYPE_NONE, 1,
                G_TYPE_BOOLEAN);

        g_type_class_add_private (klass,
                                  sizeof (MuxLightSwitchPrivate));
}

static void
mux_light_switch_init (MuxLightSwitch *self)
{
        /* Variables for dynamic guessery */
        PangoLayout *layout;
        PangoContext *context;
        GString *on, *off;
        gint label_width, label_height;
        GtkStyle *style;

        /* FIXME: probably shouldn't init values here? */
        MuxLightSwitchPrivate *priv;
        priv = MUX_LIGHT_SWITCH_GET_PRIVATE (self);
        priv->active = FALSE;
        priv->x = 0;

        /* FIXME:
         * Do some guess work as to sizes of things. Better elsewhere? */
        style = GTK_WIDGET (self)->style;
        off = g_string_new (_("Off"));
        on = g_string_new (_("On"));
        context = gdk_pango_context_get ();
        layout = pango_layout_new (context);
        g_object_unref (context);
        pango_layout_set_font_description (layout,
                                           style->font_desc);
        if (off->len > on->len)
                pango_layout_set_text (layout,
                                       off->str,
                                       off->len);
        else
                pango_layout_set_text (layout,
                                       on->str,
                                       on->len);

        pango_layout_get_pixel_size (layout,
                                     &label_width,
                                     &label_height);

        trough_width = label_width * 5;
        switch_width = trough_width / 2;
        switch_height = label_height * 2;

        /* add events, do initial draw/update, etc */
        gtk_widget_add_events (GTK_WIDGET (self),
                               GDK_BUTTON_PRESS_MASK
                               | GDK_BUTTON_RELEASE_MASK
                               | GDK_POINTER_MOTION_MASK);
}

static void
draw (GtkWidget *lightswitch, cairo_t *cr)
{
        MuxLightSwitchPrivate *priv;
        gint on_label_x, off_label_x, label_width, label_height;
        gint on_pad, off_pad;
        GtkStyle *style;
        PangoLayout *layout;
        PangoContext *context;
        GString *on, *off;
        GtkStateType trough_state;

        priv = MUX_LIGHT_SWITCH_GET_PRIVATE (lightswitch);
        style = lightswitch->style;

        /* Dynamically calculate sizes based on width of longest string */
        off = g_string_new (_("Off"));
        on = g_string_new (_("On"));

        on_label_x = (trough_width / 5) * 0.75;
        off_label_x =  (trough_width / 8) * 5;
        if (priv->active)
          trough_state = GTK_STATE_SELECTED;
        else
          trough_state = GTK_STATE_NORMAL;

        /* draw the trough */
        gtk_paint_box (style,
                       lightswitch->window,
                       trough_state,
                       GTK_SHADOW_ETCHED_IN,
                       NULL,
                       NULL,
                       NULL,
                       0,
                       0,
                       trough_width,
                       switch_height);

        /* Draw the first label; "On" */
        context = gdk_pango_context_get ();
        layout = pango_layout_new (context);
        g_object_unref (context);
        pango_layout_set_font_description (layout,
                                           style->font_desc);
        pango_layout_set_text (layout,
                               on->str,
                               on->len);
        pango_layout_get_size (layout,
                               &label_width,
                               &label_height);
        gdk_draw_layout (lightswitch->window,
                         style->fg_gc[GTK_STATE_SELECTED],
                         on_label_x,
                         (lightswitch->allocation.height
                          - (label_height / PANGO_SCALE)) / 2,
                         layout);
        /* Draw the second label; "Off" */
        pango_layout_set_text (layout,
                               off->str,
                               off->len);
        pango_layout_get_size (layout,
                               &label_width,
                               &label_height);
        gdk_draw_layout (lightswitch->window,
                         style->fg_gc[GTK_STATE_NORMAL],
                         off_label_x,
                         (lightswitch->allocation.height
                          - (label_height / PANGO_SCALE)) / 2,
                         layout);

        /* draw the switch itself */
        gtk_paint_box (style,
                       lightswitch->window,
                       GTK_STATE_ACTIVE,
                       GTK_SHADOW_ETCHED_OUT,
                       NULL,
                       NULL,
                       NULL,
                       priv->x,
                       0,
                       switch_width,
                       switch_height);

        g_string_free (on,
                       TRUE);
        g_string_free (off,
                       TRUE);
}

static void
mux_light_switch_size_request (GtkWidget *lightswitch,
                               GtkRequisition *req)
{
        req->height = switch_height;
        req->width = trough_width;
}

static gboolean
mux_light_switch_expose (GtkWidget *lightswitch,
                         GdkEventExpose *event)
{

        cairo_t *cr;

        cr = gdk_cairo_create (lightswitch->window);

        cairo_rectangle (cr,
                         event->area.x,
                         event->area.y,
                         event->area.width,
                         event->area.height);

                         cairo_clip (cr);

        draw (lightswitch, cr);

        cairo_destroy (cr);

        return FALSE;
}

static gboolean
mux_light_switch_button_press (GtkWidget *lightswitch,
                               GdkEventButton *event)
{
        MuxLightSwitchPrivate *priv;

        priv = MUX_LIGHT_SWITCH_GET_PRIVATE (lightswitch);

        if (event->x > priv->x && event->x < (priv->x + switch_width)) {
                /* we are in the lightswitch */
                priv->dragging = TRUE;
        }

        return FALSE;
}

static void
emit_state_changed_signal (MuxLightSwitch *lightswitch,
                           int x)
{
        MuxLightSwitchPrivate *priv;

        priv = MUX_LIGHT_SWITCH_GET_PRIVATE (lightswitch);

        if (priv->dragging == FALSE) {
                if (x > trough_width / 2) {
                        priv->x = trough_width - switch_width;
                        priv->active = TRUE;
                        g_signal_emit (lightswitch,
                                       mux_light_switch_signals[SWITCH_FLIPPED],
                                       0,
                                       priv->active);
                } else {
                        priv->x = 0;
                        priv->active = FALSE;
                        g_signal_emit (lightswitch,
                                       mux_light_switch_signals[SWITCH_FLIPPED],
                                       0,
                                       priv->active);
                }
        } else {
                priv->x = x;
        }

        gtk_widget_queue_draw (GTK_WIDGET (lightswitch));
}

static gboolean
mux_light_switch_motion_notify (GtkWidget *lightswitch,
                                GdkEventMotion *event)
{
        MuxLightSwitchPrivate *priv;
        gint new_x;

        priv = MUX_LIGHT_SWITCH_GET_PRIVATE (lightswitch);

        if (priv->dragging) {
                if (event->x > (trough_width - switch_width))
                        new_x = (trough_width - switch_width);
                else if (event->x < 0)
                        new_x = 0;
                else
                        new_x = event->x;
                emit_state_changed_signal (MUX_LIGHT_SWITCH (lightswitch),
                                           new_x);
        }

        return TRUE;
}

static gboolean
mux_light_switch_button_release (GtkWidget *lightswitch,
                                 GdkEventButton *event)
{
        MuxLightSwitchPrivate *priv;

        priv = MUX_LIGHT_SWITCH_GET_PRIVATE (lightswitch);

        /* detect whereabouts we are and "drop" into a state */
        if (priv->dragging) {
                priv = MUX_LIGHT_SWITCH_GET_PRIVATE (lightswitch);
                priv->dragging = FALSE;
                emit_state_changed_signal (MUX_LIGHT_SWITCH (lightswitch),
                                           event->x);
        }

        return FALSE;
}

gboolean
mux_light_switch_get_active (MuxLightSwitch *lightswitch)
{
	MuxLightSwitchPrivate *priv = MUX_LIGHT_SWITCH_GET_PRIVATE (lightswitch);

	return priv->active;
}

void
mux_light_switch_set_active (MuxLightSwitch *lightswitch, gboolean active)
{
	MuxLightSwitchPrivate *priv = MUX_LIGHT_SWITCH_GET_PRIVATE (lightswitch);

	if (priv->active == active) {
		return;
	} else {
		priv->active = active;
		if (active == TRUE) {
			priv->x = trough_width - switch_width;
		} else {
			priv->x = 0;
		}
	}
}

GtkWidget*
mux_light_switch_new (void)
{
        return g_object_new (MUX_TYPE_LIGHT_SWITCH, NULL);
}

