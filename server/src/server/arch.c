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
 * Arch related functions. */

#include <global.h>
#include <loader.h>

/** If set, does a little timing on the archetype load. */
#define TIME_ARCH_LOAD 0

/**
 * Archetype hash table structure. */
typedef struct arch_table_struct
{
	/** The archetype. */
	archetype *arch;

	/** Hash handle. */
	UT_hash_handle hh;
} arch_table_struct;

/** The archetype hash table. */
static archetype *arch_table = NULL;

/** True if doing arch initialization */
int arch_init;

static void load_archetypes();

/**
 * Finds, using the hashtable, which archetype matches the given name.
 * @return Pointer to the found archetype, otherwise NULL. */
archetype *find_archetype(const char *name)
{
	archetype *at;

	if (name == NULL)
	{
		return NULL;
	}

	HASH_FIND_STR(arch_table, name, at);

	return at;
}

/**
 * Adds an archetype to the hashtable. */
void arch_add(archetype *at)
{
	HASH_ADD_KEYPTR(hh, arch_table, at->name, strlen(at->name), at);
}

/**
 * Get the skill object for skill ID.
 * @param skillnr The skill number to find object for.
 * @return Object if found, NULL otherwise. */
archetype *get_skill_archetype(int skillnr)
{
	archetype *at;

	for (at = first_archetype; at != NULL; at = at->next)
	{
		if (at->clone.type == SKILL && at->clone.stats.sp == skillnr)
		{
			return at;
		}
	}

	return NULL;
}

/**
 * Initializes the internal linked list of archetypes (read from file).
 * Some commonly used archetype pointers like ::empty_archetype,
 * ::base_info_archetype are initialized.
 *
 * Can be called multiple times, will just return. */
void init_archetypes(void)
{
	/* Only do this once */
	if (first_archetype != NULL)
	{
		return;
	}

	arch_init = 1;
	load_archetypes();
	arch_init = 0;
	empty_archetype = find_archetype("empty_archetype");
	base_info_archetype = find_archetype("base_info");
	wp_archetype = find_archetype("waypoint");
}

/**
 * An alternative way to init the hashtable which is slower, but
 * _works_... */
static void init_archetable(void)
{
	archetype *at;

	LOG(llevDebug, "Setting up archetable...");

	for (at = first_archetype; at != NULL; at = (at->more == NULL) ? at->next : at->more)
	{
		arch_add(at);
	}

	LOG(llevDebug, " done.\n");
}

/**
 * Dumps _all_ archetypes and artifacts to debug-level output. */
void dump_all_archetypes(void)
{
	archetype *at;
	artifactlist *al;
	artifact *art = NULL;
	StringBuffer *sb;
	char *diff;

	for (at = first_archetype; at != NULL; at = (at->more == NULL) ? at->next : at->more)
	{
		sb = stringbuffer_new();
		dump_object(&at->clone, sb);
		diff = stringbuffer_finish(sb);
		LOG(llevInfo, "%s\n", diff);
		free(diff);
	}

	LOG(llevInfo, "Artifacts fake arch list:\n");

	for (al = first_artifactlist; al != NULL; al = al->next)
	{
		art = al->items;

		do
		{
			sb = stringbuffer_new();
			dump_object(&art->def_at.clone, sb);
			diff = stringbuffer_finish(sb);
			LOG(llevInfo, "%s\n", diff);
			free(diff);
			art = art->next;
		}
		while (art);
	}
}

/**
 * Frees all memory allocated to archetypes.
 *
 * After calling this, it's possible to call again init_archetypes() to
 * reload data. */
void free_all_archs(void)
{
	archetype *at, *next;
	int i = 0;

	for (at = first_archetype; at != NULL; at = next)
	{
		if (at->more)
		{
			next = at->more;
		}
		else
		{
			next = at->next;
		}

		FREE_AND_CLEAR_HASH(at->name);
		FREE_AND_CLEAR_HASH(at->clone.name);
		FREE_AND_CLEAR_HASH(at->clone.title);
		FREE_AND_CLEAR_HASH(at->clone.race);
		FREE_AND_CLEAR_HASH(at->clone.slaying);
		FREE_AND_CLEAR_HASH(at->clone.msg);
		free_key_values(&at->clone);
		free(at);
		i++;
	}

	LOG(llevDebug, "Freed %d archetypes\n", i);
}

/**
 * Allocates, initializes and returns the pointer to an archetype
 * structure.
 * @return New archetype structure, will never be NULL. */
static archetype *get_archetype_struct(void)
{
	archetype *new;

	new = (archetype *) CALLOC(1, sizeof(archetype));

	if (new == NULL)
	{
		LOG(llevError, "get_archetype_struct(): Out of memory\n");
	}

	/* To initial state other also */
	initialize_object(&new->clone);

	return new;
}

/**
 * Reads/parses the archetype-file, and copies into a linked list
 * of archetype structures.
 * @param fp Opened file descriptor which will be used to read the
 * archetypes. */
static void first_arch_pass(FILE *fp)
{
	object *op;
	void *mybuffer;
	archetype *at,*prev = NULL, *last_more = NULL;
	int i, first = 2;

	op = get_object();
	op->arch = first_archetype = at = get_archetype_struct();
	mybuffer = create_loader_buffer(fp);

	while ((i = load_object(fp, op, mybuffer, first, MAP_STYLE)))
	{
		first = 0;

		copy_object(op, &at->clone, 1);

		/* Now we have the right speed_left value for out object.
		 * copy_object() now will track down negative speed values, to
		 * alter speed_left to guarantee a random & sensible start value. */
		if (!op->layer && !QUERY_FLAG(op, FLAG_SYS_OBJECT))
		{
			LOG(llevDebug, "Archetype %s has layer 0 without being sys_object!\n", STRING_OBJ_ARCH_NAME(op));
		}

		if (op->layer && QUERY_FLAG(op, FLAG_SYS_OBJECT))
		{
			LOG(llevDebug, "Archetype %s has layer %d (!= 0) and is sys_object!\n", STRING_OBJ_ARCH_NAME(op), op->layer);
		}

		switch (i)
		{
			/* A new archetype, just link it with the previous */
			case LL_NORMAL:
				if (last_more != NULL)
				{
					last_more->next = at;
				}

				if (prev != NULL)
				{
					prev->next = at;
				}

				prev = last_more = at;

				if (!op->type)
				{
					LOG(llevDebug, "Archetype %s has no type!\n", STRING_OBJ_ARCH_NAME(op));
				}

				break;

			/* Another part of the previous archetype, link it correctly */
			case LL_MORE:
				at->head = prev;
				at->clone.head = &prev->clone;

				if (last_more != NULL)
				{
					last_more->more = at;
					last_more->clone.more = &at->clone;
				}

				last_more = at;

				break;
		}

		at = get_archetype_struct();
		initialize_object(op);
		op->arch = at;
	}

	delete_loader_buffer(mybuffer);
	/* Make sure our temp object is gc:ed */
	mark_object_removed(op);
	free(at);
}

/**
 * Reads the archetype file once more, and links all pointers between
 * archetypes and treasure lists. Must be called after first_arch_pass().
 * @param fp_start File from which to read. Won't be rewinded. */
static void second_arch_pass(FILE *fp_start)
{
	FILE *fp = fp_start;
	int comp;
	char filename[MAX_BUF], buf[MAX_BUF], *variable = buf, *argument, *cp;
	archetype *at = NULL, *other;
	object *inv;

	while (fgets(buf, MAX_BUF, fp) != NULL)
	{
		if (*buf == '#')
		{
			continue;
		}

		if ((argument = strchr(buf, ' ')) != NULL)
		{
			*argument = '\0', argument++;
			cp = argument + strlen(argument) - 1;

			while (isspace(*cp))
			{
				*cp = '\0';
				cp--;
			}
		}

		if (!strcmp("Object", variable))
		{
			if ((at = find_archetype(argument)) == NULL)
			{
				LOG(llevBug, "Failed to find arch %s\n", STRING_SAFE(argument));
			}
		}
		else if (!strcmp("other_arch", variable))
		{
			if (at != NULL && at->clone.other_arch == NULL)
			{
				if ((other = find_archetype(argument)) == NULL)
				{
					LOG(llevBug, "Failed to find other_arch %s\n", STRING_SAFE(argument));
				}
				else if (at != NULL)
				{
					at->clone.other_arch = other;
				}
			}
		}
		else if (!strcmp("randomitems", variable))
		{
			if (at != NULL)
			{
				treasurelist *tl = find_treasurelist(argument);

				if (tl == NULL)
				{
					LOG(llevBug, "Failed to link treasure to arch. (arch: %s ->%s\n", STRING_OBJ_NAME(&at->clone), STRING_SAFE(argument));
				}
				else
				{
					at->clone.randomitems = tl;
				}
			}
		}
		else if (!strcmp("arch", variable))
		{
			inv = get_archetype(argument);
			load_object(fp, inv, NULL, LO_LINEMODE, 0);

			if (at)
			{
				insert_ob_in_ob(inv, &at->clone);
			}
			else
			{
				LOG(llevError, "Got an arch %s not inside an Object.\n", argument);
			}
		}
	}

	/* Now re-parse the artifacts file too! */
	snprintf(filename, sizeof(filename), "%s/artifacts", settings.datadir);

	if ((fp = open_and_uncompress(filename, 0, &comp)) == NULL)
	{
		LOG(llevError, "Can't open %s.\n", filename);
		return;
	}

	while (fgets(buf, MAX_BUF, fp) != NULL)
	{
		if (*buf == '#')
		{
			continue;
		}

		if ((argument = strchr(buf, ' ')) != NULL)
		{
			*argument = '\0', argument++;
			cp = argument + strlen(argument) - 1;

			while (isspace(*cp))
			{
				*cp = '\0';
				cp--;
			}
		}

		/* Now we get our artifact. if we hit "def_arch", we first copy
		 * from it other_arch and treasure list to our artifact. Then we
		 * search the object for other_arch and randomitems - perhaps we
		 * override them here. */
		if (!strcmp("artifact", variable))
		{
			if ((at = find_archetype(argument)) == NULL)
			{
				LOG(llevBug, "Second artifacts pass: Failed to find artifact %s\n", STRING_SAFE(argument));
			}
		}
		else if (!strcmp("def_arch", variable))
		{
			if ((other = find_archetype(argument)) == NULL)
			{
				LOG(llevBug, "Second artifacts pass: Failed to find def_arch %s from artifact %s\n", STRING_SAFE(argument), STRING_ARCH_NAME(at));
			}

			/* now copy from real arch the stuff from above to our "fake" arches */
			at->clone.other_arch = other->clone.other_arch;
			at->clone.randomitems = other->clone.randomitems;
		}
		else if (!strcmp("other_arch", variable))
		{
			if ((other = find_archetype(argument)) == NULL)
			{
				LOG(llevBug, "Second artifacts pass: Failed to find other_arch %s\n", STRING_SAFE(argument));
			}
			else if (at != NULL)
			{
				at->clone.other_arch = other;
			}
		}
		else if (!strcmp("randomitems", variable))
		{
			treasurelist *tl = find_treasurelist(argument);

			if (tl == NULL)
			{
				LOG(llevBug, "Second artifacts pass: Failed to link treasure to arch. (arch: %s ->%s)\n", STRING_OBJ_NAME(&at->clone), STRING_SAFE(argument));
			}
			else if (at != NULL)
			{
				at->clone.randomitems = tl;
			}
		}
	}

	close_and_delete(fp, comp);
}

/**
 * Loads all archetypes and treasures.
 *
 * First initializes the archtype hash-table (init_archetable()).
 * Reads and parses the archetype file (with the first and second-pass
 * functions).
 * Then initializes treasures by calling load_treasures(). */
static void load_archetypes(void)
{
	FILE *fp;
	char filename[MAX_BUF];
	int comp;
#if TIME_ARCH_LOAD
	struct timeval tv1, tv2;
#endif

	snprintf(filename, sizeof(filename), "%s/%s", settings.datadir, settings.archetypes);
	LOG(llevDebug, "Reading archetypes from %s...\n", filename);

	if ((fp = open_and_uncompress(filename, 0, &comp)) == NULL)
	{
		LOG(llevError, "Can't open archetype file.\n");
		return;
	}

	LOG(llevDebug, "arch-pass 1...\n");

#if TIME_ARCH_LOAD
	GETTIMEOFDAY(&tv1);
#endif

	first_arch_pass(fp);

#if TIME_ARCH_LOAD
	int sec, usec;
	GETTIMEOFDAY(&tv2);
	sec = tv2.tv_sec - tv1.tv_sec;
	usec = tv2.tv_usec - tv1.tv_usec;

	if (usec < 0)
	{
		usec += 1000000;
		sec--;
	}

	LOG(llevDebug, "Load took %d.%06d seconds\n", sec, usec);
#endif

	LOG(llevDebug, " done.\n");
	init_archetable();

	/* Do a close and reopen instead of a rewind - necessary in case the
	 * file has been compressed. */
	close_and_delete(fp, comp);
	fp = open_and_uncompress(filename, 0, &comp);

	/* If not called before, reads all artifacts from file */
	init_artifacts();
	LOG(llevDebug, " loading treasure...\n");
	load_treasures();
	LOG(llevDebug, " done\n arch-pass 2...\n");
	second_arch_pass(fp);
	LOG(llevDebug, " done.\n");

	close_and_delete(fp, comp);
	LOG(llevDebug, "Reading archetypes done.\n");
}

/**
 * Creates and returns a new object which is a copy of the given
 * archetype.
 * @param at Archetype from which to get an object.
 * @return Object of specified type. */
object *arch_to_object(archetype *at)
{
	object *op;

	if (at == NULL)
	{
		LOG(llevBug, "arch_to_object(): Archetype at is NULL.\n");
		return NULL;
	}

	op = get_object();
	copy_object_with_inv(&at->clone, op);
	op->arch = at;

	return op;
}

/**
 * Creates a dummy object. This function is called by get_archetype(void)
 * if it fails to find the appropriate archetype.
 *
 * Thus get_archetype() will be guaranteed to always return
 * an object, and never NULL.
 * @param name Name to give the dummy object.
 * @return Object of specified name. It fill have the ::FLAG_NO_PICK flag
 * set. */
object *create_singularity(const char *name)
{
	object *op;
	char buf[MAX_BUF];

	snprintf(buf, sizeof(buf), "singularity (%s)", name);
	op = get_object();
	FREE_AND_COPY_HASH(op->name, buf);
	SET_FLAG(op, FLAG_NO_PICK);

	return op;
}

/**
 * Finds which archetype matches the given name, and returns a new
 * object containing a copy of the archetype.
 * @param name Archetype name.
 * @return Object of specified archetype, or a singularity. Will never be
 * NULL. */
object *get_archetype(const char *name)
{
	archetype *at = find_archetype(name);

	if (at == NULL)
	{
		return create_singularity(name);
	}

	return arch_to_object(at);
}
