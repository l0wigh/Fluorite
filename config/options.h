#include <stdlib.h>

#define STARTING_LAYOUT			CASCADE			/* CASCADE / DWM / CENTERED */ 

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
