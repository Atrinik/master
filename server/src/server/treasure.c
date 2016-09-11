/*************************************************************************
 *           Atrinik, a Multiplayer Online Role Playing Game             *
 *                                                                       *
 *   Copyright (C) 2009-2014 Alex Tokar and Atrinik Development Team     *
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
 * Everything concerning treasures.
 */

/* TREASURE_DEBUG does some checking on the treasurelists after loading.
 * It is useful for finding bugs in the treasures file.  Since it only
 * slows the startup some (and not actual game play), it is by default
 * left on */
#define TREASURE_DEBUG

/* TREASURE_VERBOSE enables copious output concerning artifact generation */
/*#define TREASURE_VERBOSE*/

#include <global.h>
#include <loader.h>
#include <arch.h>
#include <artifact.h>
#include "object_methods.h"

/** All the coin arches. */
const char *const coins[NUM_COINS + 1] = {
    "ambercoin",
    "mithrilcoin",
    "jadecoin",
    "goldcoin",
    "silvercoin",
    "coppercoin",
    NULL
};

/** Pointers to coin archetypes. */
struct archetype *coins_arch[NUM_COINS];

/** Chance fix. */
#define CHANCE_FIX (-1)

static treasure *load_treasure(FILE *fp, int *t_style, int *a_chance);
static void change_treasure(struct _change_arch *ca, object *op);
static treasurelist *get_empty_treasurelist(void);
static treasure *get_empty_treasure(void);
static void put_treasure(object *op, object *creator, int flags);
static void check_treasurelist(treasure *t, treasurelist *tl);
static void set_material_real(object *op, struct _change_arch *change_arch);
static void create_money_table(void);
static void create_all_treasures(treasure *t, object *op, int flag, int difficulty, int t_style, int a_chance, int tries, struct _change_arch *change_arch);
static void create_one_treasure(treasurelist *tl, object *op, int flag, int difficulty, int t_style, int a_chance, int tries, struct _change_arch *change_arch);
static void free_treasurestruct(treasure *t);

/**
 * Opens LIBDIR/treasure and reads all treasure declarations from it.
 *
 * Each treasure is parsed with the help of load_treasure().
 */
void load_treasures(void)
{
    FILE *fp;
    char filename[MAX_BUF], buf[MAX_BUF], name[MAX_BUF];
    treasurelist *previous = NULL;
    treasure *t;
    int t_style, a_chance;

    snprintf(filename, sizeof(filename), "%s/treasures", settings.libpath);
    fp = fopen(filename, "rb");

    if (!fp) {
        LOG(ERROR, "Can't open treasures file: %s", filename);
        exit(1);
    }

    while (fgets(buf, MAX_BUF, fp) != NULL) {
        /* Ignore comments and blank lines */
        if (*buf == '#' || *buf == '\n') {
            continue;
        }

        if (sscanf(buf, "treasureone %s\n", name) || sscanf(buf, "treasure %s\n", name)) {
            treasurelist *tl = get_empty_treasurelist();
            FREE_AND_COPY_HASH(tl->name, name);

            if (previous == NULL) {
                first_treasurelist = tl;
            } else {
                previous->next = tl;
            }

            previous = tl;
            t_style = T_STYLE_UNSET;
            a_chance = ART_CHANCE_UNSET;
            tl->items = load_treasure(fp, &t_style, &a_chance);

            if (tl->t_style == T_STYLE_UNSET) {
                tl->t_style = t_style;
            }

            if (tl->artifact_chance == ART_CHANCE_UNSET) {
                tl->artifact_chance = a_chance;
            }

            /* This is a one of the many items on the list should be generated.
             * Add up the chance total, and check to make sure the yes and no
             * fields of the treasures are not being used. */
            if (!strncmp(buf, "treasureone", 11)) {
                for (t = tl->items; t != NULL; t = t->next) {
#ifdef TREASURE_DEBUG

                    if (t->next_yes || t->next_no) {
                        LOG(BUG, "Treasure %s is one item, but on treasure %s the next_yes or next_no field is set", tl->name, t->item ? t->item->name : t->name);
                    }
#endif
                    tl->total_chance += t->chance;
                }
            }
        } else {
            LOG(ERROR, "Treasure list didn't understand: %s", buf);
            exit(1);
        }
    }

    fclose(fp);

#ifdef TREASURE_DEBUG
    /* Perform some checks on how valid the treasure data actually is.
     * Verify that list transitions work (ie, the list that it is
     * supposed to transition to exists). Also, verify that at least the
     * name or archetype is set for each treasure element. */
    for (previous = first_treasurelist; previous != NULL; previous = previous->next) {
        check_treasurelist(previous->items, previous);
    }
#endif

    create_money_table();
}

/**
 * Create money table, setting up pointers to the archetypes.
 *
 * This is done for faster access of the coins archetypes.
 */
static void create_money_table(void)
{
    int i;

    for (i = 0; coins[i]; i++) {
        coins_arch[i] = arch_find(coins[i]);

        if (!coins_arch[i]) {
            LOG(ERROR, "Can't find %s.", coins[i] ? coins[i] : "NULL");
            exit(1);
        }
    }
}

/**
 * Reads one treasure from the file, including the 'yes', 'no' and 'more'
 * options.
 * @param fp
 * File to read from.
 * @param t_style
 * Treasure style.
 * @param a_chance
 * Artifact chance.
 * @return
 * Read structure, never NULL.
 */
static treasure *load_treasure(FILE *fp, int *t_style, int *a_chance)
{
    char buf[MAX_BUF], *cp = NULL, variable[MAX_BUF];
    treasure *t = get_empty_treasure();
    int value, start_marker = 0, t_style2, a_chance2;

    while (fgets(buf, MAX_BUF, fp) != NULL) {
        if (*buf == '#') {
            continue;
        }

        if ((cp = strchr(buf, '\n')) != NULL) {
            *cp = '\0';
        }

        cp = buf;

        /* Skip blanks */
        while (!isalpha(*cp)) {
            cp++;
        }

        if (sscanf(cp, "t_style %d", &value)) {
            if (start_marker) {
                t->t_style = value;
            } else {
                /* No, it's global for the while treasure list entry */
                *t_style = value;
            }
        } else if (sscanf(cp, "artifact_chance %d", &value)) {
            if (start_marker) {
                t->artifact_chance = value;
            } else {
                /* No, it's global for the while treasure list entry */
                *a_chance = value;
            }
        } else if (sscanf(cp, "arch %s", variable)) {
            if ((t->item = arch_find(variable)) == NULL) {
                LOG(BUG, "Treasure lacks archetype: %s", variable);
            }

            start_marker = 1;
        } else if (sscanf(cp, "list %s", variable)) {
            start_marker = 1;
            FREE_AND_COPY_HASH(t->name, variable);
        } else if (sscanf(cp, "name %s", variable)) {
            FREE_AND_COPY_HASH(t->change_arch.name, cp + 5);
        } else if (sscanf(cp, "title %s", variable)) {
            FREE_AND_COPY_HASH(t->change_arch.title, cp + 6);
        } else if (sscanf(cp, "slaying %s", variable)) {
            FREE_AND_COPY_HASH(t->change_arch.slaying, cp + 8);
        } else if (sscanf(cp, "item_race %d", &value)) {
            t->change_arch.item_race = value;
        } else if (sscanf(cp, "quality %d", &value)) {
            t->change_arch.quality = value;
        } else if (sscanf(cp, "quality_range %d", &value)) {
            t->change_arch.quality_range = value;
        } else if (sscanf(cp, "material %d", &value)) {
            t->change_arch.material = value;
        } else if (sscanf(cp, "material_quality %d", &value)) {
            t->change_arch.material_quality = value;
        } else if (sscanf(cp, "material_range %d", &value)) {
            t->change_arch.material_range = value;
        } else if (sscanf(cp, "chance_fix %d", &value)) {
            t->chance_fix = (int16_t) value;
            /* Important or the chance will stay 100% when not set to 0
             * in treasure list! */
            t->chance = 0;
        } else if (sscanf(cp, "chance %d", &value)) {
            t->chance = (uint8_t) value;
        } else if (sscanf(cp, "nrof %d", &value)) {
            t->nrof = (uint16_t) value;
        } else if (sscanf(cp, "magic %d", &value)) {
            t->magic = value;
        } else if (sscanf(cp, "magic_fix %d", &value)) {
            t->magic_fix = value;
        } else if (sscanf(cp, "magic_chance %d", &value)) {
            t->magic_chance = value;
        } else if (sscanf(cp, "difficulty %d", &value)) {
            t->difficulty = value;
        } else if (!strncmp(cp, "yes", 3)) {
            t_style2 = T_STYLE_UNSET;
            a_chance2 = ART_CHANCE_UNSET;
            t->next_yes = load_treasure(fp, &t_style2, &a_chance2);

            if (t->next_yes->artifact_chance == ART_CHANCE_UNSET) {
                t->next_yes->artifact_chance = a_chance2;
            }

            if (t->next_yes->t_style == T_STYLE_UNSET) {
                t->next_yes->t_style = t_style2;
            }
        } else if (!strncmp(cp, "no", 2)) {
            t_style2 = T_STYLE_UNSET;
            a_chance2 = ART_CHANCE_UNSET;
            t->next_no = load_treasure(fp, &t_style2, &a_chance2);

            if (t->next_no->artifact_chance == ART_CHANCE_UNSET) {
                t->next_no->artifact_chance = a_chance2;
            }

            if (t->next_no->t_style == T_STYLE_UNSET) {
                t->next_no->t_style = t_style2;
            }
        } else if (!strncmp(cp, "end", 3)) {
            return t;
        } else if (!strncmp(cp, "more", 4)) {
            t_style2 = T_STYLE_UNSET;
            a_chance2 = ART_CHANCE_UNSET;
            t->next = load_treasure(fp, &t_style2, &a_chance2);

            if (t->next->artifact_chance == ART_CHANCE_UNSET) {
                t->next->artifact_chance = a_chance2;
            }

            if (t->next->t_style == T_STYLE_UNSET) {
                t->next->t_style = t_style2;
            }

            return t;
        } else {
            LOG(BUG, "Unknown treasure command: '%s', last entry %s", cp, t->name ? t->name : "null");
        }
    }

    LOG(BUG, "Treasure %s lacks 'end'.>%s<", t->name ? t->name : "NULL", cp ? cp : "NULL");

    return t;
}

/**
 * Allocate and return the pointer to an empty treasurelist structure.
 * @return
 * New structure, blanked, never NULL.
 */
static treasurelist *get_empty_treasurelist(void)
{
    treasurelist *tl = emalloc(sizeof(treasurelist));

    tl->name = NULL;
    tl->next = NULL;
    tl->items = NULL;
    /* -2 is the "unset" marker and will be virtually handled as 0 which
     * can be overruled. */
    tl->t_style = T_STYLE_UNSET;
    tl->artifact_chance = ART_CHANCE_UNSET;
    tl->chance_fix = CHANCE_FIX;
    tl->total_chance = 0;

    return tl;
}

/**
 * Allocate and return the pointer to an empty treasure structure.
 * @return
 * New structure, blanked, never NULL.
 */
static treasure *get_empty_treasure(void)
{
    treasure *t = emalloc(sizeof(treasure));

    t->change_arch.item_race = -1;
    t->change_arch.name = NULL;
    t->change_arch.slaying = NULL;
    t->change_arch.title = NULL;
    /* -2 is the "unset" marker and will be virtually handled as 0 which
     * can be overruled. */
    t->t_style = T_STYLE_UNSET;
    t->change_arch.material = -1;
    t->change_arch.material_quality = -1;
    t->change_arch.material_range = -1;
    t->change_arch.quality = -1;
    t->change_arch.quality_range = -1;
    t->chance_fix = CHANCE_FIX;
    t->item = NULL;
    t->name = NULL;
    t->next = NULL;
    t->next_yes = NULL;
    t->next_no = NULL;
    t->artifact_chance = ART_CHANCE_UNSET;
    t->chance = 100;
    t->magic_fix = 0;
    t->difficulty = 0;
    t->magic_chance = 3;
    t->magic = 0;
    t->nrof = 0;

    return t;
}

/**
 * Searches for the given treasurelist in the globally linked list of
 * treasure lists which has been built by load_treasures().
 * @param name
 * Treasure list to search for.
 */
treasurelist *find_treasurelist(const char *name)
{
    const char *tmp = find_string(name);
    treasurelist *tl;

    /* Special cases - randomitems of none is to override default.  If
     * first_treasurelist is null, it means we are on the first pass of
     * of loading archetyps, so for now, just return - second pass will
     * init these values. */
    if (!strcmp(name, "none") || !first_treasurelist) {
        return NULL;
    }

    if (tmp != NULL) {
        for (tl = first_treasurelist; tl != NULL; tl = tl->next) {
            if (tmp == tl->name) {
                return tl;
            }
        }
    }

    LOG(BUG, "Bug: Couldn't find treasurelist %s", name);
    return NULL;
}

/**
 * This is similar to the old generate treasure function. However, it
 * instead takes a treasurelist. It is really just a wrapper around
 * create_treasure(). We create a dummy object that the treasure gets
 * inserted into, and then return that treasure.
 * @param t
 * Treasure list to generate from.
 * @param difficulty
 * Treasure difficulty.
 * @return
 * Generated treasure. Can be NULL if no suitable treasure was
 * found.
 */
object *generate_treasure(treasurelist *t, int difficulty, int a_chance)
{
    object *ob = object_get(), *tmp;

    create_treasure(t, ob, 0, difficulty, t->t_style, a_chance, 0, NULL);

    /* Don't want to free the object we are about to return */
    tmp = ob->inv;

    /* Remove from inv - no move off */
    if (tmp != NULL) {
        object_remove(tmp, 0);
    }

    if (ob->inv) {
        LOG(BUG, "Created multiple objects.");
    }

    object_destroy(ob);

    return tmp;
}

/**
 * This calls the appropriate treasure creation function.
 * @param t
 * What to generate.
 * @param op
 * For who to generate the treasure.
 * @param flag
 * Combination of @ref GT_xxx values.
 * @param difficulty
 * Map difficulty.
 * @param t_style
 * Treasure style.
 * @param a_chance
 * Artifact chance.
 * @param tries
 * To avoid infinite recursion.
 * @param arch_change
 * Arch change.
 */
void create_treasure(treasurelist *t, object *op, int flag, int difficulty, int t_style, int a_chance, int tries, struct _change_arch *arch_change)
{
    if (tries++ > 100) {
        LOG(DEBUG, "create_treasure(): tries >100 for t-list %s.", t->name ? t->name : "<noname>");
        return;
    }

    if (t->t_style != T_STYLE_UNSET) {
        t_style = t->t_style;
    }

    if (t->artifact_chance != ART_CHANCE_UNSET) {
        a_chance = t->artifact_chance;
    }

    if (t->total_chance) {
        create_one_treasure(t, op, flag, difficulty, t_style, a_chance, tries, arch_change);
    } else {
        create_all_treasures(t->items, op, flag, difficulty, t_style, a_chance, tries, arch_change);
    }
}

/**
 * Creates all the treasures.
 * @param t
 * What to generate.
 * @param op
 * For who to generate the treasure.
 * @param flag
 * Combination of @ref GT_xxx values.
 * @param difficulty
 * Map difficulty.
 * @param t_style
 * Treasure style.
 * @param a_chance
 * Artifact chance.
 * @param tries
 * To avoid infinite recursion.
 * @param change_arch
 * Arch change.
 */
static void create_all_treasures(treasure *t, object *op, int flag, int difficulty, int t_style, int a_chance, int tries, struct _change_arch *change_arch)
{
    object *tmp;

    if (t->t_style != T_STYLE_UNSET) {
        t_style = t->t_style;
    }

    if (t->artifact_chance != ART_CHANCE_UNSET) {
        a_chance = t->artifact_chance;
    }

    if ((t->chance_fix != CHANCE_FIX && rndm_chance(t->chance_fix)) || (int) t->chance >= 100 || (rndm(1, 100) < (int) t->chance)) {
        if (t->name) {
            if (t->name != shstr_cons.NONE && difficulty >= t->difficulty) {
                create_treasure(find_treasurelist(t->name), op, flag, difficulty, t_style, a_chance, tries, change_arch ? change_arch : &t->change_arch);
            }
        } else if (difficulty >= t->difficulty) {
            if (t->item->clone.type != WEALTH) {
                tmp = arch_to_object(t->item);

                if (t->nrof && tmp->nrof <= 1) {
                    tmp->nrof = rndm(1, t->nrof);
                }

                /* Ret 1 = artifact is generated - don't overwrite anything
                 * here */
                set_material_real(tmp, change_arch ? change_arch : &t->change_arch);

                if (!fix_generated_item(&tmp, op, difficulty, a_chance, t_style, t->magic, t->magic_fix, t->magic_chance, flag)) {
                    change_treasure(change_arch ? change_arch : &t->change_arch, tmp);
                }

                put_treasure(tmp, op, flag);

                /* If treasure is "identified", created items are too */
                if (op->type == TREASURE && QUERY_FLAG(op, FLAG_IDENTIFIED)) {
                    SET_FLAG(tmp, FLAG_IDENTIFIED);
                }
            } else {
                /* We have a wealth object - expand it to real money */

                /* If t->magic is != 0, that's our value - if not use
                 * default setting */
                int i, value = t->magic ? t->magic : t->item->clone.value;

                value *= (difficulty / 2) + 1;

                /* So we have 80% to 120% of the fixed value */
                value = (int) ((float) value * 0.8f + (float) value * (rndm(1, 40) / 100.0f));

                for (i = 0; i < NUM_COINS; i++) {
                    if (value / coins_arch[i]->clone.value > 0) {
                        tmp = object_get();
                        object_copy(tmp, &coins_arch[i]->clone, false);
                        tmp->nrof = value / tmp->value;
                        value -= tmp->nrof * tmp->value;
                        put_treasure(tmp, op, flag);
                    }
                }
            }
        }

        if (t->next_yes != NULL) {
            create_all_treasures(t->next_yes, op, flag, difficulty, (t->next_yes->t_style == T_STYLE_UNSET) ? t_style : t->next_yes->t_style, a_chance, tries, change_arch);
        }
    } else if (t->next_no != NULL) {
        create_all_treasures(t->next_no, op, flag, difficulty, (t->next_no->t_style == T_STYLE_UNSET) ? t_style : t->next_no->t_style, a_chance, tries, change_arch);
    }

    if (t->next != NULL) {
        create_all_treasures(t->next, op, flag, difficulty, (t->next->t_style == T_STYLE_UNSET) ? t_style : t->next->t_style, a_chance, tries, change_arch);
    }
}

/**
 * Creates one treasure from the list.
 * @param tl
 * What to generate.
 * @param op
 * For who to generate the treasure.
 * @param flag
 * Combination of @ref GT_xxx values.
 * @param difficulty
 * Map difficulty.
 * @param t_style
 * Treasure style.
 * @param a_chance
 * Artifact chance.
 * @param tries
 * To avoid infinite recursion.
 * @param change_arch
 * Arch change.
 * @todo Get rid of the goto.
 */
static void create_one_treasure(treasurelist *tl, object *op, int flag, int difficulty, int t_style, int a_chance, int tries, struct _change_arch *change_arch)
{
    int value, diff_tries = 0;
    treasure *t;
    object *tmp;

    if (tries++ > 100) {
        return;
    }

    /* Well, at some point we should rework this whole system... */
create_one_treasure_again_jmp:

    if (diff_tries > 10) {
        return;
    }

    value = rndm(1, tl->total_chance) - 1;

    for (t = tl->items; t != NULL; t = t->next) {
        /* chance_fix will overrule the normal chance stuff!. */
        if (t->chance_fix != CHANCE_FIX) {
            if (rndm_chance(t->chance_fix)) {
                /* Only when allowed, we go on! */
                if (difficulty >= t->difficulty) {
                    value = 0;
                    break;
                }

                /* Ok, difficulty is bad let's try again or break! */
                if (tries++ > 100) {
                    return;
                }

                diff_tries++;
                goto create_one_treasure_again_jmp;
            }

            if (!t->chance) {
                continue;
            }
        }

        value -= t->chance;

        /* We got one! */
        if (value <= 0) {
            /* Only when allowed, we go on! */
            if (difficulty >= t->difficulty) {
                break;
            }

            /* Ok, difficulty is bad let's try again or break! */
            if (tries++ > 100) {
                return;
            }

            diff_tries++;
            goto create_one_treasure_again_jmp;
        }
    }

    if (!t || value > 0) {
        LOG(BUG, "create_one_treasure: got null object or not able to find treasure - tl:%s op:%s", tl ? tl->name : "(null)", op ? op->name : "(null)");
        return;
    }

    if (t->t_style != T_STYLE_UNSET) {
        t_style = t->t_style;
    }

    if (t->artifact_chance != ART_CHANCE_UNSET) {
        a_chance = t->artifact_chance;
    }

    if (t->name) {
        if (t->name == shstr_cons.NONE) {
            return;
        }

        if (difficulty >= t->difficulty) {
            create_treasure(find_treasurelist(t->name), op, flag, difficulty, t_style, a_chance, tries, change_arch);
        } else if (t->nrof) {
            create_one_treasure(tl, op, flag, difficulty, t_style, a_chance, tries, change_arch);
        }

        return;
    }

    if (t->item->clone.type != WEALTH) {
        tmp = arch_to_object(t->item);

        if (t->nrof && tmp->nrof <= 1) {
            tmp->nrof = rndm(1, t->nrof);
        }

        set_material_real(tmp, change_arch ? change_arch : &t->change_arch);

        if (!fix_generated_item(&tmp, op, difficulty, a_chance, (t->t_style == T_STYLE_UNSET) ? t_style : t->t_style, t->magic, t->magic_fix, t->magic_chance, flag)) {
            change_treasure(change_arch ? change_arch : &t->change_arch, tmp);
        }

        put_treasure(tmp, op, flag);

        /* If treasure is "identified", created items are too */
        if (op->type == TREASURE && QUERY_FLAG(op, FLAG_IDENTIFIED)) {
            SET_FLAG(tmp, FLAG_IDENTIFIED);
        }
    } else {
        /* We have a wealth object - expand it to real money */

        /* If t->magic is != 0, that's our value - if not use default
         * setting */
        int i;

        value = t->magic ? t->magic : t->item->clone.value;
        value *= difficulty;

        /* So we have 80% to 120% of the fixed value */
        value = (int) ((float) value * 0.8f + (float) value * (rndm(1, 40) / 100.0f));

        for (i = 0; i < NUM_COINS; i++) {
            if (value / coins_arch[i]->clone.value > 0) {
                tmp = object_get();
                object_copy(tmp, &coins_arch[i]->clone, false);
                tmp->nrof = value / tmp->value;
                value -= tmp->nrof * tmp->value;
                put_treasure(tmp, op, flag);
            }
        }
    }
}

/**
 * Inserts generated treasure where it should go.
 * @param op
 * Treasure just generated.
 * @param creator
 * For which object the treasure is being generated.
 * @param flags
 * Combination of @ref GT_xxx values.
 */
static void put_treasure(object *op, object *creator, int flags)
{
    if (flags & GT_ENVIRONMENT) {
        op->x = creator->x;
        op->y = creator->y;
        object_insert_map(op, creator->map, op, INS_NO_MERGE | INS_NO_WALK_ON);
    } else {
        object_insert_into(op, creator, 0);
    }
}

/**
 * If there are change_xxx commands in the treasure, we include the
 * changes in the generated object.
 * @param ca
 * Arch to change to.
 * @param op
 * Actual generated treasure.
 */
static void change_treasure(struct _change_arch *ca, object *op)
{
    if (ca->name) {
        FREE_AND_COPY_HASH(op->name, ca->name);
    }

    if (ca->title) {
        FREE_AND_COPY_HASH(op->title, ca->title);
    }

    if (ca->slaying) {
        FREE_AND_COPY_HASH(op->slaying, ca->slaying);
    }
}

/**
 * Difficulty to magic chance list.
 */
static const int difftomagic_list[DIFFLEVELS][MAXMAGIC + 1] = {
    /* Chance of magic  Difficulty */
    /* +0 +1 +2 +3 +4 */
    {94, 3, 2, 1, 0},   /*1*/
    {94, 3, 2, 1, 0},   /*2*/
    {94, 3, 2, 1, 0},   /*3*/
    {94, 3, 2, 1, 0},   /*4*/
    {94, 3, 2, 1, 0},   /*5*/
    {90, 4, 3, 2, 1},   /*6*/
    {90, 4, 3, 2, 1},   /*7*/
    {90, 4, 3, 2, 1},   /*8*/
    {90, 4, 3, 2, 1},   /*9*/
    {90, 4, 3, 2, 1},   /*10*/
    {85, 6, 4, 3, 2},   /*11*/
    {85, 6, 4, 3, 2},   /*12*/
    {85, 6, 4, 3, 2},   /*13*/
    {85, 6, 4, 3, 2},   /*14*/
    {85, 6, 4, 3, 2},   /*15*/
    {80, 8, 5, 4, 3},   /*16*/
    {80, 8, 5, 4, 3},   /*17*/
    {80, 8, 5, 4, 3},   /*18*/
    {80, 8, 5, 4, 3},   /*19*/
    {80, 8, 5, 4, 3},   /*20*/
    {75, 10, 6, 5, 4},  /*21*/
    {75, 10, 6, 5, 4},  /*22*/
    {75, 10, 6, 5, 4},  /*23*/
    {75, 10, 6, 5, 4},  /*24*/
    {75, 10, 6, 5, 4},  /*25*/
    {70, 12, 7, 6, 5},  /*26*/
    {70, 12, 7, 6, 5},  /*27*/
    {70, 12, 7, 6, 5},  /*28*/
    {70, 12, 7, 6, 5},  /*29*/
    {70, 12, 7, 6, 5},  /*30*/
    {70, 9, 8, 7, 6},   /*31*/
    {70, 9, 8, 7, 6},   /*32*/
    {70, 9, 8, 7, 6},   /*33*/
    {70, 9, 8, 7, 6},   /*34*/
    {70, 9, 8, 7, 6},   /*35*/
    {70, 6, 9, 8, 7},   /*36*/
    {70, 6, 9, 8, 7},   /*37*/
    {70, 6, 9, 8, 7},   /*38*/
    {70, 6, 9, 8, 7},   /*39*/
    {70, 6, 9, 8, 7},   /*40*/
    {70, 3, 10, 9, 8},  /*41*/
    {70, 3, 10, 9, 8},  /*42*/
    {70, 3, 10, 9, 8},  /*43*/
    {70, 3, 10, 9, 8},  /*44*/
    {70, 3, 10, 9, 8},  /*45*/
    {70, 2, 9, 10, 9},  /*46*/
    {70, 2, 9, 10, 9},  /*47*/
    {70, 2, 9, 10, 9},  /*48*/
    {70, 2, 9, 10, 9},  /*49*/
    {70, 2, 9, 10, 9},  /*50*/
    {70, 2, 7, 11, 10}, /*51*/
    {70, 2, 7, 11, 10}, /*52*/
    {70, 2, 7, 11, 10}, /*53*/
    {70, 2, 7, 11, 10}, /*54*/
    {70, 2, 7, 11, 10}, /*55*/
    {70, 2, 5, 12, 11}, /*56*/
    {70, 2, 5, 12, 11}, /*57*/
    {70, 2, 5, 12, 11}, /*58*/
    {70, 2, 5, 12, 11}, /*59*/
    {70, 2, 5, 12, 11}, /*60*/
    {70, 2, 3, 13, 12}, /*61*/
    {70, 2, 3, 13, 12}, /*62*/
    {70, 2, 3, 13, 12}, /*63*/
    {70, 2, 3, 13, 12}, /*64*/
    {70, 2, 3, 13, 12}, /*65*/
    {70, 2, 3, 12, 13}, /*66*/
    {70, 2, 3, 12, 13}, /*67*/
    {70, 2, 3, 12, 13}, /*68*/
    {70, 2, 3, 12, 13}, /*69*/
    {70, 2, 3, 12, 13}, /*70*/
    {70, 2, 3, 11, 14}, /*71*/
    {70, 2, 3, 11, 14}, /*72*/
    {70, 2, 3, 11, 14}, /*73*/
    {70, 2, 3, 11, 14}, /*74*/
    {70, 2, 3, 11, 14}, /*75*/
    {70, 2, 3, 10, 15}, /*76*/
    {70, 2, 3, 10, 15}, /*77*/
    {70, 2, 3, 10, 15}, /*78*/
    {70, 2, 3, 10, 15}, /*79*/
    {70, 2, 3, 10, 15}, /*80*/
    {70, 2, 3, 9, 16},  /*81*/
    {70, 2, 3, 9, 16},  /*82*/
    {70, 2, 3, 9, 16},  /*83*/
    {70, 2, 3, 9, 16},  /*84*/
    {70, 2, 3, 9, 16},  /*85*/
    {70, 2, 3, 8, 17},  /*86*/
    {70, 2, 3, 8, 17},  /*87*/
    {70, 2, 3, 8, 17},  /*88*/
    {70, 2, 3, 8, 17},  /*89*/
    {70, 2, 3, 8, 17},  /*90*/
    {70, 2, 3, 7, 18},  /*91*/
    {70, 2, 3, 7, 18},  /*92*/
    {70, 2, 3, 7, 18},  /*93*/
    {70, 2, 3, 7, 18},  /*94*/
    {70, 2, 3, 7, 18},  /*95*/
    {70, 2, 3, 6, 19},  /*96*/
    {70, 2, 3, 6, 19},  /*97*/
    {70, 2, 3, 6, 19},  /*98*/
    {70, 2, 3, 6, 19},  /*99*/
    {70, 2, 3, 6, 19},  /*100*/
    {70, 2, 3, 6, 19},  /*101*/
    {70, 2, 3, 6, 19},  /*101*/
    {70, 2, 3, 6, 19},  /*102*/
    {70, 2, 3, 6, 19},  /*103*/
    {70, 2, 3, 6, 19},  /*104*/
    {70, 2, 3, 6, 19},  /*105*/
    {70, 2, 3, 6, 19},  /*106*/
    {70, 2, 3, 6, 19},  /*107*/
    {70, 2, 3, 6, 19},  /*108*/
    {70, 2, 3, 6, 19},  /*109*/
    {70, 2, 3, 6, 19},  /*110*/
    {70, 2, 3, 6, 19},  /*111*/
    {70, 2, 3, 6, 19},  /*112*/
    {70, 2, 3, 6, 19},  /*113*/
    {70, 2, 3, 6, 19},  /*114*/
    {70, 2, 3, 6, 19},  /*115*/
    {70, 2, 3, 6, 19},  /*116*/
    {70, 2, 3, 6, 19},  /*117*/
    {70, 2, 3, 6, 19},  /*118*/
    {70, 2, 3, 6, 19},  /*119*/
    {70, 2, 3, 6, 19},  /*120*/
    {70, 2, 3, 6, 19},  /*121*/
    {70, 2, 3, 6, 19},  /*122*/
    {70, 2, 3, 6, 19},  /*123*/
    {70, 2, 3, 6, 19},  /*124*/
    {70, 2, 3, 6, 19},  /*125*/
    {70, 2, 3, 6, 19},  /*126*/
    {70, 2, 3, 6, 19},  /*127*/
    {70, 2, 3, 6, 19},  /*128*/
    {70, 2, 3, 6, 19},  /*129*/
    {70, 2, 3, 6, 19},  /*130*/
    {70, 2, 3, 6, 19},  /*131*/
    {70, 2, 3, 6, 19},  /*132*/
    {70, 2, 3, 6, 19},  /*133*/
    {70, 2, 3, 6, 19},  /*134*/
    {70, 2, 3, 6, 19},  /*135*/
    {70, 2, 3, 6, 19},  /*136*/
    {70, 2, 3, 6, 19},  /*137*/
    {70, 2, 3, 6, 19},  /*138*/
    {70, 2, 3, 6, 19},  /*139*/
    {70, 2, 3, 6, 19},  /*140*/
    {70, 2, 3, 6, 19},  /*141*/
    {70, 2, 3, 6, 19},  /*142*/
    {70, 2, 3, 6, 19},  /*143*/
    {70, 2, 3, 6, 19},  /*144*/
    {70, 2, 3, 6, 19},  /*145*/
    {70, 2, 3, 6, 19},  /*146*/
    {70, 2, 3, 6, 19},  /*147*/
    {70, 2, 3, 6, 19},  /*148*/
    {70, 2, 3, 6, 19},  /*149*/
    {70, 2, 3, 6, 19},  /*150*/
    {70, 2, 3, 6, 19},  /*151*/
    {70, 2, 3, 6, 19},  /*152*/
    {70, 2, 3, 6, 19},  /*153*/
    {70, 2, 3, 6, 19},  /*154*/
    {70, 2, 3, 6, 19},  /*155*/
    {70, 2, 3, 6, 19},  /*156*/
    {70, 2, 3, 6, 19},  /*157*/
    {70, 2, 3, 6, 19},  /*158*/
    {70, 2, 3, 6, 19},  /*159*/
    {70, 2, 3, 6, 19},  /*160*/
    {70, 2, 3, 6, 19},  /*161*/
    {70, 2, 3, 6, 19},  /*162*/
    {70, 2, 3, 6, 19},  /*163*/
    {70, 2, 3, 6, 19},  /*164*/
    {70, 2, 3, 6, 19},  /*165*/
    {70, 2, 3, 6, 19},  /*166*/
    {70, 2, 3, 6, 19},  /*167*/
    {70, 2, 3, 6, 19},  /*168*/
    {70, 2, 3, 6, 19},  /*169*/
    {70, 2, 3, 6, 19},  /*170*/
    {70, 2, 3, 6, 19},  /*171*/
    {70, 2, 3, 6, 19},  /*172*/
    {70, 2, 3, 6, 19},  /*173*/
    {70, 2, 3, 6, 19},  /*174*/
    {70, 2, 3, 6, 19},  /*175*/
    {70, 2, 3, 6, 19},  /*176*/
    {70, 2, 3, 6, 19},  /*177*/
    {70, 2, 3, 6, 19},  /*178*/
    {70, 2, 3, 6, 19},  /*179*/
    {70, 2, 3, 6, 19},  /*180*/
    {70, 2, 3, 6, 19},  /*181*/
    {70, 2, 3, 6, 19},  /*182*/
    {70, 2, 3, 6, 19},  /*183*/
    {70, 2, 3, 6, 19},  /*184*/
    {70, 2, 3, 6, 19},  /*185*/
    {70, 2, 3, 6, 19},  /*186*/
    {70, 2, 3, 6, 19},  /*187*/
    {70, 2, 3, 6, 19},  /*188*/
    {70, 2, 3, 6, 19},  /*189*/
    {70, 2, 3, 6, 19},  /*190*/
    {70, 2, 3, 6, 19},  /*191*/
    {70, 2, 3, 6, 19},  /*192*/
    {70, 2, 3, 6, 19},  /*193*/
    {70, 2, 3, 6, 19},  /*194*/
    {70, 2, 3, 6, 19},  /*195*/
    {70, 2, 3, 6, 19},  /*196*/
    {70, 2, 3, 6, 19},  /*197*/
    {70, 2, 3, 6, 19},  /*198*/
    {70, 2, 3, 6, 19},  /*199*/
    {70, 2, 3, 6, 19},  /*200*/
};

/**
 * Calculate a magic value from the specified difficulty.
 *
 * @param difficulty
 *
 * @return
 * Magic value.
 */
static int
magic_from_difficulty (int difficulty)
{
    difficulty--;
    if (difficulty < 0) {
        difficulty = 0;
    } else if (difficulty >= DIFFLEVELS) {
        difficulty = DIFFLEVELS - 1;
    }

    for (int i = 0; i < DIFFLEVELS; i++) {
        int sum = 0;
        for (int j = 0; j < MAXMAGIC+1; j++) {
            sum += difftomagic_list[i][j];
        }
        if (sum != 100) {
            LOG(ERROR, "%d", i);
        }
    }

    int roll = rndm(0, 99);

    int magic;
    for (magic = 0; magic < MAXMAGIC + 1; magic++) {
        roll -= difftomagic_list[difficulty][magic];
        if (roll < 0) {
            break;
        }
    }

    if (magic == MAXMAGIC + 1) {
        log_error("Table for difficulty %d bad", difficulty);
        magic = 0;
    }

    return rndm_chance(20) ? -magic : magic;
}

/**
 * Sets a random magical bonus in the given object based upon the given
 * difficulty, and the given max possible bonus.
 *
 * Item will be cursed if magic is negative.
 * @param difficulty
 * Difficulty we want the item to be.
 * @param op
 * The object.
 * @param max_magic
 * What should be the maximum magic of the item.
 * @param fix_magic
 * Fixed value of magic for the object.
 * @param chance_magic
 * Chance to get a magic bonus.
 * @param flags
 * Combination of @ref GT_xxx flags.
 */
static void set_magic(int difficulty, object *op, int max_magic, int fix_magic, int chance_magic, int flags)
{
    int magic;

    /* If we have a fixed value, force it */
    if (fix_magic) {
        magic = fix_magic;
    } else {
        magic = magic_from_difficulty(difficulty);
        if (magic > max_magic) {
            magic = max_magic;
        }
    }

    if ((flags & GT_ONLY_GOOD) && magic < 0) {
        magic = 0;
    }

    set_abs_magic(op, magic);

    if (magic < 0) {
        SET_FLAG(op, FLAG_CURSED);
    }

    if (magic != 0) {
        SET_FLAG(op, FLAG_IS_MAGICAL);
    }
}

/**
 * Sets magical bonus in an object, and recalculates the effect on the
 * armour variable, and the effect on speed of armour.
 *
 * This function doesn't work properly, should add use of archetypes to
 * make it truly absolute.
 * @param op
 * Object we're modifying.
 * @param magic
 * Magic modifier.
 */
void set_abs_magic(object *op, int magic)
{
    if (!magic) {
        return;
    }

    SET_FLAG(op, FLAG_IS_MAGICAL);

    op->magic = magic;

    if (op->arch) {
        if (magic == 1) {
            op->value += 5300;
        } else if (magic == 2) {
            op->value += 12300;
        } else if (magic == 3) {
            op->value += 24300;
        } else if (magic == 4) {
            op->value += 52300;
        } else {
            op->value += 88300;
        }

        if (op->type == ARMOUR) {
            ARMOUR_SPEED(op) = (ARMOUR_SPEED(&op->arch->clone) * (100 + magic * 10)) / 100;
        }

        if (magic < 0 && rndm_chance(3)) {
            magic = (-magic);
        }

        op->weight = (op->arch->clone.weight * (100 - magic * 10)) / 100;
    } else {
        if (op->type == ARMOUR) {
            ARMOUR_SPEED(op) = (ARMOUR_SPEED(op) * (100 + magic * 10)) / 100;
        }

        /* You can't just check the weight always */
        if (magic < 0 && rndm_chance(3)) {
            magic = (-magic);
        }

        op->weight = (op->weight * (100 - magic * 10)) / 100;
    }
}

/**
 * Assign a random slaying race to an object, for weapons, arrows
 * and such.
 * @param op
 * Object.
 */
static void add_random_race(object *op)
{
    ob_race *race = race_get_random();

    if (race) {
        FREE_AND_COPY_HASH(op->slaying, race->name);
    }
}

/**
 * This is called after an item is generated, in order to set it up
 * right. This produced magical bonuses, puts spells into
 * scrolls/books/wands, makes it unidentified, hides the value, etc.
 * @param op_ptr
 * Object to fix.
 * @param creator
 * For who op was created. Can be NULL.
 * @param difficulty
 * Difficulty level.
 * @param a_chance
 * Artifact chance.
 * @param t_style
 * Treasure style.
 * @param max_magic
 * Maximum magic for the item.
 * @param fix_magic
 * Fixed magic value.
 * @param chance_magic
 * Chance of magic.
 * @param flags
 * One of @ref GT_xxx
 */
int fix_generated_item(object **op_ptr, object *creator, int difficulty, int a_chance, int t_style, int max_magic, int fix_magic, int chance_magic, int flags)
{
    /* Just to make things easy */
    object *op = *op_ptr;
    int retval = 0, was_magic = op->magic;

    /* Safety and to prevent polymorphed objects giving attributes */
    if (!creator || creator->type == op->type) {
        creator = op;
    }

    if (difficulty < 1) {
        difficulty = 1;
    }

    if (!OBJECT_METHODS(op->type)->override_treasure_processing) {
        if (creator->type == 0) {
            max_magic /= 2;
        }

        if ((!op->magic && max_magic) || fix_magic) {
            set_magic(difficulty, op, max_magic, fix_magic, chance_magic, flags);
        }

        if (a_chance != 0) {
            if ((!was_magic && rndm_chance(CHANCE_FOR_ARTIFACT)) || difficulty >= 999 || rndm(1, 100) <= a_chance) {
                retval = artifact_generate(op, difficulty, t_style);
            }
        }
    }

    object *new_obj;
    int res = object_process_treasure(op,
                                      &new_obj,
                                      difficulty,
                                      t_style,
                                      flags);
    if (res == OBJECT_METHOD_ERROR) {
        /* TODO: return NULL */
        return retval;
    }

    if (res == OBJECT_METHOD_OK) {
        op = new_obj;
    } else if (!op->title || op->type == RUNE) {
        switch (op->type) {
        case PAINTING:
            artifact_generate(op, difficulty, T_STYLE_UNSET);
            break;
        }
    } else {
        /* Title is not NULL. */

        switch (op->type) {
        case ARROW:

            if (op->slaying == shstr_cons.none) {
                add_random_race(op);
            }

            break;

        case WEAPON:

            if (op->slaying == shstr_cons.none) {
                add_random_race(op);
            }

            break;
        }
    }

    if (op == NULL) {
        return retval;
    }

    if ((flags & GT_NO_VALUE) && op->type != MONEY) {
        op->value = 0;
    }

    if (flags & GT_STARTEQUIP) {
        if (op->nrof < 2 && op->type != CONTAINER && op->type != MONEY && !QUERY_FLAG(op, FLAG_IS_THROWN)) {
            SET_FLAG(op, FLAG_STARTEQUIP);
        } else if (op->type != MONEY) {
            op->value = 0;
        }
    }

    if (creator->type == TREASURE && QUERY_FLAG(creator, FLAG_NO_PICK)) {
        SET_FLAG(op, FLAG_NO_PICK);
    }

    return retval;
}

/**
 * Frees a treasure, including its yes, no and next items.
 * @param t
 * Treasure to free. Pointer is efree()d too, so becomes
 * invalid.
 */
static void free_treasurestruct(treasure *t)
{
    if (t->next) {
        free_treasurestruct(t->next);
    }

    if (t->next_yes) {
        free_treasurestruct(t->next_yes);
    }

    if (t->next_no) {
        free_treasurestruct(t->next_no);
    }

    FREE_AND_CLEAR_HASH2(t->name);
    FREE_AND_CLEAR_HASH2(t->change_arch.name);
    FREE_AND_CLEAR_HASH2(t->change_arch.slaying);
    FREE_AND_CLEAR_HASH2(t->change_arch.title);
    efree(t);
}

/**
 * Free all treasure related memory.
 */
void free_all_treasures(void)
{
    treasurelist *tl, *next;

    for (tl = first_treasurelist; tl; tl = next) {
        next = tl->next;
        FREE_AND_CLEAR_HASH2(tl->name);

        if (tl->items) {
            free_treasurestruct(tl->items);
        }

        efree(tl);
    }
}

/**
 * Set object's object::material_real.
 * @param op
 * Object to set material_real for.
 * @param change_arch
 * Change arch.
 */
static void set_material_real(object *op, struct _change_arch *change_arch)
{
    if (op->type == MONEY) {
        return;
    }

    if (change_arch->item_race != -1) {
        op->item_race = (uint8_t) change_arch->item_race;
    }

    /* This must be tested - perhaps we want that change_arch->material
     * also overrule the material_real -1 marker? */
    if (op->material_real == -1) {
        /* WARNING: material_real == -1 skips also the quality modifier.
         * this is really for objects which don't fit in the material/quality
         * system (like system objects, forces, effects and stuff). */
        op->material_real = 0;
        return;
    }

    /* We overrule the material settings in any case when this is set */
    if (change_arch->material != -1) {
        op->material_real = change_arch->material;

        /* Skip if material is 0 (aka neutralized material setting) */
        if (change_arch->material_range > 0 && change_arch->material) {
            op->material_real += rndm(0, change_arch->material_range);
        }
    }/* If 0, grab a valid material class. We should assign to all objects
         * a valid material_real value to avoid problems here. */
    else if (!op->material_real && op->material != M_ADAMANT) {
        if (op->material & M_IRON) {
            op->material_real = M_START_IRON;
        } else if (op->material & M_LEATHER) {
            op->material_real = M_START_LEATHER;
        } else if (op->material & M_PAPER) {
            op->material_real = M_START_PAPER;
        } else if (op->material & M_GLASS) {
            op->material_real = M_START_GLASS;
        } else if (op->material & M_WOOD) {
            op->material_real = M_START_WOOD;
        } else if (op->material & M_ORGANIC) {
            op->material_real = M_START_ORGANIC;
        } else if (op->material & M_STONE) {
            op->material_real = M_START_STONE;
        } else if (op->material & M_CLOTH) {
            op->material_real = M_START_CLOTH;
        } else if (op->material & M_ADAMANT) {
            op->material_real = M_START_ADAMANT;
        } else if (op->material & M_LIQUID) {
            op->material_real = M_START_LIQUID;
        } else if (op->material & M_SOFT_METAL) {
            op->material_real = M_START_SOFT_METAL;
        } else if (op->material & M_BONE) {
            op->material_real = M_START_BONE;
        } else if (op->material & M_ICE) {
            op->material_real = M_START_ICE;
        }
    }

    /* Now we do some work: we define a (material) quality and try to
     * find a best matching pre-set material_real for that item. This is
     * a bit more complex but we are with that free to define different
     * materials without having a strong fixed material table. */
    if (change_arch->material_quality != -1) {
        int i, q_tmp = -1;
        int m_range = change_arch->material_quality;

        if (change_arch->material_range > 0) {
            m_range += rndm(0, change_arch->material_range);
        }

        if (op->material_real) {
            int m_tmp = op->material_real / NROFMATERIALS_REAL;

            /* The first entry of the material_real of material table */
            m_tmp = m_tmp * 64 + 1;

            /* We should add paper & cloth here too later */
            if (m_tmp == M_START_IRON || m_tmp == M_START_WOOD || m_tmp == M_START_LEATHER) {
                for (i = 0; i < NROFMATERIALS_REAL; i++) {
                    /* We have a full hit */
                    if (materials_real[m_tmp + i].quality == m_range) {
                        op->material_real = m_tmp + i;
                        goto set_material_real;
                    }

                    /* Find nearest quality we want */
                    if (materials_real[m_tmp + i].quality >= change_arch->material_quality && materials_real[m_tmp + i].quality <= m_range && materials_real[m_tmp + i].quality > q_tmp) {
                        q_tmp = m_tmp + i;
                    }
                }

                /* If we have no match, we simply use the (always valid)
                 * first material_real entry and forcing the
                 * material_quality to quality! */
                if (q_tmp >= 0) {
                    op->material_real = m_tmp;
                    op->item_quality = change_arch->material_quality;
                    op->item_condition = op->item_quality;
                    return;
                }

                /* That's now our best match! */
                op->material_real = q_tmp;
            } else {
                /* Excluded material table! */

                op->item_quality = m_range;
                op->item_condition = op->item_quality;
                return;
            }
        } else {
            /* We have material_real 0 but we modify at least the quality! */

            op->item_quality = m_range;
            op->item_condition = op->item_quality;
            return;
        }
    }

set_material_real:

    /* Adjust quality - use material default value or quality adjustment */
    if (change_arch->quality != -1) {
        op->item_quality = change_arch->quality;
    } else {
        op->item_quality = materials_real[op->material_real].quality;
    }

    if (change_arch->quality_range > 0) {
        op->item_quality += rndm(0, change_arch->quality_range);

        if (op->item_quality > 100) {
            op->item_quality = 100;
        }
    }

    op->item_condition = op->item_quality;
}

/**
 * Gets the environment level for treasure generation for the given
 * object.
 * @param op
 * Object to get environment level of.
 * @return
 * The environment level, always at least 1.
 */
int get_environment_level(object *op)
{
    object *env;

    if (!op) {
        return 1;
    }

    /* Return object level or map level... */
    if (op->level) {
        return op->level;
    }

    if (op->map) {
        return op->map->difficulty ? op->map->difficulty : 1;
    }

    /* Let's check for env */
    env = op->env;

    while (env) {
        if (env->level) {
            return env->level;
        }

        if (env->map) {
            return env->map->difficulty ? env->map->difficulty : 1;
        }

        env = env->env;
    }

    return 1;
}

#ifdef TREASURE_DEBUG

/**
 * Checks if a treasure if valid. Will also check its yes and no options.
 * @param t
 * Treasure to check.
 * @param tl
 * Needed only so that the treasure name can be printed out.
 */
static void check_treasurelist(treasure *t, treasurelist *tl)
{
    if (t->item == NULL && t->name == NULL) {
        LOG(ERROR, "Treasurelist %s has element with no name or archetype", tl->name);
        exit(1);
    }

    if (t->chance >= 100 && t->next_yes && (t->next || t->next_no)) {
        LOG(BUG, "Treasurelist %s has element that has 100%% generation, next_yes field as well as next or next_no", tl->name);
    }

    /* find_treasurelist will print out its own error message */
    if (t->name && t->name != shstr_cons.NONE) {
        (void) find_treasurelist(t->name);
    }

    if (t->next) {
        check_treasurelist(t->next, tl);
    }

    if (t->next_yes) {
        check_treasurelist(t->next_yes, tl);
    }

    if (t->next_no) {
        check_treasurelist(t->next_no, tl);
    }
}
#endif
