/* Your copyright here */

#include <config.h>
#include <glib.h>
#include <glib/gi18n-lib.h>

#include "mnp-button-item.h"
#include <mx/mx.h>

struct _MnpButtonItemPriv {
	gpointer *user_data;
	completion_done func;
};

static GObjectClass *parent_class = NULL;

static void
button_clicked_cb (MxButton *button, gpointer data)
{
	MnpButtonItem *factory = (MnpButtonItem *)data;

	factory->priv->func(factory->priv->user_data, mx_button_get_label(button));
}

static ClutterActor *
mnp_button_item_create (MxItemFactory *factory)
{
	ClutterActor *actor = mx_button_new ();

	clutter_actor_set_name (actor, "completion-list-button");
  	g_signal_connect (actor, "clicked", G_CALLBACK (button_clicked_cb),
                    factory);

	return actor;
}

static void
mnp_button_item_finalize (GObject *object)
{
	parent_class->finalize (object);
}

static void
mnp_button_item_instance_init (GTypeInstance *instance, gpointer g_class)
{
}

static void
mx_item_factory_init (MxItemFactoryIface *klass)
{
	klass->create = mnp_button_item_create;
}

static void
mnp_button_item_class_init (MnpButtonItemClass *klass)
{
	GObjectClass *object_class;

	parent_class = g_type_class_peek_parent (klass);
	object_class = (GObjectClass*) klass;
	object_class->finalize = mnp_button_item_finalize;
}
GType
mnp_button_item_get_type (void)
{
	static GType type = 0;
	if (G_UNLIKELY(type == 0))
	{
		static const GTypeInfo info = 
		{
			sizeof (MnpButtonItemClass),
			NULL,   /* base_init */
			NULL,   /* base_finalize */
			(GClassInitFunc) mnp_button_item_class_init,   /* class_init */
			NULL,   /* class_finalize */
			NULL,   /* class_data */
			sizeof (MnpButtonItem),
			0,      /* n_preallocs */
			mnp_button_item_instance_init,    /* instance_init */
			NULL
		};


		static const GInterfaceInfo mx_item_factory_info = 
		{
			(GInterfaceInitFunc) mx_item_factory_init, /* interface_init */
			NULL,         /* interface_finalize */
			NULL          /* interface_data */
		};

		type = g_type_register_static (G_TYPE_OBJECT,
			"MnpButtonItem",
			&info, 0);

		g_type_add_interface_static (type, MX_TYPE_ITEM_FACTORY,
			&mx_item_factory_info);

	}
	return type;
}

MnpButtonItem *
mnp_button_item_new (gpointer user_data, completion_done func)
{
	MnpButtonItem *item = g_object_new (MNP_TYPE_BUTTON_ITEM_TYPE, NULL);
	
	item->priv = g_new0 (MnpButtonItemPriv, 1);

	item->priv->user_data = user_data;
	item->priv->func = func;
	return item;
}
