/***************************************************************************

  M.A.M.E.32  -  Multiple Arcade Machine Emulator for Win32
  Win32 Portions Copyright (C) 1997-2003 Michael Soderstrom and Chris Kirmse

  This file is part of MAME32, and may only be used, modified and
  distributed under the terms of the MAME license, in "readme.txt".
  By continuing to use, modify or distribute this file you indicate
  that you have read the license and understand and accept it fully.

 ***************************************************************************/
 
 /***************************************************************************

  options.c

  Stores global options and per-game options;

***************************************************************************/

#define WIN32_LEAN_AND_MEAN
#define UNICODE
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <assert.h>
#include <stdio.h>
#include <sys/stat.h>
#include <malloc.h>
#include <math.h>
#include <direct.h>
#include <ctype.h>

#include "MAME32.h"	// include this first
#include "driver.h"
#include "../options.h"
#include "bitmask.h"
#include "picker.h"
#include "dijoystick.h"
#include "treeview.h"
#include "m32util.h"
#include "splitters.h"
#include "DirectDraw.h"
#include "options.h"
#include "translate.h"
#include "directories.h"

#ifdef _MSC_VER
#define snprintf _snprintf
#endif


/***************************************************************************
    Internal structures
 ***************************************************************************/

typedef struct
{
	char *name;
	DWORD flags;
} f_flag_entry;

typedef struct
{
	f_flag_entry *entry;
	int           num;
} f_flag;

typedef struct
{
	char     *save_version;
	BOOL     reset_gui;
	INT      folder_id;
	BOOL     view;
	BOOL     show_folderlist;
	LPBITS   show_folder_flags;
	f_flag   folder_flag;
	BOOL     show_toolbar;
	BOOL     show_statusbar;
	BOOL     show_screenshot;
	BOOL     show_screenshottab;
	int      show_tab_flags;
#ifdef STORY_DATAFILE
	int      datafile_tab;
#else /* STORY_DATAFILE */
	int      history_tab;
#endif /* STORY_DATAFILE */
	char     *current_tab;
	BOOL     game_check;        /* Startup GameCheck */
	BOOL     joygui;
	BOOL     keygui;
	BOOL     broadcast;
	BOOL     random_bg;
	int      cycle_screenshot;
	BOOL     stretch_screenshot_larger;
	int      screenshot_bordersize;
	COLORREF screenshot_bordercolor;
	BOOL     inherit_filter;
	BOOL     offset_clones;
	BOOL	 game_caption;

	char     *default_game;
	int      column_widths[COLUMN_MAX];
	int      column_order[COLUMN_MAX];
	int      column_shown[COLUMN_MAX];
	int      sort_column;
	BOOL     sort_reverse;
	int      window_x;
	int      window_y;
	int      window_width;
	int      window_height;
	UINT     window_state;
	int      splitters[4];		/* NPW 5-Feb-2003 - I don't like hard coding this, but I don't have a choice */
	COLORREF custom_color[16]; /* This is how many custom colors can be shown on the standard ColorPicker */
	BOOL     use_broken_icon;
	LOGFONTA list_logfont;
	COLORREF font_color;
	COLORREF clone_color;
	COLORREF broken_color;

	// Keyboard control of ui
	KeySeq   ui_key_up;
	KeySeq   ui_key_down;
	KeySeq   ui_key_left;
	KeySeq   ui_key_right;
	KeySeq   ui_key_start;
	KeySeq   ui_key_pgup;
	KeySeq   ui_key_pgdwn;
	KeySeq   ui_key_home;
	KeySeq   ui_key_end;
	KeySeq   ui_key_ss_change;
	KeySeq   ui_key_history_up;
	KeySeq   ui_key_history_down;

	KeySeq   ui_key_context_filters;	/* CTRL F */
	KeySeq   ui_key_select_random;		/* CTRL R */
	KeySeq   ui_key_game_audit;		/* ALT A */
	KeySeq   ui_key_game_properties;	/* ALT VK_RETURN */
	KeySeq   ui_key_help_contents;		/* VK_F1 */
	KeySeq   ui_key_update_gamelist;	/* VK_F5 */
	KeySeq   ui_key_view_folders;		/* ALT D */
	KeySeq   ui_key_view_fullscreen;	/* VK_F11 */
	KeySeq   ui_key_view_pagetab;		/* ALT B */
	KeySeq   ui_key_view_picture_area;	/* ALT P */
	KeySeq   ui_key_view_status;		/* ALT S */
	KeySeq   ui_key_view_toolbars;		/* ALT T */

	KeySeq   ui_key_view_tab_cabinet;	/* ALT 3 */
	KeySeq   ui_key_view_tab_cpanel;	/* ALT 6 */
	KeySeq   ui_key_view_tab_flyer;		/* ALT 2 */
	KeySeq   ui_key_view_tab_history;	/* ALT 7 */
#ifdef STORY_DATAFILE
	KeySeq   ui_key_view_tab_story;		/* ALT 8 */
#endif /* STORY_DATAFILE */
	KeySeq   ui_key_view_tab_marquee;	/* ALT 4 */
	KeySeq   ui_key_view_tab_screenshot;	/* ALT 1 */
	KeySeq   ui_key_view_tab_title;		/* ALT 5 */
	KeySeq   ui_key_quit;			/* ALT Q */

	// Joystick control of ui
	// array of 4 is joystick index, stick or button, etc.
	int      ui_joy_up[4];
	int      ui_joy_down[4];
	int      ui_joy_left[4];
	int      ui_joy_right[4];
	int      ui_joy_start[4];
	int      ui_joy_pgup[4];
	int      ui_joy_pgdwn[4];
	int      ui_joy_home[4];
	int      ui_joy_end[4];
	int      ui_joy_ss_change[4];
	int      ui_joy_history_up[4];
	int      ui_joy_history_down[4];
	int      ui_joy_exec[4];

	char*    exec_command;  // Command line to execute on ui_joy_exec   
	int      exec_wait;     // How long to wait before executing
	BOOL     hide_mouse;    // Should mouse cursor be hidden on startup?
	BOOL     full_screen;   // Should we fake fullscreen?

#ifdef USE_SHOW_SPLASH_SCREEN
	BOOL     display_splash_screen;
#endif /* USE_SHOW_SPLASH_SCREEN */
//
// PATH AND DIRECTORY OPTIONS
//
	char*    flyer_directory;
	char*    cabinet_directory;
	char*    marquee_directory;
	char*	 title_directory;
	char*	 cpanel_directory;
	char*    icon_directory;
	char*    bkground_directory;
	char*    folder_directory;
#ifdef USE_VIEW_PCBINFO
	char*    pcbinfo_directory;
#endif /* USE_VIEW_PCBINFO */
//
// PATH AND DIRECTORY OPTIONS
//
	/* directory and datafile */
	char*	rompath;
	char*	samplepath;
	char*	inipath;
	char*	cfg_directory;
	char*	nvram_directory;
	char*	memcard_directory;
	char*	input_directory;
	char*	hiscore_directory;
	char*	state_directory;
	char*	artwork_directory;
	char*	snapshot_directory;
	char*	diff_directory;
	char*	ctrlr_directory;
	char*	comment_directory;
#ifdef USE_IPS
	char*	ips_directory;
#endif /* USE_IPS */
	char*	lang_directory;
	char*	cheat_file;
	char*	history_file;
#ifdef STORY_DATAFILE
	char*	story_file;
#endif /* STORY_DATAFILE */
	char*	mameinfo_file;
	char*	hiscore_file;
//
// CORE PALETTE OPTIONS
//
#ifdef UI_COLOR_DISPLAY
	/* ui palette */
	char*    font_blank;
	char*    font_normal;
	char*    font_special;
	char*    system_background;
	char*    system_framemedium;
	char*    system_framelight;
	char*    system_framedark;
	char*    osdbar_framemedium;
	char*    osdbar_framelight;
	char*    osdbar_framedark;
	char*    osdbar_defaultbar;
	char*    button_red;
	char*    button_yellow;
	char*    button_green;
	char*    button_blue;
	char*    button_purple;
	char*    button_pink;
	char*    button_aqua;
	char*    button_silver;
	char*    button_navy;
	char*    button_lime;
	char*    cursor;
#endif /* UI_COLOR_DISPLAY */
//
// CORE LANGUAGE OPTIONS
//
	int      langcode;
	BOOL     use_lang_list;
} settings_type; /* global settings for the UI only */

struct _backup
{
	settings_type settings;
	options_type  global;
};

// per-game data we store, not to pass to mame, but for our own use.
typedef struct
{
	int play_count;
	int play_time;
	int rom_audit_results;
	int samples_audit_results;

	BOOL options_loaded; // whether or not we've loaded the game options yet
	BOOL use_default; // whether or not we should just use default options
	int alt_index; // index for alt_option if driver is unified

} driver_variables_type;

typedef struct
{
	const char *name;
	options_type *option;
	driver_variables_type *variable;
	BOOL has_bios;
	BOOL need_vector_config;
	int driver_index; // index for driver if driver is unified
} alt_options_type;

struct _default_bios
{
	const game_driver *drv;
	alt_options_type *alt_option;
};

struct _joycodes
{
	const char *name;
	int value;
};


/***************************************************************************
    Internal function prototypes
 ***************************************************************************/

static int   regist_alt_option(const char *name);
static int   bsearch_alt_option(const char *name);
static void  build_default_bios(void);
static void  build_alt_options(void);
static void  unify_alt_options(void);

static void  options_create_entry_core(void);
static void  options_create_entry_driver(void);
static void  options_create_entry_winui(void);

static void  options_free_entry_core(void);
static void  options_free_entry_driver(void);
static void  options_free_entry_winui(void);

static void  options_free_string_core(settings_type *s);
static void  options_free_string_driver(options_type *p);
static void  options_free_string_winui(settings_type *p);

static void  options_get_core(settings_type *p);
static void  options_get_driver(options_type *p);
static void  options_get_winui(settings_type *p);

static void  options_duplicate_settings(const settings_type *source, settings_type *dest);
static void  options_duplicate_driver(const options_type *source, options_type *dest);

static BOOL  options_compare_driver(const options_type *p1, const options_type *p2);
static void  options_set_mark_driver(const options_type *p, const options_type *parent);

static int   options_load_default_config(void);
static int   options_load_driver_config(int driver_index);
static int   options_load_alt_config(alt_options_type *alt_option);
static int   options_load_winui_config(void);

static int   options_save_default_config(void);
static int   options_save_driver_config(int driver_index);
static int   options_save_alt_config(alt_options_type *alt_option);
static int   options_save_winui_config(void);

static void  validate_driver_option(options_type *opt);

static void  CopySettings(const settings_type *source, settings_type *dest);
static void  FreeSettings(settings_type *p);

static void  SaveGlobalOptions(void);
static void  SaveAltOptions(alt_options_type *alt_option);
static void  LoadOptions(void);
static void  LoadGameOptions(int driver_index);
static void  LoadAltOptions(alt_options_type *alt_option);

static BOOL  IsOptionEqual(options_type *o1, options_type *o2);

static void  set_folder_flag(f_flag *flag, const char *folderName, DWORD dwFlags);
static void  free_folder_flag(f_flag *flag);


/***************************************************************************
    Internal defines
 ***************************************************************************/

#define FRAMESKIP_LEVELS	12
#define MAX_WINDOWS		4

#define ALLOC_FOLDERFLAG	8
#define ALLOC_FOLDERS		100

#define CSV_ARRAY_MAX		256

#define WINUI_INI MAME32NAME "ui.ini"
#define MAME_INI MAMENAME ".ini"


/***************************************************************************
    Internal variables
 ***************************************************************************/

static settings_type settings;

static struct _backup backup;

static options_type global; // Global 'default' options
static options_type *driver_options;  // Array of Game specific options
static driver_variables_type *driver_variables;  // Array of driver specific extra data

static int  num_alt_options = 0;
static int alt_options_len = 700;
alt_options_type *alt_options;  // Array of Folder specific options

// default bios setting
static struct _default_bios default_bios[MAX_SYSTEM_BIOS];

/* Global UI options */
static int  num_drivers = 0;

// Screen shot Page tab control text
// these must match the order of the options flags in options.h
// (TAB_...)
const char* image_tabs_long_name[MAX_TAB_TYPES] =
{
	"Snapshot",
	"Flyer",
	"Cabinet",
	"Marquee",
	"Title",
	"Control Panel",
#ifdef STORY_DATAFILE
	"History",
	"Story"
#else /* STORY_DATAFILE */
	"History"
#endif /* STORY_DATAFILE */

};

const char* image_tabs_short_name[MAX_TAB_TYPES] =
{
	"snapshot",
	"flyer",
	"cabinet",
	"marquee",
	"title",
	"cpanel",
#ifdef STORY_DATAFILE
	"history",
	"story"
#else /* STORY_DATAFILE */
	"history"
#endif /* STORY_DATAFILE */
};

static const char *view_modes[VIEW_MAX] = 
{
	"Large Icons",
	"Small Icons",
	"List",
	"Details",
	"Grouped"
};


#define DEFINE_JOYCODE_START(name)	static struct _joycodes name[] = {
#define DEFINE_JOYCODE(name)	{ #name, name },
#define DEFINE_JOYCODE_END	{ NULL, 0 } };

DEFINE_JOYCODE_START(joycode_axis)
	DEFINE_JOYCODE(JOYCODE_STICK_BTN)
	DEFINE_JOYCODE(JOYCODE_STICK_AXIS)
	DEFINE_JOYCODE(JOYCODE_STICK_POV)
DEFINE_JOYCODE_END

DEFINE_JOYCODE_START(joycode_dir)
	DEFINE_JOYCODE(JOYCODE_DIR_BTN)
	DEFINE_JOYCODE(JOYCODE_DIR_NEG)
	DEFINE_JOYCODE(JOYCODE_DIR_POS)
DEFINE_JOYCODE_END


#ifdef UI_COLOR_DISPLAY
struct ui_palette_assign
{
	int code;
	char **data;
};

static struct ui_palette_assign ui_palette_tbl[] =
{
	{ FONT_COLOR_BLANK,  &settings.font_blank },
	{ FONT_COLOR_NORMAL,  &settings.font_normal },
	{ FONT_COLOR_SPECIAL,  &settings.font_special },
	{ SYSTEM_COLOR_BACKGROUND,  &settings.system_background },
	{ SYSTEM_COLOR_FRAMEMEDIUM,  &settings.system_framemedium },
	{ SYSTEM_COLOR_FRAMELIGHT,  &settings.system_framelight },
	{ SYSTEM_COLOR_FRAMEDARK,  &settings.system_framedark },
	{ OSDBAR_COLOR_FRAMEMEDIUM,  &settings.osdbar_framemedium },
	{ OSDBAR_COLOR_FRAMELIGHT,  &settings.osdbar_framelight },
	{ OSDBAR_COLOR_FRAMEDARK,  &settings.osdbar_framedark },
	{ OSDBAR_COLOR_DEFAULTBAR,  &settings.osdbar_defaultbar },
	{ BUTTON_COLOR_RED,  &settings.button_red },
	{ BUTTON_COLOR_YELLOW,  &settings.button_yellow },
	{ BUTTON_COLOR_GREEN,  &settings.button_green },
	{ BUTTON_COLOR_BLUE,  &settings.button_blue },
	{ BUTTON_COLOR_PURPLE,  &settings.button_purple },
	{ BUTTON_COLOR_PINK,  &settings.button_pink },
	{ BUTTON_COLOR_AQUA,  &settings.button_aqua },
	{ BUTTON_COLOR_SILVER,  &settings.button_silver },
	{ BUTTON_COLOR_NAVY,  &settings.button_navy },
	{ BUTTON_COLOR_LIME,  &settings.button_lime },
	{ CURSOR_COLOR,  &settings.cursor },
	{ MAX_COLORTABLE, NULL }
};
#endif /* UI_COLOR_DISPLAY */


static char reload_config_msg[] =
	MAME32NAME
	" has changed *.ini file directory.\n\n"
	"Would you like to migrate old configurations to the new directory?";


/***************************************************************************
    External functions  
 ***************************************************************************/

void OptionsInit()
{
	driver_variables_type default_variables;
	int i;

	memset(&settings, 0, sizeof (settings));
	memset(&global, 0, sizeof (global));
	memset(&backup, 0, sizeof (backup));

	num_drivers = GetNumGames();
	code_init();

	options_free_entries();

	options_create_entry_core();
	options_get_core(&settings);
	options_get_driver(&global);

	options_create_entry_driver();

	/* Setup default font */
	GetTranslatedFont(&settings.list_logfont);

	default_variables.play_count  = 0;
	default_variables.play_time = 0;
	default_variables.rom_audit_results = UNKNOWN;
	default_variables.samples_audit_results = UNKNOWN;
	default_variables.options_loaded = FALSE;
	default_variables.use_default = TRUE;
	default_variables.alt_index = -1;

	/* This allocation should be checked */
	driver_options = (options_type *)malloc(num_drivers * sizeof(options_type));
	driver_variables = (driver_variables_type *)malloc(num_drivers * sizeof(driver_variables_type));

	memset(driver_options, 0, num_drivers * sizeof(options_type));
	for (i = 0; i < num_drivers; i++)
		driver_variables[i] = default_variables;

	build_alt_options();
	build_default_bios();

	options_create_entry_winui();
	options_get_winui(&settings);

	// Create Backup
	CopySettings(&settings, &backup.settings);
	CopyGameOptions(&global, &backup.global);

	LoadOptions();

	unify_alt_options();

	// have our mame core (file code) know about our rom path
	// this leaks a little, but the win32 file core writes to this string
	SetCorePathList(FILETYPE_ROM, settings.rompath);
	SetCorePathList(FILETYPE_IMAGE, settings.rompath);
	SetCorePathList(FILETYPE_SAMPLE, settings.samplepath);
#ifdef MESS
	SetCorePathList(FILETYPE_HASH, settings.mess.hashdir);
#endif
}

void OptionsExit(void)
{
	int i;

	for (i = 0; i < num_drivers; i++)
		FreeGameOptions(&driver_options[i]);

	for (i = 0; i < num_alt_options; i++)
		FreeGameOptions(alt_options[i].option);

	free(driver_options);
	free(driver_variables);
	free(alt_options);

	FreeGameOptions(&global);
	FreeGameOptions(&backup.global);

	FreeSettings(&settings);
	FreeSettings(&backup.settings);

	options_free_entry_core();
	options_free_entry_driver();
	options_free_entry_winui();
}

static void CopySettings(const settings_type *source, settings_type *dest)
{
	options_duplicate_settings(source, dest);
}

static void FreeSettings(settings_type *p)
{
	options_free_string_core(p);
	options_free_string_winui(p);
}

void CopyGameOptions(const options_type *source, options_type *dest)
{
	options_duplicate_driver(source, dest);
}

void FreeGameOptions(options_type *o)
{
	options_free_string_driver(o);
}

static BOOL IsOptionEqual(options_type *o1, options_type *o2)
{
	options_type opt1, opt2;

	validate_driver_option(o1);
	validate_driver_option(o2);

	opt1 = *o1;
	opt2 = *o2;

	if (options_compare_driver(&opt1, &opt2))
		return FALSE;

	return TRUE;
}

BOOL GetGameUsesDefaults(int driver_index)
{
	assert (0 <= driver_index && driver_index < num_drivers);

	return driver_variables[driver_index].use_default;
}

BOOL GetFolderUsesDefaults(const char *name)
{
	int alt_index = bsearch_alt_option(name);

	assert (0 <= alt_index && alt_index < num_alt_options);

	return alt_options[alt_index].variable->use_default;
}

void SetGameUsesDefaults(int driver_index, BOOL use_defaults)
{
	assert (0 <= driver_index && driver_index < num_drivers);

	driver_variables[driver_index].use_default = use_defaults;
}

void SetFolderUsesDefaults(const char *name, BOOL use_defaults)
{
	int alt_index = bsearch_alt_option(name);

	assert (0 <= alt_index && alt_index < num_alt_options);

	alt_options[alt_index].variable->use_default = use_defaults;
}

const char *GetUnifiedFolder(int driver_index)
{
	assert (0 <= driver_index && driver_index < num_drivers);

	if (driver_variables[driver_index].alt_index == -1)
		return NULL;

	return alt_options[driver_variables[driver_index].alt_index].name;
}

int GetUnifiedDriver(const char *name)
{
	int alt_index = bsearch_alt_option(name);

	assert (0 <= alt_index && alt_index < num_alt_options);

	return alt_options[alt_index].driver_index;
}

static options_type * GetAltOptions(alt_options_type *alt_option)
{
	if (alt_option->variable->use_default)
	{
		options_type *opt = GetDefaultOptions();
		char *bios = NULL;

#ifdef USE_IPS
		// HACK: DO NOT INHERIT IPS CONFIGURATION
		char *ips = alt_option->option->ips;

		alt_option->option->ips = NULL;
#endif /* USE_IPS */

		// if bios has been loaded, save it
		if (alt_option->option->bios)
			bios = strdup(alt_option->option->bios);

		// try vector.ini
		if (alt_option->need_vector_config)
			opt = GetVectorOptions();

		// free strings what will be never used now
		FreeGameOptions(alt_option->option);

		CopyGameOptions(opt,alt_option->option);

		// DO NOT OVERRIDE bios by default setting
		if (bios)
		{
			FreeIfAllocated(&alt_option->option->bios);
			alt_option->option->bios = bios;
		}

#ifdef USE_IPS
		alt_option->option->ips = ips;
#endif /* USE_IPS */
	}

	if (alt_option->variable->options_loaded == FALSE)
		LoadAltOptions(alt_option);

	return alt_option->option;
}

BOOL FolderHasVector(const char *name)
{
	int alt_index = bsearch_alt_option(name);

	assert (0 <= alt_index && alt_index < num_alt_options);

	return alt_options[alt_index].need_vector_config;
}

options_type * GetFolderOptions(const char *name)
{
	int alt_index = bsearch_alt_option(name);

	assert (0 <= alt_index && alt_index < num_alt_options);

	return GetAltOptions(&alt_options[alt_index]);
}

options_type * GetDefaultOptions(void)
{
	return &global;
}

options_type* GetVectorOptions(void)
{
	return GetFolderOptions("Vector");
}

options_type* GetSourceOptions(int driver_index)
{
	assert (0 <= driver_index && driver_index < num_drivers);

	return GetFolderOptions(GetDriverFilename(driver_index));
}

options_type* GetParentOptions(int driver_index)
{
	assert (0 <= driver_index && driver_index < num_drivers);

	if (DriverIsClone(driver_index))
		return GetGameOptions(DriverParentIndex(driver_index));

	return GetSourceOptions(driver_index);
}

options_type * GetGameOptions(int driver_index)
{
	assert (0 <= driver_index && driver_index < num_drivers);

	if (driver_variables[driver_index].use_default)
	{
		options_type *opt = GetParentOptions(driver_index);
#ifdef USE_IPS
		// HACK: DO NOT INHERIT IPS CONFIGURATION
		char *ips = driver_options[driver_index].ips;

		driver_options[driver_index].ips = NULL;
#endif /* USE_IPS */

		// DO NOT OVERRIDE if driver name is same as parent
		if (opt != &driver_options[driver_index])
		{
			// free strings what will be never used now
			FreeGameOptions(&driver_options[driver_index]);

			CopyGameOptions(opt,&driver_options[driver_index]);
		}

#ifdef USE_IPS
		driver_options[driver_index].ips = ips;
#endif /* USE_IPS */
	}

	if (driver_variables[driver_index].options_loaded == FALSE)
		LoadGameOptions(driver_index);

	return &driver_options[driver_index];
}

const game_driver *GetSystemBiosInfo(int bios_index)
{
	assert (0 <= bios_index && bios_index < MAX_SYSTEM_BIOS);

	return default_bios[bios_index].drv;
}

const char *GetDefaultBios(int bios_index)
{
	assert (0 <= bios_index && bios_index < MAX_SYSTEM_BIOS);

	if (default_bios[bios_index].drv)
	{
		options_type *opt = GetAltOptions(default_bios[bios_index].alt_option);

		return opt->bios;
	}

	return NULL;
}

void SetDefaultBios(int bios_index, const char *value)
{
	assert (0 <= bios_index && bios_index < MAX_SYSTEM_BIOS);

	if (default_bios[bios_index].drv)
	{
		options_type *opt = GetAltOptions(default_bios[bios_index].alt_option);

		FreeIfAllocated(&opt->bios);
		opt->bios = strdup(value);
	}
}

void ResetGUI(void)
{
	settings.reset_gui = TRUE;
}

int GetLangcode(void)
{
	if (settings.langcode < 0)
	{
		int langcode = lang_find_codepage(GetACP());
		return langcode < 0 ? UI_LANG_EN_US : langcode;
	}

	return settings.langcode;
}

void SetLangcode(int langcode)
{
	if (langcode >= UI_LANG_MAX)
		langcode = -1;
	else if (langcode >= 0)
	{
		UINT codepage = ui_lang_info[langcode].codepage;

		if (OnNT())
		{
			if (!IsValidCodePage(codepage))
			{
				fprintf(stderr, "codepage %d is not supported\n", ui_lang_info[langcode].codepage);
				langcode = -1;
			}
		}
		else if ((langcode != UI_LANG_EN_US) && (codepage != GetACP()))
		{
			fprintf(stderr, "codepage %d is not supported\n", ui_lang_info[langcode].codepage);
			langcode = -1;
		}
	}

	settings.langcode = langcode;

	/* apply to options.langcode for datafile.c */
	options.langcode = GetLangcode();
	InitTranslator(options.langcode);
}

BOOL UseLangList(void)
{
    return settings.use_lang_list;
}

void SetUseLangList(BOOL is_use)
{
    settings.use_lang_list = is_use;

    /* apply to options.use_lang_list for datafile.c */
    options.use_lang_list = is_use;
}

const char * GetImageTabLongName(int tab_index)
{
	return image_tabs_long_name[tab_index];
}

const char * GetImageTabShortName(int tab_index)
{
	return image_tabs_short_name[tab_index];
}

#ifdef UI_COLOR_DISPLAY
const char *GetUIPaletteString(int n)
{
	int i;

	for (i = 0; ui_palette_tbl[i].data; i++)
		if (ui_palette_tbl[i].code == n)
			return *ui_palette_tbl[i].data;

	return NULL;
}

void SetUIPaletteString(int n, const char *s)
{
	int i;

	for (i = 0; ui_palette_tbl[i].data; i++)
		if (ui_palette_tbl[i].code == n)
		{
			FreeIfAllocated(ui_palette_tbl[i].data);
			*ui_palette_tbl[i].data = strdup(s);
		}
}
#endif /* UI_COLOR_DISPLAY */

void SetViewMode(int val)
{
	settings.view = val;
}

int GetViewMode(void)
{
	return settings.view;
}

void SetGameCheck(BOOL game_check)
{
	settings.game_check = game_check;
}

BOOL GetGameCheck(void)
{
	return settings.game_check;
}

void SetJoyGUI(BOOL joygui)
{
	settings.joygui = joygui;
}

BOOL GetJoyGUI(void)
{
	return settings.joygui;
}

void SetKeyGUI(BOOL keygui)
{
	settings.keygui = keygui;
}

BOOL GetKeyGUI(void)
{
	return settings.keygui;
}

void SetCycleScreenshot(int cycle_screenshot)
{
	settings.cycle_screenshot = cycle_screenshot;
}

int GetCycleScreenshot(void)
{
	return settings.cycle_screenshot;
}

void SetStretchScreenShotLarger(BOOL stretch)
{
	settings.stretch_screenshot_larger = stretch;
}

BOOL GetStretchScreenShotLarger(void)
{
	return settings.stretch_screenshot_larger;
}

void SetScreenshotBorderSize(int size)
{
	settings.screenshot_bordersize = size;
}

int GetScreenshotBorderSize(void)
{
	return settings.screenshot_bordersize;
}

void SetScreenshotBorderColor(COLORREF uColor)
{
	if (settings.screenshot_bordercolor == GetSysColor(COLOR_3DFACE))
		settings.screenshot_bordercolor = (COLORREF)-1;
	else
		settings.screenshot_bordercolor = uColor;
}

COLORREF GetScreenshotBorderColor(void)
{
	if (settings.screenshot_bordercolor == (COLORREF)-1)
		return (GetSysColor(COLOR_3DFACE));

	return settings.screenshot_bordercolor;
}

void SetFilterInherit(BOOL inherit)
{
	settings.inherit_filter = inherit;
}

BOOL GetFilterInherit(void)
{
	return settings.inherit_filter;
}

void SetOffsetClones(BOOL offset)
{
	settings.offset_clones = offset;
}

BOOL GetOffsetClones(void)
{
	return settings.offset_clones;
}

void SetGameCaption(BOOL caption)
{
	settings.game_caption = caption;
}

BOOL GetGameCaption(void)
{
	return settings.game_caption;
}

void SetBroadcast(BOOL broadcast)
{
	settings.broadcast = broadcast;
}

BOOL GetBroadcast(void)
{
	return settings.broadcast;
}

void SetRandomBackground(BOOL random_bg)
{
	settings.random_bg = random_bg;
}

BOOL GetRandomBackground(void)
{
	return settings.random_bg;
}

void SetSavedFolderID(UINT val)
{
	settings.folder_id = val;
}

UINT GetSavedFolderID(void)
{
	return settings.folder_id;
}

void SetShowScreenShot(BOOL val)
{
	settings.show_screenshot = val;
}

BOOL GetShowScreenShot(void)
{
	return settings.show_screenshot;
}

void SetShowFolderList(BOOL val)
{
	settings.show_folderlist = val;
}

BOOL GetShowFolderList(void)
{
	return settings.show_folderlist;
}

BOOL GetShowFolder(int folder)
{
	return TestBit(settings.show_folder_flags, folder);
}

void SetShowFolder(int folder, BOOL show)
{
	if (show)
		SetBit(settings.show_folder_flags, folder);
	else
		ClearBit(settings.show_folder_flags, folder);
}

void SetShowStatusBar(BOOL val)
{
	settings.show_statusbar = val;
}

BOOL GetShowStatusBar(void)
{
	return settings.show_statusbar;
}

void SetShowTabCtrl(BOOL val)
{
	settings.show_screenshottab = val;
}

BOOL GetShowTabCtrl(void)
{
	return settings.show_screenshottab;
}

void SetShowToolBar(BOOL val)
{
	settings.show_toolbar = val;
}

BOOL GetShowToolBar(void)
{
	return settings.show_toolbar;
}

void SetCurrentTab(const char *shortname)
{
	FreeIfAllocated(&settings.current_tab);
	if (shortname != NULL)
		settings.current_tab = strdup(shortname);
}

const char *GetCurrentTab(void)
{
	return settings.current_tab;
}

void SetDefaultGame(const char *name)
{
	FreeIfAllocated(&settings.default_game);

	if (name != NULL)
		settings.default_game = strdup(name);
}

const char *GetDefaultGame(void)
{
	return settings.default_game;
}

void SetWindowArea(AREA *area)
{
	settings.window_x = area->x;
	settings.window_y = area->y;
	settings.window_width = area->width;
	settings.window_height = area->height;
}

void GetWindowArea(AREA *area)
{
	area->x = settings.window_x;
	area->y = settings.window_y;
	area->width = settings.window_width;
	area->height = settings.window_height;
}

void SetWindowState(UINT state)
{
	settings.window_state = state;
}

UINT GetWindowState(void)
{
	return settings.window_state;
}

void SetCustomColor(int iIndex, COLORREF uColor)
{
	settings.custom_color[iIndex] = uColor;
}

COLORREF GetCustomColor(int iIndex)
{
	if (settings.custom_color[iIndex] == (COLORREF)-1)
		return (COLORREF)RGB(0,0,0);

	return settings.custom_color[iIndex];
}

void SetUseBrokenIcon(BOOL use_broken_icon)
{
	settings.use_broken_icon = use_broken_icon;
}

BOOL GetUseBrokenIcon(void)
{
	return settings.use_broken_icon;
}

void SetListFont(LOGFONTA *font)
{
	memcpy(&settings.list_logfont, font, sizeof(LOGFONTA));
}

void GetListFont(LOGFONTA *font)
{
	memcpy(font, &settings.list_logfont, sizeof(LOGFONTA));
}

void SetListFontColor(COLORREF uColor)
{
	if (settings.font_color == GetSysColor(COLOR_WINDOWTEXT))
		settings.font_color = (COLORREF)-1;
	else
		settings.font_color = uColor;
}

COLORREF GetListFontColor(void)
{
	if (settings.font_color == (COLORREF)-1)
		return (GetSysColor(COLOR_WINDOWTEXT));

	return settings.font_color;
}

void SetListCloneColor(COLORREF uColor)
{
	if (settings.clone_color == GetSysColor(COLOR_WINDOWTEXT))
		settings.clone_color = (COLORREF)-1;
	else
		settings.clone_color = uColor;
}

COLORREF GetListCloneColor(void)
{
	if (settings.clone_color == (COLORREF)-1)
		return (GetSysColor(COLOR_WINDOWTEXT));

	return settings.clone_color;

}

void SetListBrokenColor(COLORREF uColor)
{
	if (settings.broken_color == GetSysColor(COLOR_WINDOWTEXT))
		settings.broken_color = (COLORREF)-1;
	else
		settings.broken_color = uColor;
}

COLORREF GetListBrokenColor(void)
{
	if (settings.broken_color == (COLORREF)-1)
		return (GetSysColor(COLOR_WINDOWTEXT));

	return settings.broken_color;

}

int GetShowTab(int tab)
{
	return (settings.show_tab_flags & (1 << tab)) != 0;
}

void SetShowTab(int tab, BOOL show)
{
	if (show)
		settings.show_tab_flags |= 1 << tab;
	else
		settings.show_tab_flags &= ~(1 << tab);
}

// don't delete the last one
BOOL AllowedToSetShowTab(int tab, BOOL show)
{
	int show_tab_flags = settings.show_tab_flags;

	if (show == TRUE)
		return TRUE;

	show_tab_flags &= ~(1 << tab);
	return show_tab_flags != 0;
}

int GetHistoryTab(void)
{
#ifdef STORY_DATAFILE
	return settings.datafile_tab;
#else /* STORY_DATAFILE */
	return settings.history_tab;
#endif /* STORY_DATAFILE */
}

void SetHistoryTab(int tab, BOOL show)
{
#ifdef STORY_DATAFILE
	if (show)
		settings.datafile_tab = tab;
	else
		settings.datafile_tab = TAB_NONE;
#else /* STORY_DATAFILE */
	if (show)
		settings.history_tab = tab;
	else
		settings.history_tab = TAB_NONE;
#endif /* STORY_DATAFILE */
}

void SetColumnWidths(int width[])
{
	int i;

	for (i = 0; i < COLUMN_MAX; i++)
		settings.column_widths[i] = width[i];
}

void GetColumnWidths(int width[])
{
	int i;

	for (i = 0; i < COLUMN_MAX; i++)
		width[i] = settings.column_widths[i];
}

void SetSplitterPos(int splitterId, int pos)
{
	if (splitterId < GetSplitterCount())
		settings.splitters[splitterId] = pos;
}

int  GetSplitterPos(int splitterId)
{
	if (splitterId < GetSplitterCount())
		return settings.splitters[splitterId];

	return -1; /* Error */
}

void SetColumnOrder(int order[])
{
	int i;

	for (i = 0; i < COLUMN_MAX; i++)
		settings.column_order[i] = order[i];
}

void GetColumnOrder(int order[])
{
	int i;

	for (i = 0; i < COLUMN_MAX; i++)
		order[i] = settings.column_order[i];
}

void SetColumnShown(int shown[])
{
	int i;

	for (i = 0; i < COLUMN_MAX; i++)
		settings.column_shown[i] = shown[i];
}

void GetColumnShown(int shown[])
{
	int i;

	for (i = 0; i < COLUMN_MAX; i++)
		shown[i] = settings.column_shown[i];
}

void SetSortColumn(int column)
{
	settings.sort_column = column;
}

int GetSortColumn(void)
{
	return settings.sort_column;
}

void SetSortReverse(BOOL reverse)
{
	settings.sort_reverse = reverse;
}

BOOL GetSortReverse(void)
{
	return settings.sort_reverse;
}

#ifdef USE_SHOW_SPLASH_SCREEN
void SetDisplaySplashScreen (BOOL val)
{
	settings.display_splash_screen = val;
}

BOOL GetDisplaySplashScreen (void)
{
	return settings.display_splash_screen;
}
#endif /* USE_SHOW_SPLASH_SCREEN */

const char* GetRomDirs(void)
{
	return settings.rompath;
}

void SetRomDirs(const char* paths)
{
	FreeIfAllocated(&settings.rompath);

	if (paths != NULL)
	{
		settings.rompath = strdup(paths);

		// have our mame core (file code) know about it
		// this leaks a little, but the win32 file core writes to this string
		SetCorePathList(FILETYPE_ROM, settings.rompath);
		SetCorePathList(FILETYPE_IMAGE, settings.rompath);
	}
}

const char* GetSampleDirs(void)
{
	return settings.samplepath;
}

void SetSampleDirs(const char* paths)
{
	FreeIfAllocated(&settings.samplepath);

	if (paths != NULL)
	{
		settings.samplepath = strdup(paths);
		
		// have our mame core (file code) know about it
		// this leaks a little, but the win32 file core writes to this string
		SetCorePathList(FILETYPE_SAMPLE, settings.samplepath);
	}
}

const char* GetIniDir(void)
{
	return settings.inipath;
}

void SetIniDir(const char* path)
{
	if (!strcmp(path, settings.inipath))
		return;

	FreeIfAllocated(&settings.inipath);

	if (path != NULL)
		settings.inipath = strdup(path);

	if (MessageBox(0, _Unicode(reload_config_msg), TEXT("Reload configurations"), MB_YESNO | MB_ICONQUESTION) == IDNO)
	{
		int i;

		for (i = 0 ; i < num_drivers; i++)
			LoadGameOptions(i);
	}

	_mkdir(path);
}

const char* GetCtrlrDir(void)
{
	return settings.ctrlr_directory;
}

void SetCtrlrDir(const char* path)
{
	FreeIfAllocated(&settings.ctrlr_directory);

	if (path != NULL)
		settings.ctrlr_directory = strdup(path);
}

const char* GetCfgDir(void)
{
	return settings.cfg_directory;
}

void SetCfgDir(const char* path)
{
	FreeIfAllocated(&settings.cfg_directory);

	if (path != NULL)
		settings.cfg_directory = strdup(path);
}

const char* GetHiDir(void)
{
	return settings.hiscore_directory;
}

void SetHiDir(const char* path)
{
	FreeIfAllocated(&settings.hiscore_directory);

	if (path != NULL)
		settings.hiscore_directory = strdup(path);
}

const char* GetNvramDir(void)
{
	return settings.nvram_directory;
}

void SetNvramDir(const char* path)
{
	FreeIfAllocated(&settings.nvram_directory);

	if (path != NULL)
		settings.nvram_directory = strdup(path);
}

const char* GetInpDir(void)
{
	return settings.input_directory;
}

void SetInpDir(const char* path)
{
	FreeIfAllocated(&settings.input_directory);

	if (path != NULL)
		settings.input_directory = strdup(path);
}

const char* GetImgDir(void)
{
	return settings.snapshot_directory;
}

void SetImgDir(const char* path)
{
	FreeIfAllocated(&settings.snapshot_directory);

	if (path != NULL)
		settings.snapshot_directory = strdup(path);
}

const char* GetStateDir(void)
{
	return settings.state_directory;
}

void SetStateDir(const char* path)
{
	FreeIfAllocated(&settings.state_directory);

	if (path != NULL)
		settings.state_directory = strdup(path);
}

const char* GetArtDir(void)
{
	return settings.artwork_directory;
}

void SetArtDir(const char* path)
{
	FreeIfAllocated(&settings.artwork_directory);

	if (path != NULL)
		settings.artwork_directory = strdup(path);
}

const char* GetMemcardDir(void)
{
	return settings.memcard_directory;
}

void SetMemcardDir(const char* path)
{
	FreeIfAllocated(&settings.memcard_directory);

	if (path != NULL)
		settings.memcard_directory = strdup(path);
}

const char* GetFlyerDir(void)
{
	return settings.flyer_directory;
}

void SetFlyerDir(const char* path)
{
	FreeIfAllocated(&settings.flyer_directory);

	if (path != NULL)
		settings.flyer_directory = strdup(path);
}

const char* GetCabinetDir(void)
{
	return settings.cabinet_directory;
}

void SetCabinetDir(const char* path)
{
	FreeIfAllocated(&settings.cabinet_directory);

	if (path != NULL)
		settings.cabinet_directory = strdup(path);
}

const char* GetMarqueeDir(void)
{
	return settings.marquee_directory;
}

void SetMarqueeDir(const char* path)
{
	FreeIfAllocated(&settings.marquee_directory);

	if (path != NULL)
		settings.marquee_directory = strdup(path);
}

const char* GetTitlesDir(void)
{
	return settings.title_directory;
}

void SetTitlesDir(const char* path)
{
	FreeIfAllocated(&settings.title_directory);

	if (path != NULL)
		settings.title_directory = strdup(path);
}

const char * GetControlPanelDir(void)
{
	return settings.cpanel_directory;
}

void SetControlPanelDir(const char *path)
{
	FreeIfAllocated(&settings.cpanel_directory);
	if (path != NULL)
		settings.cpanel_directory = strdup(path);
}

const char* GetDiffDir(void)
{
	return settings.diff_directory;
}

void SetDiffDir(const char* path)
{
	FreeIfAllocated(&settings.diff_directory);

	if (path != NULL)
		settings.diff_directory = strdup(path);
}

const char* GetCommentDir(void)
{
	return settings.comment_directory;
}

void SetCommentDir(const char* path)
{
	FreeIfAllocated(&settings.comment_directory);

	if (path != NULL)
		settings.comment_directory = strdup(path);
}

#ifdef USE_IPS
const char *GetPatchDir(void)
{
	return settings.ips_directory;
}

void SetPatchDir(const char *path)
{
	FreeIfAllocated(&settings.ips_directory);

	if (path != NULL)
		settings.ips_directory = strdup(path);
}
#endif /* USE_IPS */

const char* GetLangDir(void)
{
	return settings.lang_directory;
}

void SetLangDir(const char* path)
{
	FreeIfAllocated(&settings.lang_directory);

	if (path != NULL)
		settings.lang_directory = strdup(path);
}

const char* GetIconsDir(void)
{
	return settings.icon_directory;
}

void SetIconsDir(const char* path)
{
	FreeIfAllocated(&settings.icon_directory);

	if (path != NULL)
		settings.icon_directory = strdup(path);
}

const char* GetBgDir(void)
{
	return settings.bkground_directory;
}

void SetBgDir(const char* path)
{
	FreeIfAllocated(&settings.bkground_directory);

	if (path != NULL)
		settings.bkground_directory = strdup(path);
}

const char *GetFolderDir(void)
{
	return settings.folder_directory;
}

void SetFolderDir(const char *path)
{
	FreeIfAllocated(&settings.folder_directory);

	if (path != NULL)
		settings.folder_directory = strdup(path);
}

const char* GetCheatFile(void)
{
	return settings.cheat_file;
}

void SetCheatFile(const char* path)
{
	FreeIfAllocated(&settings.cheat_file);

	if (path != NULL)
		settings.cheat_file = strdup(path);
}

const char* GetHistoryFile(void)
{
	return settings.history_file;
}

void SetHistoryFile(const char* path)
{
	FreeIfAllocated(&settings.history_file);

	if (path != NULL)
		settings.history_file = strdup(path);
}

#ifdef STORY_DATAFILE
const char* GetStoryFile(void)
{
	return settings.story_file;
}

void SetStoryFile(const char* path)
{
	FreeIfAllocated(&settings.story_file);

	if (path != NULL)
		settings.story_file = strdup(path);
}
#endif /* STORY_DATAFILE */

#ifdef USE_VIEW_PCBINFO
const char* GetPcbinfoDir(void)
{
	return settings.pcbinfo_directory;
}

void SetPcbinfoDir(const char* path)
{
	FreeIfAllocated(&settings.pcbinfo_directory);

	if (path != NULL)
		settings.pcbinfo_directory = mame_strdup(path);
}
#endif /* USE_VIEW_PCBINFO */

const char* GetMAMEInfoFile(void)
{
	return settings.mameinfo_file;
}

void SetMAMEInfoFile(const char* path)
{
	FreeIfAllocated(&settings.mameinfo_file);

	if (path != NULL)
		settings.mameinfo_file = strdup(path);
}

const char* GetHiscoreFile(void)
{
	return settings.hiscore_file;
}

void SetHiscoreFile(const char* path)
{
	FreeIfAllocated(&settings.hiscore_file);

	if (path != NULL)
		settings.hiscore_file = strdup(path);
}

void ResetGameOptions(int driver_index)
{
	assert(0 <= driver_index && driver_index < num_drivers);

	// make sure it's all loaded up.
	GetGameOptions(driver_index);

	if (!driver_variables[driver_index].use_default)
	{
		FreeGameOptions(&driver_options[driver_index]);
		driver_variables[driver_index].use_default = TRUE;
		
		// this will delete the custom file
		SaveGameOptions(driver_index);
	}
}

static void ResetAltOptions(alt_options_type *alt_option)
{
	// make sure it's all loaded up.
	GetAltOptions(alt_option);

	if (!alt_option->variable->use_default)
	{
		FreeGameOptions(alt_option->option);
		alt_option->variable->use_default = TRUE;

		// this will delete the custom file
		SaveAltOptions(alt_option);
	}
}

void ResetGameDefaults(void)
{
	FreeGameOptions(&global);
	CopyGameOptions(&backup.global, &global);
}

void ResetAllGameOptions(void)
{
	int i;

	for (i = 0; i < num_drivers; i++)
		ResetGameOptions(i);

	for (i = 0; i < num_alt_options; i++)
		ResetAltOptions(&alt_options[i]);
}

int GetRomAuditResults(int driver_index)
{
	assert(0 <= driver_index && driver_index < num_drivers);

	return driver_variables[driver_index].rom_audit_results;
}

void SetRomAuditResults(int driver_index, int audit_results)
{
	assert(0 <= driver_index && driver_index < num_drivers);

	driver_variables[driver_index].rom_audit_results = audit_results;
}

int GetSampleAuditResults(int driver_index)
{
	assert(0 <= driver_index && driver_index < num_drivers);

	return driver_variables[driver_index].samples_audit_results;
}

void SetSampleAuditResults(int driver_index, int audit_results)
{
	assert(0 <= driver_index && driver_index < num_drivers);

	driver_variables[driver_index].samples_audit_results = audit_results;
}

void IncrementPlayCount(int driver_index)
{
	assert(0 <= driver_index && driver_index < num_drivers);

	driver_variables[driver_index].play_count++;

	// maybe should do this
	//SavePlayCount(driver_index);
}

int GetPlayCount(int driver_index)
{
	assert(0 <= driver_index && driver_index < num_drivers);

	return driver_variables[driver_index].play_count;
}

void ResetPlayCount(int driver_index)
{
	int i = 0;
	assert(driver_index < num_drivers);
	if ( driver_index < 0 )
	{
		//All drivers
		for ( i= 0; i< num_drivers; i++ )
			driver_variables[i].play_count = 0;
	}
	else
	{
		driver_variables[driver_index].play_count = 0;
	}
}

void ResetPlayTime(int driver_index)
{
	int i = 0;
	assert(driver_index < num_drivers);
	if ( driver_index < 0 )
	{
		//All drivers
		for ( i= 0; i< num_drivers; i++ )
			driver_variables[i].play_time = 0;
	}
	else
	{
		driver_variables[driver_index].play_time = 0;
	}
}

int GetPlayTime(int driver_index)
{
	assert(0 <= driver_index && driver_index < num_drivers);

	return driver_variables[driver_index].play_time;
}

void IncrementPlayTime(int driver_index, int playtime)
{
	assert(0 <= driver_index && driver_index < num_drivers);
	driver_variables[driver_index].play_time += playtime;
}

void GetTextPlayTime(int driver_index, char *buf)
{
	int hour, minute, second;
	int temp = driver_variables[driver_index].play_time;

	assert(0 <= driver_index && driver_index < num_drivers);

	hour = temp / 3600;
	temp = temp - 3600*hour;
	minute = temp / 60; //Calc Minutes
	second = temp - 60*minute;

	if (hour == 0)
		sprintf(buf, "%d:%02d", minute, second );
	else
		sprintf(buf, "%d:%02d:%02d", hour, minute, second );
}


input_seq *Get_ui_key_up(void)
{
	return &settings.ui_key_up.is;
}
input_seq *Get_ui_key_down(void)
{
	return &settings.ui_key_down.is;
}
input_seq *Get_ui_key_left(void)
{
	return &settings.ui_key_left.is;
}
input_seq *Get_ui_key_right(void)
{
	return &settings.ui_key_right.is;
}
input_seq *Get_ui_key_start(void)
{
	return &settings.ui_key_start.is;
}
input_seq *Get_ui_key_pgup(void)
{
	return &settings.ui_key_pgup.is;
}
input_seq *Get_ui_key_pgdwn(void)
{
	return &settings.ui_key_pgdwn.is;
}
input_seq *Get_ui_key_home(void)
{
	return &settings.ui_key_home.is;
}
input_seq *Get_ui_key_end(void)
{
	return &settings.ui_key_end.is;
}
input_seq *Get_ui_key_ss_change(void)
{
	return &settings.ui_key_ss_change.is;
}
input_seq *Get_ui_key_history_up(void)
{
	return &settings.ui_key_history_up.is;
}
input_seq *Get_ui_key_history_down(void)
{
	return &settings.ui_key_history_down.is;
}


input_seq *Get_ui_key_context_filters(void)
{
	return &settings.ui_key_context_filters.is;
}
input_seq *Get_ui_key_select_random(void)
{
	return &settings.ui_key_select_random.is;
}
input_seq *Get_ui_key_game_audit(void)
{
	return &settings.ui_key_game_audit.is;
}
input_seq *Get_ui_key_game_properties(void)
{
	return &settings.ui_key_game_properties.is;
}
input_seq *Get_ui_key_help_contents(void)
{
	return &settings.ui_key_help_contents.is;
}
input_seq *Get_ui_key_update_gamelist(void)
{
	return &settings.ui_key_update_gamelist.is;
}
input_seq *Get_ui_key_view_folders(void)
{
	return &settings.ui_key_view_folders.is;
}
input_seq *Get_ui_key_view_fullscreen(void)
{
	return &settings.ui_key_view_fullscreen.is;
}
input_seq *Get_ui_key_view_pagetab(void)
{
	return &settings.ui_key_view_pagetab.is;
}
input_seq *Get_ui_key_view_picture_area(void)
{
	return &settings.ui_key_view_picture_area.is;
}
input_seq *Get_ui_key_view_status(void)
{
	return &settings.ui_key_view_status.is;
}
input_seq *Get_ui_key_view_toolbars(void)
{
	return &settings.ui_key_view_toolbars.is;
}

input_seq *Get_ui_key_view_tab_cabinet(void)
{
	return &settings.ui_key_view_tab_cabinet.is;
}
input_seq *Get_ui_key_view_tab_cpanel(void)
{
	return &settings.ui_key_view_tab_cpanel.is;
}
input_seq *Get_ui_key_view_tab_flyer(void)
{
	return &settings.ui_key_view_tab_flyer.is;
}
input_seq *Get_ui_key_view_tab_history(void)
{
	return &settings.ui_key_view_tab_history.is;
}
#ifdef STORY_DATAFILE
input_seq *Get_ui_key_view_tab_story(void)
{
	return &settings.ui_key_view_tab_story.is;
}
#endif /* STORY_DATAFILE */
input_seq *Get_ui_key_view_tab_marquee(void)
{
	return &settings.ui_key_view_tab_marquee.is;
}
input_seq *Get_ui_key_view_tab_screenshot(void)
{
	return &settings.ui_key_view_tab_screenshot.is;
}
input_seq *Get_ui_key_view_tab_title(void)
{
	return &settings.ui_key_view_tab_title.is;
}
input_seq *Get_ui_key_quit(void)
{
	return &settings.ui_key_quit.is;
}


int GetUIJoyUp(int joycodeIndex)
{
	assert(0 <= joycodeIndex && joycodeIndex < 4);
	
	return settings.ui_joy_up[joycodeIndex];
}

void SetUIJoyUp(int joycodeIndex, int val)
{
	assert(0 <= joycodeIndex && joycodeIndex < 4);

	settings.ui_joy_up[joycodeIndex] = val;
}

int GetUIJoyDown(int joycodeIndex)
{
	assert(0 <= joycodeIndex && joycodeIndex < 4);

	return settings.ui_joy_down[joycodeIndex];
}

void SetUIJoyDown(int joycodeIndex, int val)
{
	assert(0 <= joycodeIndex && joycodeIndex < 4);

	settings.ui_joy_down[joycodeIndex] = val;
}

int GetUIJoyLeft(int joycodeIndex)
{
	assert(0 <= joycodeIndex && joycodeIndex < 4);

	return settings.ui_joy_left[joycodeIndex];
}

void SetUIJoyLeft(int joycodeIndex, int val)
{
	assert(0 <= joycodeIndex && joycodeIndex < 4);

	settings.ui_joy_left[joycodeIndex] = val;
}

int GetUIJoyRight(int joycodeIndex)
{
	assert(0 <= joycodeIndex && joycodeIndex < 4);

	return settings.ui_joy_right[joycodeIndex];
}

void SetUIJoyRight(int joycodeIndex, int val)
{
	assert(0 <= joycodeIndex && joycodeIndex < 4);

	settings.ui_joy_right[joycodeIndex] = val;
}

int GetUIJoyStart(int joycodeIndex)
{
	assert(0 <= joycodeIndex && joycodeIndex < 4);

	return settings.ui_joy_start[joycodeIndex];
}

void SetUIJoyStart(int joycodeIndex, int val)
{
	assert(0 <= joycodeIndex && joycodeIndex < 4);

	settings.ui_joy_start[joycodeIndex] = val;
}

int GetUIJoyPageUp(int joycodeIndex)
{
	assert(0 <= joycodeIndex && joycodeIndex < 4);

	return settings.ui_joy_pgup[joycodeIndex];
}

void SetUIJoyPageUp(int joycodeIndex, int val)
{
	assert(0 <= joycodeIndex && joycodeIndex < 4);

	settings.ui_joy_pgup[joycodeIndex] = val;
}

int GetUIJoyPageDown(int joycodeIndex)
{
	assert(0 <= joycodeIndex && joycodeIndex < 4);

	return settings.ui_joy_pgdwn[joycodeIndex];
}

void SetUIJoyPageDown(int joycodeIndex, int val)
{
	assert(0 <= joycodeIndex && joycodeIndex < 4);

	settings.ui_joy_pgdwn[joycodeIndex] = val;
}

int GetUIJoyHome(int joycodeIndex)
{
	assert(0 <= joycodeIndex && joycodeIndex < 4);

	return settings.ui_joy_home[joycodeIndex];
}

void SetUIJoyHome(int joycodeIndex, int val)
{
	assert(0 <= joycodeIndex && joycodeIndex < 4);

	settings.ui_joy_home[joycodeIndex] = val;
}

int GetUIJoyEnd(int joycodeIndex)
{
	assert(0 <= joycodeIndex && joycodeIndex < 4);

	return settings.ui_joy_end[joycodeIndex];
}

void SetUIJoyEnd(int joycodeIndex, int val)
{
	assert(0 <= joycodeIndex && joycodeIndex < 4);

	settings.ui_joy_end[joycodeIndex] = val;
}

int GetUIJoySSChange(int joycodeIndex)
{
	assert(0 <= joycodeIndex && joycodeIndex < 4);

	return settings.ui_joy_ss_change[joycodeIndex];
}

void SetUIJoySSChange(int joycodeIndex, int val)
{
	assert(0 <= joycodeIndex && joycodeIndex < 4);

	settings.ui_joy_ss_change[joycodeIndex] = val;
}

int GetUIJoyHistoryUp(int joycodeIndex)
{
	assert(0 <= joycodeIndex && joycodeIndex < 4);

	return settings.ui_joy_history_up[joycodeIndex];
}

void SetUIJoyHistoryUp(int joycodeIndex, int val)
{
	assert(0 <= joycodeIndex && joycodeIndex < 4);
  
	settings.ui_joy_history_up[joycodeIndex] = val;
}

int GetUIJoyHistoryDown(int joycodeIndex)
{
	assert(0 <= joycodeIndex && joycodeIndex < 4);

	return settings.ui_joy_history_down[joycodeIndex];
}

void SetUIJoyHistoryDown(int joycodeIndex, int val)
{
	assert(0 <= joycodeIndex && joycodeIndex < 4);

	settings.ui_joy_history_down[joycodeIndex] = val;
}

void SetUIJoyExec(int joycodeIndex, int val)
{
	assert(0 <= joycodeIndex && joycodeIndex < 4);

	settings.ui_joy_exec[joycodeIndex] = val;
}

int GetUIJoyExec(int joycodeIndex)
{
	assert(0 <= joycodeIndex && joycodeIndex < 4);

	return settings.ui_joy_exec[joycodeIndex];
}

char * GetExecCommand(void)
{
	static char empty = '\0';

	if (settings.exec_command)
		return settings.exec_command;

	return &empty;
}

void SetExecCommand(char *cmd)
{
	settings.exec_command = cmd;
}

int GetExecWait(void)
{
	return settings.exec_wait;
}

void SetExecWait(int wait)
{
	settings.exec_wait = wait;
}
 
BOOL GetHideMouseOnStartup(void)
{
	return settings.hide_mouse;
}

void SetHideMouseOnStartup(BOOL hide)
{
	settings.hide_mouse = hide;
}

BOOL GetRunFullScreen(void)
{
	return settings.full_screen;
}

void SetRunFullScreen(BOOL fullScreen)
{
	settings.full_screen = fullScreen;
}

char* GetVersionString(void)
{
	return build_version;
}

void SetFolderFlags(const char *folderName, DWORD dwFlags)
{
	set_folder_flag(&settings.folder_flag, folderName, dwFlags);
}

DWORD GetFolderFlags(const char *folderName)
{
	int i;

	if (settings.folder_flag.entry == NULL)
		return 0;

	for (i = 0; i < settings.folder_flag.num; i++)
		if (settings.folder_flag.entry[i].name
		 && strcmp(folderName, settings.folder_flag.entry[i].name) == 0)
			return settings.folder_flag.entry[i].flags;

	return 0;
}

void SaveGameOptions(int driver_index)
{
	int i;

	assert (0 <= driver_index && driver_index < num_drivers);

	options_save_driver_config(driver_index);

	for (i = 0; i < num_drivers; i++)
		if (DriverParentIndex(i) == driver_index)
		{
			driver_variables[i].use_default = TRUE;
			driver_variables[i].options_loaded = FALSE;
		}
}

static void InvalidateGameOptionsInDriver(const char *name)
{
	int i;

	for (i = 0; i < num_drivers; i++)
	{
		if (driver_variables[i].alt_index != -1)
			continue;

		if (strcmp(GetDriverFilename(i), name) == 0)
		{
			driver_variables[i].use_default = TRUE;
			driver_variables[i].options_loaded = FALSE;
		}
	}
}

static void SaveAltOptions(alt_options_type *alt_option)
{
	options_save_alt_config(alt_option);

	if (alt_option->option == GetVectorOptions())
	{
		int i;

		for (i = 0; i < num_alt_options; i++)
			if (alt_options[i].need_vector_config)
			{
				alt_options[i].variable->use_default = TRUE;
				alt_options[i].variable->options_loaded = FALSE;
				InvalidateGameOptionsInDriver(alt_options[i].name);
			}
	}

	InvalidateGameOptionsInDriver(alt_option->name);
}

void SaveFolderOptions(const char *name)
{
	int alt_index = bsearch_alt_option(name);

	assert (0 <= alt_index && alt_index < num_alt_options);

	SaveAltOptions(&alt_options[alt_index]);
}

void SaveDefaultOptions(void)
{
	int i;

	options_save_default_config();

	for (i = 0; i < num_alt_options; i++)
	{
		alt_options[i].variable->use_default = TRUE;
		alt_options[i].variable->options_loaded = FALSE;
	}

	for (i = 0; i < num_drivers; i++)
	{
		driver_variables[i].use_default = TRUE;
		driver_variables[i].options_loaded = FALSE;
	}

	/* default option has bios tab. so save default bios */
	for (i = 0; i < num_alt_options; i++)
		if (alt_options[i].option->bios)
		{
			char *bios = strdup(alt_options[i].option->bios);

			GetAltOptions(&alt_options[i]);

			FreeIfAllocated(&alt_options[i].option->bios);
			alt_options[i].option->bios = bios;

			options_save_alt_config(&alt_options[i]);
		}
}

void SaveOptions(void)
{
	int i;

	options_save_winui_config();
	options_save_default_config();

	for (i = 0; i < num_drivers; i++)
		options_save_driver_config(i);

	for (i = 0; i < num_alt_options; i++)
		options_save_alt_config(&alt_options[i]);
}

/***************************************************************************
    Internal functions
 ***************************************************************************/

static int regist_alt_option(const char *name)
{
	int s = 0;
	int n = num_alt_options;

	while (n > 0)
	{
		int n2 = n / 2;
		int result;

		if (name == alt_options[s + n2].name)
			return -1;

		result = strcmp(name, alt_options[s + n2].name);
		if (!result)
		{
			alt_options[s + n2].name = name;
			return -1;
		}

		if (result < 0)
			n = n2;
		else
		{
			s += n2 + 1;
			n -= n2 + 1;
		}
	}

	// not found, add it.
	if (num_alt_options == alt_options_len)
	{
		alt_options_len += ALLOC_FOLDERS;
		alt_options = (alt_options_type *)realloc(alt_options, alt_options_len * sizeof (alt_options_type));

		if (!alt_options)
			exit(0);
	}

	for (n = num_alt_options++; n > s; n--)
		alt_options[n].name = alt_options[n - 1].name;

	alt_options[s].name = name;

	return s;
}

static int bsearch_alt_option(const char *name)
{
	int s = 0;
	int n = num_alt_options;

	while (n > 0)
	{
		int n2 = n / 2;
		int result;

		result = strcmp(name, alt_options[s + n2].name);
		if (!result)
			return s + n2;

		if (result < 0)
			n = n2;
		else
		{
			s += n2 + 1;
			n -= n2 + 1;
		}
	}

	return -1;
}

static void build_default_bios(void)
{
	int i;

	for (i = 0; i < num_drivers; i++)
	{
		if (drivers[i]->bios)
		{
			const game_driver *drv = drivers[i];
			int n;

			while (!(drv->flags & NOT_A_DRIVER) && driver_get_clone(drv))
				drv = driver_get_clone(drv);

			for (n = 0; n < MAX_SYSTEM_BIOS; n++)
			{
				if (default_bios[n].drv == NULL)
				{
					int alt_index = bsearch_alt_option(GetFilename(drv->source_file));

					assert(0 <= alt_index && alt_index < num_alt_options);

					default_bios[n].drv = drv;
					default_bios[n].alt_option = &alt_options[alt_index];
					default_bios[n].alt_option->has_bios = TRUE;
					break;
				}
				else if (default_bios[n].drv == drv)
					break;
			}
		}
	}

}

static void build_alt_options(void)
{
	options_type *pOpts;
	driver_variables_type *pVars;
	int i;

	alt_options = (alt_options_type *)malloc(alt_options_len * sizeof (alt_options_type));
	num_alt_options = 0;

	if (!alt_options)
		exit(0);

	regist_alt_option("Vector");

	for (i = 0; i < num_drivers; i++)
		regist_alt_option(GetDriverFilename(i));

	pOpts = (options_type *)malloc(num_alt_options * sizeof (options_type));
	pVars = (driver_variables_type *)malloc(num_alt_options * sizeof (driver_variables_type));

	if (!pOpts || !pVars)
		exit(0);

	memset(pOpts, 0, num_alt_options * sizeof (options_type));
	memset(pVars, 0, num_alt_options * sizeof (driver_variables_type));

	for (i = 0; i < num_alt_options; i++)
	{
		alt_options[i].option = &pOpts[i];
		alt_options[i].variable = &pVars[i];
		alt_options[i].variable->options_loaded = FALSE;
		alt_options[i].variable->use_default = TRUE;
		alt_options[i].has_bios = FALSE;
		alt_options[i].need_vector_config = FALSE;
		alt_options[i].driver_index = -1;
	}

	for (i = 0; i < num_drivers; i++)
	{
		const char *src = GetDriverFilename(i);
		int n = bsearch_alt_option(src);

		if (!alt_options[n].need_vector_config && DriverIsVector(i))
			alt_options[n].need_vector_config = TRUE;
	}
}

static void unify_alt_options(void)
{
	int i;

	for (i = 0; i < num_drivers; i++)
	{
		char buf[16];
		int n;

		sprintf(buf, "%s.c", drivers[i]->name);
		n = bsearch_alt_option(buf);
		if (n == -1)
			continue;

		dprintf("Unify %s", drivers[i]->name);

		driver_variables[i].alt_index = n;
		alt_options[n].option = &driver_options[i];
		alt_options[n].variable = &driver_variables[i];
		alt_options[n].driver_index = i;
	}
}

static const char *get_base_config_directory(void)
{
	char full[_MAX_PATH];
	char dir[_MAX_DIR];
	static char path[_MAX_PATH];

	GetModuleFileNameA(GetModuleHandle(NULL), full, _MAX_PATH);
	_splitpath(full, path, dir, NULL, NULL);
	strcat(path, dir);

	if (path[strlen(path) - 1] == '\\')
		path[strlen(path) - 1] = '\0';

	return path;
}

static options_type *update_driver_use_default(int driver_index)
{
	options_type *opt = GetParentOptions(driver_index);
#ifdef USE_IPS
	// HACK: DO NOT INHERIT IPS CONFIGURATION
	char *ips;
#endif /* USE_IPS */

	if (opt == &driver_options[driver_index])
		return NULL;

#ifdef USE_IPS
	ips = driver_options[driver_index].ips;
	driver_options[driver_index].ips = NULL;
#endif /* USE_IPS */

	driver_variables[driver_index].use_default = IsOptionEqual(&driver_options[driver_index], opt);

#ifdef USE_IPS
	if (driver_variables[driver_index].use_default && ips)
		dprintf("%s: use_default with ips", drivers[driver_index]->name);

	driver_options[driver_index].ips = ips;
#endif /* USE_IPS */

	return opt;
}

static options_type *update_alt_use_default(alt_options_type *alt_option)
{
	options_type *opt = GetDefaultOptions();
	char *bios;
#ifdef USE_IPS
	// HACK: DO NOT INHERIT IPS CONFIGURATION
	char *ips;
#endif /* USE_IPS */

	// try vector.ini
	if (alt_option->need_vector_config)
		opt = GetVectorOptions();

	bios = alt_option->option->bios;
	alt_option->option->bios = global.bios;

#ifdef USE_IPS
	ips = alt_option->option->ips;
	alt_option->option->ips = NULL;
#endif /* USE_IPS */

	alt_option->variable->use_default = IsOptionEqual(alt_option->option, opt);

#ifdef USE_IPS
	if (alt_option->variable->use_default && ips)
		dprintf("%s: use_default with ips", alt_option->name);

	alt_option->option->ips = ips;
#endif /* USE_IPS */

	alt_option->option->bios = bios;

	return opt;
}

static void validate_resolution(char **p)
{
	if (strcmp(*p, "0x0x0@0") == 0)
	{
		FreeIfAllocated(p);
		*p = strdup("auto");
	}
}

static void validate_driver_option(options_type *opt)
{
	validate_resolution(&opt->resolution0);
	validate_resolution(&opt->resolution1);
	validate_resolution(&opt->resolution2);
	validate_resolution(&opt->resolution3);

	//if (DirectDraw_GetNumDisplays() < 2)
	//	FreeIfAllocated(&opt->screen);
}

static void set_folder_flag(f_flag *flag, const char *folderName, DWORD dwFlags)
{
	int i;

	if (flag->entry == NULL)
	{
		flag->entry = malloc(ALLOC_FOLDERFLAG * sizeof(*flag->entry));
		if (!flag->entry)
		{
			dprintf("error: malloc failed in set_folder_flag\n");
			return;
		}

		flag->num = ALLOC_FOLDERFLAG;
		memset(flag->entry, 0, flag->num * sizeof(*flag->entry));
	}

	for (i = 0; i < flag->num; i++)
		if (flag->entry[i].name
		 && strcmp(flag->entry[i].name, folderName) == 0)
		{
			if (dwFlags == 0)
			{
				free(flag->entry[i].name);
				flag->entry[i].name = NULL;
			}
			else
				flag->entry[i].flags = dwFlags;

			return;
		}

	if (dwFlags == 0)
		return;

	for (i = 0; i < flag->num; i++)
		if (flag->entry[i].name == NULL)
			break;

	if (i == flag->num)
	{
		f_flag_entry *tmp;

		tmp = realloc(flag->entry, (flag->num + ALLOC_FOLDERFLAG) * sizeof(*tmp));
		if (!tmp)
		{
			dprintf("error: realloc failed in set_folder_flag\n");
			return;
		}

		flag->entry = tmp;
		memset(tmp + flag->num, 0, ALLOC_FOLDERFLAG * sizeof(*tmp));
		flag->num += ALLOC_FOLDERFLAG;
	}

	flag->entry[i].name = strdup(folderName);
	flag->entry[i].flags = dwFlags;
}

static void free_folder_flag(f_flag *flag)
{
	int i;

	for (i = 0; i < flag->num; i++)
		FreeIfAllocated(&flag->entry[i].name);

	free(flag->entry);
	flag->entry = NULL;
	flag->num = 0;
}

static void LoadGameOptions(int driver_index)
{
	assert (0 <= driver_index && driver_index < num_drivers);

	options_load_driver_config(driver_index);
}

static void LoadAltOptions(alt_options_type *alt_option)
{
	options_load_alt_config(alt_option);
}

static void LoadDefaultOptions(void)
{
	options_load_default_config();
}

static void LoadOptions(void)
{
	LoadDefaultOptions();
	options_load_winui_config();
	SetLangcode(settings.langcode);
	SetUseLangList(UseLangList());

#if 0
	if (!settings.reset_gui)
	{
		static char oldInfoMsg[400] = 
MAME32NAME " has detected outdated configuration data.\n\n\
The detected configuration data is from Version %s of " MAME32NAME ".\n\
The current version is %s. It is recommended that the\n\
configuration is set to the new defaults.\n\n\
Would you like to use the new configuration?";

		char *current_version;
		const char *save_version = settings.save_version;

		current_version = GetVersionString();

		if (save_version && *save_version && strcmp(save_version, current_version) != 0)
		{
			char msg[400];
			sprintf(msg,_UI(oldInfoMsg), save_version, current_version);
			if (MessageBox(0, _Unicode(msg), _Unicode(_UI("Version Mismatch")), MB_YESNO | MB_ICONQUESTION) == IDYES)
				settings.reset_gui = TRUE;
		}
	}
#endif

	if (settings.reset_gui)
	{
		FreeSettings(&settings);
		CopySettings(&backup.settings, &settings);
		SetLangcode(settings.langcode);
		SetUseLangList(UseLangList());
	}

	settings.reset_gui = FALSE;
}


//============================================================

static void *options_core;	// all options
static void *options_driver;	// all options except fileio, palette, and language 
static void *options_winui;	// only GUI related

const options_entry winui_opts[] =
{
	{ NULL,                          NULL,                         OPTION_HEADER,     "PATH AND DIRECTORY OPTIONS"},
	{ "flyer_directory",             "flyers",                     0,                 "directory for flyers"},
	{ "cabinet_directory",           "cabinets",                   0,                 "directory for cabinets"},
	{ "marquee_directory",           "marquees",                   0,                 "directory for marquees"},
	{ "title_directory",             "titles",                     0,                 "directory for titles"},
	{ "cpanel_directory",            "cpanel",                     0,                 "directory for control panel"},
	{ "icon_directory",              "icons",                      0,                 "directory for icons"},
	{ "bkground_directory",          "bkground",                   0,                 "directory for bkground"},
	{ "folder_directory",            "folders",                    0,                 "directory for folders-ini"},
#ifdef USE_VIEW_PCBINFO
	{ "pcbinfo_directory",           "pcb",                        0,                 "directory for pcb info"},
#endif /* USE_VIEW_PCBINFO */

	{ NULL,                          NULL,                         OPTION_HEADER,     "INTERFACE OPTIONS"},
	{ "save_version",                NULL,                         0,                 "save version"},
	{ "reset_gui",                   "0",                          OPTION_BOOLEAN,    "enable version mismatch warning"},
	{ "game_check",                  "1",                          OPTION_BOOLEAN,    "search for new games"},
	{ "joygui",                      "0",                          OPTION_BOOLEAN,    "allow game selection by a joystick"},
	{ "keygui",                      "0",                          OPTION_BOOLEAN,    "allow game selection by a keyboard"},
	{ "broadcast",                   "0",                          OPTION_BOOLEAN,    "broadcast selected game to all windows"},
	{ "random_bg",                   "1",                          OPTION_BOOLEAN,    "random select background image"},
	{ "cycle_screenshot",            "0",                          0,                 "cycle screen shot image"},
	{ "stretch_screenshot_larger",   "0",                          OPTION_BOOLEAN,    "stretch screenshot larger"},
	{ "screenshot_bordersize",       "11",                         0,                 "screen shot border size"},
	{ "screenshot_bordercolor",      "-1",                         0,                 "screen shot border color"},
	{ "inherit_filter",              "0",                          OPTION_BOOLEAN,    "inheritable filters"},
	{ "offset_clones",               "1",                          OPTION_BOOLEAN,    "no offset for clones missing parent in view"},
	{ "game_caption",                "1",                          OPTION_BOOLEAN,    "show game caption"},
#ifdef USE_SHOW_SPLASH_SCREEN
	{ "display_splash_screen",       "0",                          OPTION_BOOLEAN,    "display splash screen on start"},
#endif /* USE_SHOW_SPLASH_SCREEN */

	{ NULL,                          NULL,                         OPTION_HEADER,     "GENERAL OPTIONS"},
#ifdef MESS
	{ "default_system",              "nes",                        0,                 "last selected system name"},
#else
	{ "default_game",                "puckman",                    0,                 "last selected game name"},
#endif
	{ "show_toolbar",                "1",                          OPTION_BOOLEAN,    "show tool bar"},
	{ "show_statusbar",              "1",                          OPTION_BOOLEAN,    "show status bar"},
	{ "show_folderlist",             "1",                          OPTION_BOOLEAN,    "show folder list"},
	{ "show_screenshot",             "1",                          OPTION_BOOLEAN,    "show image picture"},
	{ "show_screenshottab",          "1",                          OPTION_BOOLEAN,    "show tab control"},
	{ "show_tab_flags",              "63",                         0,                 "show tab control flags"},
	{ "current_tab",                 "snapshot",                   0,                 "current image picture"},
#ifdef STORY_DATAFILE
	// TAB_ALL = 10
	{ "datafile_tab",                "10",                         0,                 "where to show history on tab"},
#else /* STORY_DATAFILE */
	// TAB_ALL = 9
	{ "history_tab",                 "9",                          0,                 "where to show history on tab"},
#endif /* STORY_DATAFILE */
	{ "exec_command",                NULL,                         0,                 "execute command line"},
	{ "exec_wait",                   "0",                          0,                 "execute wait"},
	{ "hide_mouse",                  "0",                          OPTION_BOOLEAN,    "hide mouse"},
	{ "full_screen",                 "0",                          OPTION_BOOLEAN,    "full screen"},

	{ NULL,                          NULL,                         OPTION_HEADER,     "WINDOW POSITION OPTIONS"},
	{ "window_x",                    "0",                          0,                 "window left position"},
	{ "window_y",                    "0",                          0,                 "window top position"},
	{ "window_width",                "640",                        0,                 "window width"},
	{ "window_height",               "400",                        0,                 "window height"},
	{ "window_state",                "1",                          0,                 "window state"},

	{ NULL,                          NULL,                         OPTION_HEADER,     "LISTVIEW OPTIONS"},
	{ "list_mode",                   "Grouped",                    0,                 "view mode"},
	{ "splitters",                   "150,300",                    0,                 "splitter position"},
	{ "column_widths",               "186,68,84,84,64,88,74,108,60,144,84,60", 0,     "column width settings"},
	{ "column_order",                "0,2,3,4,5,6,7,8,9,10,11,1",  0,                 "column order settings"},
	{ "column_shown",                "1,0,1,1,1,1,1,1,1,1,1,1",    0,                 "show or hide column settings"},
	{ "sort_column",                 "0",                          0,                 "sort column"},
	{ "sort_reverse",                "0",                          OPTION_BOOLEAN,    "sort descending"},
	{ "folder_id",                   "0",                          0,                 "last selected folder id"},
	{ "use_broken_icon",             "1",                          OPTION_BOOLEAN,    "use broken icon for not working games"},

	{ NULL,                          NULL,                         OPTION_HEADER,     "LIST FONT OPTIONS"},
	{ "list_font",                   "-8,0,0,0,400,0,0,0,0,0,0,0,0", 0,               "game list font size"},
	{ "list_fontface",               "MS Sans Serif",              0,                 "game list font face"},
	{ "font_color",                  "-1",                         0,                 "game list font color"},
	{ "clone_color",                 "8421504",                    0,                 "clone game list font color"},
	{ "broken_color",                "202",                        0,                 "broken game list font color"},
	{ "custom_color",                "0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0", 0,            "custom colors"},

	{ NULL,                          NULL,                         OPTION_HEADER,     "FOLDER LIST HIDE OPTIONS"},
	{ "folder_hide",                 NULL,                         0,                 "hide selected item in folder list"},
	{ "folder_flag",                 NULL,                         0,                 "folder list filters settings" },

	{ NULL,                          NULL,                         OPTION_HEADER,     "GUI JOYSTICK OPTIONS"},
	{ "ui_joy_up",                   "1,JOYCODE_STICK_AXIS,2,JOYCODE_DIR_NEG", 0,     "joystick to up"},
	{ "ui_joy_down",                 "1,JOYCODE_STICK_AXIS,2,JOYCODE_DIR_POS", 0,     "joystick to down"},
	{ "ui_joy_left",                 "1,JOYCODE_STICK_AXIS,1,JOYCODE_DIR_NEG", 0,     "joystick to left"},
	{ "ui_joy_right",                "1,JOYCODE_STICK_AXIS,1,JOYCODE_DIR_POS", 0,     "joystick to right"},
	{ "ui_joy_start",                "1,JOYCODE_STICK_BTN,1,JOYCODE_DIR_BTN", 0,      "joystick to start game"},
	{ "ui_joy_pgup",                 "2,JOYCODE_STICK_AXIS,2,JOYCODE_DIR_NEG", 0,     "joystick to page-up"},
	{ "ui_joy_pgdwn",                "2,JOYCODE_STICK_AXIS,2,JOYCODE_DIR_POS", 0,     "joystick to page-down"},
	{ "ui_joy_home",                 NULL,                         0,                 "joystick to home"},
	{ "ui_joy_end",                  NULL,                         0,                 "joystick to end"},
	{ "ui_joy_ss_change",            "2,JOYCODE_STICK_BTN,3,JOYCODE_DIR_BTN", 0,      "joystick to change picture"},
	{ "ui_joy_history_up",           "2,JOYCODE_STICK_BTN,4,JOYCODE_DIR_BTN", 0,      "joystick to scroll history up"},
	{ "ui_joy_history_down",         "2,JOYCODE_STICK_BTN,1,JOYCODE_DIR_BTN", 0,      "joystick to scroll history down"},
	{ "ui_joy_exec",                 NULL,                         0,                 "joystick execute commandline"},

	{ NULL,                          NULL,                         OPTION_HEADER,     "GUI KEYBOARD OPTIONS"},
	{ "ui_key_up",                   "KEYCODE_UP",                 0,                 "keyboard to up"},
	{ "ui_key_down",                 "KEYCODE_DOWN",               0,                 "keyboard to down"},
	{ "ui_key_left",                 "KEYCODE_LEFT",               0,                 "keyboard to left"},
	{ "ui_key_right",                "KEYCODE_RIGHT",              0,                 "keyboard to right"},
	{ "ui_key_start",                "KEYCODE_ENTER NOT KEYCODE_LALT", 0,             "keyboard to start game"},
	{ "ui_key_pgup",                 "KEYCODE_PGUP",               0,                 "keyboard to page-up"},
	{ "ui_key_pgdwn",                "KEYCODE_PGDN",               0,                 "keyboard to page-down"},
	{ "ui_key_home",                 "KEYCODE_HOME",               0,                 "keyboard to home"},
	{ "ui_key_end",                  "KEYCODE_END",                0,                 "keyboard to end"},
	{ "ui_key_ss_change",            "KEYCODE_LALT KEYCODE_0",     0,                 "keyboard to change picture"},
	{ "ui_key_history_up",           "KEYCODE_INSERT",             0,                 "keyboard to history up"},
	{ "ui_key_history_down",         "KEYCODE_DEL",                0,                 "keyboard to history down"},

	{ "ui_key_context_filters",      "KEYCODE_LCONTROL KEYCODE_F", 0,                 "keyboard to context filters"},
	{ "ui_key_select_random",        "KEYCODE_LCONTROL KEYCODE_R", 0,                 "keyboard to select random"},
	{ "ui_key_game_audit",           "KEYCODE_LALT KEYCODE_A",     0,                 "keyboard to game audit"},
	{ "ui_key_game_properties",      "KEYCODE_LALT KEYCODE_ENTER", 0,                 "keyboard to game properties"},
	{ "ui_key_help_contents",        "KEYCODE_F1",                 0,                 "keyboard to help contents"},
	{ "ui_key_update_gamelist",      "KEYCODE_F5",                 0,                 "keyboard to update game list"},
	{ "ui_key_view_folders",         "KEYCODE_LALT KEYCODE_D",     0,                 "keyboard to view folders"},
	{ "ui_key_view_fullscreen",      "KEYCODE_F11",                0,                 "keyboard to full screen"},
	{ "ui_key_view_pagetab",         "KEYCODE_LALT KEYCODE_B",     0,                 "keyboard to view page tab"},
	{ "ui_key_view_picture_area",    "KEYCODE_LALT KEYCODE_P",     0,                 "keyboard to view picture area"},
	{ "ui_key_view_status",          "KEYCODE_LALT KEYCODE_S",     0,                 "keyboard to view status"},
	{ "ui_key_view_toolbars",        "KEYCODE_LALT KEYCODE_T",     0,                 "keyboard to view toolbars"},

	{ "ui_key_view_tab_cabinet",     "KEYCODE_LALT KEYCODE_3",     0,                 "keyboard to view tab cabinet"},
	{ "ui_key_view_tab_cpanel",      "KEYCODE_LALT KEYCODE_6",     0,                 "keyboard to view tab control panel"},
	{ "ui_key_view_tab_flyer",       "KEYCODE_LALT KEYCODE_2",     0,                 "keyboard to view tab flyer"},
	{ "ui_key_view_tab_history",     "KEYCODE_LALT KEYCODE_7",     0,                 "keyboard to view tab history"},
#ifdef STORY_DATAFILE
	{ "ui_key_view_tab_story",       "KEYCODE_LALT KEYCODE_8",     0,                 "keyboard to view tab story"},
#endif /* STORY_DATAFILE */
	{ "ui_key_view_tab_marquee",     "KEYCODE_LALT KEYCODE_4",     0,                 "keyboard to view tab marquee"},
	{ "ui_key_view_tab_screenshot",  "KEYCODE_LALT KEYCODE_1",     0,                 "keyboard to view tab screen shot"},
	{ "ui_key_view_tab_title",       "KEYCODE_LALT KEYCODE_5",     0,                 "keyboard to view tab title"},
	{ "ui_key_quit",                 "KEYCODE_LALT KEYCODE_Q",     0,                 "keyboard to quit application"},

	{ NULL }
};

static const char *driver_flag_names[] =
{
	"_playcount",
	"_play_time",
	"_rom_audit",
	"_samples_audit"
};

static char driver_flag_unknown[3];

static options_entry driver_flag_opts[] =
{
	{ NULL, NULL,                OPTION_HEADER, "DRIVER FLAGS"},
	{ NULL, "0",                 0,             "Play Counts" },
	{ NULL, "0",                 0,             "Play Time" },
	{ NULL, driver_flag_unknown, 0,             "Has Roms" },
	{ NULL, driver_flag_unknown, 0,             "Has Samples" },
	{ NULL }
};

static void options_add_driver_flag_opts(void)
{
	int i, j;

	sprintf(driver_flag_unknown, "%d", UNKNOWN);

	for (i = 0; i < num_drivers; i++)
	{
		char buf[256];
		char *p = buf;

		driver_flag_opts[0].description = drivers[i]->description;

		for (j = 0; j < sizeof (driver_flag_names) / sizeof (*driver_flag_names); j++)
		{

			driver_flag_opts[j + 1].name = p;
			strcpy(p, drivers[i]->name);
			p += strlen(p);
			strcat(p, driver_flag_names[j]);
			p += strlen(p) + 1;
		}

		options_add_entries(driver_flag_opts);
	}
}

static void options_get_driver_flag_opts(void)
{
	int i, j;

	for (i = 0; i < num_drivers; i++)
	{
		int *flags[] =
		{
			&driver_variables[i].play_count,
			&driver_variables[i].play_time,
			&driver_variables[i].rom_audit_results,
			&driver_variables[i].samples_audit_results
		};
		char buf[256];
		char *p;

		strcpy(buf, drivers[i]->name);
		p = buf + strlen(buf);

		for (j = 0; j < sizeof (driver_flag_names) / sizeof (*driver_flag_names); j++)
		{
			strcpy(p, driver_flag_names[j]);
			*flags[j] = options_get_int(buf, FALSE);
		}
	}
}

static void options_set_driver_flag_opts(void)
{
	int i, j;

	for (i = 0; i < num_drivers; i++)
	{
		int *flags[] =
		{
			&driver_variables[i].play_count,
			&driver_variables[i].play_time,
			&driver_variables[i].rom_audit_results,
			&driver_variables[i].samples_audit_results
		};
		char buf[256];
		char *p;

		strcpy(buf, drivers[i]->name);
		p = buf + strlen(buf);

		for (j = 0; j < sizeof (driver_flag_names) / sizeof (*driver_flag_names); j++)
		{
			strcpy(p, driver_flag_names[j]);
			options_set_int(buf, *flags[j]);
		}
	}
}


//============================================================

static void options_set_core(const settings_type *p);
static void options_set_driver(const options_type *p);
static void options_set_winui(const settings_type *p);

static void options_create_entry_core(void)
{
	options_set_datalist(NULL);

	options_add_entries(fileio_opts);
	options_add_entries(config_opts);
	options_add_entries(input_opts);
	options_add_entries(video_opts);
	options_add_entries(palette_opts);
	options_add_entries(language_opts);

	options_core = options_get_datalist();
}

static void options_create_entry_driver(void)
{
	options_set_datalist(NULL);

	options_add_entries(config_opts);
	options_add_entries(input_opts);
	options_add_entries(video_opts);

	options_driver = options_get_datalist();
}

static void options_create_entry_winui(void)
{
	options_set_datalist(NULL);

	options_add_entries(winui_opts);
	options_add_driver_flag_opts();

	options_winui = options_get_datalist();
}

static void options_free_entry_core(void)
{
	options_set_datalist(options_core);
	options_free_entries();
	options_core = NULL;
}

static void options_free_entry_driver(void)
{
	options_set_datalist(options_driver);
	options_free_entries();
	options_driver = NULL;
}

static void options_free_entry_winui(void)
{
	options_set_datalist(options_winui);
	options_free_entries();
	options_winui = NULL;
}


static int options_load_default_config(void)
{
	char filename[_MAX_PATH];
	mame_file *file;
	int retval = 0;

	SetCorePathList(FILETYPE_INI, get_base_config_directory());
	strcpy(filename, MAME_INI);

	if (!(file = mame_fopen(filename, NULL, FILETYPE_INI, 0)))
		return 0;

	options_set_datalist(options_core);
	options_set_core(&backup.settings);
	options_set_driver(&backup.global);

	retval = options_parse_ini_file(file);
	options_get_core(&settings);
	options_get_driver(&global);

	mame_fclose(file);

	return retval;
}

static int options_load_driver_config(int driver_index)
{
	char filename[_MAX_PATH];
	mame_file *file;
	int retval;
	int alt_index = driver_variables[driver_index].alt_index;

	if (alt_index != -1)
		return options_load_alt_config(&alt_options[alt_index]);

	driver_variables[driver_index].options_loaded = TRUE;
	driver_variables[driver_index].use_default = TRUE;
	SetCorePathList(FILETYPE_INI, settings.inipath);
	sprintf(filename, "%s.ini", drivers[driver_index]->name);

	if (!(file = mame_fopen(filename, NULL, FILETYPE_INI, 0)))
		return 0;

	options_set_datalist(options_driver);
	options_set_driver(&driver_options[driver_index]);

	retval = options_parse_ini_file(file);
	options_get_driver(&driver_options[driver_index]);

	update_driver_use_default(driver_index);

	mame_fclose(file);

	return retval;
}

static int options_load_alt_config(alt_options_type *alt_option)
{
	char filename[_MAX_PATH];
	mame_file *file;
	int len;
	int retval;

	alt_option->variable->options_loaded = TRUE;
	alt_option->variable->use_default = TRUE;
	SetCorePathList(FILETYPE_INI, settings.inipath);
	sprintf(filename, "%s", alt_option->name);
	len = strlen(filename);

	if (len > 2 && filename[len - 2] == '.' && filename[len - 1] == 'c')
		filename[len - 2] = '\0';
	strcat(filename, ".ini");

	if (!(file = mame_fopen(filename, NULL, FILETYPE_INI, 0)))
		return 0;

	options_set_datalist(options_driver);
	options_set_driver(alt_option->option);

	retval = options_parse_ini_file(file);
	options_get_driver(alt_option->option);

	update_alt_use_default(alt_option);

	mame_fclose(file);

	return retval;
}

static int options_load_winui_config(void)
{
	const char *filename;
	mame_file *file;
	int retval;

	SetCorePathList(FILETYPE_INI, settings.inipath);
	filename = strlower(WINUI_INI);

	if (!(file = mame_fopen(filename, NULL, FILETYPE_INI, 0)))
		return 0;

	options_set_datalist(options_winui);
	options_set_winui(&backup.settings);

	FreeIfAllocated(&settings.save_version);
	settings.save_version = strdup("(unknown)");

	retval = options_parse_ini_file(file);
	options_get_winui(&settings);

	options_get_driver_flag_opts();

	mame_fclose(file);

	return retval;
}


static int options_save_default_config(void)
{
	char filename[_MAX_PATH];
	FILE *file;

	validate_driver_option(&global);

	strcpy(filename, strlower(MAME_INI));

	if (!(file = fopen(filename, "wt")))
		return -1;

	options_set_datalist(options_core);
	options_set_core(&settings);
	options_set_driver(&global);

	options_output_ini_file(file);

	fclose(file);

	return 0;
}

static int options_save_driver_config(int driver_index)
{
	char filename[_MAX_PATH];
	FILE *file;
	const char *inipath = GetIniDir();
	options_type *parent;
	int alt_index = driver_variables[driver_index].alt_index;

	if (driver_variables[driver_index].options_loaded == FALSE)
		return 0;

	if (alt_index != -1)
		return options_save_alt_config(&alt_options[alt_index]);

	parent = update_driver_use_default(driver_index);
	if (parent == NULL)
		return 0;

	sprintf(filename, "%s\\%s.ini", settings.inipath, strlower(drivers[driver_index]->name));

	mkdir(inipath);

#ifdef USE_IPS
	// HACK: DO NOT INHERIT IPS CONFIGURATION
	if (driver_variables[driver_index].use_default && !driver_options[driver_index].ips)
#else /* USE_IPS */
	if (driver_variables[driver_index].use_default)
#endif /* USE_IPS */
	{
		unlink(filename);
		return 0;
	}

	if (!(file = fopen(filename, "wt")))
		return -1;

	options_set_datalist(options_driver);
	options_set_driver(&driver_options[driver_index]);

	options_set_mark_driver(&driver_options[driver_index], parent);
	options_output_ini_file_marked(file);

	fclose(file);

	return 0;
}

static int options_save_alt_config(alt_options_type *alt_option)
{
	char filename[_MAX_PATH];
	FILE *file;
	const char *inipath = GetIniDir();
	options_type *parent;
	int len;

	if (alt_option->variable->options_loaded == FALSE)
		return 0;

	parent = update_alt_use_default(alt_option);

	sprintf(filename, "%s\\%s", inipath, strlower(alt_option->name));

	len = strlen(filename);
	if (len > 2 && stricmp(filename + (len - 2), ".c") == 0)
		filename[len - 2] = '\0';

	strcat(filename, ".ini");

	mkdir(inipath);

#ifdef USE_IPS
	// HACK: DO NOT INHERIT IPS CONFIGURATION
	if (alt_option->variable->use_default && !alt_option->has_bios && !alt_option->option->ips)
#else /* USE_IPS */
	if (alt_option->variable->use_default && !alt_option->has_bios)
#endif /* USE_IPS */
	{
		unlink(filename);
		return 0;
	}

	if (!(file = fopen(filename, "wt")))
		return -1;

	options_set_datalist(options_driver);
	options_set_driver(alt_option->option);

	options_set_mark_driver(alt_option->option, parent);
	options_output_ini_file_marked(file);

	fclose(file);

	return 0;
}

static int options_save_winui_config(void)
{
	FILE *file;
	char filename[_MAX_PATH];
	const char *inipath = GetIniDir();

	mkdir(inipath);
	sprintf(filename, "%s\\%s", inipath, strlower(WINUI_INI));

	if (!(file = fopen(filename, "wt")))
		return -1;

	FreeIfAllocated(&settings.save_version);
	settings.save_version = strdup(GetVersionString());

	options_set_datalist(options_winui);
	options_set_winui(&settings);
	options_set_driver_flag_opts();

	options_output_ini_file(file);

	fclose(file);

	return 0;
}


//============================================================

static void options_duplicate_core(const settings_type *source, settings_type *dest);
static void options_duplicate_winui(const settings_type *source, settings_type *dest);

static void options_duplicate_settings(const settings_type *source, settings_type *dest)
{
	options_set_datalist(options_core);
	options_duplicate_core(source, dest);

	options_set_datalist(options_winui);
	options_duplicate_winui(source, dest);
}


//============================================================

#define _options_get_bool(p,name)	do { *p = options_get_bool(name, FALSE); } while (0)
#define options_copy_bool(src,dest)	do { *dest = src; } while (0)
#define options_free_bool(p)
#define _options_compare_bool(v1,v2)	do { if (v1 != v2) return TRUE; } while (0)

INLINE BOOL options_compare_bool(BOOL v1, BOOL v2)
{
	_options_compare_bool(v1, v2);
	return FALSE;
}

#define _options_get_int(p,name)	do { *p = options_get_int(name, FALSE); } while (0)
#define options_copy_int(src,dest)	do { *dest = src; } while (0)
#define options_free_int(p)
#define _options_compare_int(v1,v2)	do { if (v1 != v2) return TRUE; } while (0)

INLINE BOOL options_compare_int(int v1, int v2)
{
	_options_compare_int(v1, v2);
	return FALSE;
}

#define _options_get_float(p,name)	do { *p = options_get_float(name, FALSE); } while (0)
#define options_copy_float(src,dest)	do { *dest = src; } while (0)
#define options_free_float(p)
#define _options_compare_float(v1,v2)	do { if (v1 != v2) return TRUE; } while (0)

INLINE BOOL options_compare_float(float v1, float v2)
{
	_options_compare_float(v1, v2);
	return FALSE;
}


//============================================================

static void _options_get_csv_int(int *dest, int numitems, const char *name)
{
	const char *stemp = options_get_string(name, FALSE);
	int array[CSV_ARRAY_MAX];
	char buf[256];
	char *p;
	int i;

	if (numitems > CSV_ARRAY_MAX)
	{
		dprintf("too many arrays: %d", numitems);
		return;
	}

	if (stemp == NULL)
		return;

	p = buf;
	if (*stemp == '-')
		*p++ = *stemp++;

	if (!isdigit(*stemp))
		return;

	while (isdigit(*stemp))
		*p++ = *stemp++;
	*p = '\0';

	array[0] = atoi(buf);

	for (i = 1; i < numitems; i++)
	{
		while (*stemp == ' ')
			stemp++;

		if (*stemp++ != ',')
			return;

		p = buf;
		if (*stemp == '-')
			*p++ = *stemp++;

		if (!isdigit(*stemp))
			return;

		while (isdigit(*stemp))
			*p++ = *stemp++;
		*p = '\0';

		array[i] = atoi(buf);
	}

	if (*stemp != '\0')
		return;

	for (i = 0; i < numitems; i++)
		dest[i] = array[i];
}

static void options_set_csv_int(const char *name, const int *src, int numitems)
{
	char buf[1024];
	char *p;
	int i;

	sprintf(buf, "%d", src[0]);
	p = buf + strlen(buf);

	for (i = 1; i < numitems; i++)
	{
		sprintf(p, ",%d", src[i]);
		p += strlen(p);
	}

	options_set_string(name, buf);
}

INLINE void options_copy_csv_int(const int *src, int *dest, int numitems)
{
	int i;

	for (i = 0; i < numitems; i++)
		dest[i] = src[i];
}

#define options_free_csv_int(p,num)


//============================================================

INLINE void _options_get_string_allow_null(char **p, const char *name)
{
	const char *stemp = options_get_string(name, FALSE);

	FreeIfAllocated(p);
	if (stemp)
		*p = strdup(stemp);
}

#define options_set_string_allow_null(name,value)	options_set_string(name,value)

INLINE void options_copy_string_allow_null(const char *src, char **dest)
{
	FreeIfAllocated(dest);

	if (src)
		*dest = strdup(src);
}

#define options_free_string_allow_null			FreeIfAllocated
#define _options_compare_string_allow_null(s1,s2)	do { if (s1 != s2) { if (!s1 || !s2) return TRUE; if (strcmp(s1, s2) != 0) return TRUE; } } while (0)

INLINE BOOL options_compare_string_allow_null(const char *s1, const char *s2)
{
	_options_compare_string_allow_null(s1, s2);
	return FALSE;
}

INLINE void _options_get_string(char **p, const char *name)
{
	const char *stemp = options_get_string(name, FALSE);

	if (stemp && *stemp)
	{
		FreeIfAllocated(p);
		*p = strdup(stemp);
	}
}

INLINE void options_copy_string(const char *src, char **dest)
{
	FreeIfAllocated(dest);

	*dest = strdup(src);
}

#define options_free_string			FreeIfAllocated
#define _options_compare_string(s1,s2)		do { if (strcmp(s1, s2) != 0) return TRUE; } while (0)

INLINE BOOL options_compare_string(const char *s1, const char *s2)
{
	_options_compare_string(s1, s2);
	return FALSE;
}


//============================================================

INLINE void _options_get_int_min(int *p, const char *name, int min)
{
	int val = options_get_int(name, FALSE);
	if (val >= min)
		*p = val;
}

#define options_set_min			options_set_int
#define options_copy_int_min		options_copy_int
#define options_free_int_min		options_free_int
#define _options_compare_int_min	_options_compare_int


//============================================================

INLINE void _options_get_int_max(int *p, const char *name, int max)
{
	int val = options_get_int(name, FALSE);
	if (val <= max)
		*p = val;
}

#define options_set_max			options_set_int
#define options_copy_int_max		options_copy_int
#define options_free_int_max		options_free_int
#define _options_compare_int_max	_options_compare_int


//============================================================

INLINE void _options_get_int_min_max(int *p, const char *name, int min, int max)
{
	int val = options_get_int(name, FALSE);
	if (val >= min && val <= max)
		*p = val;
}

#define options_set_min_max		options_set_int
#define options_copy_int_min_max	options_copy_int
#define options_free_int_min_max	options_free_int
#define _options_compare_int_min_max	_options_compare_int


//============================================================

INLINE void _options_get_int_positive(int *p, const char *name)
{
	int val = options_get_int(name, FALSE);
	if (val >= 0)
		*p = val;
}

#define options_set_int_positive	options_set_int
#define options_copy_int_positive	options_copy_int
#define options_free_int_positive	options_free_int
#define _options_compare_int_positive	_options_compare_int
#define options_compare_int_positive	options_compare_int


//============================================================

INLINE void _options_get_float_min(float *p, const char *name, float min)
{
	float val = options_get_float(name, FALSE);
	if (val >= min)
		*p = val;
}

#define options_set_float_min		options_set_float
#define options_copy_float_min		options_copy_float
#define options_free_float_min		options_free_float
#define _options_compare_float_min	_options_compare_float


//============================================================

INLINE void _options_get_float_max(float *p, const char *name, float max)
{
	float val = options_get_float(name, FALSE);
	if (val <= max)
		*p = val;
}

#define options_set_float_max		options_set_float
#define options_copy_float_max		options_copy_float
#define options_free_float_max		options_free_float
#define _options_compare_float_max	_options_compare_float


//============================================================

INLINE void _options_get_float_min_max(float *p, const char *name, float min, float max)
{
	float val = options_get_float(name, FALSE);
	if (val >= min && val <= max)
		*p = val;
}

#define options_set_float_min_max	options_set_float
#define options_copy_float_min_max	options_copy_float
#define options_free_float_min_max	options_free_float
#define _options_compare_float_min_max	_options_compare_float


//============================================================

#define _options_get_samplerate(p,name)	_options_get_int_min_max(p, name, 5000, 50000)
#define options_set_samplerate		options_set_int
#define options_copy_samplerate		options_copy_int
#define options_free_samplerate		options_free_int
#define _options_compare_samplerate	_options_compare_int
#define options_compare_samplerate	options_compare_int


//============================================================

#define _options_get_volume(p,name)	_options_get_int_min_max(p, name, -32, 0)
#define options_set_volume		options_set_int
#define options_copy_volume		options_copy_int
#define options_free_volume		options_free_int
#define _options_compare_volume		_options_compare_int
#define options_compare_volume		options_compare_int


//============================================================

#define _options_get_audio_latency(p,name)	_options_get_int_min_max(p, name, 1, 4)
#define options_set_audio_latency		options_set_int
#define options_copy_audio_latency		options_copy_int
#define options_free_audio_latency		options_free_int
#define _options_compare_audio_latency		_options_compare_int
#define options_compare_audio_latency		options_compare_int


//============================================================

#ifdef USE_IPS
#define _options_get_ips		_options_get_string_allow_null
#define options_set_ips			options_set_string_allow_null
#define options_copy_ips		options_copy_string_allow_null
#define options_free_ips		options_free_string_allow_null
#define _options_compare_ips(s1,s2)	do { ; } while (0)

INLINE BOOL options_compare_ips(const char *s1, const char *s2)
{
	if (s1)
		return TRUE;
}

#endif /* USE_IPS */


//============================================================

#if (HAS_M68000 || HAS_M68008 || HAS_M68010 || HAS_M68EC020 || HAS_M68020 || HAS_M68040)
INLINE void _options_get_m68k_core(int *p, const char *name)
{
	const char *stemp = options_get_string(name, FALSE);

	if (stemp != NULL)
	{
		if (stricmp(stemp, "c") == 0)
			*p= 0;
		else if (stricmp(stemp, "drc") == 0)
			*p= 1;
		else if (stricmp(stemp, "asm") == 0)
			*p= 2;
		else
		{
			int value = options_get_int("m68k_core", FALSE);

			if (value >= 0 && value <= 2)
				*p = value;
		}
	}
}

INLINE void options_set_m68k_core(const char *name, int value)
{
	switch (value)
	{
	case 0:
	default:
		options_set_string(name, "c");
		break;
	case 1:
		options_set_string(name, "drc");
		break;
	case 2:
		options_set_string(name, "asm");
		break;
	}
}

#define options_copy_m68k_core		options_copy_int
#define options_free_m68k_core		options_free_int
#define _options_compare_m68k_core	_options_compare_int
#define options_compare_m68k_core	options_compare_int
#endif /* (HAS_M68000 || HAS_M68008 || HAS_M68010 || HAS_M68EC020 || HAS_M68020 || HAS_M68040) */


//============================================================

#ifdef TRANS_UI
#define _options_get_ui_transparency(p,name)	_options_get_int_min_max(p, name, 0, 255)
#define options_set_ui_transparency		options_set_int
#define options_copy_ui_transparency		options_copy_int
#define options_free_ui_transparency		options_free_int
#define _options_compare_ui_transparency	_options_compare_int
#define options_compare_ui_transparency		options_compare_int
#endif /* TRANS_UI */


//============================================================

INLINE void _options_get_ui_lines(int *p, const char *name)
{
	const char *stemp = options_get_string(name, FALSE);

	if (stemp != NULL)
	{
		if (stricmp(stemp, "auto") == 0)
			*p = 0;
		else
		{
			int val = options_get_int(name, FALSE);

			if (val >= 16 && val <= 64)
				*p = val;
		}
	}
}

INLINE void options_set_ui_lines(const char *name, int value)
{
	if (value == 0)
		options_set_string(name, "auto");
	else
		options_set_int(name, value);
}

#define options_copy_ui_lines		options_copy_int
#define options_free_ui_lines		options_free_int
#define _options_compare_ui_lines	_options_compare_int
#define options_compare_ui_lines	options_compare_int


//============================================================

#define _options_get_brightness(p,name)	_options_get_float_min_max(p, name, 0.5, 2.0)
#define options_set_brightness		options_set_float
#define options_copy_brightness		options_copy_float
#define options_free_brightness		options_free_float
#define _options_compare_brightness	_options_compare_float
#define options_compare_brightness	options_compare_float


//============================================================

#define _options_get_beam(p,name)	_options_get_float_min_max(p, name, 0.1, 16.0)
#define options_set_beam		options_set_float
#define options_copy_beam		options_copy_float
#define options_free_beam		options_free_float
#define _options_compare_beam		_options_compare_float
#define options_compare_beam		options_compare_float


//============================================================

#define _options_get_flicker(p,name)	_options_get_float_min_max(p, name, 0.0, 100.0)
#define options_set_flicker		options_set_float
#define options_copy_flicker		options_copy_float
#define options_free_flicker		options_free_float
#define _options_compare_flicker	_options_compare_float
#define options_compare_flicker		options_compare_float


//============================================================

#define _options_get_intensity(p,name)	_options_get_float_min_max(p, name, 0.5, 3.0)
#define options_set_intensity		options_set_float
#define options_copy_intensity		options_copy_float
#define options_free_intensity		options_free_float
#define _options_compare_intensity	_options_compare_float
#define options_compare_intensity	options_compare_float


//============================================================

INLINE void _options_get_analog_select(char **p, const char *name)
{
	const char *stemp = options_get_string(name, FALSE);

	if (stemp && *stemp)
	{
		if (strcmp(stemp, "keyboard") == 0
		 || strcmp(stemp, "mouse") == 0
		 || strcmp(stemp, "joystick") == 0
		 || strcmp(stemp, "lightgun") == 0)
		{
			FreeIfAllocated(p);
			*p = strdup(stemp);
		}
	}
}

#define options_set_analog_select	options_set_string
#define options_copy_analog_select	options_copy_string
#define options_free_analog_select	options_free_string
#define _options_compare_analog_select	_options_compare_string
#define options_compare_analog_select	options_compare_string


//============================================================

#define MAX_JOYSTICKS		8
#define MAX_AXES		8

INLINE void _options_get_digital(char **p, const char *name)
{
	const char *stemp = options_get_string(name, FALSE);

	if (stemp && *stemp)
	{
		if (strcmp(stemp, "none") == 0
		 || strcmp(stemp, "all") == 0)
		{
			FreeIfAllocated(p);
			*p = strdup(stemp);
			return;
		}

		/* scan the string */
		while (1)
		{
			int joynum = 0;
			int axisnum = 0;

			/* stop if we hit the end */
			if (stemp[0] == 0)
				break;

			/* we require the next bits to be j<N> */
			if (tolower(stemp[0]) != 'j' || sscanf(&stemp[1], "%d", &joynum) != 1)
				return;
			stemp++;
			while (stemp[0] != 0 && isdigit(stemp[0]))
				stemp++;

			/* if we are followed by a comma or an end, mark all the axes digital */
			if (stemp[0] == 0 || stemp[0] == ',')
			{
				if (joynum == 0 || joynum > MAX_JOYSTICKS)
					return;
				if (stemp[0] == 0)
					break;
				stemp++;
				continue;
			}

			/* loop over axes */
			while (1)
			{
				/* stop if we hit the end */
				if (stemp[0] == 0)
					break;

				/* if we hit a comma, skip it and break out */
				if (stemp[0] == ',')
				{
					stemp++;
					break;
				}

				/* we require the next bits to be a<N> */
				if (tolower(stemp[0]) != 'a' || sscanf(&stemp[1], "%d", &axisnum) != 1)
					return;
				stemp++;
				while (stemp[0] != 0 && isdigit(stemp[0]))
					stemp++;

				/* set that axis to digital */
				if (joynum == 0 || joynum > MAX_JOYSTICKS || axisnum >= MAX_AXES)
					return;
			}
		}

		FreeIfAllocated(p);
		*p = strdup(stemp);
	}
}

#define options_set_digital		options_set_string
#define options_copy_digital		options_copy_string
#define options_free_digital		options_free_string
#define _options_compare_digital	_options_compare_string
#define options_compare_digital		options_compare_string


//============================================================

INLINE void _options_get_led_mode(char **p, const char *name)
{
	const char *stemp = options_get_string(name, FALSE);

	if (stemp && *stemp)
	{
		if (strcmp(stemp, "ps/2") == 0
		 || strcmp(stemp, "usb") == 0)
		{
			FreeIfAllocated(p);
			*p = strdup(stemp);
		}
	}
}

#define options_set_led_mode		options_set_string
#define options_copy_led_mode		options_copy_string
#define options_free_led_mode		options_free_string
#define _options_compare_led_mode	_options_compare_string
#define options_compare_led_mode	options_compare_string


//============================================================

INLINE void _options_get_video(char **p, const char *name)
{
	const char *stemp = options_get_string(name, FALSE);

	if (stemp && *stemp)
	{
		if (strcmp(stemp, "gdi") == 0
		 || strcmp(stemp, "ddraw") == 0
		 || strcmp(stemp, "d3d") == 0)
		{
			FreeIfAllocated(p);
			*p = strdup(stemp);
		}
	}
}

#define options_set_video		options_set_string
#define options_copy_video		options_copy_string
#define options_free_video		options_free_string
#define _options_compare_video	_options_compare_string
#define options_compare_video	options_compare_string


//============================================================

INLINE void _options_get_aspect(char **p, const char *name)
{
	const char *stemp = options_get_string(name, FALSE);

	if (stemp && *stemp)
	{
		int num, den;

		if (strcmp(stemp, "auto") == 0
		 || (sscanf(stemp, "%d:%d", &num, &den) == 2 && num > 0 && den > 0))
		{
			FreeIfAllocated(p);
			*p = strdup(stemp);
		}
	}
}

#define options_set_aspect	options_set_string
#define options_copy_aspect	options_copy_string
#define options_free_aspect	options_free_string
#define _options_compare_aspect	_options_compare_string
#define options_compare_aspect	options_compare_string


//============================================================

INLINE void _options_get_resolution(char **p, const char *name)
{
	const char *stemp = options_get_string(name, FALSE);

	if (stemp && *stemp)
	{
		int width, height, depth, refresh;

		if (strcmp(stemp, "auto") == 0
		 || sscanf(stemp, "%dx%dx%d@%d", &width, &height, &depth, &refresh) >= 2)
		{
			FreeIfAllocated(p);
			*p = strdup(stemp);
		}
	}
}

#define options_set_resolution		options_set_string
#define options_copy_resolution		options_copy_string
#define options_free_resolution		options_free_string
#define _options_compare_resolution	_options_compare_string
#define options_compare_resolution	options_compare_string


//============================================================

#define _options_get_frameskip(p,name)	_options_get_int_min_max(p, name, 0, FRAMESKIP_LEVELS - 1)
#define options_set_frameskip		options_set_int
#define options_copy_frameskip		options_copy_int
#define options_free_frameskip		options_free_int
#define _options_compare_frameskip	_options_compare_int
#define options_compare_frameskip	options_compare_int


//============================================================

#define _options_get_priority(p,name)	_options_get_int_min_max(p, name, -15, 1)
#define options_set_priority		options_set_int
#define options_copy_priority		options_copy_int
#define options_free_priority		options_free_int
#define _options_compare_priority	_options_compare_int
#define options_compare_priority	options_compare_int


//============================================================

#define _options_get_numscreens(p,name)	_options_get_int_min_max(p, name, 1, MAX_WINDOWS)
#define options_set_numscreens		options_set_int
#define options_copy_numscreens		options_copy_int
#define options_free_numscreens		options_free_int
#define _options_compare_numscreens	_options_compare_int
#define options_compare_numscreens	options_compare_int


//============================================================

#define _options_get_full_screen_gamma(p,name)	_options_get_float_min_max(p, name, 0.0, 4.0)
#define options_set_full_screen_gamma		options_set_float
#define options_copy_full_screen_gamma		options_copy_float
#define options_free_full_screen_gamma		options_free_float
#define _options_compare_full_screen_gamma	_options_compare_float
#define options_compare_full_screen_gamma	options_compare_float


//============================================================

#define _options_get_d3dversion(p,name)	_options_get_int_min_max(p, name, 8, 9)
#define options_set_d3dversion		options_set_int
#define options_copy_d3dversion		options_copy_int
#define options_free_d3dversion		options_free_int
#define _options_compare_d3dversion	_options_compare_int
#define options_compare_d3dversion	options_compare_int


//============================================================

INLINE void _options_get_langcode(int *p, const char *name)
{
	const char *langname = options_get_string("language", FALSE);
	int langcode;

	if (langname == NULL)
		langcode = -1;
	else
		langcode = stricmp(langname, "auto") ? lang_find_langname(langname) : -1;

	*p = langcode;
}

INLINE void options_set_langcode(const char *name, int langcode)
{
	options_set_string("language", langcode < 0 ? "auto" : ui_lang_info[langcode].name);
}

#define options_copy_langcode		options_copy_int
#define options_free_langcode(p)


//============================================================

#ifdef UI_COLOR_DISPLAY
INLINE void _options_get_palette(char **p, const char *name)
{
	const char *stemp = options_get_string(name, FALSE);
	int pal[3];
	int i;

	if (stemp == NULL)
		return;

	if (sscanf(stemp, "%d,%d,%d", &pal[0], &pal[1], &pal[2]) != 3)
		return;

	for (i = 0; i < 3; i++)
		if (pal[i] < 0 || pal[i] > 255)
			return;

	FreeIfAllocated(p);
	*p = strdup(stemp);
}

#define options_set_palette(name,value)	options_set_string(name,value)
#define options_copy_palette		options_copy_string
#define options_free_palette		FreeIfAllocated
#endif /* UI_COLOR_DISPLAY */


//============================================================

#ifdef STORY_DATAFILE
#define _options_get_datafile_tab(p,name)	_options_get_int_min_max(p, name, 0, MAX_TAB_TYPES+TAB_SUBTRACT)
#define options_copy_datafile_tab		options_copy_int
#define options_free_datafile_tab		options_free_int
#define _options_compare_datafile_tab		_options_compare_int
#else /* STORY_DATAFILE */
#define _options_get_history_tab(p,name)	_options_get_int_min_max(p, name, 0, MAX_TAB_TYPES+TAB_SUBTRACT)
#define options_copy_history_tab		options_copy_int
#define options_free_history_tab		options_free_int
#define _options_compare_history_tab		_options_compare_int
#endif /* STORY_DATAFILE */


//============================================================

INLINE void _options_get_list_mode(int *view, const char *name)
{
	const char *stemp = options_get_string("list_mode", FALSE);
	int i;

	if (stemp == NULL)
		return;

	for (i = 0; i < VIEW_MAX; i++)
		if (strcmp(stemp, view_modes[i]) == 0)
		{
			*view = i;
			break;
		}
}

#define options_set_list_mode(name,view)	options_set_string("list_mode", view_modes[view])
#define options_copy_list_mode			options_copy_int
#define options_free_list_mode(p)


//============================================================

INLINE void _options_get_list_font(LOGFONTA *f, const char *name)
{
	const char *stemp = options_get_string("list_font", FALSE);
	LONG temp[13];
	char buf[256];
	char *p;
	int i;

	if (stemp == NULL)
		return;

	p = buf;
	if (*stemp == '-')
		*p++ = *stemp++;

	if (!isdigit(*stemp))
		return;

	while (isdigit(*stemp))
		*p++ = *stemp++;
	*p = '\0';

	temp[0] = atol(buf);

	for (i = 1; i < sizeof (temp) / sizeof (*temp); i++)
	{
		while (*stemp == ' ')
			stemp++;

		if (*stemp++ != ',')
			return;

		p = buf;
		if (*stemp == '-')
			*p++ = *stemp++;

		if (!isdigit(*stemp))
			return;

		while (isdigit(*stemp))
			*p++ = *stemp++;
		*p = '\0';

		temp[i] = atol(buf);
	}

	if (*stemp != '\0')
		return;

	for (i = 5; i < sizeof (temp) / sizeof (*temp); i++)
		if (temp[i] < 0 || temp[i] > 255)
			return;

	f->lfHeight         = temp[0];
	f->lfWidth          = temp[1];
	f->lfEscapement     = temp[2];
	f->lfOrientation    = temp[3];
	f->lfWeight         = temp[4];

	f->lfItalic         = temp[5];
	f->lfUnderline      = temp[6];
	f->lfStrikeOut      = temp[7];
	f->lfCharSet        = temp[8];
	f->lfOutPrecision   = temp[9];
	f->lfClipPrecision  = temp[10];
	f->lfQuality        = temp[11];
	f->lfPitchAndFamily = temp[12];
}

INLINE void _options_get_list_fontface(LOGFONTA *f, const char *name)
{
	const char *stemp = options_get_string("list_fontface", FALSE);

	if (stemp == NULL || *stemp == '\0')
		return;

	if (strlen(stemp) + 1 > sizeof (f->lfFaceName))
		return;

	strcpy(f->lfFaceName, stemp);
}

INLINE void options_set_list_font(const char *name, const LOGFONTA *f)
{
	char buf[512];

	sprintf(buf, "%ld,%ld,%ld,%ld,%ld,%d,%d,%d,%d,%d,%d,%d,%d",
	             f->lfHeight,
	             f->lfWidth,
	             f->lfEscapement,
	             f->lfOrientation,
	             f->lfWeight,
	             f->lfItalic,
	             f->lfUnderline,
	             f->lfStrikeOut,
	             f->lfCharSet,
	             f->lfOutPrecision,
	             f->lfClipPrecision,
	             f->lfQuality,
	             f->lfPitchAndFamily);

	options_set_string("list_font", buf);
}

#define options_set_list_fontface(name,f)	options_set_string("list_fontface", (f)->lfFaceName)

INLINE void options_copy_list_font(const LOGFONTA *src, LOGFONTA *dest)
{
	dest->lfHeight         = src->lfHeight;
	dest->lfWidth          = src->lfWidth;
	dest->lfEscapement     = src->lfEscapement;
	dest->lfOrientation    = src->lfOrientation;
	dest->lfWeight         = src->lfWeight;
	dest->lfItalic         = src->lfItalic;
	dest->lfUnderline      = src->lfUnderline;
	dest->lfStrikeOut      = src->lfStrikeOut;
	dest->lfCharSet        = src->lfCharSet;
	dest->lfOutPrecision   = src->lfOutPrecision;
	dest->lfClipPrecision  = src->lfClipPrecision;
	dest->lfQuality        = src->lfQuality;
	dest->lfPitchAndFamily = src->lfPitchAndFamily;
}

INLINE void options_copy_list_fontface(const LOGFONTA *src, LOGFONTA *dest)
{
	strcpy(dest->lfFaceName, src->lfFaceName);
}

#define options_free_list_font(p)
#define options_free_list_fontface(p)


//============================================================

#define _options_get_csv_color(dest,numitems,name)	_options_get_csv_int((int *)dest, numitems, name)
#define options_set_csv_color(name,src,numitems)	options_set_csv_int(name, (const int *)src, numitems)
#define options_copy_csv_color(src,dest,numitems)	options_copy_csv_int((const int *)src, (int *)dest, numitems)
#define options_free_csv_color(p,num)


//============================================================

#define _options_get_sort_column(p,name)	_options_get_int_min_max(p, name, 0, COLUMN_MAX-1)
#define options_set_sort_column			options_set_int
#define options_copy_sort_column		options_copy_int
#define options_free_sort_column		options_free_int
#define _options_compare_sort_column		_options_compare_int


//============================================================

INLINE void _options_get_ui_joy(int *array, const char *name)
{
	const char *stemp = options_get_string(name, FALSE);
	char buf[256];
	char *p;
	int  i;

	for (i = 0; i < 4; i++)
		array[0] = 0;

	if (stemp == NULL)
		return;

	if (!isdigit(*stemp))
		return;

	p = buf;
	while (isdigit(*stemp))
		*p++ = *stemp++;
	*p = '\0';

	array[0] = atoi(buf);

	for (i = 1; i < 4; i++)
	{
		int j;

		if (*stemp++ != ',')
			return;

		p = buf;
		while (*stemp != ',' && *stemp != '\0')
			*p++ = *stemp++;
		*p = '\0';

		switch (i)
		{
		case 2:
			array[i] = atoi(buf);
			break;

		case 1:
			for (j = 0; ; j++)
			{
				if (joycode_axis[j].name == NULL)
					return;

				if (!strcmp(joycode_axis[j].name, buf))
				{
					array[i] = joycode_axis[j].value;
					break;
				}
			}
			break;

		case 3:
			for (j = 0; ; j++)
			{
				if (joycode_dir[j].name == NULL)
					return;

				if (!strcmp(joycode_dir[j].name, buf))
				{
					array[i] = joycode_dir[j].value;
					break;
				}
			}
			break;
		}
	}

	if (*stemp != '\0')
		return;
}

INLINE void options_set_ui_joy(const char *name, const int *array)
{
	char buf[80];
	int axis, dir;

	options_set_string(name, NULL);

	if (array[0] == 0)
		return;

	for (axis = 0; ; axis++)
	{
		if (joycode_axis[axis].name == NULL)
			return;

		if (joycode_axis[axis].value == array[1])
			break;
	}

	for (dir = 0; ; dir++)
	{
		if (joycode_dir[dir].name == NULL)
			return;

		if (joycode_dir[dir].value == array[3])
			break;
	}

	sprintf(buf, "%d,%s,%d,%s", array[0], joycode_axis[axis].name, array[2], joycode_dir[dir].name);
	options_set_string(name, buf);
}

#define options_copy_ui_joy(src,dest)	options_copy_csv_int(src,dest,4)
#define options_free_ui_joy(p)


//============================================================

INLINE void _options_get_ui_key(KeySeq *ks, const char *name)
{
	const char *stemp = options_get_string(name, FALSE);

	FreeIfAllocated(&ks->seq_string);

	if (stemp == NULL)
		return;

	ks->seq_string = strdup(stemp);

	string_to_seq(ks->seq_string, &ks->is);
	//dprintf("seq=%s,,,%04i %04i %04i %04i \n",stemp,ks->is.code[0],ks->is.code[1],ks->is.code[2],ks->is.code[3]);
}

INLINE void options_set_ui_key(const char *name, const KeySeq *ks)
{
	options_set_string(name, ks->seq_string);
}

INLINE void options_copy_ui_key(const KeySeq *src, KeySeq *dest)
{
	FreeIfAllocated(&dest->seq_string);

	if (src->seq_string == NULL)
		return;

	dest->seq_string = strdup(src->seq_string);

	string_to_seq(dest->seq_string, &dest->is);
	//dprintf("seq=%s,,,%04i %04i %04i %04i \n",stemp,ks->is.code[0],ks->is.code[1],ks->is.code[2],ks->is.code[3]);
}

INLINE void options_free_ui_key(KeySeq *ks)
{
	FreeIfAllocated(&ks->seq_string);
}


//============================================================

INLINE void _options_get_folder_hide(LPBITS *flags, const char *name)
{
	const char *stemp = options_get_string("folder_hide", FALSE);

	if (*flags)
		DeleteBits(*flags);

	*flags = NewBits(MAX_FOLDERS);
	SetAllBits(*flags ,TRUE);

	if (stemp == NULL)
		return;

	while (*stemp)
	{
		char buf[256];
		char *p;
		int i;

		if (*stemp == ',')
			break;

		p = buf;
		while (*stemp != '\0' && *stemp != ',')
			*p++ = *stemp++;
		*p = '\0';

		for (i = 0; g_folderData[i].m_lpTitle != NULL; i++)
		{
			if (strcmp(g_folderData[i].short_name, buf) == 0)
			{
				ClearBit(*flags, g_folderData[i].m_nFolderId);
				break;
			}
		}

		if (*stemp++ != ',')
			return;

		while (*stemp == ' ')
			stemp++;
	}
}

INLINE void options_set_folder_hide(const char *name, LPBITS flags)
{
	char buf[1024];
	char *p;
	int i;

	p = buf;
	for (i = 0; i < MAX_FOLDERS; i++)
		if (TestBit(flags, i) == FALSE)
		{
			int j;

			for (j = 0; g_folderData[j].m_lpTitle != NULL; j++)
			{
				if (g_folderData[j].m_nFolderId == i && g_folderData[j].short_name)
				{
					if (p != buf)
						*p++ = ',';

					strcpy(p, g_folderData[j].short_name);
					p += strlen(p);
					break;
				}
			}
		}
	*p = '\0';

	options_set_string("folder_hide", buf);
}

INLINE void options_copy_folder_hide(const LPBITS src, LPBITS *dest)
{
	if (*dest)
		DeleteBits(*dest);

	*dest = DuplicateBits(src);
}

INLINE void options_free_folder_hide(LPBITS *flags)
{
	if (*flags)
		DeleteBits(*flags);
}


//============================================================

INLINE void _options_get_folder_flag(f_flag *flags, const char *name)
{
	const char *stemp = options_get_string(name, FALSE);

	free_folder_flag(flags);

	if (stemp == NULL)
		return;

	while (*stemp)
	{
		char buf[256];
		char *p1;
		char *p2;

		if (*stemp == ',')
			break;

		p1 = buf;
		while (*stemp != '\0' && *stemp != ',')
			*p1++ = *stemp++;
		*p1++ = '\0';

		if (*stemp++ != ',')
			return;

		while (*stemp == ' ')
			stemp++;

		if (!isdigit(*stemp))
			return;

		p2 = p1;
		while (isdigit(*stemp))
			*p2++ = *stemp++;
		*p2 = '\0';

		set_folder_flag(flags, buf, atoi(p1));

		if (*stemp++ != ',')
			return;

		while (*stemp == ' ')
			stemp++;
	}
}

INLINE void options_set_folder_flag(const char *name, const f_flag *flags)
{
	char buf[1024];
	char *p;
	int i;

	p = buf;
	for (i = 0; i < flags->num; i++)
		if (flags->entry[i].name != NULL)
		{
			if (p != buf)
				*p++ = ',';

			sprintf(p, "%s,%ld", flags->entry[i].name, flags->entry[i].flags);

			p += strlen(p);
		}
	*p = '\0';

	options_set_string("folder_flag", buf);
}

INLINE void options_copy_folder_flag(const f_flag *src, f_flag *dest)
{
	int i;

	free_folder_flag(dest);

	for (i = 0; i < src->num; i++)
		if (src->entry[i].name != NULL)
			set_folder_flag(dest, src->entry[i].name, src->entry[i].flags);
}

#define options_free_folder_flag	free_folder_flag


//============================================================

#undef START_OPT_FUNC_CORE
#undef END_OPT_FUNC_CORE
#undef START_OPT_FUNC_DRIVER
#undef END_OPT_FUNC_DRIVER
#undef START_OPT_FUNC_WINUI
#undef END_OPT_FUNC_WINUI
#undef DEFINE_OPT
#undef DEFINE_OPT_CSV
#undef DEFINE_OPT_STRUCT
#undef DEFINE_OPT_ARRAY

#define START_OPT_FUNC_CORE	static void options_get_core(settings_type *p) {
#define END_OPT_FUNC_CORE	}
#define START_OPT_FUNC_DRIVER	static void options_get_driver(options_type *p) {
#define END_OPT_FUNC_DRIVER	}
#define START_OPT_FUNC_WINUI	static void options_get_winui(settings_type *p) {
#define END_OPT_FUNC_WINUI	}
#define DEFINE_OPT(type,name)	_options_get_##type((&p->name), #name);
#define DEFINE_OPT_CSV(type,name)	_options_get_csv_##type((p->name), sizeof (p->name) / sizeof (*p->name), #name);
#define DEFINE_OPT_STRUCT		DEFINE_OPT
#define DEFINE_OPT_ARRAY(type,name)	_options_get_##type((p->name), #name);
#include "optdef.h"


#undef START_OPT_FUNC_CORE
#undef END_OPT_FUNC_CORE
#undef START_OPT_FUNC_DRIVER
#undef END_OPT_FUNC_DRIVER
#undef START_OPT_FUNC_WINUI
#undef END_OPT_FUNC_WINUI
#undef DEFINE_OPT
#undef DEFINE_OPT_CSV
#undef DEFINE_OPT_STRUCT
#undef DEFINE_OPT_ARRAY

#define START_OPT_FUNC_CORE	static void options_set_core(const settings_type *p) {
#define END_OPT_FUNC_CORE	}
#define START_OPT_FUNC_DRIVER	static void options_set_driver(const options_type *p) {
#define END_OPT_FUNC_DRIVER	}
#define START_OPT_FUNC_WINUI	static void options_set_winui(const settings_type *p) {
#define END_OPT_FUNC_WINUI	}
#define DEFINE_OPT(type,name)	options_set_##type(#name, (p->name));
#define DEFINE_OPT_CSV(type,name)	options_set_csv_##type(#name, (p->name), sizeof (p->name) / sizeof (*p->name));
#define DEFINE_OPT_STRUCT(type,name)	options_set_##type(#name, (&p->name));
#define DEFINE_OPT_ARRAY(type,name)	options_set_##type(#name, (p->name));
#include "optdef.h"


#undef START_OPT_FUNC_CORE
#undef END_OPT_FUNC_CORE
#undef START_OPT_FUNC_DRIVER
#undef END_OPT_FUNC_DRIVER
#undef START_OPT_FUNC_WINUI
#undef END_OPT_FUNC_WINUI
#undef DEFINE_OPT
#undef DEFINE_OPT_CSV
#undef DEFINE_OPT_STRUCT
#undef DEFINE_OPT_ARRAY

#define START_OPT_FUNC_CORE	static void options_free_string_core(settings_type *p) { \
				options_set_datalist(options_core);
#define END_OPT_FUNC_CORE	}
#define START_OPT_FUNC_DRIVER	static void options_free_string_driver(options_type *p) { \
				options_set_datalist(options_driver);
#define END_OPT_FUNC_DRIVER	}
#define START_OPT_FUNC_WINUI	static void options_free_string_winui(settings_type *p) { \
				options_set_datalist(options_winui);
#define END_OPT_FUNC_WINUI	}
#define DEFINE_OPT(type,name)	options_free_##type(&p->name);
#define DEFINE_OPT_CSV(type,name)	options_free_csv_##type((p->name), sizeof (p->name) / sizeof (*p->name));
#define DEFINE_OPT_STRUCT		DEFINE_OPT
#define DEFINE_OPT_ARRAY(type,name)	options_free_##type(p->name);
#include "optdef.h"


#undef START_OPT_FUNC_CORE
#undef END_OPT_FUNC_CORE
#undef START_OPT_FUNC_DRIVER
#undef END_OPT_FUNC_DRIVER
#undef START_OPT_FUNC_WINUI
#undef END_OPT_FUNC_WINUI
#undef DEFINE_OPT
#undef DEFINE_OPT_CSV
#undef DEFINE_OPT_STRUCT
#undef DEFINE_OPT_ARRAY

#define START_OPT_FUNC_CORE	static void options_duplicate_core(const settings_type *source, settings_type *dest) { \
				options_set_datalist(options_core);
#define END_OPT_FUNC_CORE	}
#define START_OPT_FUNC_DRIVER	static void options_duplicate_driver(const options_type *source, options_type *dest) { \
				options_set_datalist(options_driver);
#define END_OPT_FUNC_DRIVER	}
#define START_OPT_FUNC_WINUI	static void options_duplicate_winui(const settings_type *source, settings_type *dest) { \
				options_set_datalist(options_winui);
#define END_OPT_FUNC_WINUI	}
#define DEFINE_OPT(type,name)	options_copy_##type((source->name), (&dest->name));
#define DEFINE_OPT_CSV(type,name)	options_copy_csv_##type((source->name), (dest->name), sizeof (dest->name) / sizeof (*dest->name));
#define DEFINE_OPT_STRUCT(type,name)	options_copy_##type((&source->name), (&dest->name));
#define DEFINE_OPT_ARRAY(type,name)	options_copy_##type((source->name), (dest->name));
#include "optdef.h"


#undef START_OPT_FUNC_CORE
#undef END_OPT_FUNC_CORE
#undef START_OPT_FUNC_DRIVER
#undef END_OPT_FUNC_DRIVER
#undef START_OPT_FUNC_WINUI
#undef END_OPT_FUNC_WINUI
#undef DEFINE_OPT
#undef DEFINE_OPT_CSV
#undef DEFINE_OPT_STRUCT
#undef DEFINE_OPT_ARRAY

#define START_OPT_FUNC_DRIVER	static BOOL options_compare_driver(const options_type *p1, const options_type *p2) { \
				options_set_datalist(options_driver);
#define END_OPT_FUNC_DRIVER	return 0; \
				}
#define DEFINE_OPT(type,name)	_options_compare_##type((p1->name), (p2->name));
#include "optdef.h"

#undef START_OPT_FUNC_CORE
#undef END_OPT_FUNC_CORE
#undef START_OPT_FUNC_DRIVER
#undef END_OPT_FUNC_DRIVER
#undef START_OPT_FUNC_WINUI
#undef END_OPT_FUNC_WINUI
#undef DEFINE_OPT
#undef DEFINE_OPT_CSV
#undef DEFINE_OPT_STRUCT
#undef DEFINE_OPT_ARRAY

#define START_OPT_FUNC_DRIVER	static void options_set_mark_driver(const options_type *p, const options_type *parent) { \
				options_set_datalist(options_driver); \
				options_clear_output_mark();
#define END_OPT_FUNC_DRIVER	}
#define DEFINE_OPT(type,name)	do { if (options_compare_##type((p->name), (parent->name))) options_set_##type(#name, (p->name)); } while (0);
#include "optdef.h"

/* End of options.c */
