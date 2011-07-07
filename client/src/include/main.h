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
 * Header file for things that are generally used in many places. */

#ifndef MAIN_H
#define MAIN_H

#define HUGE_BUF 4096
#define MAX_BUF 256
/** Maximum frames per second. */
#define FRAMES_PER_SECOND 30
#define COLOR_BUF 7

#define SDL_DEFAULT_REPEAT_INTERVAL 30

/* For hash table (bmap, ...) */
#define MAXSTRING 20

/** Keymap structure */
typedef struct _keymap
{
	/** The command text, submitted to server when key is pressed. */
	char text[256];

	/** Key name. */
	char keyname[256];

	/** Scan code of key. */
	int key;

	/** If true, key will be repeated when pressed. */
	int repeatflag;
}_keymap;

/** The servers list, as given by the metaserver. */
typedef struct server_struct
{
	/** Next server in the list. */
	struct server_struct *next;

	/** IP of the server. */
	char *ip;

	/** Name of the server. */
	char *name;

	/** Server version. */
	char *version;

	/** Server description. */
	char *desc;

	/** Number of players online. */
	int player;

	/** Server port. */
	int port;
} server_struct;

/**
 * Message animation structure. Used when NDI_ANIM is passed to
 * DrawInfoCmd2(). */
typedef struct msg_anim_struct
{
	/** The message to play. */
	char message[MAX_BUF];

	/** Tick when it started. */
	uint32 tick;

	/** Color of the message animation. */
	char color[COLOR_BUF];
} msg_anim_struct;

extern struct msg_anim_struct msg_anim;

#ifndef SYSPATH
#define SYSPATH "./"
#endif

#define FILE_ATRINIK_P0 "data/atrinik.p0"

/* Face requested from server - do it only one time */
#define FACE_REQUESTED		16

typedef struct _face_struct
{
	/* Our face data. if != null, face is loaded */
	struct _Sprite *sprite;

	/* Our face name. if != null, face is requested */
	char *name;

	/* Checksum of face */
	uint32 checksum;

	int flags;
}_face_struct;

#define NUM_STATS 7

/** Option windows max options */
#define OPTWIN_MAX_OPT 26

#define DIALOG_LIST_ENTRY	26
#define OPTWIN_MAX_TABLEN	14

/** Bindkey list max */
#define BINDKEY_LIST_MAX 10

/** Bindkey list structure */
typedef struct _bindkey_list
{
	/** Entry */
	_keymap entry[DIALOG_LIST_ENTRY];

	/** Name */
	char name[OPTWIN_MAX_TABLEN];

	/** Size */
	int size;
}_bindkey_list;

/** Dialog list structure */
typedef struct _dialog_list_set
{
	/** Group number */
	int group_nr;

	/** Entry number */
	int entry_nr;

	/** For spell-list => spell, prayer, ... */
	int class_nr;

	/** Key change */
	int key_change;
}_dialog_list_set;

typedef struct spell_entry_struct
{
	/** Name of the spell. */
	char name[MAX_BUF];

	/** Icon name */
	char icon_name[MAX_BUF];

	/** Description. */
	char desc[HUGE_BUF];

	/** Spell's icon. */
	int icon;

	/** Cost of spell. */
	int cost;

	/**
	 * Contains what kind of relationship the player has with the spell's path:
	 * - a: Attuned
	 * - r: Repelled
	 * - d: Denied */
	char path;

	/** 1 if the player knows this spell, 0 otherwise. */
	uint8 known;

	/** Type of the spell (spell/prayer). */
	uint8 type;
} spell_entry_struct;

/**
 * Maximum number of spell paths. The last one is always 'all' and holds
 * pointers to spells in the other spell paths. */
#define SPELL_PATH_NUM 21

typedef struct skill_entry_struct
{
	/** Name of the skill. */
	char name[MAX_BUF];

	/** Icon name */
	char icon_name[MAX_BUF];

	/** Description. */
	char desc[HUGE_BUF];

	/** Skill's icon. */
	int icon;

	/** 1 if the player knows this skill, 0 otherwise. */
	uint8 known;

	int level;

	sint64 exp;
} skill_entry_struct;

#define SKILL_LIST_TYPES 7

/** Fire mode structure */
typedef struct _fire_mode
{
	/** Item */
	int item;

	/** Ammunition */
	int amun;

	spell_entry_struct *spell;

	/** Skill */
	skill_entry_struct *skill;

	/** Name */
	char name[128];
}_fire_mode;

/** Help files structure. */
typedef struct help_files_struct
{
	/** Help name, like "main", or "apply". */
	char helpname[MAX_BUF];

	/** Help title (shown at the start of the book). */
	char title[MAX_BUF];

	/** The help message. */
	char message[HUGE_BUF * 12];

	/** Is this for DMs only? */
	int dm_only;

	/** Can the command be autocompleted using the tab key? */
	int autocomplete;

	/** Next help entry. */
	struct help_files_struct *next;
} help_files_struct;

extern help_files_struct *help_files;

typedef enum _fire_mode_id
{
	FIRE_MODE_BOW,
	FIRE_MODE_SPELL,
	FIRE_MODE_WAND,
	FIRE_MODE_SKILL,
	FIRE_MODE_THROW,
	FIRE_MODE_INIT
}_fire_mode_id;

/** If no fire item */
#define FIRE_ITEM_NO -1

/** Game statuses. */
typedef enum _game_status
{
	/** Call this add start to autoinit */
	GAME_STATUS_INIT,

	/** To to connect to meta server */
	GAME_STATUS_META,

	/** Start all up without full reset or meta calling */
	GAME_STATUS_START,

	/** We are NOT connected to anything */
	GAME_STATUS_WAITLOOP,

	/** We have a server + port, init and start */
	GAME_STATUS_STARTCONNECT,

	/** If this is set, we start connecting */
	GAME_STATUS_CONNECT,

	/** Now the steps: Connect, we send version */
	GAME_STATUS_VERSION,

	/** We ready to send setup commands */
	GAME_STATUS_SETUP,

	/** We wait for server response */
	GAME_STATUS_WAITSETUP,

	/** After we get response from setup, we request files if needed */
	GAME_STATUS_REQUEST_FILES,

	/** All setup is done, now try to enter game */
	GAME_STATUS_ADDME,

	/** Now we wait for LOGIN request of the server */
	GAME_STATUS_LOGIN,

	/** All this here is tricky */
	GAME_STATUS_NAME,

	/** Server will trigger this when asking for password */
	GAME_STATUS_PSWD,

	/** Client will then show input panel or so */
	GAME_STATUS_VERIFYPSWD,

	/** Show new char creation screen and send /nc command when finished */
	GAME_STATUS_NEW_CHAR,

	/** We simply wait for game start.
	 * Means, this is not a serial stepping here */
	GAME_STATUS_WAITFORPLAY,

	/** We are in quit menu */
	GAME_STATUS_QUIT,

	/** We play now. */
	GAME_STATUS_PLAY
} _game_status;

extern int f_custom_cursor;
extern int x_custom_cursor;
extern int y_custom_cursor;

extern _game_status GameStatus;
extern uint32 LastTick;
extern server_struct *selected_server;
extern int map_udate_flag, map_redraw_flag;

enum
{
	ESC_MENU_KEYS,
	ESC_MENU_SETTINGS,
	ESC_MENU_LOGOUT,
	ESC_MENU_BACK,

	/* Last index */
	ESC_MENU_INDEX
};

/* With this, we overrule bitmap loading params.
 * For example, we need for fonts an attached palette, and not the native vid mode */

/** Surface must stay in palette mode, not in vid mode */
#define SURFACE_FLAG_PALETTE 	1
/** Use this when you want a colkey in a true color picture - color should be 0 */
#define SURFACE_FLAG_COLKEY_16M 2
#define SURFACE_FLAG_DISPLAYFORMAT 4
#define SURFACE_FLAG_DISPLAYFORMATALPHA 8

/** Types of pictures. */
typedef enum _pic_type
{
	PIC_TYPE_DEFAULT, PIC_TYPE_PALETTE, PIC_TYPE_TRANS, PIC_TYPE_ALPHA
} _pic_type;

/** Bitmap name structure */
typedef struct _bitmap_name
{
	/** Name */
	char *name;

	/** Type */
	_pic_type type;
} _bitmap_name;

typedef enum _bitmap_index
{
	BITMAP_INTRO,

	BITMAP_DOLL,

	BITMAP_LOGIN_INP,
	BITMAP_INVSLOT,

	/* Status bars */
	BITMAP_HP,
	BITMAP_SP,
	BITMAP_GRACE,
	BITMAP_FOOD,
	BITMAP_HP_BACK,
	BITMAP_SP_BACK,
	BITMAP_GRACE_BACK,
	BITMAP_FOOD_BACK,

	BITMAP_APPLY,
	BITMAP_UNPAID,
	BITMAP_CURSED,
	BITMAP_DAMNED,
	BITMAP_LOCK,
	BITMAP_MAGIC,
	BITMAP_FIRE_READY,

	BITMAP_RANGE,
	BITMAP_RANGE_MARKER,
	BITMAP_RANGE_SKILL,
	BITMAP_RANGE_SKILL_NO,
	BITMAP_RANGE_THROW,
	BITMAP_RANGE_THROW_NO,
	BITMAP_RANGE_TOOL,
	BITMAP_RANGE_TOOL_NO,
	BITMAP_RANGE_WIZARD,
	BITMAP_RANGE_WIZARD_NO,

	BITMAP_CMARK_START,
	BITMAP_CMARK_END,
	BITMAP_CMARK_MIDDLE,

	BITMAP_TWIN_SCROLL,
	BITMAP_INV_SCROLL,
	BITMAP_BELOW_SCROLL,

	BITMAP_NUMBER,
	BITMAP_INVSLOT_U,

	BITMAP_DEATH,
	BITMAP_SLEEP,
	BITMAP_CONFUSE,
	BITMAP_PARALYZE,
	BITMAP_SCARED,
	BITMAP_BLIND,

	BITMAP_QUICKSLOTS,
	BITMAP_QUICKSLOTSV,
	BITMAP_INVENTORY,
	BITMAP_INV_BG,

	BITMAP_EXP_BORDER,
	BITMAP_EXP_SLIDER,
	BITMAP_EXP_BUBBLE1,
	BITMAP_EXP_BUBBLE2,

	BITMAP_STATS_BG,
	BITMAP_BELOW,

	BITMAP_TARGET_ATTACK,
	BITMAP_TARGET_TALK,
	BITMAP_TARGET_NORMAL,

	BITMAP_WARN_HP,
	BITMAP_WARN_FOOD,

	BITMAP_RANGE_BUTTONS_OFF,
	BITMAP_RANGE_BUTTONS_LEFT,
	BITMAP_RANGE_BUTTONS_RIGHT,

	BITMAP_TARGET_HP,
	BITMAP_TARGET_HP_B,

	BITMAP_TEXTWIN_MASK,
	BITMAP_SLIDER_UP,
	BITMAP_SLIDER_DOWN,
	BITMAP_SLIDER,

	BITMAP_EXP_SKILL_BORDER,
	BITMAP_EXP_SKILL_LINE,
	BITMAP_EXP_SKILL_BUBBLE,

	BITMAP_TRAPPED,
	BITMAP_PRAY,
	BITMAP_BOOK,
	BITMAP_REGION_MAP,
	BITMAP_SLIDER_LONG,
	BITMAP_INVSLOT_MARKED,
	BITMAP_MSCURSOR_MOVE,
	BITMAP_RESIST_BG,
	BITMAP_MAIN_LVL_BG,
	BITMAP_SKILL_EXP_BG,
	BITMAP_REGEN_BG,
	BITMAP_SKILL_LVL_BG,
	BITMAP_MENU_BUTTONS,
	BITMAP_PLAYER_INFO,
	BITMAP_TARGET_BG,
	BITMAP_TEXTINPUT,

	BITMAP_SQUARE_HIGHLIGHT,
	BITMAP_SERVERS_BG,
	BITMAP_SERVERS_BG_OVER,
	BITMAP_NEWS_BG,
	BITMAP_EYES,
	BITMAP_POPUP,
	BITMAP_ARROW_UP,
	BITMAP_ARROW_UP2,
	BITMAP_ARROW_DOWN,
	BITMAP_ARROW_DOWN2,
	BITMAP_BUTTON_ROUND,
	BITMAP_BUTTON_ROUND_DOWN,
	BITMAP_BUTTON_RECT,
	BITMAP_BUTTON_RECT_HOVER,
	BITMAP_BUTTON_RECT_DOWN,
	BITMAP_MAP_MARKER,
	BITMAP_LOADING_OFF,
	BITMAP_LOADING_ON,
	BITMAP_BUTTON,
	BITMAP_BUTTON_DOWN,
	BITMAP_CHECKBOX,
	BITMAP_CHECKBOX_ON,
	BITMAP_CONTENT,
	BITMAP_ICON_MUSIC,
	BITMAP_ICON_MAGIC,
	BITMAP_ICON_SKILL,
	BITMAP_ICON_PARTY,
	BITMAP_ICON_MAP,
	BITMAP_ICON_COGS,
	BITMAP_ICON_QUEST,
	BITMAP_FPS,

	BITMAP_INIT
}_bitmap_index;

/* For custom cursors */
enum
{
	MSCURSOR_MOVE = 1
};

extern char text_input_string[MAX_INPUT_STRING];
extern int text_input_count;
extern int text_input_string_flag;
extern int text_input_string_end_flag;
extern int text_input_string_esc_flag;
uint32 text_input_opened;

/* Range table */
extern struct _fire_mode fire_mode_tab[FIRE_MODE_INIT];
extern int RangeFireMode;

extern struct _Sprite *Bitmaps[];

/* Face data */
extern _face_struct FaceList[MAX_FACE_TILES];

extern SDL_Surface *ScreenSurface;

/* Server's attributes */
extern struct sockaddr_in insock;


#endif
