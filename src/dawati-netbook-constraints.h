
#ifndef DAWATI_CONSTRAINTS_H
#define DAWATI_CONSTRAINTS_H

#include "dawati-netbook.h"

gboolean
dawati_netbook_constrain_window (MutterPlugin       *plugin,
                                 MetaWindow         *window,
                                 ConstraintInfo     *info,
                                 ConstraintPriority  priority,
                                 gboolean            check_only);

#endif
