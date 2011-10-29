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
 * @ref BEACON "Beacon" related code.
 *
 * @author Alex Tokar */

#include <global.h>

/**
 * One beacon. */
typedef struct beacon_struct
{
	/** The beacon object. */
	object *ob;

	/** Hash handle. */
	UT_hash_handle hh;
} beacon_struct;

/** Beacons hashtable. */
static beacon_struct *beacons = NULL;

/**
 * Add a beacon to ::beacons_list.
 * @param ob Beacon to add. */
void beacon_add(object *ob)
{
	beacon_struct *beacon;

	beacon = malloc(sizeof(beacon_struct));
	beacon->ob = ob;
	HASH_ADD(hh, beacons, ob->name, sizeof(shstr *), beacon);
}

/**
 * Remove a beacon from ::beacons_list.
 * @param ob Beacon to remove. */
void beacon_remove(object *ob)
{
	beacon_struct *beacon;

	HASH_FIND(hh, beacons, &ob->name, sizeof(shstr *), beacon);

	if (beacon)
	{
		HASH_DEL(beacons, beacon);
		free(beacon);
	}
	else
	{
		LOG(llevBug, "beacon_remove(): Could not remove beacon %s from hashtable.\n", ob->name);
	}
}

/**
 * Locate a beacon object in ::beacons_list.
 * @param name Name of the beacon to locate. Must be a shared string.
 * @return The beacon object if found, NULL otherwise. */
object *beacon_locate(shstr *name)
{
	beacon_struct *beacon;

	HASH_FIND(hh, beacons, &name, sizeof(shstr *), beacon);

	if (beacon)
	{
		return beacon->ob;
	}

	return NULL;
}
