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
 * This file is the one and only DRAWINFO output module.
 *
 * All player communication using drawinfo is handled here - except the
 * few messages we send to the client using DRAWINFO before we had setup
 * any player structure - for example when an outdated client logs in and
 * we send "update your client" directly to the info windows.
 *
 * But if the player is logged in, all DRAWINFO are generated here. */

#include <global.h>
#include <stdarg.h>

#define DRAW_INFO_FORMAT_CONSTRUCT() \
	char buf[HUGE_BUF]; \
	va_list ap; \
	va_start(ap, format); \
	vsnprintf(buf, sizeof(buf), format, ap); \
	va_end(ap);

/**
 * Draw a message in the text windows for player's client.
 * @param flags Various @ref NDI_xxx "flags". Mostly color, but also some others.
 * @param pl The player object to write the information to - if flags has
 * @ref NDI_ALL, this is unused and can be NULL.
 * @param buf The message to draw. */
void draw_info_full(int flags, const char *color, StringBuffer *sb_capture, object *pl, const char *buf)
{
	/* Handle global messages. */
	if (flags & NDI_ALL)
	{
		player *tmppl;

		for (tmppl = first_player; tmppl; tmppl = tmppl->next)
		{
			draw_info_full((flags & ~NDI_ALL), color, NULL, tmppl->ob, buf);
		}

		return;
	}

	if (!pl || pl->type != PLAYER)
	{
		return;
	}

	if (CONTR(pl)->state != ST_PLAYING)
	{
		return;
	}

	if (sb_capture)
	{
		stringbuffer_append_string(sb_capture, buf);
		stringbuffer_append_string(sb_capture, "\n");
	}
	else
	{
		unsigned char info_string[HUGE_BUF];
		size_t len;
		SockList sl;

		sl.buf = info_string;
		SOCKET_SET_BINARY_CMD(&sl, BINARY_CMD_DRAWINFO2);
		SockList_AddShort(&sl, flags);
		SockList_AddString(&sl, color);
		/* Make sure we don't copy more bytes than available space in the buffer. */
		len = MIN(strlen(buf), sizeof(info_string) - sl.len - 1);
		memcpy((char *) sl.buf + sl.len, buf, len);
		sl.len += len;

		/* Terminate the string. */
		SockList_AddChar(&sl, '\0');
		Send_With_Handling(&CONTR(pl)->socket, &sl);
	}
}

/**
 * Similar to draw_info() but allows to use printf style
 * formatting.
 * @param flags Flags.
 * @param pl Player.
 * @param format Format.
 * @see draw_info() */
void draw_info_full_format(int flags, const char *color, StringBuffer *sb_capture, object *pl, const char *format, ...)
{
	DRAW_INFO_FORMAT_CONSTRUCT();
	draw_info_full(flags, color, sb_capture, pl, buf);
}

void draw_info_flags(int flags, const char *color, object *pl, const char *buf)
{
	draw_info_full(flags, color, NULL, pl, buf);
}

void draw_info_flags_format(int flags, const char *color, object *pl, const char *format, ...)
{
	DRAW_INFO_FORMAT_CONSTRUCT();
	draw_info_full(flags, color, NULL, pl, buf);
}

void draw_info(const char *color, object *pl, const char *buf)
{
	draw_info_full(0, color, NULL, pl, buf);
}

void draw_info_format(const char *color, object *pl, const char *format, ...)
{
	DRAW_INFO_FORMAT_CONSTRUCT();
	draw_info_full(0, color, NULL, pl, buf);
}

/**
 * Writes to everyone on the specified map.
 *
 * Tiled maps will be considered.
 * @param flags A combination of @ref NDI_xxx.
 * @param color One of @ref COLOR_xxx.
 * @param map Map to write on.
 * @param x X position where the message is coming from.
 * @param x Y position where the message is coming from.
 * @param dist Maximum distance from xy a player may be in order to see
 * the message.
 * @param op Will not write to this player.
 * @param op2 Will not write to this player either.
 * @param buf What to write. */
void draw_info_map(int flags, const char *color, mapstruct *map, int x, int y, int dist, object *op, object *op2, const char *buf)
{
	int distance, i;
	object *pl;
	rv_vector rv;

	if (!map || map->in_memory != MAP_IN_MEMORY)
	{
		return;
	}

	if (dist == MAP_INFO_ALL)
	{
		for (pl = map->player_first; pl; pl = CONTR(pl)->map_above)
		{
			if (pl != op && pl != op2)
			{
				draw_info_flags(flags, color, pl, buf);
			}
		}

		return;
	}

	distance = POW2(dist);

	/* Any players on this map? */
	for (pl = map->player_first; pl; pl = CONTR(pl)->map_above)
	{
		if (pl != op && pl != op2 && (POW2(pl->x - x) + POW2(pl->y - y)) <= distance)
		{
			draw_info_flags(flags, color, pl, buf);
		}
	}

	/* Try tiled maps. */
	for (i = 0; i < TILED_MAPS; i++)
	{
		if (map->tile_map[i] && map->tile_map[i]->in_memory == MAP_IN_MEMORY)
		{
			for (pl = map->tile_map[i]->player_first; pl; pl = CONTR(pl)->map_above)
			{
				if (pl != op && pl != op2 && get_rangevector_from_mapcoords(map, x, y, pl->map, pl->x, pl->y, &rv, RV_NO_DISTANCE) && POW2(rv.distance_x) + POW2(rv.distance_y) <= distance)
				{
					draw_info_flags(flags, color, pl, buf);
				}
			}
		}
	}
}

/**
 * Send a socket message, similar to draw_info() but the message will
 * be sent using Write_String_To_Socket() instead.
 *
 * Used for messages that are sent to player before they have finished
 * logging in.
 * @param flags Flags to send.
 * @param ns Socket to send to.
 * @param buf Message to send. */
void send_socket_message(const char *color, socket_struct *ns, const char *buf)
{
	SockList sl;
	unsigned char sockbuf[HUGE_BUF];
	size_t len;

	sl.buf = sockbuf;
	SOCKET_SET_BINARY_CMD(&sl, BINARY_CMD_DRAWINFO);

	SockList_AddString(&sl, color);

	/* Make sure we don't copy more bytes than available space in the buffer. */
	len = MIN(strlen(buf), sizeof(sockbuf) - sl.len - 1);
	memcpy(sl.buf + sl.len, buf, len);
	sl.len += len;

	/* Terminate the string. */
	SockList_AddChar(&sl, '\0');
	Send_With_Handling(ns, &sl);
}
