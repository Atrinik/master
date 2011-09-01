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
 * Implements the interface used by NPCs and the like. */

#include <global.h>

/**
 * The current interface data. */
static interface_struct *interface_data = NULL;

/**
 * Destroy the interface data, if any. */
static void interface_destroy(void)
{
	if (!interface_data)
	{
		return;
	}

	free(interface_data->message);
	free(interface_data->title);

	if (interface_data->icon)
	{
		free(interface_data->icon);
	}

	utarray_free(interface_data->links);
	free(interface_data);

	interface_data = NULL;
}

/** @copydoc text_anchor_handle_func */
static int text_anchor_handle(const char *anchor_action, const char *buf, size_t len)
{
	(void) len;

	if (anchor_action[0] == '\0' && buf[0] != '/')
	{
		StringBuffer *sb = stringbuffer_new();
		char *cp;

		stringbuffer_append_printf(sb, "/t_tell %s", buf);
		cp = stringbuffer_finish(sb);
		send_command_check(cp);
		free(cp);

		return 1;
	}
	else if (!strcmp(anchor_action, "close"))
	{
		interface_data->destroy = 1;
	}

	return 0;
}

static void interface_execute_link(size_t link_id)
{
	char **p;
	text_blit_info info;

	p = (char **) utarray_eltptr(interface_data->links, link_id);

	if (!p)
	{
		return;
	}

	text_anchor_parse(&info, *p);
	text_set_anchor_handle(text_anchor_handle);
	text_anchor_execute(&info);
	text_set_anchor_handle(NULL);
}

/** @copydoc popup_struct::draw_func */
static int popup_draw_func(popup_struct *popup)
{
	if (interface_data->redraw)
	{
		_BLTFX bltfx;
		SDL_Rect box;

		bltfx.surface = popup->surface;
		bltfx.flags = 0;
		bltfx.alpha = 0;
		sprite_blt(Bitmaps[popup->bitmap_id], 0, 0, NULL, &bltfx);

		if (interface_data->icon)
		{
			string_blt_format(popup->surface, FONT_ARIAL10, INTERFACE_ICON_STARTX, INTERFACE_ICON_STARTY, COLOR_WHITE, TEXT_MARKUP, NULL, "<icon=%s %d %d>", interface_data->icon, INTERFACE_ICON_WIDTH, INTERFACE_ICON_HEIGHT);
		}

		text_offset_set(popup->x, popup->y);
		box.w = INTERFACE_TITLE_WIDTH;
		box.h = FONT_HEIGHT(FONT_SERIF14);
		string_blt(popup->surface, FONT_SERIF14, interface_data->title, INTERFACE_TITLE_STARTX, INTERFACE_TITLE_STARTY + INTERFACE_TITLE_HEIGHT / 2 - box.h / 2, COLOR_HGOLD, TEXT_MARKUP | TEXT_WORD_WRAP, &box);

		box.w = INTERFACE_TEXT_WIDTH;
		box.h = INTERFACE_TEXT_HEIGHT;
		box.x = 0;
		box.y = interface_data->scroll_offset;
		text_set_anchor_handle(text_anchor_handle);
		string_blt(popup->surface, interface_data->font, interface_data->message, INTERFACE_TEXT_STARTX, INTERFACE_TEXT_STARTY, COLOR_WHITE, TEXT_WORD_WRAP | TEXT_MARKUP | TEXT_LINES_SKIP, &box);
		text_set_anchor_handle(NULL);
		text_offset_reset();

		interface_data->redraw = 0;
	}

	return !interface_data->destroy;
}

/** @copydoc popup_struct::draw_func_post */
static int popup_draw_func_post(popup_struct *popup)
{
	scrollbar_render(&interface_data->scrollbar, ScreenSurface, popup->x + 432, popup->y + 71);

	if (button_show(BITMAP_BUTTON_LARGE, BITMAP_BUTTON_LARGE_HOVER, BITMAP_BUTTON_LARGE_DOWN, popup->x + INTERFACE_BUTTON_HELLO_STARTX, popup->y + INTERFACE_BUTTON_HELLO_STARTY, "Hello", FONT_ARIAL13, COLOR_WHITE, COLOR_BLACK, COLOR_HGOLD, COLOR_BLACK, 0))
	{
		send_command_check("/t_tell hello");
	}

	if (button_show(BITMAP_BUTTON_LARGE, BITMAP_BUTTON_LARGE_HOVER, BITMAP_BUTTON_LARGE_DOWN, popup->x + INTERFACE_BUTTON_CLOSE_STARTX, popup->y + INTERFACE_BUTTON_CLOSE_STARTY, "Close", FONT_ARIAL13, COLOR_WHITE, COLOR_BLACK, COLOR_HGOLD, COLOR_BLACK, 0))
	{
		return 0;
	}

	if (text_input_string_flag)
	{
		SDL_Rect dst;

		dst.x = popup->x + popup->surface->w / 2 - Bitmaps[BITMAP_LOGIN_INP]->bitmap->w / 2;
		dst.y = popup->y + popup->surface->h - Bitmaps[BITMAP_LOGIN_INP]->bitmap->h - 15;
		dst.w = Bitmaps[BITMAP_LOGIN_INP]->bitmap->w;
		dst.h = Bitmaps[BITMAP_LOGIN_INP]->bitmap->h;
		text_input_show(ScreenSurface, dst.x, dst.y, FONT_ARIAL11, text_input_string, COLOR_WHITE, 0, BITMAP_LOGIN_INP, NULL);
	}

	sprite_blt(Bitmaps[BITMAP_INTERFACE_BORDER], popup->x, popup->y, NULL, NULL);
	return 1;
}

/** @copydoc popup_struct::destroy_callback_func */
static int popup_destroy_callback(popup_struct *popup)
{
	(void) popup;
	interface_destroy();
	text_input_close();
	return 1;
}

/** @copydoc popup_button::event_func */
static int popup_button_event_func(popup_button *button)
{
	(void) button;
	help_show("npc interface");
	return 1;
}

/** @copydoc popup_struct::event_func */
static int popup_event_func(popup_struct *popup, SDL_Event *event)
{
	if (scrollbar_event(&interface_data->scrollbar, event))
	{
		return 1;
	}
	else if (event->type == SDL_KEYDOWN)
	{
		if (text_input_string_flag)
		{
			if (event->key.keysym.sym == SDLK_RETURN || event->key.keysym.sym == SDLK_KP_ENTER || event->key.keysym.sym == SDLK_TAB)
			{
				StringBuffer *sb = stringbuffer_new();
				char *cp;

				stringbuffer_append_printf(sb, "/t_tell %s", text_input_string);
				cp = stringbuffer_finish(sb);
				send_command_check(cp);
				free(cp);
				text_input_close();
				return 1;
			}
			else if (event->key.keysym.sym == SDLK_ESCAPE)
			{
				text_input_close();
				return 1;
			}
			else if (text_input_handle(&event->key))
			{
				return 1;
			}
		}

		switch (event->key.keysym.sym)
		{
			case SDLK_DOWN:
				scrollbar_scroll_adjust(&interface_data->scrollbar, 1);
				return 1;

			case SDLK_UP:
				scrollbar_scroll_adjust(&interface_data->scrollbar, -1);
				return 1;

			case SDLK_PAGEDOWN:
				scrollbar_scroll_adjust(&interface_data->scrollbar, interface_data->scrollbar.max_lines);
				return 1;

			case SDLK_PAGEUP:
				scrollbar_scroll_adjust(&interface_data->scrollbar, -interface_data->scrollbar.max_lines);
				return 1;

			case SDLK_1:
			case SDLK_2:
			case SDLK_3:
			case SDLK_4:
			case SDLK_5:
			case SDLK_6:
			case SDLK_7:
			case SDLK_8:
			case SDLK_9:
				interface_execute_link(event->key.keysym.sym - SDLK_1);
				return 1;

			case SDLK_KP1:
			case SDLK_KP2:
			case SDLK_KP3:
			case SDLK_KP4:
			case SDLK_KP5:
			case SDLK_KP6:
			case SDLK_KP7:
			case SDLK_KP8:
			case SDLK_KP9:
				interface_execute_link(event->key.keysym.sym - SDLK_KP1);
				return 1;

			case SDLK_RETURN:
			case SDLK_KP_ENTER:
				text_input_open(255);
				return 1;

			default:
				break;
		}
	}
	else if (event->type == SDL_MOUSEBUTTONDOWN && event->motion.x >= popup->x && event->motion.x < popup->x + Bitmaps[popup->bitmap_id]->bitmap->w && event->motion.y >= popup->y && event->motion.y < popup->y + Bitmaps[popup->bitmap_id]->bitmap->h)
	{
		if (event->button.button == SDL_BUTTON_WHEELDOWN)
		{
			scrollbar_scroll_adjust(&interface_data->scrollbar, 1);
			return 1;
		}
		else if (event->button.button == SDL_BUTTON_WHEELUP)
		{
			scrollbar_scroll_adjust(&interface_data->scrollbar, -1);
			return 1;
		}
		else if (event->button.button == SDL_BUTTON_LEFT)
		{
			interface_data->redraw = 1;
		}
	}

	return -1;
}

/**
 * Handle interface binary command.
 * @param data Data to parse.
 * @param len Length of 'data'. */
void cmd_interface(uint8 *data, int len)
{
	int pos = 0;
	uint8 text_input = 0;
	char type, text_input_content[HUGE_BUF];
	StringBuffer *sb_message;
	size_t links_len, i;
	SDL_Rect box;

	if (!interface_data)
	{
		popup_struct *popup;

		popup = popup_create(BITMAP_INTERFACE);
		popup->draw_func = popup_draw_func;
		popup->draw_func_post = popup_draw_func_post;
		popup->destroy_callback_func = popup_destroy_callback;
		popup->event_func = popup_event_func;
		popup->disable_bitmap_blit = 1;

		popup->button_left.event_func = popup_button_event_func;
		popup->button_left.x = 380;
		popup->button_left.y = 4;
		popup_button_set_text(&popup->button_left, "?");

		popup->button_right.x = 411;
		popup->button_right.y = 4;
	}

	/* Make sure text input is not open. */
	text_input_close();

	/* Destroy previous interface data. */
	interface_destroy();

	/* Create new interface. */
	interface_data = calloc(1, sizeof(*interface_data));
	interface_data->redraw = 1;
	interface_data->font = FONT_ARIAL11;
	utarray_new(interface_data->links, &ut_str_icd);
	sb_message = stringbuffer_new();

	/* Parse the data. */
	while (pos < len)
	{
		type = data[pos++];

		switch (type)
		{
			case CMD_INTERFACE_TEXT:
			{
				char message[HUGE_BUF * 8];

				GetString_String(data, &pos, message, sizeof(message));
				stringbuffer_append_string(sb_message, message);
				break;
			}

			case CMD_INTERFACE_LINK:
			{
				char interface_link[HUGE_BUF], *cp;

				GetString_String(data, &pos, interface_link, sizeof(interface_link));
				cp = interface_link;
				utarray_push_back(interface_data->links, &cp);
				break;
			}

			case CMD_INTERFACE_ICON:
			{
				char icon[MAX_BUF];

				GetString_String(data, &pos, icon, sizeof(icon));
				interface_data->icon = strdup(icon);
				break;
			}

			case CMD_INTERFACE_TITLE:
			{
				char title[HUGE_BUF];

				GetString_String(data, &pos, title, sizeof(title));
				interface_data->title = strdup(title);
				break;
			}

			case CMD_INTERFACE_INPUT:
				text_input = 1;
				GetString_String(data, &pos, text_input_content, sizeof(text_input_content));
				break;

			default:
				break;
		}
	}

	if (text_input)
	{
		text_input_open(255);
		text_input_add_string(text_input_content);
	}

	links_len = utarray_len(interface_data->links);

	if (links_len)
	{
		stringbuffer_append_string(sb_message, "\n");
	}

	for (i = 0; i < links_len; i++)
	{
		stringbuffer_append_string(sb_message, "\n");

		if (i < 9)
		{
			stringbuffer_append_printf(sb_message, "<c=#AF7817>[%"FMT64U"]</c> ", (uint64) i + 1);
		}

		stringbuffer_append_string(sb_message, *((char **) utarray_eltptr(interface_data->links, i)));
	}

	interface_data->message = stringbuffer_finish(sb_message);

	box.w = INTERFACE_TEXT_WIDTH;
	box.h = INTERFACE_TEXT_HEIGHT;
	string_blt(NULL, interface_data->font, interface_data->message, INTERFACE_TEXT_STARTX, INTERFACE_TEXT_STARTY, COLOR_WHITE, TEXT_WORD_WRAP | TEXT_MARKUP | TEXT_LINES_CALC, &box);
	interface_data->num_lines = box.h;

	scrollbar_create(&interface_data->scrollbar, 11, 434, &interface_data->scroll_offset, &interface_data->num_lines, box.y);
	interface_data->scrollbar.redraw = &interface_data->redraw;
}

/**
 * Redraw the interface. */
void interface_redraw(void)
{
	if (interface_data)
	{
		interface_data->redraw = 1;
	}
}
