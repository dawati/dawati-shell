
#include "mwb-utils.h"

gboolean
mwb_utils_focus_on_click_cb (ClutterActor       *actor,
                             ClutterButtonEvent *event,
                             gpointer            swallow_event)
{
  ClutterActor *stage = clutter_actor_get_stage (actor);

  if (stage && CLUTTER_IS_STAGE (stage))
    clutter_stage_set_key_focus (CLUTTER_STAGE (stage), actor);

  return GPOINTER_TO_INT (swallow_event);
}

NbtkWidget *
mwb_utils_button_new ()
{
  NbtkWidget *button = nbtk_fade_button_new ();
  return button;
}

