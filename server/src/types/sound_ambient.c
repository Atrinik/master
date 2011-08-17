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
 * Handles @ref SOUND_AMBIENT "ambient sound effect" objects. */

#include <global.h>

/**
 * Initialize ambient sound effect object.
 * @param ob The object to initialize. */
void sound_ambient_init(object *ob)
{
	MapSpace *msp;

	/* Must be on map... */
	if (!ob->map)
	{
		LOG(llevBug, "Ambient sound effect object not on map.\n");
		return;
	}

	if (!ob->race || *ob->race == '\0')
	{
		LOG(llevBug, "Ambient sound effect object is missing sound effect filename.\n");
		return;
	}

	msp = GET_MAP_SPACE_PTR(ob->map, ob->x, ob->y);
	msp->sound_ambient = ob;
	msp->sound_ambient_count = ob->count;
}
