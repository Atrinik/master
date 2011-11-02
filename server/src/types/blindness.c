/************************************************************************
*            Atrinik, a Multiplayer Online Role Playing Game            *
*                                                                       *
*    Copyright (C) 2009-2011 Alex Tokar and Atrinik Development Team    *
*                                                                       *
* Fork from Daimonin (Massive Multiplayer Online Role Playing Game)     *
* and Crossfire (Multiplayer game for X-windows).                       *
*                                                                       *
* This program is free software; you can redistribute it and/or modify  *
* it under the terms of the GNU General Public License as published by  *
* the Free Software Foundation; either version 2 of the License, or     *
* (at your option) any later version.                                   *
*                                                                       *
* This program is distributed in the hope that it will be useful,       *
* but WITHOUT ANY WARRANTY; without even the implied warranty of        *
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
* GNU General Public License for more details.                          *
*                                                                       *
* You should have received a copy of the GNU General Public License     *
* along with this program; if not, write to the Free Software           *
* Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.             *
*                                                                       *
* The author can be reached at admin@atrinik.org                        *
************************************************************************/

/**
 * @file
 * Handles code for @ref BLINDNESS "blindness" objects.
 *
 * @author Alex Tokar */

#include <global.h>

/** @copydoc object_methods::process_func */
static void process_func(object *op)
{
	if (--op->stats.food > 0)
	{
		return;
	}

	if (op->env)
	{
		CLEAR_FLAG(op, FLAG_APPLIED);
		change_abil(op->env, op);
		fix_player(op->env);
	}

	remove_ob(op);
	object_destroy(op);
}

/**
 * Initialize the blindness type object methods. */
void object_type_init_blindness(void)
{
	object_type_methods[BLINDNESS].process_func = process_func;
}