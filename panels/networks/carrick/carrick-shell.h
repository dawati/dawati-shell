/*
 * Carrick - a connection panel for the MeeGo Netbook
 * Copyright (C) 2009 Intel Corporation. All rights reserved.
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
 */

/*
 * Request that the shell (specifically the panel) request that it be focused.
 */
void carrick_shell_request_focus (void);

/*
 * Hide the shell
 */
void carrick_shell_hide (void);

void carrick_shell_show (void);

/*
 * If the shell is focused.
 */
gboolean carrick_shell_is_visible (void);


/*
 * Close @dialog if the shell is hidden. Specifically, send the DELETE_EVENT
 * response.
 */
void carrick_shell_close_dialog_on_hide (GtkDialog *dialog);
