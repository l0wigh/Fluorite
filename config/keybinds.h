#include <X11/X.h>
#include <X11/keysym.h>
#include <X11/XF86keysym.h>

#define METAKEY					Mod4Mask		/* key that will be used for bindings */

typedef struct
{
	unsigned int	mod;
	KeySym			key;
	void			(*func)();
} Bindings;

// Helpers for configuration (don't change values)
#define FOCUS_TOP			10
#define FOCUS_BOTTOM		11
#define SLAVES_UP			12
#define SLAVES_DOWN			13
#define BIGGER_MASTER		14
#define SMALLER_MASTER		15
#define BASE_MASTER			16
#define STACKING_TOGGLE		17
#define FULLSCREEN_TOGGLE	18
#define SWAP_FOCUS			19
#define FLOATING_TOGGLE		20
#define FLOATING_HIDE_SHOW	21
#define ORGANIZER_TOGGLE	22
#define SELECT_NEXT			23
#define SELECT_PREV			24
#define MOVE_RIGHT			25
#define MOVE_LEFT			26

/*  These definitions are used for the execute command. You need to pass GUI for an app that will open a new window.
 *  Pass NOGUI if it's just a background script or app
 *  Be carefull with this, it can create bugs and and crashes ! */
#define GUI      30
#define NOGUI    31

// Functions that can be called from Keybinds
void fluorite_execute(char *argument, int mode);
void fluorite_close_window();
void fluorite_change_layout(int mode);
void fluorite_user_close();
void fluorite_change_workspace(int new_workspace, int mode);
void fluorite_organizer_mapping(int mode);
void fluorite_focus_next_monitor();
void fluorite_next_workspace(int mode);
void fluorite_prev_workspace(int mode);
void fluorite_reload_xresources();

// User functions (use it or create yours with these examples)
/* static void fluorite_terminal() { char prog[255] = "alacritty"; fluorite_execute(prog, GUI); } */
static void fluorite_terminal() { char prog[255] = "st"; fluorite_execute(prog, GUI); }
static void fluorite_filemanager() { char prog[255] = "thunar"; fluorite_execute(prog, GUI); }
static void fluorite_dmenu() { char prog[255] = "dmenu_run_desktop"; fluorite_execute(prog, GUI); }
static void fluorite_dmenu_cmd() { char prog[255] = "dmenu_run"; fluorite_execute(prog, GUI); }
static void fluorite_webbrowser() { char prog[255] = "firefox"; fluorite_execute(prog, GUI); }
static void fluorite_exit() { fluorite_user_close(); }
static void fluorite_next_focus() { fluorite_change_layout(FOCUS_TOP); }
static void fluorite_prev_focus() { fluorite_change_layout(FOCUS_BOTTOM); }
static void fluorite_stack_rotate_up() { fluorite_change_layout(SLAVES_UP); }
static void fluorite_stack_rotate_down() { fluorite_change_layout(SLAVES_DOWN); }
static void fluorite_bigger_master() { fluorite_change_layout(BIGGER_MASTER); }
static void fluorite_smaller_master() { fluorite_change_layout(SMALLER_MASTER); }
static void fluorite_base_master() { fluorite_change_layout(BASE_MASTER); }
static void fluorite_stacking_toggle() { fluorite_change_layout(STACKING_TOGGLE); }
static void fluorite_fullscreen_toggle() { fluorite_change_layout(FULLSCREEN_TOGGLE); }
static void fluorite_swap_focus() { fluorite_change_layout(SWAP_FOCUS); }
static void fluorite_floating_toggle() { fluorite_change_layout(FLOATING_TOGGLE); }
static void fluorite_floating_hide_show() { fluorite_change_layout(FLOATING_HIDE_SHOW); }
static void fluorite_brightness_up() { char prog[255] = "brightnessctl set 5%+"; fluorite_execute(prog, NOGUI); }
static void fluorite_brightness_down() { char prog[255] = "brightnessctl set 5%-"; fluorite_execute(prog, NOGUI); }
static void fluorite_volume_up() { char prog[255] = "pactl set-sink-volume 0 +5%"; fluorite_execute(prog, NOGUI); }
static void fluorite_volume_down() { char prog[255] = "pactl set-sink-volume 0 -5%"; fluorite_execute(prog, NOGUI); }
static void fluorite_volume_mute() { char prog[255] = "pactl set-sink-mute 0 toggle"; fluorite_execute(prog, NOGUI); }
static void fluorite_locking() { char prog[255] = "elogind-inhibit --what=sleep xsecurelock"; fluorite_execute(prog, NOGUI); }
static void fluorite_toggle_organizer() { fluorite_change_layout(ORGANIZER_TOGGLE); }
static void fluorite_organizer_next() { fluorite_organizer_mapping(SELECT_NEXT); }
static void fluorite_organizer_prev() { fluorite_organizer_mapping(SELECT_PREV); }
static void fluorite_organizer_right() { fluorite_organizer_mapping(MOVE_RIGHT); }
static void fluorite_organizer_left() { fluorite_organizer_mapping(MOVE_LEFT); }
static void fluorite_custom_launcher() { char prog[255] = "~/tools/scripts/rofi_custom.sh"; fluorite_execute(prog, GUI); }
static void fluorite_print_screen() { char prog[255] = "~/tools/suckless_tools/scripts/print_screen.sh"; fluorite_execute(prog, NOGUI); }
static void fluorite_dmenu_xresources() { char prog[255] = "~/tools/suckless_tools/scripts/dmenu_xresources.sh"; fluorite_execute(prog, NOGUI); }
static void fluorite_dmenu_theme() { char prog[255] = "~/tools/suckless_tools/scripts/fluorite_theme.sh"; fluorite_execute(prog, NOGUI); }
static void fluorite_st_music() { char prog[255] = "st -c st_music -g 100x30 mocp"; fluorite_execute(prog, NOGUI); }
static void fluorite_dmenu_music() { char prog[255] = "~/tools/suckless_tools/scripts/dmenu_music.sh"; fluorite_execute(prog, NOGUI); }

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
static void fluorite_goto_next_workspace() { fluorite_next_workspace(0); }
static void fluorite_goto_prev_workspace() { fluorite_prev_workspace(0); }

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
static void fluorite_appto_next_workspace() { fluorite_next_workspace(1); }
static void fluorite_appto_prev_workspace() { fluorite_prev_workspace(1); }
 
static const Bindings bind[] = {
	{METAKEY,				XK_Return,					fluorite_terminal},
	{METAKEY,				XK_a,						fluorite_filemanager},
	{METAKEY,				XK_d,						fluorite_dmenu},
	{METAKEY,				XK_j,						fluorite_smaller_master},
	{METAKEY,				XK_k,						fluorite_next_focus},
	{METAKEY,				XK_l,						fluorite_prev_focus},
	{METAKEY,				XK_m,						fluorite_bigger_master},
	{METAKEY,				XK_equal,					fluorite_base_master},
	{METAKEY,				XK_w,						fluorite_webbrowser},
	{METAKEY,				XK_s,						fluorite_stacking_toggle},
	{METAKEY,				XK_f,						fluorite_fullscreen_toggle},
	{METAKEY,				XK_n,						fluorite_swap_focus},
	{METAKEY,				XK_space,					fluorite_floating_toggle},
	{METAKEY,				XK_o,						fluorite_toggle_organizer},
	{METAKEY,				XK_Up,						fluorite_organizer_next},
	{METAKEY, 				XK_Down,					fluorite_organizer_prev},
	{METAKEY, 				XK_Right,					fluorite_organizer_right},
	{METAKEY, 				XK_Left,					fluorite_organizer_left},
	{METAKEY, 				XK_Print,					fluorite_print_screen},
	{METAKEY, 				XK_c,						fluorite_dmenu_xresources},
	{METAKEY, 				XK_t,						fluorite_dmenu_theme},

	{METAKEY|ShiftMask, 	XK_p,						fluorite_exit},
	{METAKEY|ShiftMask,		XK_q,						fluorite_close_window},
	{METAKEY|ShiftMask,		XK_k,						fluorite_stack_rotate_up},
	{METAKEY|ShiftMask,		XK_l,						fluorite_stack_rotate_down},
	{METAKEY|ShiftMask,		XK_e,						fluorite_locking},
	{METAKEY|ShiftMask,		XK_space,					fluorite_floating_hide_show},
	{METAKEY|ShiftMask,		XK_n,						fluorite_focus_next_monitor},
	{METAKEY|ShiftMask,		XK_r,						fluorite_reload_xresources},
	{METAKEY|ShiftMask,		XK_d,						fluorite_custom_launcher},
	{METAKEY|ShiftMask,		XK_d,						fluorite_dmenu_cmd},
	{METAKEY|ShiftMask,		XK_m,						fluorite_dmenu_music},

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
	{METAKEY,				XK_agrave,  				  fluorite_goto_workspace_ten},
	{METAKEY|ControlMask,	XK_j,						fluorite_goto_prev_workspace},
	{METAKEY|ControlMask,	XK_m,						fluorite_goto_next_workspace},

	// App Workspaces switching
	{METAKEY|ShiftMask,		XK_ampersand, 				fluorite_appto_workspace_one},
	{METAKEY|ShiftMask,		XK_eacute,    				fluorite_appto_workspace_two},
	{METAKEY|ShiftMask,		XK_quotedbl,  				fluorite_appto_workspace_three},
	{METAKEY|ShiftMask,		XK_apostrophe,				fluorite_appto_workspace_four},
	{METAKEY|ShiftMask,		XK_parenleft, 				fluorite_appto_workspace_five},
	{METAKEY|ShiftMask,		XK_minus,     				fluorite_appto_workspace_six},
	{METAKEY|ShiftMask,		XK_egrave,    				fluorite_appto_workspace_seven},
	{METAKEY|ShiftMask,		XK_underscore,				fluorite_appto_workspace_eight},
	{METAKEY|ShiftMask,		XK_ccedilla,  				fluorite_appto_workspace_nine},
	{METAKEY|ShiftMask,		XK_agrave,  				  fluorite_appto_workspace_ten},
	{METAKEY|ControlMask|ShiftMask,	XK_j,				fluorite_appto_prev_workspace},
	{METAKEY|ControlMask|ShiftMask,	XK_m,				fluorite_appto_next_workspace},

	// No METAKEY bindings
	{0,						XF86XK_MonBrightnessUp,		fluorite_brightness_up},
	{0,						XF86XK_MonBrightnessDown,	fluorite_brightness_down},
	{0,						XF86XK_AudioLowerVolume,	fluorite_volume_down},
	{0,						XF86XK_AudioRaiseVolume,	fluorite_volume_up},
	{0,						XF86XK_AudioMute,			fluorite_volume_mute},
	{0,						XF86XK_Favorites,			fluorite_st_music},
};
