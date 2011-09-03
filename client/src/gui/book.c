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
 * Book GUI related code. */

#include <global.h>

/** The book's content. */
static char *book_content = NULL;
/** Name of the book. */
static char book_name[HUGE_BUF];
/** Number of lines in the book. */
static uint32 book_lines = 0;
/** Number of lines at the end. */
static uint32 book_scroll_lines = 0;
/** Lines scrolled. */
static uint32 book_scroll = 0;
/** Is it time to redraw? */
static uint8 redraw = 1;
/** Help history - used for the 'Back' button. */
UT_array *book_help_history = NULL;
/** Whether the help history is enabled for this book GUI. */
static uint8 book_help_history_enabled = 0;
/** Scrollbar in the book GUI. */
static scrollbar_struct scrollbar;

/**
 * Change the book's displayed name.
 * @param name The name to change to.
 * @param len Length of the name. */
void book_name_change(const char *name, size_t len)
{
	len = MIN(sizeof(book_name) - 1, len);
	strncpy(book_name, name, len);
	book_name[len] = '\0';
}

/** @copydoc popup_struct::draw_func */
static int popup_draw_func(popup_struct *popup)
{
	if (redraw)
	{
		_BLTFX bltfx;
		SDL_Rect box;

		bltfx.surface = popup->surface;
		bltfx.flags = 0;
		bltfx.alpha = 0;
		sprite_blt(Bitmaps[popup->bitmap_id], 0, 0, NULL, &bltfx);

		redraw = 0;

		/* Draw the book name. */
		box.w = BOOK_TITLE_WIDTH;
		box.h = BOOK_TITLE_HEIGHT;
		text_offset_set(popup->x, popup->y);
		string_blt(popup->surface, FONT_SERIF16, book_name, BOOK_TITLE_STARTX, BOOK_TITLE_STARTY, COLOR_HGOLD, TEXT_WORD_WRAP | TEXT_MARKUP | TEXT_ALIGN_CENTER, &box);

		/* Draw the content. */
		box.w = BOOK_TEXT_WIDTH;
		box.h = BOOK_TEXT_HEIGHT;
		box.y = book_scroll;
		text_color_set(0, 0, 255);
		string_blt(popup->surface, FONT_ARIAL11, book_content, BOOK_TEXT_STARTX, BOOK_TEXT_STARTY, COLOR_BLACK, TEXT_WORD_WRAP | TEXT_MARKUP | TEXT_LINES_SKIP, &box);
		text_offset_reset();
	}

	return 1;
}

/** @copydoc popup_struct::draw_func_post */
static int popup_draw_func_post(popup_struct *popup)
{
	scrollbar_render(&scrollbar, ScreenSurface, popup->x + BOOK_SCROLLBAR_STARTX, popup->y + BOOK_SCROLLBAR_STARTY);

	if (book_help_history_enabled)
	{
		button_tooltip(&popup->button_left.button, FONT_ARIAL10, "Go back");
	}

	sprite_blt(Bitmaps[BITMAP_BOOK_BORDER], popup->x, popup->y, NULL, NULL);

	return 1;
}

/** @copydoc popup_button::event_func */
static int popup_button_event_func(popup_button *button)
{
	size_t len;

	(void) button;

	len = utarray_len(book_help_history);

	if (len >= 2)
	{
		size_t pos;
		char **p;

		pos = len - 2;
		p = (char **) utarray_eltptr(book_help_history, pos);

		if (p)
		{
			help_show(*p);
			utarray_erase(book_help_history, pos, 2);
		}
	}
	else
	{
		utarray_clear(book_help_history);
		help_show("main");
	}

	return 1;
}

/** @copydoc popup_struct::event_func */
static int popup_event_func(popup_struct *popup, SDL_Event *event)
{
	if (scrollbar_event(&scrollbar, event))
	{
		return 1;
	}

	/* Mouse event and the mouse is inside the book. */
	if (event->type == SDL_MOUSEBUTTONDOWN && event->motion.x >= popup->x && event->motion.x < popup->x + popup->surface->w && event->motion.y >= popup->y && event->motion.y < popup->y + popup->surface->h)
	{
		/* Scroll the book. */
		if (event->button.button == SDL_BUTTON_WHEELDOWN)
		{
			scrollbar_scroll_adjust(&scrollbar, 1);
			return 1;
		}
		else if (event->button.button == SDL_BUTTON_WHEELUP)
		{
			scrollbar_scroll_adjust(&scrollbar, -1);
			return 1;
		}
		else if (event->button.button == SDL_BUTTON_LEFT)
		{
			redraw = 1;
		}
	}
	else if (event->type == SDL_KEYDOWN)
	{
		/* Scrolling. */
		if (event->key.keysym.sym == SDLK_DOWN)
		{
			scrollbar_scroll_adjust(&scrollbar, 1);
			return 1;
		}
		else if (event->key.keysym.sym == SDLK_UP)
		{
			scrollbar_scroll_adjust(&scrollbar, -1);
			return 1;
		}
		else if (event->key.keysym.sym == SDLK_PAGEDOWN)
		{
			scrollbar_scroll_adjust(&scrollbar, book_scroll_lines);
			return 1;
		}
		else if (event->key.keysym.sym == SDLK_PAGEUP)
		{
			scrollbar_scroll_adjust(&scrollbar, -book_scroll_lines);
			return 1;
		}
	}

	return -1;
}

/** @copydoc popup_struct::destroy_callback_func */
static int popup_destroy_callback(popup_struct *popup)
{
	(void) popup;

	if (book_help_history)
	{
		utarray_free(book_help_history);
		book_help_history = NULL;
	}

	book_help_history_enabled = 0;
	return 1;
}

/**
 * Load the book interface.
 * @param data Book's content.
 * @param len Length of 'data'. */
void book_load(const char *data, int len)
{
	SDL_Rect box;
	int pos;

	/* Nothing to do. */
	if (!data || !len)
	{
		return;
	}

	/* Free old book data and reset the values. */
	if (book_content)
	{
		free(book_content);
		book_lines = 0;
		book_scroll_lines = 0;
		book_scroll = 0;
	}

	/* Store the data. */
	book_content = strdup(data);
	book_name_change("Book", 4);

	/* Strip trailing newlines. */
	for (pos = len - 1; pos >= 0; pos--)
	{
		if (book_content[pos] != '\n')
		{
			break;
		}

		book_content[pos] = '\0';
	}

	/* No data... */
	if (book_content[0] == '\0')
	{
		return;
	}

	/* Calculate the line numbers. */
	box.w = BOOK_TEXT_WIDTH;
	box.h = BOOK_TEXT_HEIGHT;
	string_blt(NULL, FONT_ARIAL11, book_content, BOOK_TEXT_STARTX, BOOK_TEXT_STARTY, COLOR_WHITE, TEXT_WORD_WRAP | TEXT_MARKUP | TEXT_LINES_CALC, &box);
	book_lines = box.h;
	book_scroll_lines = box.y;
	redraw = 1;

	/* Create the book popup if it doesn't exist yet. */
	if (!popup_get_head() || popup_get_head()->bitmap_id != BITMAP_BOOK)
	{
		popup_struct *popup;

		popup = popup_create(BITMAP_BOOK);
		popup->draw_func = popup_draw_func;
		popup->draw_func_post = popup_draw_func_post;
		popup->event_func = popup_event_func;
		popup->destroy_callback_func = popup_destroy_callback;
		popup->disable_bitmap_blit = 1;

		popup->button_left.x = 25;
		popup->button_left.y = 25;

		if (book_help_history_enabled)
		{
			popup->button_left.event_func = popup_button_event_func;
			popup_button_set_text(&popup->button_left, "<");
		}

		popup->button_right.x = 649;
		popup->button_right.y = 25;
	}

	scrollbar_create(&scrollbar, BOOK_SCROLLBAR_WIDTH, BOOK_SCROLLBAR_HEIGHT, &book_scroll, &book_lines, book_scroll_lines);
	scrollbar.redraw = &redraw;
}

/**
 * Redraw the book GUI. */
void book_redraw(void)
{
	redraw = 1;
}

/**
 * Enable book help history. */
void book_add_help_history(const char *name)
{
	if (!book_help_history_enabled)
	{
		book_help_history_enabled = 1;
		utarray_new(book_help_history, &ut_str_icd);
	}

	utarray_push_back(book_help_history, &name);
}
