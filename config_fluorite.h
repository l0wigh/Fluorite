#include <X11/X.h>
#include <X11/keysym.h>
#include <X11/XF86keysym.h>
#include <stdlib.h>

#define BORDER_WIDTH	3
/* #define BORDER_COLORS	0x35e5dc */
/* #define BORDER_COLORS	0x9c082d */
#define BORDER_COLORS	0xde4a2c
#define GAPS			10
#define STACK_OFFSET	20
#define TOPBAR_GAPS		0
#define BOTTOMBAR_GAPS	0
#define METAKEY			Mod4Mask
#define FOLLOW_WINDOWS	True

// Helpers for configuration (don't change values)
#define FOCUS_TOP			10
#define FOCUS_BOTTOM		11
#define SLAVES_UP			12
#define SLAVES_DOWN			13
#define BIGGER_MASTER		14
#define SMALLER_MASTER		15
#define FULLSCREEN_TOGGLE	16
 
typedef struct
{
	unsigned int	mod;
	KeySym			key;
	void			(*func)();
} Bindings;

// Functions that can be called from Keybinds
void fluorite_execute(char *argument);
void fluorite_close_window();
void fluorite_layout_change(int mode);
void fluorite_user_close();
void fluorite_change_workspace(int new_workspace, int mode);

// User functions (use it or create yours with these examples)
static void fluorite_terminal() { char prog[255] = "kitty"; fluorite_execute(prog); }
static void fluorite_filemanager() { char prog[255] = "thunar"; fluorite_execute(prog); }
static void fluorite_dmenu() { char prog[255] = "rofi -show run"; fluorite_execute(prog); }
static void fluorite_webbrowser() { char prog[255] = "firefox"; fluorite_execute(prog); }
static void fluorite_exit() { fluorite_user_close(); }
static void fluorite_next_focus() { fluorite_layout_change(FOCUS_TOP); }
static void fluorite_prev_focus() { fluorite_layout_change(FOCUS_BOTTOM); }
static void fluorite_stack_rotate_up() { fluorite_layout_change(SLAVES_UP); }
static void fluorite_stack_rotate_down() { fluorite_layout_change(SLAVES_DOWN); }
static void fluorite_bigger_master() { fluorite_layout_change(BIGGER_MASTER); }
static void fluorite_smaller_master() { fluorite_layout_change(SMALLER_MASTER); }
static void fluorite_fullscreen_toggle() { fluorite_layout_change(FULLSCREEN_TOGGLE); }
static void fluorite_brightness_up() { char prog[255] = "brightnessctl set 50+"; fluorite_execute(prog); }
static void fluorite_brightness_down() { char prog[255] = "brightnessctl set 50-"; fluorite_execute(prog); }

// Workspaces switch function
static void	fluorite_goto_workspace_one() { fluorite_change_workspace(0, 0); }
static void	fluorite_goto_workspace_two() { fluorite_change_workspace(1, 0); }
static void	fluorite_goto_workspace_three() { fluorite_change_workspace(2, 0); }
static void	fluorite_goto_workspace_four() { fluorite_change_workspace(3, 0); }
static void	fluorite_goto_workspace_five() { fluorite_change_workspace(4, 0); }
static void	fluorite_goto_workspace_six() { fluorite_change_workspace(5, 0); }
static void	fluorite_goto_workspace_seven() { fluorite_change_workspace(6, 0); }
static void	fluorite_goto_workspace_eight() { fluorite_change_workspace(7, 0); }
static void	fluorite_goto_workspace_nine() { fluorite_change_workspace(8, 0); }
static void	fluorite_goto_workspace_ten() { fluorite_change_workspace(9, 0); }

// App workspaces switch function
static void	fluorite_appto_workspace_one() { fluorite_change_workspace(0, 1); }
static void	fluorite_appto_workspace_two() { fluorite_change_workspace(1, 1); }
static void	fluorite_appto_workspace_three() { fluorite_change_workspace(2, 1); }
static void	fluorite_appto_workspace_four() { fluorite_change_workspace(3, 1); }
static void	fluorite_appto_workspace_five() { fluorite_change_workspace(4, 1); }
static void	fluorite_appto_workspace_six() { fluorite_change_workspace(5, 1); }
static void	fluorite_appto_workspace_seven() { fluorite_change_workspace(6, 1); }
static void	fluorite_appto_workspace_eight() { fluorite_change_workspace(7, 1); }
static void	fluorite_appto_workspace_nine() { fluorite_change_workspace(8, 1); }
static void	fluorite_appto_workspace_ten() { fluorite_change_workspace(9, 1); }

static const Bindings bind[] = {
	{METAKEY,				XK_Return,					fluorite_terminal},
	{METAKEY,				XK_a,						fluorite_filemanager},
	{METAKEY,				XK_d,						fluorite_dmenu},
	{METAKEY,				XK_j,						fluorite_smaller_master},
	{METAKEY,				XK_k,						fluorite_next_focus},
	{METAKEY,				XK_l,						fluorite_prev_focus},
	{METAKEY,				XK_m,						fluorite_bigger_master},
	{METAKEY,				XK_w,						fluorite_webbrowser},
	{METAKEY,				XK_f,						fluorite_fullscreen_toggle},

	// Workspaces switching
	{METAKEY,				XK_ampersand, 				fluorite_goto_workspace_one},
	{METAKEY,				XK_eacute,    				fluorite_goto_workspace_two},
	{METAKEY,				XK_quotedbl,  				fluorite_goto_workspace_three},
	{METAKEY,				XK_apostrophe,				fluorite_goto_workspace_four},
	{METAKEY,				XK_parenleft, 				fluorite_goto_workspace_five},
	{METAKEY,				XK_minus,     				fluorite_goto_workspace_six},
	{METAKEY,				XK_egrave,    				fluorite_goto_workspace_seven},
	{METAKEY,				XK_underscore,				fluorite_goto_workspace_eight},
	{METAKEY,				XK_ccedilla,  				fluorite_goto_workspace_nine},
	{METAKEY,				XK_agrave,  				fluorite_goto_workspace_ten},

	// App Workspaces switching
	{METAKEY|ShiftMask,				XK_ampersand, 				fluorite_appto_workspace_one},
	{METAKEY|ShiftMask,				XK_eacute,    				fluorite_appto_workspace_two},
	{METAKEY|ShiftMask,				XK_quotedbl,  				fluorite_appto_workspace_three},
	{METAKEY|ShiftMask,				XK_apostrophe,				fluorite_appto_workspace_four},
	{METAKEY|ShiftMask,				XK_parenleft, 				fluorite_appto_workspace_five},
	{METAKEY|ShiftMask,				XK_minus,     				fluorite_appto_workspace_six},
	{METAKEY|ShiftMask,				XK_egrave,    				fluorite_appto_workspace_seven},
	{METAKEY|ShiftMask,				XK_underscore,				fluorite_appto_workspace_eight},
	{METAKEY|ShiftMask,				XK_ccedilla,  				fluorite_appto_workspace_nine},
	{METAKEY|ShiftMask,				XK_agrave,  				fluorite_appto_workspace_ten},

	{METAKEY|ShiftMask, 	XK_p,						fluorite_exit},
	{METAKEY|ShiftMask,		XK_q,						fluorite_close_window},
	{METAKEY|ShiftMask,		XK_k,						fluorite_stack_rotate_up},
	{METAKEY|ShiftMask,		XK_l,						fluorite_stack_rotate_down},

	{METAKEY,				XF86XK_MonBrightnessUp,		fluorite_brightness_up},
	{METAKEY,				XF86XK_MonBrightnessDown,	fluorite_brightness_down},
};
