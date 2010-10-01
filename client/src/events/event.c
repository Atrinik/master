/************************************************************************
*            Atrinik, a Multiplayer Online Role Playing Game            *
*                                                                       *
*    Copyright (C) 2009-2010 Alex Tokar and Atrinik Development Team    *
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
 * This file controls various event functions, like character mouse movement,
 * parsing macro keys etc. */

#include "include.h"

int  old_mouse_y = 0;

int KeyScanFlag;
int cursor_type = 0;

uint32 MouseState = IDLE;
int MouseEvent = 0; /* do not set to IDLE, EVER */
int itemExamined = 0;

/* src:  (if != DRAG_GET_STATUS) set actual dragging source.
 * item: (if != NULL) set actual dragging item.
 * ret:  the actual dragging source. */
int draggingInvItem(int src)
{
	static int drag_src = DRAG_NONE;

	if (src != DRAG_GET_STATUS)
		drag_src = src;

	return drag_src;
}

void resize_window(int width, int height)
{
	options.resolution_x = width;
	options.resolution_y = height;

	Screensize->x = width;
	Screensize->y = height;
}

/**
 * Poll input device like mouse, keys, etc.
 * @return 1 if the the quit key was pressed, 0 otherwise */
int Event_PollInputDevice()
{
	SDL_Event event;
	int x, y, done = 0;
	static int active_scrollbar = 0;
	/* only print text once per dnd */
	static int itemExamined  = 0;
	static Uint32 Ticks= 0;
	Uint32 videoflags = get_video_flags();

	if ((SDL_GetTicks() - Ticks > 10) || !Ticks)
	{
		Ticks = SDL_GetTicks();
		if (GameStatus >= GAME_STATUS_PLAY)
		{
			if (InputStringFlag && cpl.input_mode == INPUT_MODE_NUMBER)
				mouse_InputNumber();
		}
	}

	while (SDL_PollEvent(&event))
	{
		static int old_mouse_y = 0, old_map_mouse_x = 0, old_map_mouse_y = 0;

		x = event.motion.x;
		y = event.motion.y;

		switch (event.type)
		{
			case SDL_VIDEORESIZE:
				if ((ScreenSurface = SDL_SetVideoMode(event.resize.w, event.resize.h, options.used_video_bpp, videoflags)) == NULL)
				{
					LOG(llevError, "Unable to grab surface after resize event: %s\n", SDL_GetError());
				}

				resize_window(event.resize.w, event.resize.h);
				options.resolution = 0;
				break;

			case SDL_MOUSEBUTTONUP:
				/* Get the mouse state and set an event (event removed at end of main loop) */
				if (event.button.button == SDL_BUTTON_LEFT)
					MouseEvent = LB_UP;
				else if (event.button.button == SDL_BUTTON_MIDDLE)
					MouseEvent = MB_UP;
				else if (event.button.button == SDL_BUTTON_RIGHT)
					MouseEvent = RB_UP;
				else
					MouseEvent = IDLE;

				/* No button is down */
				MouseState = IDLE;

				cursor_type = 0;

				if (GameStatus < GAME_STATUS_PLAY)
					break;

				mb_clicked = 0;
				active_scrollbar = 0;

				if (cur_widget[SHOP_ID]->show && draggingInvItem(DRAG_GET_STATUS) > DRAG_IWIN_BELOW)
				{
					if (shop_put_item(x, y))
					{
						break;
					}
				}

				/* Widget has higher priority than anything below, exept menus
				 * so break if we had a widget event */
				if (widget_event_mouseup(x,y, &event))
				{
					/* NOTE: Place here special handlings that have to be done, even if a widget owns it */

					/* Sanity handling */
					draggingInvItem(DRAG_NONE);

					/* Ready for next item */
					itemExamined = 0;

					break;
				}

				if (InputStringFlag && cpl.input_mode == INPUT_MODE_NUMBER)
					break;

				/* Only drop to ground has to be handled, the rest do the widget handlers */
				if (draggingInvItem(DRAG_GET_STATUS) > DRAG_IWIN_BELOW)
				{
					/* KEYFUNC_APPLY and KEYFUNC_DROP works only if cpl.inventory_win = IWIN_INV. The tag must
					 * be placed in cpl.win_inv_tag. So we do this and after DnD we restore the old values. */
					int old_inv_win = cpl.inventory_win;
					int old_inv_tag = cpl.win_inv_tag;
					cpl.inventory_win = IWIN_INV;

					/* Drop to ground */
					if (mouse_to_tile_coords(x, y, NULL, NULL))
					{
						if (draggingInvItem(DRAG_GET_STATUS) != DRAG_QUICKSLOT_SPELL)
						{
							process_macro_keys(KEYFUNC_DROP, 0);
						}
					}

					cpl.inventory_win = old_inv_win;
					cpl.win_inv_tag = old_inv_tag;
				}

				draggingInvItem(DRAG_NONE);

				/* Ready for next item */
				itemExamined  = 0;
				break;

			case SDL_MOUSEMOTION:
			{
				int map_mouse_x, map_mouse_y;

				mb_clicked = 0;

				if (GameStatus < GAME_STATUS_PLAY)
				{
					break;
				}

				x_custom_cursor = x;
				y_custom_cursor = y;

				/* We have to break now when menu is active - menu is higher priority than any widget! */
				if (cpl.menustatus != MENU_NO)
				{
					break;
				}

				if (mouse_to_tile_coords(x, y, &map_mouse_x, &map_mouse_y) && (map_mouse_x != old_map_mouse_x || map_mouse_y != old_map_mouse_y))
				{
					map_redraw_flag = 1;
					old_map_mouse_x = map_mouse_x;
					old_map_mouse_y = map_mouse_y;
				}

				if (widget_event_mousemv(x, y, &event))
				{
					/* NOTE: place here special handlings that have to be done, even if a widget owns it */

					break;
				}

				break;
			}

			case SDL_MOUSEBUTTONDOWN:
			{
				int tx, ty;

				/* get the mouse state and set an event (event removed at end of main loop) */
				if (event.button.button == SDL_BUTTON_LEFT)
					MouseEvent = MouseState = LB_DN;
				else if (event.button.button == SDL_BUTTON_MIDDLE)
					MouseEvent = MouseState = MB_DN;
				else if (event.button.button == SDL_BUTTON_RIGHT)
					MouseEvent = MouseState = RB_DN;
				else
					MouseEvent = MouseState = IDLE;

				mb_clicked = 1;

				if (GameStatus == GAME_STATUS_WAITLOOP)
				{
					metaserver_mouse(&event);
					break;
				}

				if (GameStatus < GAME_STATUS_PLAY)
					break;

				/* For party GUI, we call the mouse event function that handles scrolling, selecting, etc. */
				if (cpl.menustatus == MENU_PARTY)
				{
					if (event.button.button == 4 || event.button.button == 5 || event.button.button == SDL_BUTTON_LEFT)
					{
						gui_party_interface_mouse(&event);
						break;
					}
				}

				/* If this is book GUI, handle the click */
				if (cpl.menustatus == MENU_BOOK && gui_interface_book && event.button.button == SDL_BUTTON_LEFT)
				{
					book_gui_handle_mouse(x, y);
				}

				/* Beyond here only when no menu is active. */
				if (cpl.menustatus != MENU_NO)
				{
					break;
				}

				/* Widget System */
				if (widget_event_mousedn(x, y, &event))
				{
					/* NOTE: Place here special handlings that have to be done, even if a widget owns it */

					break;
				}

				/* Mouse in play field */
				if (mouse_to_tile_coords(x, y, &tx, &ty))
				{
					uint8 state = SDL_GetMouseState(NULL, NULL);

					cpl.inventory_win = IWIN_BELOW;

					/* Targeting */
					if (state == SDL_BUTTON(SDL_BUTTON_RIGHT))
					{
						char tbuf[32];

						snprintf(tbuf, sizeof(tbuf), "/target !%d %d", tx - MAP_MAX_SIZE / 2, ty - MAP_MAX_SIZE / 2);
						send_command(tbuf);
					}
					/* Running */
					else if (state == SDL_BUTTON(SDL_BUTTON_LEFT))
					{
						SockList sl;
						uint8 buf[HUGE_BUF];

						sl.buf = buf;
						sl.len = 0;
						SockList_AddString(&sl, "mp ");
						SockList_AddChar(&sl, tx);
						SockList_AddChar(&sl, ty);
						send_socklist(sl);
					}

					break;
				}

				break;
			}

			case SDL_KEYUP:
			case SDL_KEYDOWN:
				done = event_poll_key(&event);
				break;

			case SDL_QUIT:
				done = 1;
				break;

			default:
				break;
		}
		old_mouse_y = y;
	}

	/* OK, now we have processed all real events.
	 * Now run through the list of keybinds and control repeat time value.
	 * If the key is still marked as pressed in our keyboard mirror table,
	 * and the time this is pressed <= keybind press value + repeat value,
	 * we assume a repeat flag is true.
	 * Sadly, SDL doesn't have a tick count inside the event messages, which
	 * means the tick value when the event really was triggered. So, the
	 * client can't simulate the buffered "rythm" of the key pressings when
	 * the client lags. */
	key_repeat();

	return done;
}
