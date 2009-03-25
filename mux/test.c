#include "mux-switch-box.h"

#include <gtk/gtk.h>

static void
switch_flipped_cb (GtkWidget *lswitch, gboolean state, gpointer data)
{
        g_print ("::switch-toggled\n");
}

int
main (int argc, char **argv)
{
        GtkWidget *window, *vbox;
        GtkWidget *switch1, *switch2, *switch3;

        gtk_init (&argc, &argv);

        window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
        vbox = gtk_vbox_new (FALSE, 2);

        switch1 = mux_switch_box_new ("rob");
        gtk_box_pack_start (GTK_BOX (vbox), switch1, FALSE, FALSE, 0);
        switch2 = mux_switch_box_new ("bob");
        gtk_box_pack_start (GTK_BOX (vbox), switch2, FALSE, FALSE, 0);
        switch3 = mux_switch_box_new ("neil");
        gtk_box_pack_start (GTK_BOX (vbox), switch3, FALSE, FALSE, 0);

        gtk_container_add (GTK_CONTAINER (window), GTK_WIDGET (vbox));

        g_signal_connect (window, "destroy",
                          G_CALLBACK (gtk_main_quit), NULL);

        g_signal_connect (switch1, "switch-toggled",
                          G_CALLBACK (switch_flipped_cb), NULL);

        gtk_widget_show_all (window);

        gtk_main ();

        return 0;
}
