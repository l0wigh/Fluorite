#include <stdlib.h>

#define FOLLOW_WINDOWS			False			/* do you want to change workspace when sending a window to another workspace */
#define AUTO_FLOATING			True			/* When False, floating windows, will open in tiled layout */
#define OPEN_IN_FLOAT			False			/* When True, windows will be opened in floating be default. Not applied on fixed windows */
#define XRESOURCES_AUTO_RELOAD	True			/* When True, monitor if the ~/.Xresources file is modified and reload the config if so */
#define STARTING_LAYOUT			CASCADE			/* CASCADE / DWM / CENTERED */ 
#define WARP_CURSOR				True			/* (True is recommended) When True, move the cursor to the newly focused window */
#define JUMP_TO_URGENT			True			/* When True, move to the workspaces where a window need attention */
#define POLYBAR_IPC				True			/* When True, Polybar will receive IPC from Fluorite */

typedef struct
{
	char	wm_class[255];
} Rules;

// Configure this to set predefined workspaces to monitors
// Use xrandr --listactivemonitor to know the order they are sets
// Make sure to not have a workspaces set for two monitors. 
static int default_monitor_workspace[10] = { 1, 0, 3, 4, 5, 6, 7, 8, 9, 2 };

static const char *workspace_names[10] = { "1", "2", "3", "4", "5", "6", "7", "8", "9", "0" };
// static const char *workspace_names[10] = { "I", "II", "III", "IV", "V", "VI", "VII", "VIII", "IX", "X" };
// static const char *workspace_names[10] = { " ", "󰈹", " ", "4", "5", "6", "7", "8", "9", "0" };

// Use xprop on a window to get the WM_CLASS name used by a window.
static const Rules default_floating[] = {
	{"spectacle"},
	// {"ghidra-Ghidra"},
	{"spotify"},
	{"thunar"},
	{"ymuse"},
	{"force_float"},
	{"steam"},
	{"Steam"},
	{"steamwebhelper"},
	{"st_music"},
	{"spacefm"},
};
static const Rules default_fixed[] = {
	{"Conky"},
};
static const Rules default_swallowing[] = {
	{"st"},
	{"kitty"},
	{"alacritty"}
};
