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
 * Interface header file. */

#ifndef INTERFACE_H
#define INTERFACE_H

/**
 * Interface data. */
typedef struct interface_struct
{
	/** Message contents. */
	char *message;

	/** Title text. */
	char *title;

	/** Icon ID. */
	int icon;

	int font;

	/** Array of the shortcut-supporting links. */
	UT_array *links;

	/** Whether to redraw the interface. */
	uint8 redraw;

	/** Whether the interface should be destroyed. */
	uint8 destroy;

	uint32 scroll_offset;

	uint32 num_lines;

	/** Scrollbar. */
	scrollbar_struct scrollbar;
} interface_struct;

/**
 * @defgroup CMD_INTERFACE_xxx Interface command types
 * Interface command types.
 *@{*/
/** Text; the NPC message contents. */
#define CMD_INTERFACE_TEXT 0
/**
 * Link, follows the actual text, but is a special command in order to
 * support link shortcuts. */
#define CMD_INTERFACE_LINK 1
/** Icon; the image in the upper left corner square. */
#define CMD_INTERFACE_ICON 2
/** Title; text next to the icon. */
#define CMD_INTERFACE_TITLE 3
/**
 * If found in the command, will open the console with any text followed
 * by this. */
#define CMD_INTERFACE_INPUT 4
/*@}*/

/**
 * @defgroup INTERFACE_ICON_xxx Interface icon coords
 * Interface icon coordinates.
 *@{*/
/** X position of the icon. */
#define INTERFACE_ICON_STARTX 8
/** Y position of the icon. */
#define INTERFACE_ICON_STARTY 8
/** Width of the icon. */
#define INTERFACE_ICON_WIDTH 55
/** Height of the icon. */
#define INTERFACE_ICON_HEIGHT 55
/*@}*/

/**
 * @defgroup INTERFACE_TEXT_xxx Interface text coords
 * Interface text coordinates.
 *@{*/
/** X position of the text. */
#define INTERFACE_TEXT_STARTX 10
/** Y position of the text. */
#define INTERFACE_TEXT_STARTY 73
/** Maximum width of the text. */
#define INTERFACE_TEXT_WIDTH 412
/** Maximum height of the text. */
#define INTERFACE_TEXT_HEIGHT 430
/*@}*/

/**
 * @defgroup INTERFACE_TITLE_xxx Interface title coords
 * Interface title coordinates.
 *@{*/
/** X position of the title. */
#define INTERFACE_TITLE_STARTX 80
/** Y position of the title. */
#define INTERFACE_TITLE_STARTY 38
/** Maximum width of the title. */
#define INTERFACE_TITLE_WIDTH 350
/** Maximum height of the title. */
#define INTERFACE_TITLE_HEIGHT 22
/*@}*/

/**
 * @defgroup INTERFACE_BUTTON_xxx Interface button coords
 * Interface button coordinates.
 *@{*/
/** X position of the 'hello' button. */
#define INTERFACE_BUTTON_HELLO_STARTX 20
/** Y position of the 'hello' button. */
#define INTERFACE_BUTTON_HELLO_STARTY 520

/** X position of the 'close' button. */
#define INTERFACE_BUTTON_CLOSE_STARTX 375
/** Y position of the 'close' button. */
#define INTERFACE_BUTTON_CLOSE_STARTY 520
/*@}*/

#endif
