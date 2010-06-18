
#ifndef MEEGO_CONSTRAINTS_H
#define MEEGO_CONSTRAINTS_H

#include "meego-netbook.h"

gboolean
meego_netbook_constrain_window (MutterPlugin       *plugin,
                                 MetaWindow         *window,
                                 ConstraintInfo     *info,
                                 ConstraintPriority  priority,
                                 gboolean            check_only);

#endif
