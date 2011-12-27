/*
 * Carrick - a connection panel for the Dawati Netbook
 * Copyright (C) 2011 Intel Corporation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License version
 * 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 *
 * Written by - Jussi Kukkonen <jussi.kukkonen@intel.com>
 */

#ifndef _CARRICK_OFONO_AGENT
#define _CARRICK_OFONO_AGENT

#include <glib-object.h>

G_BEGIN_DECLS

#define CARRICK_TYPE_OFONO_AGENT carrick_ofono_agent_get_type()

#define CARRICK_OFONO_AGENT(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), CARRICK_TYPE_OFONO_AGENT, CarrickOfonoAgent))

#define CARRICK_OFONO_AGENT_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), CARRICK_TYPE_OFONO_AGENT, CarrickOfonoAgentClass))

#define CARRICK_IS_OFONO_AGENT(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CARRICK_TYPE_OFONO_AGENT))

#define CARRICK_IS_OFONO_AGENT_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), CARRICK_TYPE_OFONO_AGENT))

#define CARRICK_OFONO_AGENT_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), CARRICK_TYPE_OFONO_AGENT, CarrickOfonoAgentClass))

typedef struct _CarrickOfonoAgentPrivate CarrickOfonoAgentPrivate;

typedef struct {
  GObject parent;
  CarrickOfonoAgentPrivate *priv;
} CarrickOfonoAgent;

typedef struct {
  GObjectClass parent_class;

} CarrickOfonoAgentClass;

GType carrick_ofono_agent_get_type (void);

CarrickOfonoAgent* carrick_ofono_agent_new (void);

void
carrick_ofono_agent_enter_pin (CarrickOfonoAgent *self,
                               const char        *modem_path,
                               const char        *pin_type,
                               const char        *pin);

void
carrick_ofono_agent_reset_pin (CarrickOfonoAgent *self,
                               const char        *modem_path,
                               const char        *puk_type,
                               const char        *puk,
                               const char        *new_pin);

void
carrick_ofono_agent_change_pin (CarrickOfonoAgent *self,
                                const char        *modem_path,
                                const char        *pin_type,
                                const char        *pin,
                                const char        *new_pin);

int
carrick_ofono_agent_get_retries (CarrickOfonoAgent *self,
                                 const char        *modem_path,
                                 const char        *pin_type);

G_END_DECLS
#endif /* _CARRICK_OFONO_AGENT */
