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
 * Implements the skills widget. */

#include <global.h>

/** The skill list. */
static skill_entry_struct **skill_list[SKILL_LIST_TYPES];
/** Number of skills contained in each skill type array in ::skill_list. */
static size_t skill_list_num[SKILL_LIST_TYPES];

/** Currently selected skill type. */
static size_t skill_list_type = 0;
/** If 1, only show known skills in the list of skills. */
static uint8 skill_list_filter_known;
/** Button buffer. */
static button_struct button_type_left, button_type_right, button_close, button_filter_known, button_help;

/**
 * Handle double click inside the skills list.
 * @param list The skills list. */
static void list_handle_enter(list_struct *list)
{
	/* Ready the selected skill, if any. */
	if (list->text)
	{
		char buf[MAX_BUF];

		snprintf(buf, sizeof(buf), "/ready_skill %s", list->text[list->row_selected - 1][0]);
		client_command_check(buf);
	}
}

/**
 * Handle list::text_color_hook in the skills list. */
static const char *list_text_color_hook(list_struct *list, const char *default_color, uint32 row, uint32 col)
{
	size_t id;

	/* If the skill is not known, use gray instead of the default color. */
	if (skill_find_type_selected(list->text[row][col], &id) && !skill_list[skill_list_type][id]->known)
	{
		return COLOR_GRAY;
	}

	return default_color;
}

/**
 * Reload the skills list, due to a change of the skill type, for example. */
static void skill_list_reload()
{
	list_struct *list;
	size_t i;
	uint32 offset, rows, selected;

	list = list_exists(LIST_SKILLS);

	if (!list)
	{
		return;
	}

	offset = list->row_offset;
	selected = list->row_selected;
	rows = list->rows;
	list_clear(list);

	for (i = 0; i < skill_list_num[skill_list_type]; i++)
	{
		if (skill_list_filter_known && !skill_list[skill_list_type][i]->known)
		{
			continue;
		}

		list_add(list, list->rows, 0, skill_list[skill_list_type][i]->name);
	}

	list_sort(list, LIST_SORT_ALPHA);

	if (list->rows == rows)
	{
		list->row_offset = offset;
		list->row_selected = selected;
	}

	cur_widget[SKILLS_ID]->redraw = 1;
}

/**
 * Handle button repeating (and actual pressing).
 * @param button The button. */
static void button_repeat_func(button_struct *button)
{
	int path = skill_list_type;

	if (button == &button_type_left)
	{
		path--;
	}
	else
	{
		path++;
	}

	if (path < 0)
	{
		path = SKILL_LIST_TYPES - 1;
	}
	else if (path > SKILL_LIST_TYPES - 1)
	{
		path = 0;
	}

	skill_list_type = path;
	skill_list_reload();
}

/**
 * Render the skill list widget.
 * @param widget The widget to render. */
void widget_skills_render(widgetdata *widget)
{
	list_struct *list;
	SDL_Rect box, box2;

	/* Create the surface. */
	if (!widget->widgetSF)
	{
		widget->widgetSF = SDL_ConvertSurface(Bitmaps[BITMAP_CONTENT]->bitmap, Bitmaps[BITMAP_CONTENT]->bitmap->format, Bitmaps[BITMAP_CONTENT]->bitmap->flags);
	}

	list = list_exists(LIST_SKILLS);

	/* Create the skill list. */
	if (!list)
	{
		skill_list_filter_known = 0;

		list = list_create(LIST_SKILLS, 12, 1, 8);
		list->handle_enter_func = list_handle_enter;
		list->text_color_hook = list_text_color_hook;
		list->surface = widget->widgetSF;
		list_scrollbar_enable(list);
		list_set_column(list, 0, 130, 7, NULL, -1);
		list_set_font(list, FONT_ARIAL10);
		skill_list_reload();

		/* Create various buttons... */
		button_create(&button_type_left);
		button_create(&button_type_right);
		button_create(&button_close);
		button_create(&button_filter_known);
		button_create(&button_help);
		button_type_left.repeat_func = button_type_right.repeat_func = button_repeat_func;
		button_close.bitmap = button_type_left.bitmap = button_type_right.bitmap = button_help.bitmap = BITMAP_BUTTON_ROUND;
		button_close.bitmap_pressed = button_type_left.bitmap_pressed = button_type_right.bitmap_pressed = button_help.bitmap_pressed = BITMAP_BUTTON_ROUND_DOWN;
	}

	if (widget->redraw)
	{
		_BLTFX bltfx;
		size_t skill_id;

		bltfx.surface = widget->widgetSF;
		bltfx.flags = 0;
		bltfx.alpha = 0;
		sprite_blt(Bitmaps[BITMAP_CONTENT], 0, 0, NULL, &bltfx);

		widget->redraw = 0;

		box.h = 0;
		box.w = widget->wd;
		string_blt(widget->widgetSF, FONT_SERIF12, "Skills", 0, 3, COLOR_HGOLD, TEXT_ALIGN_CENTER, &box);
		list->focus = 1;
		list_set_parent(list, widget->x1, widget->y1);
		list_show(list, 10, 2);

		box.w = 160;
		string_blt(widget->widgetSF, FONT_SERIF12, s_settings->skill_types[skill_list_type], 0, widget->ht - FONT_HEIGHT(FONT_SERIF12) - 7, COLOR_HGOLD, TEXT_ALIGN_CENTER, &box);

		/* Show the skill's description. */
		if (list->text && skill_find_type_selected(list->text[list->row_selected - 1][0], &skill_id))
		{
			box.h = 120;
			box.w = 150;
			string_blt(widget->widgetSF, FONT_ARIAL10, skill_list[skill_list_type][skill_id]->desc, 160, 40, COLOR_WHITE, TEXT_WORD_WRAP | TEXT_MARKUP, &box);
		}

		/* Show the skill's icon, if it's known. */
		if (list->text && skill_list[skill_list_type][skill_id]->known)
		{
			_Sprite *icon = FaceList[skill_list[skill_list_type][skill_id]->icon].sprite;
			char level_buf[MAX_BUF], exp_buf[MAX_BUF];

			if (skill_list[skill_list_type][skill_id]->exp == -1)
			{
				strncpy(level_buf, "<b>Level</b>: n/a", sizeof(level_buf) - 1);
				level_buf[sizeof(level_buf) - 1] = '\0';
			}
			else
			{
				snprintf(level_buf, sizeof(level_buf), "<b>Level</b>: %d", skill_list[skill_list_type][skill_id]->level);
			}

			if (skill_list[skill_list_type][skill_id]->exp >= 0)
			{
				snprintf(exp_buf, sizeof(exp_buf), "<b>Exp</b>: %"FMT64, skill_list[skill_list_type][skill_id]->exp);
			}
			else
			{
				strncpy(exp_buf, "<b>Exp</b>: n/a", sizeof(exp_buf) - 1);
				exp_buf[sizeof(exp_buf) - 1] = '\0';
			}

			string_blt(widget->widgetSF, FONT_ARIAL10, level_buf, 160, widget->ht - 30, COLOR_WHITE, TEXT_MARKUP, NULL);
			string_blt(widget->widgetSF, FONT_ARIAL10, exp_buf, 160, widget->ht - 18, COLOR_WHITE, TEXT_MARKUP, NULL);

			draw_frame(widget->widgetSF, widget->wd - 6 - icon->bitmap->w, widget->ht - 6 - icon->bitmap->h, icon->bitmap->w + 1, icon->bitmap->h + 1);
			sprite_blt(icon, widget->wd - 5 - icon->bitmap->w, widget->ht - 5 - icon->bitmap->h, NULL, &bltfx);
		}
	}

	box2.x = widget->x1;
	box2.y = widget->y1;
	SDL_BlitSurface(widget->widgetSF, NULL, ScreenSurface, &box2);

	/* Render the various buttons. */
	button_close.x = widget->x1 + widget->wd - Bitmaps[BITMAP_BUTTON_ROUND]->bitmap->w - 4;
	button_close.y = widget->y1 + 4;
	button_render(&button_close, "X");

	button_help.x = widget->x1 + widget->wd - Bitmaps[BITMAP_BUTTON_ROUND]->bitmap->w * 2 - 4;
	button_help.y = widget->y1 + 4;
	button_render(&button_help, "?");

	button_type_left.x = widget->x1 + 6;
	button_type_left.y = widget->y1 + widget->ht - Bitmaps[BITMAP_BUTTON_ROUND]->bitmap->h - 5;
	button_render(&button_type_left, "<");

	button_type_right.x = widget->x1 + 6 + 130;
	button_type_right.y = widget->y1 + widget->ht - Bitmaps[BITMAP_BUTTON_ROUND]->bitmap->h - 5;
	button_render(&button_type_right, ">");

	if (skill_list_filter_known)
	{
		button_filter_known.pressed_forced = 1;
	}

	button_filter_known.x = widget->x1 + 158;
	button_filter_known.y = widget->y1 + 22;
	button_render(&button_filter_known, "Known");
}

/**
 * Handle mouse events inside the skills widget.
 * @param widget The skills widget.
 * @param event The event to handle. */
void widget_skills_mevent(widgetdata *widget, SDL_Event *event)
{
	list_struct *list = list_exists(LIST_SKILLS);

	/* If the list has handled the mouse event, we need to redraw the
	 * widget. */
	if (list && list_handle_mouse(list, event->motion.x - widget->x1, event->motion.y - widget->y1, event))
	{
		widget->redraw = 1;
	}

	if (button_event(&button_type_left, event))
	{
		button_repeat_func(&button_type_left);
	}
	else if (button_event(&button_type_right, event))
	{
		button_repeat_func(&button_type_right);
	}
	else if (button_event(&button_close, event))
	{
		widget->show = 0;
	}
	else if (button_event(&button_filter_known, event))
	{
		skill_list_filter_known = !skill_list_filter_known;
		skill_list_reload();
	}
	else if (button_event(&button_help, event))
	{
		help_show("skill list");
	}
}

/**
 * Find a skill in the ::skill_list based on its name.
 *
 * Partial skill names will be matched.
 * @param name Skill name to find.
 * @param[out] type Will contain the skill's type.
 * @param[out] id Will contain the skill's ID.
 * @return 1 if the skill was found, 0 otherwise. */
int skill_find(const char *name, size_t *type, size_t *id)
{
	for (*type = 0; *type < SKILL_LIST_TYPES - 1; *type += 1)
	{
		for (*id = 0; *id < skill_list_num[*type]; *id += 1)
		{
			if (!strncasecmp(skill_list[*type][*id]->name, name, strlen(name)))
			{
				return 1;
			}
		}
	}

	return 0;
}

/**
 * Find a skill in the ::skill_list based on its name, but only look
 * inside the currently selected skill type list.
 *
 * Partial skill names will be matched.
 * @param name Skill name to find.
 * @param[out] id Will contain the skill's ID.
 * @return 1 if the skill was found, 0 otherwise. */
int skill_find_type_selected(const char *name, size_t *id)
{
	for (*id = 0; *id < skill_list_num[skill_list_type]; *id += 1)
	{
		if (!strncasecmp(skill_list[skill_list_type][*id]->name, name, strlen(name)))
		{
			return 1;
		}
	}

	return 0;
}

/**
 * Get skill from the ::skill_list structure.
 * @param type Skill type.
 * @param id Skill ID.
 * @return The skill. */
skill_entry_struct *skill_get(size_t type, size_t id)
{
	return skill_list[type][id];
}

/**
 * Read skills from file. */
void skills_init()
{
	FILE *fp;
	char line[HUGE_BUF];
	size_t i, j, num_skills;

	/* Free previously allocated skills. */
	for (i = 0; i < SKILL_LIST_TYPES; i++)
	{
		if (skill_list[i])
		{
			if (i != SKILL_LIST_TYPES - 1)
			{
				for (j = 0; j < skill_list_num[i]; j++)
				{
					free(skill_list[i][j]);
				}
			}

			free(skill_list[i]);
		}
	}

	memset(&skill_list, 0, sizeof(*skill_list) * SKILL_LIST_TYPES);
	memset(&skill_list_num, 0, sizeof(*skill_list_num) * SKILL_LIST_TYPES);
	skill_list_type = 0;
	num_skills = 0;

	fp = server_file_open(SERVER_FILE_SKILLS);

	if (!fp)
	{
		return;
	}

	while (fgets(line, sizeof(line) - 1, fp))
	{
		char *name, *icon, desc[HUGE_BUF];
		int type;

		line[strlen(line) - 1] = '\0';
		name = strdup(line);

		if (!fgets(line, sizeof(line) - 1, fp))
		{
			LOG(llevBug, "  Got unexpected EOF reading skills file.\n");
			break;
		}

		type = atoi(line);

		if (!fgets(line, sizeof(line) - 1, fp))
		{
			LOG(llevBug, "  Got unexpected EOF reading skills file.\n");
			break;
		}

		line[strlen(line) - 1] = '\0';
		icon = strdup(line);
		desc[0] = '\0';

		while (fgets(line, sizeof(line) - 1, fp))
		{
			if (!strcmp(line, "end\n"))
			{
				skill_entry_struct *entry;

				/* Resize the skill list array for this skill type. */
				skill_list[type] = realloc(skill_list[type], sizeof(*skill_list[type]) * (skill_list_num[type] + 1));
				entry = skill_list[type][skill_list_num[type]] = malloc(sizeof(**skill_list[type]));
				skill_list_num[type]++;

				/* Store the data. */
				strncpy(entry->name, name, sizeof(entry->name) - 1);
				entry->name[sizeof(entry->name) - 1] = '\0';
				strncpy(entry->desc, desc, sizeof(entry->desc) - 1);
				entry->desc[sizeof(entry->desc) - 1] = '\0';
				strncpy(entry->icon_name, icon, sizeof(entry->icon_name) - 1);
				entry->icon_name[sizeof(entry->icon_name) - 1] = '\0';
				entry->icon = get_bmap_id(entry->icon_name);
				entry->known = 0;

				free(icon);
				free(name);
				num_skills++;
				break;
			}

			strncat(desc, line, sizeof(desc) - strlen(desc) - 1);
		}
	}

	fclose(fp);

	if (num_skills)
	{
		skill_list[SKILL_LIST_TYPES - 1] = malloc(sizeof(*skill_list[SKILL_LIST_TYPES - 1]) * num_skills);

		for (i = 0; i < SKILL_LIST_TYPES - 1; i++)
		{
			for (j = 0; j < skill_list_num[i]; j++)
			{
				skill_list[SKILL_LIST_TYPES - 1][skill_list_num[SKILL_LIST_TYPES - 1]] = skill_list[i][j];
				skill_list_num[SKILL_LIST_TYPES - 1]++;
			}
		}
	}
}

/**
 * Reload the icon IDs, as they may have changed due to an update. */
void skills_reload()
{
	size_t type, id;

	for (type = 0; type < SKILL_LIST_TYPES - 1; type++)
	{
		for (id = 0; id < skill_list_num[type]; id++)
		{
			skill_list[type][id]->icon = get_bmap_id(skill_list[type][id]->icon_name);
		}
	}
}

/**
 * Skill list command. Used to update player's skill list.
 * @param data The incoming data */
void SkilllistCmd(char *data)
{
	char *tmp_data, *cp;
	size_t skill_type, skill_id;
	int mode;

	tmp_data = strdup(data);
	cp = strtok(tmp_data, "/");

	mode = atoi(data);

	while (cp)
	{
		char *tmp[3];

		if (split_string(cp, tmp, sizeof(tmp) / sizeof(*tmp), '|') != 3)
		{
			cp = strtok(NULL, "/");
			continue;
		}

		/* If the skill exists, mark it as known, and store the level/exp. */
		if (skill_find(tmp[0], &skill_type, &skill_id))
		{
			skill_entry_struct *skill = skill_get(skill_type, skill_id);

			if (mode == SPLIST_MODE_REMOVE)
			{
				skill->known = 0;
			}
			else
			{
				skill->known = 1;
				skill->level = atoi(tmp[1]);
				skill->exp = atoll(tmp[2]);
				WIDGET_REDRAW_ALL(SKILL_EXP_ID);
			}
		}

		cp = strtok(NULL, "/");
	}

	free(tmp_data);
	skill_list_reload();
}
