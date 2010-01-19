
#ifndef MOBLIN_CONSTRAINTS_H
#define MOBLIN_CONSTRAINTS_H

#include "moblin-netbook.h"

gboolean
moblin_netbook_constrain_window (MutterPlugin       *plugin,
                                 MetaWindow         *window,
                                 ConstraintInfo     *info,
                                 ConstraintPriority  priority,
                                 gboolean            check_only);

#endif
