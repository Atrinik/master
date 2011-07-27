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
 * Handles keybindings.
 *
 * Whenever a keyboard event occurs and the user is logged into the game,
 * keybindings are checked for a match. First keybindings with modifier
 * keys are checked to see if they match the event key and modifier state,
 * then keybindings without modifier keys are checked and will work
 * regardless of the current key modifier state.
 *
 * This is done to ensure user commands will work correctly, even if they
 * have modifier keys. For example, CTRL+c would not work if it was near
 * the bottom of the keybinding list, because the 'c' keybinding near the
 * top would work first.
 *
 * Also if the keybindings with no modifier keys were not triggered
 * regardless of the current keyboard modifier state, it would not be
 * possible to do actions such as alt+numpad, or ctrl+numpad.
 *
 * @author Alex Tokar */

#include <global.h>

/** The keybindings. */
keybind_struct **keybindings = NULL;
/** Number of keybindings. */
size_t keybindings_num = 0;

/**
 * Load keybindings. */
void keybind_load()
{
	FILE *fp;
	char buf[HUGE_BUF], *cp;
	keybind_struct *keybind = NULL;

	fp = fopen_wrapper(FILE_KEYBIND, "r");

	while (fgets(buf, sizeof(buf) - 1, fp))
	{
		cp = strchr(buf, '\n');

		if (cp)
		{
			*cp = '\0';
		}

		cp = buf;

		while (*cp != '\0')
		{
			if (isspace(*cp))
			{
				cp++;
			}
			else
			{
				break;
			}
		}

		if (*cp == '#' || *cp == '\0')
		{
			continue;
		}

		/* End of a single keybinding definition, add it to the list. */
		if (!strcmp(cp, "end"))
		{
			keybindings = realloc(keybindings, sizeof(keybindings) * (keybindings_num + 1));
			keybindings[keybindings_num] = keybind;
			keybindings_num++;
			keybind = NULL;
		}
		/* Are we inside a keybinding definition? */
		else if (keybind)
		{
			if (!strncmp(cp, "command ", 8))
			{
				keybind->command = strdup(cp + 8);
			}
			else if (!strncmp(cp, "key ", 4))
			{
				keybind->key = atoi(cp + 4);
			}
			else if (!strncmp(cp, "mod ", 4))
			{
				keybind->mod = atoi(cp + 4);
			}
			else if (!strncmp(cp, "repeat ", 7))
			{
				keybind->repeat = atoi(cp + 7);
			}
		}
		/* Keybinding definition start. */
		else if (!strcmp(cp, "bind"))
		{
			keybind = calloc(1, sizeof(*keybind));
		}
	}

	fclose(fp);
}

/**
 * Save the keybindings. */
void keybind_save()
{
	FILE *fp;
	size_t i;

	fp = fopen_wrapper(FILE_KEYBIND, "w");

	for (i = 0; i < keybindings_num; i++)
	{
		fprintf(fp, "bind\n");
		fprintf(fp, "\t# %s\n\tkey %d\n", SDL_GetKeyName(keybindings[i]->key), keybindings[i]->key);

		if (keybindings[i]->mod)
		{
			fprintf(fp, "\tmod %d\n", keybindings[i]->mod);
		}

		if (keybindings[i]->repeat)
		{
			fprintf(fp, "\trepeat %d\n", keybindings[i]->repeat);
		}

		if (keybindings[i]->command)
		{
			fprintf(fp, "\tcommand %s\n", keybindings[i]->command);
		}

		fprintf(fp, "end\n");
	}

	fclose(fp);
}

/**
 * Free a single keybinding entry.
 * @param keybind Keybinding to free. */
void keybind_free(keybind_struct *keybind)
{
	free(keybind->command);
	free(keybind);
}

/**
 * Deinitialize all keybindings. */
void keybind_deinit()
{
	size_t i;

	/* Save them... */
	keybind_save();

	for (i = 0; i < keybindings_num; i++)
	{
		keybind_free(keybindings[i]);
	}

	if (keybindings)
	{
		free(keybindings);
		keybindings = NULL;
	}

	keybindings_num = 0;
}

/**
 * Adjust SDLMod state value. This is done because the state may have
 * other flags we do not care about, and we do not want to save those
 * to file. It also simplifies keyboard modifier state checks.
 * @param mod State to adjust.
 * @return Adjusted state. */
static SDLMod keybind_adjust_kmod(SDLMod mod)
{
	/* We only care about left/right shift, ctrl, alt, and super
	 * modifiers, so remove any others. */
	mod &= KMOD_SHIFT | KMOD_CTRL | KMOD_ALT | KMOD_META;

	/* The following code deals with making sure that if the modifier
	 * contains only for example left shift modifier, right shift is also
	 * added to the modifier, in order to simplify saving and state
	 * checks. */
	if (mod & KMOD_SHIFT)
	{
		mod |= KMOD_SHIFT;
	}

	if (mod & KMOD_CTRL)
	{
		mod |= KMOD_CTRL;
	}

	if (mod & KMOD_ALT)
	{
		mod |= KMOD_ALT;
	}

	if (mod & KMOD_META)
	{
		mod |= KMOD_META;
	}

	return mod;
}

/**
 * Add a keybinding to the ::keybindings array.
 * @param key Key the keybinding uses.
 * @param mod Modifier for the keybinding shortcut. Will be adjusted by
 * keybind_adjust_kmod().
 * @param command Command to execute when the keybinding is activated.
 * @return The added keybinding. */
keybind_struct *keybind_add(SDLKey key, SDLMod mod, const char *command)
{
	keybind_struct *keybind;

	/* Allocate a new keybinding, and store the values. */
	keybind = calloc(1, sizeof(*keybind));
	keybind->key = key;
	keybind->mod = keybind_adjust_kmod(mod);
	keybind->command = strdup(command);

	/* Expand the keybindings array, and store the new keybinding. */
	keybindings = realloc(keybindings, sizeof(keybindings) * (keybindings_num + 1));
	keybindings[keybindings_num] = keybind;
	keybindings_num++;

	return keybind;
}

/**
 * Edit the specified keybinding.
 * @param i Index inside the ::keybindings array to edit.
 * @param key Key to change.
 * @param mod Modifier to change.
 * @param command Command to change. */
void keybind_edit(size_t i, SDLKey key, SDLMod mod, const char *command)
{
	/* Sanity check. */
	if (i >= keybindings_num)
	{
		return;
	}

	/* Store the values. */
	keybindings[i]->key = key;
	keybindings[i]->mod = keybind_adjust_kmod(mod);
	free(keybindings[i]->command);
	keybindings[i]->command = strdup(command);
}

/**
 * Remove a keybinding from ::keybindings.
 * @param i Index in the ::keybindings array to remove. */
void keybind_remove(size_t i)
{
	size_t j;

	/* Sanity check. */
	if (i >= keybindings_num)
	{
		return;
	}

	/* Free the entry. */
	keybind_free(keybindings[i]);

	/* Shift entries below the removed keybinding, if any. */
	for (j = i + 1; j < keybindings_num; j++)
	{
		keybindings[j - 1] = keybindings[j];
	}

	/* Shrink the array. */
	keybindings_num--;
	keybindings = realloc(keybindings, sizeof(*keybindings) * keybindings_num);
}

/**
 * Toggle the repeat state of a keybinding.
 * @param i Index in the ::keybindings array to toggle the repeat state
 * of. */
void keybind_repeat_toggle(size_t i)
{
	/* Sanity check. */
	if (i >= keybindings_num)
	{
		return;
	}

	keybindings[i]->repeat = !keybindings[i]->repeat;
}

/**
 * Construct a text representation of a keybinding shortcut.
 * @param key Key of the shortcut.
 * @param mod Keyboard modifier.
 * @param buf Where to store the result.
 * @param len Size of 'buf'.
 * @return 'buf'. */
char *keybind_get_key_shortcut(SDLKey key, SDLMod mod, char *buf, size_t len)
{
	buf[0] = '\0';

	/* Prefix with the keyboard modifier. */
	if (mod & KMOD_SHIFT)
	{
		strncat(buf, "shift + ", len - strlen(buf) - 1);
	}

	if (mod & KMOD_CTRL)
	{
		strncat(buf, "ctrl + ", len - strlen(buf) - 1);
	}

	if (mod & KMOD_ALT)
	{
		strncat(buf, "alt + ", len - strlen(buf) - 1);
	}

	if (mod & KMOD_META)
	{
		strncat(buf, "super + ", len - strlen(buf) - 1);
	}

	strncat(buf, SDL_GetKeyName(key), len - strlen(buf) - 1);

	return buf;
}

/**
 * Finds keybinding structure by command name.
 * @param cmd The command to find.
 * @return Keybinding if found, NULL otherwise. */
keybind_struct *keybind_find_by_command(const char *cmd)
{
	size_t i;

	for (i = 0; i < keybindings_num; i++)
	{
		if (!strcmp(cmd, keybindings[i]->command))
		{
			return keybindings[i];
		}
	}

	return NULL;
}

/**
 * Check if the specified keybinding command matches a keyboard event.
 * @param cmd The keybinding command.
 * @return 1 if it matches, 0 otherwise. */
int keybind_command_matches_event(const char *cmd, SDL_KeyboardEvent *event)
{
	keybind_struct *keybind = keybind_find_by_command(cmd);

	if (!keybind)
	{
		return 0;
	}

	if (event->keysym.sym == keybind->key && (!keybind->mod || keybind->mod == keybind_adjust_kmod(event->keysym.mod)))
	{
		return 1;
	}

	return 0;
}

/**
 * Check if the specified keybinding command matches the current keyboard
 * state.
 * @param cmd The keybinding command.
 * @return 1 if it matches, 0 otherwise. */
int keybind_command_matches_state(const char *cmd)
{
	size_t i;

	for (i = 0; i < keybindings_num; i++)
	{
		if (!strcmp(cmd, keybindings[i]->command))
		{
			if (keys[keybindings[i]->key].pressed && (!keybindings[i]->mod || keybindings[i]->mod == keybind_adjust_kmod(SDL_GetModState())))
			{
				return 1;
			}
		}
	}

	return 0;
}

/**
 * Attempt to process a keyboard event.
 * @param event The event to process.
 * @return 1 if the event was handled, 0 otherwise. */
int keybind_process_event(SDL_KeyboardEvent *event)
{
	size_t i;

	/* Try to handle keybindings with modifier keys first. */
	for (i = 0; i < keybindings_num; i++)
	{
		if (event->keysym.sym == keybindings[i]->key && keybindings[i]->mod == keybind_adjust_kmod(event->keysym.mod))
		{
			keybind_process(keybindings[i], event->type);
			return 1;
		}
	}

	/* Now handle keys with no modifier keys, regardless of what the
	 * current keyboard modifier combination is. */
	for (i = 0; i < keybindings_num; i++)
	{
		if (event->keysym.sym == keybindings[i]->key && !keybindings[i]->mod)
		{
			keybind_process(keybindings[i], event->type);
			return 1;
		}
	}

	return 0;
}

/**
 * Process a keybinding.
 * @param keybind The keybinding to process.
 * @param type Either SDL_KEYDOWN or SDL_KEYUP. */
void keybind_process(keybind_struct *keybind, uint8 type)
{
	char command[MAX_BUF], *cp;

	/* Do not repeat keys that should not be repeated. */
	if (!keybind->repeat && keys[keybind->key].repeated)
	{
		return;
	}

	strncpy(command, keybind->command, sizeof(command) - 1);
	command[sizeof(command) - 1] = '\0';

	cp = strtok(command, ";");

	while (cp)
	{
		while (*cp == ' ')
		{
			cp++;
		}

		if (type == SDL_KEYDOWN)
		{
			keybind_process_command(cp);
		}
		else
		{
			keybind_process_command_up(cp);
		}

		cp = strtok(NULL, ";");
	}
}

/**
 * Handle keybinding 'key up' event.
 * @param cmd Keybinding command to handle.
 * @return 1 if the command was handled, 0 otherwise. */
int keybind_process_command_up(const char *cmd)
{
	const char *cmd_orig = cmd;

	if (*cmd == '?')
	{
		cmd++;

		if (!strcmp(cmd, "INVENTORY"))
		{
			cpl.inventory_win = IWIN_BELOW;
		}
		else if (!strcmp(cmd, "RUNON"))
		{
			send_command("/run_stop");
			cpl.run_on = 0;
		}
		else if (!strcmp(cmd, "FIREON"))
		{
			cpl.fire_on = 0;
		}
		else if (!strncmp(cmd, "MOVE_", 5))
		{
			keybind_struct *keybind;

			cmd += 5;

			if (strcmp(cmd, "STAY") && (keybind = keybind_find_by_command(cmd_orig)) && keys[keybind->key].repeated)
			{
				move_keys(5);
			}
		}

		return 1;
	}

	return 0;
}

/**
 * Ensure that keybindings which should trigger on 'key up' event have
 * done so, even if the 'key up' event was handled by something else. */
void keybind_state_ensure()
{
	if (cpl.inventory_win != IWIN_BELOW && !keybind_command_matches_state("?INVENTORY"))
	{
		keybind_process_command_up("?INVENTORY");
	}

	if (cpl.run_on && !keybind_command_matches_state("?RUNON"))
	{
		keybind_process_command_up("?RUNON");
	}

	if (cpl.fire_on && !keybind_command_matches_state("?FIREON"))
	{
		keybind_process_command_up("?FIREON");
	}
}

/**
 * Handle keybinding 'key down' event.
 * @param cmd Keybinding command to handle.
 * @return 1 if the command was handled, 0 otherwise. */
int keybind_process_command(const char *cmd)
{
	if (*cmd == '?')
	{
		int tag = 0, loc = 0;
		object *it;

		cmd++;

		if (!strncmp(cmd, "MOVE_", 5))
		{
			cmd += 5;

			if (!strcmp(cmd, "N"))
			{
				move_keys(8);
			}
			else if (!strcmp(cmd, "NE"))
			{
				move_keys(9);
			}
			else if (!strcmp(cmd, "E"))
			{
				move_keys(6);
			}
			else if (!strcmp(cmd, "SE"))
			{
				move_keys(3);
			}
			else if (!strcmp(cmd, "S"))
			{
				move_keys(2);
			}
			else if (!strcmp(cmd, "SW"))
			{
				move_keys(1);
			}
			else if (!strcmp(cmd, "W"))
			{
				move_keys(4);
			}
			else if (!strcmp(cmd, "NW"))
			{
				move_keys(7);
			}
			else if (!strcmp(cmd, "N"))
			{
				move_keys(8);
			}
			else if (!strcmp(cmd, "STAY"))
			{
				move_keys(5);
			}
		}
		else if (!strncmp(cmd, "PAGE", 4))
		{
			widgetdata *widget;
			int scroll_adjust = 0;

			cmd += 4;

			if (!strncmp(cmd, "UP", 2))
			{
				widget = cur_widget[*(cmd + 2) == '\0' ? CHATWIN_ID : MSGWIN_ID];
				scroll_adjust = -TEXTWIN_ROWS_VISIBLE(widget);
			}
			else if (!strncmp(cmd, "DOWN", 4))
			{
				widget = cur_widget[*(cmd + 4) == '\0' ? CHATWIN_ID : MSGWIN_ID];
				scroll_adjust = TEXTWIN_ROWS_VISIBLE(widget);
			}

			if (scroll_adjust)
			{
				TEXTWIN(widget)->scroll += scroll_adjust;
				textwin_scroll_adjust(widget);
				WIDGET_REDRAW(widget);
			}
		}
		else if (!strcmp(cmd, "CONSOLE"))
		{
			if (cpl.input_mode == INPUT_MODE_NO)
			{
				cpl.input_mode = INPUT_MODE_CONSOLE;
				text_input_open(253);
			}
			else if (cpl.input_mode == INPUT_MODE_CONSOLE)
			{
				cpl.input_mode = INPUT_MODE_NO;
			}
		}
		else if (!strcmp(cmd, "APPLY"))
		{
			if (cpl.inventory_win == IWIN_BELOW)
			{
				tag = cpl.win_below_tag;
			}
			else
			{
				tag = cpl.win_inv_tag;
			}

			if (tag == -1 || !object_find(tag))
			{
				return 0;
			}

			draw_info_format(COLOR_DGOLD, "apply %s", object_find(tag)->s_name);
			client_send_apply(tag);
		}
		else if (!strcmp(cmd, "EXAMINE"))
		{
			if (cpl.inventory_win == IWIN_BELOW)
			{
				tag = cpl.win_below_tag;
			}
			else
			{
				tag = cpl.win_inv_tag;
			}

			if (tag == -1 || !object_find(tag))
			{
				return 0;
			}

			draw_info_format(COLOR_DGOLD, "examine %s", object_find(tag)->s_name);
			client_send_examine(tag);
		}
		else if (!strcmp(cmd, "MARK"))
		{
			if (cpl.inventory_win == IWIN_BELOW)
			{
				tag = cpl.win_below_tag;
			}
			else
			{
				tag = cpl.win_inv_tag;
			}

			if (tag == -1 || !object_find(tag))
			{
				return 0;
			}

			it = object_find(tag);
			draw_info_format(COLOR_DGOLD, "%smark %s", it->tag == cpl.mark_count ? "un" : "", it->s_name);
			object_send_mark(it);
		}
		else if (!strcmp(cmd, "LOCK"))
		{
			if (cpl.inventory_win == IWIN_BELOW)
			{
				tag = cpl.win_below_tag;
			}
			else
			{
				tag = cpl.win_inv_tag;
			}

			if (tag == -1  || !object_find(tag))
			{
				return 0;
			}

			it = object_find(tag);
			toggle_locked(it);

			if (it->flags & F_LOCKED)
			{
				draw_info_format(COLOR_DGOLD, "unlock %s", it->s_name);
			}
			else
			{
				draw_info_format(COLOR_DGOLD, "lock %s", it->s_name);
			}
		}
		else if (!strcmp(cmd, "GET"))
		{
			int nrof = 1;

			/* From below to inv. */
			if (cpl.inventory_win == IWIN_BELOW)
			{
				tag = cpl.win_below_tag;

				if (cpl.container)
				{
					if (cpl.container->tag != cpl.win_below_ctag)
					{
						loc = cpl.container->tag;
					}
					else
					{
						loc = cpl.ob->tag;
					}
				}
				else
				{
					loc = cpl.ob->tag;
				}
			}
			/* Inventory */
			else
			{
				if (cpl.container)
				{
					if (cpl.container->tag == cpl.win_inv_ctag)
					{
						tag = cpl.win_inv_tag;
						loc = cpl.ob->tag;
					}
					/* From inventory to container - if the container is in inv. */
					else
					{
						object *tmp;

						tag = -1;

						if (cpl.ob)
						{
							for (tmp = cpl.ob->inv; tmp; tmp = tmp->next)
							{
								if (tmp->tag == cpl.container->tag)
								{
									tag = cpl.win_inv_tag;
									loc = cpl.container->tag;
									break;
								}
							}

							if (tag == -1)
							{
								draw_info(COLOR_DGOLD, "You already have it.");
							}
						}
					}
				}
				else
				{
					draw_info(COLOR_DGOLD, "You have no open container to put it in.");
					tag = -1;
				}
			}

			if (tag == -1 || !object_find(tag))
			{
				return 0;
			}

			it = object_find(tag);
			nrof = it->nrof;

			if (nrof == 1)
			{
				nrof = 0;
			}
			else if (!(setting_get_int(OPT_CAT_GENERAL, OPT_COLLECT_MODE) & 1))
			{
				char buf[MAX_BUF];

				cpl.input_mode = INPUT_MODE_NUMBER;
				text_input_open(22);
				cpl.loc = loc;
				cpl.tag = tag;
				cpl.nrof = nrof;
				cpl.nummode = NUM_MODE_GET;
				snprintf(buf, sizeof(buf), "%d", nrof);
				text_input_add_string(buf);
				strncpy(cpl.num_text, it->s_name, sizeof(cpl.num_text) - 1);
				cpl.num_text[sizeof(cpl.num_text) - 1] = '\0';
				return 0;
			}

			draw_info_format(COLOR_DGOLD, "get %s", it->s_name);
			client_send_move(loc, tag, nrof);
			sound_play_effect("get.ogg", 100);
		}
		else if (!strcmp(cmd, "DROP"))
		{
			int nrof = 1;

			/* Drop from inventory */
			if (cpl.inventory_win == IWIN_INV)
			{
				object *tmp;

				tag = cpl.win_inv_tag;
				loc = cpl.below->tag;

				if (cpl.win_inv_ctag == -1 && cpl.container && cpl.below)
				{
					for (tmp = cpl.below->inv; tmp; tmp = tmp->next)
					{
						if (tmp->tag == cpl.container->tag)
						{
							tag = cpl.win_inv_tag;
							loc = cpl.container->tag;
							break;
						}
					}
				}
			}
			else
			{
				draw_info(COLOR_DGOLD, "The item is already on the floor.");
				return 0;
			}

			if (tag == -1 || !object_find(tag))
			{
				return 0;
			}

			it = object_find(tag);
			nrof = it->nrof;

			if (it->flags & F_LOCKED)
			{
				draw_info(COLOR_DGOLD, "Unlock item first!");
				return 0;
			}

			if (nrof == 1)
			{
				nrof = 0;
			}
			else if (!(setting_get_int(OPT_CAT_GENERAL, OPT_COLLECT_MODE) & 2))
			{
				char buf[MAX_BUF];

				cpl.input_mode = INPUT_MODE_NUMBER;
				text_input_open(22);
				cpl.loc = loc;
				cpl.tag = tag;
				cpl.nrof = nrof;
				cpl.nummode = NUM_MODE_DROP;
				snprintf(buf, sizeof(buf), "%d", nrof);
				text_input_add_string(buf);
				strncpy(cpl.num_text, it->s_name, sizeof(cpl.num_text) - 1);
				cpl.num_text[sizeof(cpl.num_text) - 1] = '\0';
				return 0;
			}

			draw_info_format(COLOR_DGOLD, "drop %s", it->s_name);
			client_send_move(loc, tag, nrof);
			sound_play_effect("drop.ogg", 100);
		}
		else if (!strcmp(cmd, "HELP"))
		{
			show_help("main");
		}
		else if (!strcmp(cmd, "QLIST"))
		{
			cs_write_string("qlist", 5);
		}
		else if (!strcmp(cmd, "RANGE"))
		{
			if (RangeFireMode++ == FIRE_MODE_INIT - 1)
			{
				RangeFireMode = 0;
			}
		}
		else if (!strcmp(cmd, "TARGET_ENEMY"))
		{
			send_command("/target 0");
		}
		else if (!strcmp(cmd, "TARGET_FRIEND"))
		{
			send_command("/target 1");
		}
		else if (!strcmp(cmd, "TARGET_SELF"))
		{
			send_command("/target 2");
		}
		else if (!strcmp(cmd, "COMBAT"))
		{
			send_command("/combat");
		}
		else if (!strcmp(cmd, "FIRE_READY"))
		{
			if (cpl.inventory_win == IWIN_INV && cpl.win_inv_tag != -1)
			{
				ready_object(object_find(cpl.win_inv_tag));
			}
		}
		else if (!strcmp(cmd, "SPELL_LIST"))
		{
			cur_widget[SPELLS_ID]->show = !cur_widget[SPELLS_ID]->show;
			SetPriorityWidget(cur_widget[SPELLS_ID]);
		}
		else if (!strcmp(cmd, "SKILL_LIST"))
		{
			cur_widget[SKILLS_ID]->show = !cur_widget[SKILLS_ID]->show;
			SetPriorityWidget(cur_widget[SKILLS_ID]);
		}
		else if (!strcmp(cmd, "PARTY_LIST"))
		{
			if (cur_widget[PARTY_ID]->show)
			{
				cur_widget[PARTY_ID]->show = 0;
			}
			else
			{
				send_command("/party list");
			}
		}
		else if (!strncmp(cmd, "MCON ", 4))
		{
			keybind_process_command("?CONSOLE");
			text_input_add_string(cmd + 4);
		}
		else if (!strcmp(cmd, "UP"))
		{
			cursor_keys(0);
		}
		else if (!strcmp(cmd, "DOWN"))
		{
			cursor_keys(1);
		}
		else if (!strcmp(cmd, "LEFT"))
		{
			cursor_keys(2);
		}
		else if (!strcmp(cmd, "RIGHT"))
		{
			cursor_keys(3);
		}
		else if (!strncmp(cmd, "INVENTORY", 9))
		{
			if (!strcmp(cmd + 9, "_TOGGLE"))
			{
				if (cpl.inventory_win == IWIN_INV)
				{
					cpl.inventory_win = IWIN_BELOW;
					return 1;
				}
			}

			SetPriorityWidget(cur_widget[MAIN_INV_ID]);

			if (!setting_get_int(OPT_CAT_GENERAL, OPT_PLAYERDOLL))
			{
				SetPriorityWidget(cur_widget[PDOLL_ID]);
			}

			cpl.inventory_win = IWIN_INV;
		}
		else if (!strncmp(cmd, "RUNON", 5))
		{
			if (!strcmp(cmd + 5, "_TOGGLE"))
			{
				cpl.run_on = !cpl.run_on;
			}
			else
			{
				cpl.run_on = 1;
			}
		}
		else if (!strncmp(cmd, "FIREON", 6))
		{
			if (!strcmp(cmd + 6, "_TOGGLE"))
			{
				cpl.fire_on = !cpl.fire_on;
			}
			else
			{
				cpl.fire_on = 1;
			}
		}
		else if (!strncmp(cmd, "QUICKSLOT_", 10))
		{
			cmd += 10;

			if (!strcmp(cmd, "GROUP_PREV"))
			{
				quickslot_group--;

				if (quickslot_group < 1)
				{
					quickslot_group = MAX_QUICKSLOT_GROUPS;
				}
			}
			else if (!strcmp(cmd, "GROUP_NEXT"))
			{
				quickslot_group++;

				if (quickslot_group > MAX_QUICKSLOT_GROUPS)
				{
					quickslot_group = 1;
				}
			}
			else
			{
				quickslots_handle_key(MAX(1, MIN(8, atoi(cmd))) - 1);
			}
		}
		else if (!strcmp(cmd, "COPY"))
		{
			textwin_handle_copy();
		}

		return 1;
	}
	else
	{
		draw_info(COLOR_DGOLD, cmd);

		if (!client_command_check(cmd))
		{
			send_command(cmd);
		}
	}

	return 0;
}
