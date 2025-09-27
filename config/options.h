#include <stdlib.h>

#define STARTING_LAYOUT			CASCADE			/* CASCADE / DWM / CENTERED */ 

typedef struct
{
	char	wm_class[255];
} Rules;

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
