#include <X11/X.h>
#include <X11/keysym.h>
#include <X11/XF86keysym.h>
#include <stdlib.h>

#define BORDER_WIDTH		3
#define BORDER_FOCUSED		0x35e5dc
#define BORDER_UNFOCUSED	0xf576e4
#define BORDER_INACTIVE		0x9c082d
#define GAPS				5
#define STACK_OFFSET		5
#define TOPBAR_GAPS			40
#define BOTTOMBAR_GAPS		0
#define METAKEY				Mod4Mask
#define FOLLOW_WINDOWS		False

// For now only single character names are working
// You might be able to change polybar config to handle nerdfont and other custom names
static const char *workspace_names[10] = { "1", "2", "3", "4", "5", "6", "7", "8", "9", "0" };

// Helpers for configuration (don't change values)
#define FOCUS_TOP			10
#define FOCUS_BOTTOM		11
#define SLAVES_UP			12
#define SLAVES_DOWN			13
#define BIGGER_MASTER		14
#define SMALLER_MASTER		15
#define FULLSCREEN_TOGGLE	16
 
// Functions that can be called from user functions or direct bindings
void fluorite_execute(char *argument);
void fluorite_close_window();
void fluorite_layout_change(int mode);
void fluorite_user_close();
void fluorite_change_workspace(int new_workspace, int mode);

// User functions (use it or create yours with these examples)
static void fluorite_terminal() { char prog[255] = "alacritty"; fluorite_execute(prog); }
static void fluorite_filemanager() { char prog[255] = "thunar"; fluorite_execute(prog); }
static void fluorite_dmenu() { char prog[255] = "rofi -show run"; fluorite_execute(prog); }
static void fluorite_webbrowser() { char prog[255] = "firefox"; fluorite_execute(prog); }
static void fluorite_locking() { char prog[255] = "i3lock --color 1e1e1e; systemctl suspend"; fluorite_execute(prog); }

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
static void fluorite_volume_up() { char prog[255] = "pactl set-sink-volume 0 +5%"; fluorite_execute(prog); }
static void fluorite_volume_down() { char prog[255] = "pactl set-sink-volume 0 -5%"; fluorite_execute(prog); }
static void fluorite_volume_mute() { char prog[255] = "pactl set-sink-mute 0 toggle"; fluorite_execute(prog); }

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

// Bindings struct
typedef struct
{
	unsigned int	mod;
	KeySym			key;
	void			(*func)();
} Bindings;

static const Bindings bind[] = {
	{METAKEY,						XK_Return,					fluorite_terminal},
	{METAKEY,						XK_a,						fluorite_filemanager},
	{METAKEY,						XK_d,						fluorite_dmenu},
	{METAKEY,						XK_h,						fluorite_smaller_master},
	{METAKEY,						XK_j,						fluorite_next_focus},
	{METAKEY,						XK_k,						fluorite_prev_focus},
	{METAKEY,						XK_l,						fluorite_bigger_master},
	{METAKEY,						XK_w,						fluorite_webbrowser},
	{METAKEY,						XK_f,						fluorite_fullscreen_toggle},

	{METAKEY,						XF86XK_MonBrightnessUp,		fluorite_brightness_up},
	{METAKEY,						XF86XK_MonBrightnessDown,	fluorite_brightness_down},
	{METAKEY,						XF86XK_AudioLowerVolume,	fluorite_volume_down},
	{METAKEY,						XF86XK_AudioRaiseVolume,	fluorite_volume_up},
	{METAKEY,						XF86XK_AudioMute,			fluorite_volume_mute},

	{METAKEY|ShiftMask,				XK_p,						fluorite_exit},
	{METAKEY|ShiftMask,				XK_q,						fluorite_close_window},
	{METAKEY|ShiftMask,				XK_j,						fluorite_stack_rotate_up},
	{METAKEY|ShiftMask,				XK_k,						fluorite_stack_rotate_down},
	{METAKEY|ShiftMask,				XK_e,						fluorite_locking},

	// Workspaces switching
	{METAKEY,						XK_1,						fluorite_goto_workspace_one},
	{METAKEY,						XK_2,						fluorite_goto_workspace_two},
	{METAKEY,						XK_3,						fluorite_goto_workspace_three},
	{METAKEY,						XK_4,						fluorite_goto_workspace_four},
	{METAKEY,						XK_5, 						fluorite_goto_workspace_five},
	{METAKEY,						XK_6,						fluorite_goto_workspace_six},
	{METAKEY,						XK_7,    					fluorite_goto_workspace_seven},
	{METAKEY,						XK_8,						fluorite_goto_workspace_eight},
	{METAKEY,						XK_9,  						fluorite_goto_workspace_nine},
	{METAKEY,						XK_0,  						fluorite_goto_workspace_ten},

	// App Workspaces switching
	{METAKEY|ShiftMask,				XK_1,						fluorite_appto_workspace_one},
	{METAKEY|ShiftMask,				XK_2,  						fluorite_appto_workspace_two},
	{METAKEY|ShiftMask,				XK_3,  						fluorite_appto_workspace_three},
	{METAKEY|ShiftMask,				XK_4,						fluorite_appto_workspace_four},
	{METAKEY|ShiftMask,				XK_5, 						fluorite_appto_workspace_five},
	{METAKEY|ShiftMask,				XK_6,  						fluorite_appto_workspace_six},
	{METAKEY|ShiftMask,				XK_7,  						fluorite_appto_workspace_seven},
	{METAKEY|ShiftMask,				XK_8,						fluorite_appto_workspace_eight},
	{METAKEY|ShiftMask,				XK_9,  						fluorite_appto_workspace_nine},
	{METAKEY|ShiftMask,				XK_0,  						fluorite_appto_workspace_ten},
};
