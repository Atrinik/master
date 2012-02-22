/************************************************************************
*            Atrinik, a Multiplayer Online Role Playing Game            *
*                                                                       *
*    Copyright (C) 2009-2012 Alex Tokar and Atrinik Development Team    *
*                                                                       *
* Fork from Crossfire (Multiplayer game for X-windows).                 *
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
 * Generic lists implementation.
 *
 * @author Alex Tokar */

#include <global.h>

/**
 * Draw a frame in which the rows will be drawn.
 * @param list List to draw the frame for. */
static void list_draw_frame(list_struct *list)
{
	draw_frame(list->surface, list->x + list->frame_offset, LIST_ROWS_START(list), list->width, LIST_ROWS_HEIGHT(list));
}

/**
 * Colorize a row.
 * @param list List.
 * @param row Row number, 0-[max visible rows].
 * @param box Contains base x/y/width/height information to use. */
static void list_row_color(list_struct *list, int row, SDL_Rect box)
{
	if (row & 1)
	{
		SDL_FillRect(list->surface, &box, SDL_MapRGB(list->surface->format, 0x55, 0x55, 0x55));
	}
	else
	{
		SDL_FillRect(list->surface, &box, SDL_MapRGB(list->surface->format, 0x45, 0x45, 0x45));
	}
}

/**
 * Highlight a row (due to mouse being over it).
 * @param list List.
 * @param box Contains base x/y/width/height information to use. */
static void list_row_highlight(list_struct *list, SDL_Rect box)
{
	SDL_FillRect(list->surface, &box, SDL_MapRGB(list->surface->format, 0x00, 0x80, 0x00));
}

/**
 * Color a selected row.
 * @param list List.
 * @param box Contains base x/y/width/height information to use. */
static void list_row_selected(list_struct *list, SDL_Rect box)
{
	SDL_FillRect(list->surface, &box, SDL_MapRGB(list->surface->format, 0x00, 0x00, 0xef));
}

/**
 * Update list's parent X/Y coordinates.
 * @param list The list.
 * @param px Parent X.
 * @param py Parent Y. */
void list_set_parent(list_struct *list, int px, int py)
{
	list->px = px;
	list->py = py;
}

/**
 * Create new list.
 * @param max_rows Maximum number of visible rows to show.
 * @param cols How many columns per row.
 * @param spacing Spacing between column names and the actual rows start.
 * @return The created list. */
list_struct *list_create(uint32 max_rows, uint32 cols, int spacing)
{
	list_struct *list = calloc(1, sizeof(list_struct));

	if (max_rows == 0)
	{
		logger_print(LOG(BUG), "Attempted to create a list with 0 max rows, changing to 1.");
		max_rows = 1;
	}

	/* Store the values. */
	list->max_rows = max_rows;
	list->cols = cols;
	list->spacing = spacing;
	list->font = FONT_SANS10;
	list->surface = ScreenSurface;
	list->focus = 1;

	/* Initialize defaults. */
	list->frame_offset = -2;
	list->header_height = 12;
	list->row_selected = 1;

	/* Generic functions. */
	list->draw_frame_func = list_draw_frame;
	list->row_color_func = list_row_color;
	list->row_highlight_func = list_row_highlight;
	list->row_selected_func = list_row_selected;

	/* Initialize column data. */
	list->col_widths = calloc(1, sizeof(*list->col_widths) * list->cols);
	list->col_spacings = calloc(1, sizeof(*list->col_spacings) * list->cols);
	list->col_names = calloc(1, sizeof(*list->col_names) * list->cols);
	list->col_centered = calloc(1, sizeof(*list->col_centered) * list->cols);

	return list;
}

/**
 * Add text to list.
 * @param list List to add to.
 * @param row Row ID to add to. If it doesn't exist yet, it will be
 * allocated.
 * @param col Column ID.
 * @param str Text to add. */
void list_add(list_struct *list, uint32 row, uint32 col, const char *str)
{
	if (!list)
	{
		return;
	}

	if (col > list->cols)
	{
		logger_print(LOG(BUG), "Attempted to add column #%u, but columns max is %u.", col, list->cols);
		return;
	}

	/* Add new rows. */
	if (row + 1 > list->rows)
	{
		uint32 i;

		/* Update rows count and resize the array of rows. */
		list->rows = row + 1;
		list->text = realloc(list->text, sizeof(*list->text) * list->rows);

		/* Allocate columns for the new row(s). */
		for (i = row; i < list->rows; i++)
		{
			list->text[i] = calloc(1, sizeof(**list->text) * list->cols);
		}
	}

	list->text[row][col] = str ? strdup(str) : NULL;
}

/**
 * Remove row from a list.
 * @param list List.
 * @param row Row ID to remove. */
void list_remove_row(list_struct *list, uint32 row)
{
	uint32 col, row2;

	/* Sanity checks. */
	if (!list || !list->text || row >= list->rows)
	{
		return;
	}

	/* Free the columns of the row that is being removed. */
	for (col = 0; col < list->cols; col++)
	{
		free(list->text[row][col]);
	}

	/* If there are any rows below the one that is being removed, they
	 * need to be moved up. */
	for (row2 = row + 1; row2 < list->rows; row2++)
	{
		for (col = 0; col < list->cols; col++)
		{
			list->text[row2 - 1][col] = list->text[row2][col];
		}
	}

	list->rows--;
	list->text = realloc(list->text, sizeof(*list->text) * list->rows);
}

/**
 * Set options for one column.
 * @param list List.
 * @param col Column ID.
 * @param width The column's ID. -1 to leave default (0).
 * @param spacing Spacing between columns. -1 to leave default (0).
 * @param name Name of the column. NULL to leave default (no name shown).
 * @param centered Whether to center the drawn name/text in the column.
 * -1 to leave default (not centered). */
void list_set_column(list_struct *list, uint32 col, int width, int spacing, const char *name, int centered)
{
	if (col > list->cols)
	{
		logger_print(LOG(BUG), "Attempted to change column #%u, but columns max is %u.", col, list->cols);
		return;
	}

	/* Set width. */
	if (width != -1)
	{
		list->col_widths[col] = width;
		list->width += width;
	}

	/* Set spacing. */
	if (spacing != -1)
	{
		list->col_spacings[col] = spacing;
		list->width += spacing;
	}

	/* Set the column's name. */
	if (name)
	{
		/* There shouldn't be one previously, but just in case. */
		if (list->col_names[col])
		{
			free(list->col_names[col]);
		}

		list->col_names[col] = strdup(name);
	}

	/* Is the column centered? */
	if (centered != -1)
	{
		list->col_centered[col] = centered;
	}
}

/**
 * Change list's font.
 * @param list Which list to change font for.
 * @param font Font to use. */
void list_set_font(list_struct *list, int font)
{
	list->font = font;
}

/**
 * Enable scrollbar.
 * @param list List to enable scrollbar on. */
void list_scrollbar_enable(list_struct *list)
{
	list->scrollbar_enabled = 1;
	scrollbar_create(&list->scrollbar, 9, LIST_ROWS_HEIGHT(list) + 1, &list->row_offset, &list->rows, list->max_rows);
}

/**
 * Check whether the list needs redrawing.
 * @param list List to check.
 * @return 1 if the list needs redrawing, 0 otherwise. */
int list_need_redraw(list_struct *list)
{
	if (!list)
	{
		return 0;
	}

	if (list->scrollbar_enabled && scrollbar_need_redraw(&list->scrollbar))
	{
		return 1;
	}

	return 0;
}

/**
 * Show one list.
 * @param list List to show.
 * @param x X position.
 * @param y Y position. */
void list_show(list_struct *list, int x, int y)
{
	uint32 row, col;
	int w = 0, extra_width = 0;
	SDL_Rect box;

	if (!list)
	{
		return;
	}

	list->x = x;
	list->y = y;

	/* Draw a frame, if needed. */
	if (list->draw_frame_func)
	{
		list->draw_frame_func(list);
	}

	/* Draw the column names. */
	for (col = 0; col < list->cols; col++)
	{
		extra_width = 0;

		/* Center it? */
		if (list->col_centered[col])
		{
			extra_width = list->col_widths[col] / 2 - string_get_width(list->font, list->col_names[col], 0) / 2;
		}

		/* Actually draw the column name. */
		if (list->col_names[col])
		{
			string_blt_shadow(list->surface, list->font, list->col_names[col], list->x + w + extra_width, list->y, list->focus ? COLOR_WHITE : COLOR_GRAY, COLOR_BLACK, 0, NULL);
		}

		w += list->col_widths[col] + list->col_spacings[col];
	}

	/* Initialize default values for coloring rows. */
	box.x = list->x + list->frame_offset;
	box.w = list->width;
	box.h = LIST_ROW_HEIGHT(list);

	if (list->scrollbar_enabled)
	{
		scrollbar_render(&list->scrollbar, list->surface, list->x + list->frame_offset + 1 + w, LIST_ROWS_START(list));
	}

	/* Doing coloring of each row? */
	if (list->row_color_func)
	{
		for (row = 0; row < list->max_rows; row++)
		{
			box.y = LIST_ROWS_START(list) + (row * LIST_ROW_HEIGHT(list));
			list->row_color_func(list, row, box);
		}
	}

	/* Start printing out rows from the offset to the maximum. */
	for (row = list->row_offset; row < list->rows; row++)
	{
		/* Stop if we reached maximum number of visible rows. */
		if (LIST_ROW_OFFSET(row, list) == list->max_rows)
		{
			break;
		}

		/* Color selected row. */
		if (list->row_selected_func && (row + 1) == list->row_selected)
		{
			box.y = LIST_ROWS_START(list) + (LIST_ROW_OFFSET(row, list) * LIST_ROW_HEIGHT(list));
			list->row_selected_func(list, box);
		}
		/* Color highlighted row. */
		else if (list->row_highlight_func && (row + 1) == list->row_highlighted)
		{
			box.y = LIST_ROWS_START(list) + (LIST_ROW_OFFSET(row, list) * LIST_ROW_HEIGHT(list));
			list->row_highlight_func(list, box);
		}

		w = 0;

		/* Show all the columns. */
		for (col = 0; col < list->cols; col++)
		{
			/* Is there any text to show? */
			if (list->text[row][col])
			{
				const char *text_color;
				SDL_Rect text_rect;

				extra_width = 0;

				/* Center it. */
				if (list->col_centered[col])
				{
					extra_width = list->col_widths[col] / 2 - string_get_width(list->font, list->text[row][col], TEXT_WORD_WRAP) / 2;
				}

				text_color = list->focus ? COLOR_WHITE : COLOR_GRAY;

				if (list->text_color_hook)
				{
					text_color = list->text_color_hook(list, text_color, row, col);
				}

				/* Add width limit on the string. */
				text_rect.w = list->col_widths[col] + list->col_spacings[col];
				text_rect.h = LIST_ROW_HEIGHT(list);
				/* Output the text. */
				string_blt_shadow(list->surface, list->font, list->text[row][col], list->x + w + extra_width, LIST_ROWS_START(list) + (LIST_ROW_OFFSET(row, list) * LIST_ROW_HEIGHT(list)) + 1, text_color, COLOR_BLACK, TEXT_WORD_WRAP | list->text_flags, &text_rect);
			}

			if (list->post_column_func)
			{
				list->post_column_func(list, row, col);
			}

			w += list->col_widths[col] + list->col_spacings[col];
		}
	}
}

/**
 * Clear the list's rows.
 * @param list The list. */
void list_clear_rows(list_struct *list)
{
	uint32 row, col;

	if (!list || !list->text)
	{
		return;
	}

	/* Free the texts. */
	for (row = 0; row < list->rows; row++)
	{
		for (col = 0; col < list->cols; col++)
		{
			if (list->text[row][col])
			{
				free(list->text[row][col]);
			}
		}

		free(list->text[row]);
	}

	free(list->text);
	list->text = NULL;
	list->rows = 0;
}

/**
 * Clear and free list's entries.
 * @param list List. */
void list_clear(list_struct *list)
{
	list_clear_rows(list);

	list->row_selected = 1;
	list->row_highlighted = 0;
	list->row_offset = 0;
}

/**
 * Ensure the list's offsets are in a valid range. The offsets could be
 * invalid due to a row removal, for example.
 * @param list List to ensure for. */
void list_offsets_ensure(list_struct *list)
{
	if (list->row_selected >= list->rows)
	{
		list->row_selected = list->rows;
	}

	if (list->rows < list->max_rows)
	{
		list->row_offset = 0;
	}
	else if (list->row_offset >= list->rows - list->max_rows)
	{
		list->row_offset = list->rows - list->max_rows;
	}
}

/**
 * Remove the specified list from the linked list of visible lists and
 * deinitialize it.
 * @param list List to remove. */
void list_remove(list_struct *list)
{
	uint32 col;

	if (!list)
	{
		return;
	}

	if (list->data)
	{
		free(list->data);
	}

	list_clear(list);

	free(list->col_widths);
	free(list->col_spacings);
	free(list->col_centered);

	/* Free column names. */
	for (col = 0; col < list->cols; col++)
	{
		if (list->col_names[col])
		{
			free(list->col_names[col]);
		}
	}

	free(list->col_names);
	free(list);
}

/**
 * Scroll the list in the specified direction by the specified amount.
 * @param list List to scroll.
 * @param up If 1, scroll the list upwards, otherwise downwards.
 * @param scroll Amount to scroll by. */
static void list_scroll(list_struct *list, int up, int scroll)
{
	/* The actual values are unsigned. Changing them to signed here
	 * makes it easier to check for overflows below. */
	sint32 row_selected = list->row_selected, row_offset = list->row_offset;
	sint32 max_rows, rows;

	/* Number of rows. */
	rows = list->rows;
	/* Number of visible rows. */
	max_rows = list->max_rows;

	/* Scrolling upward. */
	if (up)
	{
		row_selected -= scroll;

		/* Adjust row offset if needed. */
		if (row_offset > (row_selected - max_rows + 1))
		{
			row_offset -= scroll;
		}
	}
	/* Downward otherwise. */
	else
	{
		row_selected += scroll;

		/* Adjust row offset if needed. */
		if (row_selected >= max_rows + row_offset)
		{
			row_offset += scroll;
		}
	}

	/* Make sure row offset is within bounds. */
	if (row_offset < 0 || rows < max_rows)
	{
		row_offset = 0;
	}
	else if (row_offset >= rows - max_rows)
	{
		row_offset = rows - max_rows;
	}

	/* Make sure selected row is within bounds. */
	if (row_selected < 1)
	{
		row_selected = 1;
	}
	else if (row_selected >= rows)
	{
		row_selected = list->rows;
	}

	/* Set the values. */
	list->row_selected = row_selected;
	list->row_offset = row_offset;
}

/**
 * Handle keyboard event for the specified list.
 * @param list List.
 * @param event The event.
 * @return 1 if we handled the event, 0 otherwise. */
int list_handle_keyboard(list_struct *list, SDL_Event *event)
{
	if (!list)
	{
		return 0;
	}

	if (event->type != SDL_KEYDOWN)
	{
		return 0;
	}

	if (list->key_event_func)
	{
		int ret = list->key_event_func(list, event->key.keysym.sym);

		if (ret != -1)
		{
			return ret;
		}
	}

	switch (event->key.keysym.sym)
	{
		/* Up arrow. */
		case SDLK_UP:
			list_scroll(list, 1, 1);
			return 1;

		/* Down arrow. */
		case SDLK_DOWN:
			list_scroll(list, 0, 1);
			return 1;

		/* Page up. */
		case SDLK_PAGEUP:
			list_scroll(list, 1, list->max_rows);
			return 1;

		/* Page down. */
		case SDLK_PAGEDOWN:
			list_scroll(list, 0, list->max_rows);
			return 1;

		/* Esc, let the list creator handle this if they want to. */
		case SDLK_ESCAPE:
			if (list->handle_esc_func)
			{
				list->handle_esc_func(list);
			}

			return 1;

		/* Enter. */
		case SDLK_RETURN:
		case SDLK_KP_ENTER:
			if (list->handle_enter_func)
			{
				list->handle_enter_func(list);
			}

			return 1;

		/* Unhandled key. */
		default:
			break;
	}

	return 0;
}

/**
 * Handle mouse events for one list. Checking whether the mouse is over
 * the list should have been done before calling this.
 * @param list The list.
 * @param event Event.
 * @return 1 if the event was handled, 0 otherwise. */
int list_handle_mouse(list_struct *list, SDL_Event *event)
{
	uint32 row, col, old_highlighted, old_selected;
	int mx, my;

	if (!list)
	{
		return 0;
	}

	if (event->type != SDL_MOUSEBUTTONDOWN && event->type != SDL_MOUSEBUTTONUP && event->type != SDL_MOUSEMOTION)
	{
		return 0;
	}

	mx = event->motion.x - list->px;
	my = event->motion.y - list->py;

	if (list->scrollbar_enabled)
	{
		list->scrollbar.px = list->px;
		list->scrollbar.py = list->py;

		if (scrollbar_event(&list->scrollbar, event))
		{
			return 1;
		}
	}

	if (!LIST_MOUSE_OVER(list, mx, my))
	{
		return 0;
	}

	old_highlighted = list->row_highlighted;
	old_selected = list->row_selected;

	/* No row is highlighted now. Will be switched back on as needed
	 * below. */
	list->row_highlighted = 0;

	if (list_mouse_get_pos(list, event->motion.x, event->motion.y, &row, &col))
	{
		if (list->handle_mouse_row_func)
		{
			list->handle_mouse_row_func(list, row, event);
		}

		/* Mouse click? */
		if (event->type == SDL_MOUSEBUTTONDOWN && event->button.button == SDL_BUTTON_LEFT)
		{
			/* See if we clicked on this row earlier, and whether this
			 * should be considered a double click. */
			if (SDL_GetTicks() - list->click_tick < DOUBLE_CLICK_DELAY)
			{
				/* Double click, handle it as if enter was used. */
				if (list->handle_enter_func)
				{
					list->handle_enter_func(list);
					list->click_tick = 0;
				}

				/* Update selected row (in case enter handling
				 * function did not actually jump to another GUI,
				 * thus removing the need for this list). */
				list->row_selected = row + 1;
			}
			/* Normal click. */
			else
			{
				/* Update selected row and click ticks for above
				 * double click calculation. */
				list->row_selected = row + 1;
				list->click_tick = SDL_GetTicks();
			}
		}
		/* Not a mouse click, so update highlighted row. */
		else
		{
			list->row_highlighted = row + 1;
		}
	}

	/* Handle mouse wheel for scrolling. */
	if (event->button.button == SDL_BUTTON_WHEELUP || event->button.button == SDL_BUTTON_WHEELDOWN)
	{
		list_scroll(list, event->button.button == SDL_BUTTON_WHEELUP, 1);
		return 1;
	}

	if (old_highlighted != list->row_highlighted || old_selected != list->row_selected)
	{
		return 1;
	}

	return 0;
}

int list_mouse_get_pos(list_struct *list, int mx, int my, uint32 *row, uint32 *col)
{
	uint32 w;

	mx -= list->px;
	my -= list->py;

	/* See which row the mouse is over. */
	for (*row = list->row_offset; *row < list->rows; (*row)++)
	{
		/* Stop if we reached maximum number of visible rows. */
		if (LIST_ROW_OFFSET(*row, list) == list->max_rows)
		{
			break;
		}

		/* Is the mouse over this row? */
		if ((uint32) my >= (LIST_ROWS_START(list) + LIST_ROW_OFFSET(*row, list) * LIST_ROW_HEIGHT(list)) && (uint32) my < LIST_ROWS_START(list) + (LIST_ROW_OFFSET(*row, list) + 1) * LIST_ROW_HEIGHT(list))
		{
			w = 0;

			for (*col = 0; *col < list->cols; (*col)++)
			{
				if ((uint32) mx >= list->x + list->frame_offset + w && (uint32) mx < list->x + list->frame_offset + w + list->col_widths[*col] + list->col_spacings[*col])
				{
					return 1;
				}

				w += list->col_widths[*col] + list->col_spacings[*col];
			}
		}
	}

	return 0;
}

/**
 * Used for alphabetical sorting in list_sort().
 * @param a What to compare.
 * @param b What to compare against.
 * @return Return value of strcmp() against the two entries. */
static int list_compare_alpha(const void *a, const void *b)
{
	return strcmp(((char ***) a)[0][0], ((char ***) b)[0][0]);
}

/**
 * Sort a list's entries.
 * @param list List to sort.
 * @param type How to sort, one of @ref LIST_SORT_xxx.
 * @note Sorting is done by looking at the first column of each row. */
void list_sort(list_struct *list, int type)
{
	if (!list->text)
	{
		return;
	}

	/* Alphabetical sort. */
	if (type == LIST_SORT_ALPHA)
	{
		qsort((void *) list->text, list->rows, sizeof(*list->text), (void *) (int (*)()) list_compare_alpha);
	}
}

/**
 * Set the selected row based on column's text.
 * @param list The list.
 * @param str Text to search for in all the rows.
 * @param col The column to check value of in each row.
 * @return 1 if new selected row was set, 0 otherwise. */
int list_set_selected(list_struct *list, const char *str, uint32 col)
{
	uint32 row;

	for (row = 0; row < list->rows; row++)
	{
		if (!strcmp(list->text[row][col], str))
		{
			list->row_selected = row + 1;
			list->row_offset = MIN(list->rows - list->max_rows, row);
			return 1;
		}
	}

	return 0;
}
