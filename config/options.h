#include <stdlib.h>

#define FOLLOW_WINDOWS			False			/* do you want to change workspace when sending a window to another workspace */
#define MAX_WINDOWS				15				/* number of windows per workspaces */
#define AUTO_FLOATING			True			/* When False, floating windows, will open in tiled layout */
#define OPEN_IN_FLOAT			False			/* When True, windows will be opened in floating be default. Not applied on fixed windows */
#define PRIMARY_BAR_ONLY		True			/* When True, Fluorite will considere that only the primary monitor is having a bar. When false Fluorite will assume that every monitor have one */
#define XRESOURCES_AUTO_RELOAD	True			/* When True, monitor if the ~/.Xresources file is modified and reload the config if so */
#define STARTING_LAYOUT			FLUORITE
#define WARP_CURSOR				True

typedef struct
{
	char	wm_class[255];
} Rules;

// Configure this to set predefined workspaces to monitors
// Use xrandr --listactivemonitor to know the order they are sets
// Make sure to not have a workspaces set for two monitors. 
static int default_monitor_workspace[10] = { 1, 0, 3, 4, 5, 6, 7, 8, 9, 2 };

static const char *workspace_names[10] = { "1", "2", "3", "4", "5", "6", "7", "8", "9", "0" };
// static const char *workspace_names[10] = { " ", "󰈹", " ", "4", "5", "6", "7", "8", "9", "0" };

// Use xprop on a to get the WM_CLASS name used by a window.
static const Rules default_floating[] = {
	{"spectacle"},
	{"ghidra-Ghidra"},
	{"spotify"},
	{"thunar"},
	{"ymuse"},
	{"force_float"},
	{"steam"},
	{"Steam"},
	{"steamwebhelper"},
	{"st_music"},
};
static const Rules default_fixed[] = {
	{"Conky"},
};
