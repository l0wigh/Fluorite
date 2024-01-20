#include <X11/X.h>
#include <X11/keysym.h>
#include <X11/XF86keysym.h>
#include <stdlib.h>

#define BORDER_WIDTH	3
#define BORDER_COLORS	0x35e5dc
#define GAPS			10
#define STACK_OFFSET	20
#define TOPBAR_GAPS		0
#define BOTTOMBAR_GAPS	0
#define METAKEY			Mod4Mask

// Helpers for configuration
#define FOCUS_TOP			1
#define FOCUS_BOTTOM		-1
#define SLAVES_UP			4
#define SLAVES_DOWN			5
#define BIGGER_MASTER		6
#define SMALLER_MASTER		7

typedef struct {
	unsigned int	mod;
	KeySym			key;
	void			(*func)();
} Bindings;

// Functions that can be called from Keybinds
void fluorite_execute(char *argument);
void fluorite_close_window();
void fluorite_layout_change(int mode);

// User functions (use it or create yours with these examples)
static void fluorite_terminal() { char prog[255] = "kitty"; fluorite_execute(prog); }
static void fluorite_filemanager() { char prog[255] = "thunar"; fluorite_execute(prog); }
static void fluorite_dmenu() { char prog[255] = "dmenu_run -m 0"; fluorite_execute(prog); }
static void fluorite_webbrowser() { char prog[255] = "firefox"; fluorite_execute(prog); }
static void fluorite_exit() { char prog[255] = "killall Fluorite"; fluorite_execute(prog); }
static void fluorite_next_focus() { fluorite_layout_change(FOCUS_TOP); }
static void fluorite_prev_focus() { fluorite_layout_change(FOCUS_BOTTOM); }
static void fluorite_stack_rotate_up() { fluorite_layout_change(SLAVES_UP); }
static void fluorite_stack_rotate_down() { fluorite_layout_change(SLAVES_DOWN); }
static void fluorite_bigger_master() { fluorite_layout_change(BIGGER_MASTER); }
static void fluorite_smaller_master() { fluorite_layout_change(SMALLER_MASTER); }
static void fluorite_brightness_up() { char prog[255] = "brightnessctl set 50+"; fluorite_execute(prog); }
static void fluorite_brightness_down() { char prog[255] = "brightnessctl set 50-"; fluorite_execute(prog); }

static const Bindings bind[] = {
	{METAKEY,				XK_Return,					fluorite_terminal},
	{METAKEY,				XK_a,						fluorite_filemanager},
	{METAKEY,				XK_d,						fluorite_dmenu},
	{METAKEY,				XK_j,						fluorite_smaller_master},
	{METAKEY,				XK_k,						fluorite_next_focus},
	{METAKEY,				XK_l,						fluorite_prev_focus},
	{METAKEY,				XK_m,						fluorite_bigger_master},
	{METAKEY,				XK_w,						fluorite_webbrowser},

	{METAKEY|ShiftMask, 	XK_p,						fluorite_exit},
	{METAKEY|ShiftMask,		XK_q,						fluorite_close_window},
	{METAKEY|ShiftMask,		XK_k,						fluorite_stack_rotate_up},
	{METAKEY|ShiftMask,		XK_l,						fluorite_stack_rotate_down},

	{METAKEY,				XF86XK_MonBrightnessUp,		fluorite_brightness_up},
	{METAKEY,				XF86XK_MonBrightnessDown,	fluorite_brightness_down},
};
