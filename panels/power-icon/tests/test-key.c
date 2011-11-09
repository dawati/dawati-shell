
#include <stdlib.h>
#include <fakekey/fakekey.h>
#include <gdk/gdkx.h>
#include <gtk/gtk.h>

int
main (int     argc,
      char  **argv)
{
  FakeKey *fk;
  KeySym   keysym;
  KeyCode  keycode;

  if (argc < 2)
  {
    printf ("Need keysym\n");
    return EXIT_SUCCESS;
  }

  gtk_init (&argc, &argv);
  fk = fakekey_init (GDK_DISPLAY ());

  sscanf (argv[1], "%X", (unsigned int *) &keysym);
  printf ("%X\n", (unsigned int) keysym);

  keycode = XKeysymToKeycode (GDK_DISPLAY (), keysym);
  fakekey_send_keyevent (fk, keycode, True, 0);
  fakekey_send_keyevent (fk, keycode, False, 0);

  return EXIT_SUCCESS;
}

