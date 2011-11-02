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
 * Handles code for @ref BUTTON "button".
 *
 * @author Alex Tokar */

#include <global.h>

/** @copydoc object_methods::move_on_func */
static int move_on_func(object *op, object *victim, object *originator, int state)
{
	(void) victim;
	(void) originator;

	connection_trigger_button(op, state);

	return OBJECT_METHOD_OK;
}

/** @copydoc object_methods::trigger_func */
static int trigger_func(object *op, object *cause, int state)
{
	(void) cause;
	op->value = state;
	return OBJECT_METHOD_OK;
}

/** @copydoc object_methods::trigger_button_func */
static int trigger_button_func(object *op, object *cause, int state)
{
	object *tmp, *head;
	sint32 total;

	(void) cause;
	(void) state;

	total = 0;

	for (tmp = GET_MAP_OB(op->map, op->x, op->y); tmp; tmp = tmp->above)
	{
		head = HEAD(tmp);

		if (head != op && (QUERY_FLAG(head, FLAG_FLYING) ? QUERY_FLAG(op, FLAG_FLY_ON) : QUERY_FLAG(op, FLAG_WALK_ON)))
		{
			total += head->weight * MAX(1, (sint32) head->nrof) + head->carrying;
		}
	}

	op->value = total >= op->weight;

	return OBJECT_METHOD_OK;
}

/**
 * Initialize the button type object methods. */
void object_type_init_button(void)
{
	object_type_methods[BUTTON].move_on_func = move_on_func;
	object_type_methods[BUTTON].trigger_func = trigger_func;
	object_type_methods[BUTTON].trigger_button_func = trigger_button_func;
}